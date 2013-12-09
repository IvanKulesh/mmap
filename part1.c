#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>

#define DATESIZE 26
#define TYPESIZE 18
#define PERMSIZE 10
#define KEYFILE "./part1.c"
#define RESULTFILE "./ls.tmp"
#define HELLO 1
#define READY 2

typedef struct struct_for_file_info {
	off_t file_size;
	char create_date [DATESIZE];
	char last_change_date [DATESIZE];
	char file_type [TYPESIZE];
	char file_permissions [PERMSIZE];
	size_t name_lenght;
	char file_name_user_group [];
} file_info;

int count_of_alocated_pointers = 0;
void** array_of_alocated_pointers = NULL;

char** list_of_files = NULL;

void add_pointer (void* new_pointer);

void my_error (char* message);

void free_all_pointers ();

int get_list_of_files ( DIR* directory );

void get_properties ( int number_of_files , file_info ** files_properties, char ** argv);

void increase_sem (int sem_id);
	
void decrease_sem (int sem_id);

char* get_permissions (mode_t file_mode);

int get_sem_id (char key_mod);

void* get_mmap (char* path , size_t file_size);

int main (int argc, char** argv) {
	DIR* destination_directory;
	int number_of_files = 0;
	int i;
	file_info ** files_properties = NULL;
	int  msg_id;
	key_t msg_key;
	void * mmapped_file = NULL; 
	void *tmp_mmapped_file = NULL;
	size_t file_size = 0;
	struct first_msgbuf {
		long mtype;
		size_t file_lenght;
	} first_msg;

	umask (0);

	if (argc !=2)
		my_error ("Error! You should enter a path as an argument!\n\0");
	
	if ( ( destination_directory = opendir ( argv[1] ) ) == NULL ) {
		if (errno == ENOTDIR)
			my_error ("The argument is not a directory name!\n\0");
		if (errno == ENOENT)
			my_error ("Directory does not exist!\n\0");
		if (errno == EACCES)
			my_error ("Permission denied!\n\0");
		my_error ("Unrecognized error occured!\n\0");
	}
	
	msg_key = ftok (KEYFILE , 1);
	if (msg_key < 0)
		my_error ("ERROR WHEN CALL FTOK!\n\0");
	
	msg_id = msgget (msg_key, IPC_CREAT | 0666);
	if (msg_id < 0)
		my_error ("ERROR WHEN CALL MSGGET!\n\0");
		
	add_pointer (list_of_files);
	number_of_files = get_list_of_files (destination_directory);
	
	add_pointer ( files_properties );
	files_properties = (file_info **) malloc (sizeof (file_info*) * number_of_files);
	
	get_properties (number_of_files , files_properties , argv );

	for ( i=0 ; i < number_of_files ; ++i )
		file_size += (files_properties[i])->name_lenght ; 

	mmapped_file = get_mmap (RESULTFILE , file_size + sizeof (int));
	tmp_mmapped_file = mmapped_file;
	* ((int * ) tmp_mmapped_file ) = number_of_files;
	tmp_mmapped_file += sizeof (int);

	for ( i=0 ; i < number_of_files ; ++i) {
		* ((file_info * ) tmp_mmapped_file )= * (files_properties[i]) ;
		strcpy ( ((file_info * ) tmp_mmapped_file ) -> file_name_user_group , (files_properties[i])-> file_name_user_group);
		tmp_mmapped_file += (files_properties[i])->name_lenght ;
		
	}
	first_msg.mtype = HELLO;
	first_msg.file_lenght = file_size + sizeof (int);
	if (msgsnd (msg_id, &first_msg , sizeof (size_t) , 0) < 0) {
		msgctl (msg_id , IPC_RMID , NULL );
		my_error ("Can not send a message!\n\0");
	}

	//decrease_sem (sem_id);

	if ( msgrcv (msg_id , &first_msg, 0 , READY , 0) < 0)
		my_error ("Can't recieve a message!\n\0");

	if ( msgctl ( msg_id , IPC_RMID , NULL ) < 0 )
		my_error ("Can not delete message!\n\0");

	if ( closedir (destination_directory) != 0)
		my_error ("Can not close directory!\n\0");
	if ( munmap ( mmapped_file , file_size + sizeof (int) ) < 0 )
		my_error ("Error when unmapping!\n\0");
	for (i=0 ; i < number_of_files ; ++i)
		free (files_properties[i]);
	free_all_pointers ();
	return 0;
}

void add_pointer (void* new_pointer) {
	count_of_alocated_pointers ++;
	if (array_of_alocated_pointers == NULL )
		array_of_alocated_pointers = (void**) malloc (sizeof (void*) );
	else 
		array_of_alocated_pointers = (void**) realloc (array_of_alocated_pointers , count_of_alocated_pointers * sizeof (void*) );
	array_of_alocated_pointers [count_of_alocated_pointers - 1] = new_pointer;
	return;
}

void my_error (char* message) {
	int i;
	write (STDERR_FILENO, message, strlen (message));
	for (i = 0; i < count_of_alocated_pointers; ++i)
		free (array_of_alocated_pointers [i]);
	free (array_of_alocated_pointers);
	exit (-1);
}

void free_all_pointers () {
	int i;
	for (i = 0; i < count_of_alocated_pointers; ++i)
		free (array_of_alocated_pointers [i]);
	free (array_of_alocated_pointers);
	return;
}

int get_list_of_files ( DIR* directory ) {
	struct dirent * new_record = NULL;
	int result = 0 ;
	
	while ( ( new_record = readdir ( directory ) ) != NULL) {
		++result;
		if ( list_of_files == NULL)
			list_of_files = (char**) malloc ( sizeof (char*) );
		else
			list_of_files = (char**) realloc ( list_of_files , sizeof (char*) * result);
		
		list_of_files [result -1 ] = malloc ( sizeof (char) * ( strlen ( new_record -> d_name) +1 ) );
		add_pointer (list_of_files [result -1 ]);
		strcpy ( list_of_files [result -1 ] ,  new_record -> d_name );
	}
	if (errno == EBADF)
		my_error ("FATAL ERROR!!!11\n\0");
	return result;
}

void get_properties (int number_of_files , file_info ** files_properties, char ** argv) {
	struct stat * tmp_file_info = NULL;
	int i, string_size = 0 ;
	struct passwd * tmp_user_info = NULL; //must not free this!
	struct group * tmp_group_info = NULL; //must not free this!
	char * user_name = NULL, * group_name = NULL , *tmp_file_name = NULL , *tmp_perm = NULL;
	size_t total_size = 0;
	
	tmp_file_info = (struct stat *) malloc ( sizeof (struct stat) );

	for ( i = 0 ; i < number_of_files ; ++i) {
		total_size = 0;
		tmp_file_name = (char*) malloc (sizeof (char) * ( strlen (argv[1]) + strlen (list_of_files[i]) + 1) );
		strcpy ( tmp_file_name , argv[1] );
		if ( argv[1][strlen(argv[1])-1] != '/') {
			tmp_file_name = (char*) realloc (tmp_file_name , sizeof (char) * ( strlen (argv[1]) + strlen (list_of_files[i]) + 2) );
			strcat ( tmp_file_name , "/\0" );
		}
		strcat ( tmp_file_name , list_of_files[i] );
		if ( lstat ( tmp_file_name , tmp_file_info ) != 0 ) {
			switch (errno) {
				case EACCES	: printf ("Search  permission  is  denied! "); 		break;
				case ELOOP	: printf ("Too many symbolic links! "); 		break;
				case ENOENT	: printf ("A component of path does not exist! ");	break;
				case ENOTDIR	: printf ("Path is not a directory! ");			break; 
				default		: printf ("Another error! ");
			}
			my_error ("Can not get file information!\n\0");
		}
		tmp_user_info = getpwuid ( tmp_file_info -> st_uid ); 
		if ( tmp_user_info == NULL)
			my_error ("Something goes wrong!\n\0");
		
		string_size = strlen (tmp_user_info -> pw_name) + 1;
		user_name = (char*) malloc ( sizeof(char) * string_size );
		strcpy ( user_name , tmp_user_info -> pw_name );
		
		tmp_group_info = getgrgid ( tmp_file_info -> st_gid );
		if ( tmp_group_info == NULL)
			my_error ("Something goes wrong!\n\0");

		group_name = (char*) malloc ( sizeof (char) * (strlen (tmp_group_info -> gr_name) + 1) );
		strcpy ( group_name, tmp_group_info -> gr_name);

		total_size += ( sizeof (file_info) + ( string_size + strlen(group_name) + 2 + strlen (list_of_files[i]) ) * sizeof (char) ) ; 

		files_properties[i] = (file_info *) malloc ( total_size );
		
		if ( files_properties [i] == NULL ) 
			my_error ("Malloc failed!\n\0");
		(files_properties[i])->name_lenght = total_size;
		strcpy ( (files_properties[i])->file_name_user_group , list_of_files[i]);
		strcat ( (files_properties[i])->file_name_user_group , "\t\0");
		strcat ( (files_properties[i])->file_name_user_group , user_name);
		strcat ( (files_properties[i])->file_name_user_group , "\t\0");
		strcat ( (files_properties[i])->file_name_user_group , group_name);
		strcpy ( (files_properties[i])->create_date , ctime (&(tmp_file_info -> st_ctime)) );
		strcpy ( (files_properties[i])->last_change_date , ctime (&(tmp_file_info -> st_mtime)) );
		switch ( (tmp_file_info -> st_mode) & S_IFMT) {
           		case S_IFBLK:  strcpy ( (files_properties[i])->file_type , "block device\n\0" );            break;
           		case S_IFCHR:  strcpy ( (files_properties[i])->file_type , "character device\n\0" );        break;
           		case S_IFDIR:  strcpy ( (files_properties[i])->file_type , "directory\n\0" );               break;
           		case S_IFIFO:  strcpy ( (files_properties[i])->file_type , "FIFO/pipe\n\0" );               break;
           		case S_IFLNK:  strcpy ( (files_properties[i])->file_type , "symlink\n\0" );                 break;
           		case S_IFREG:  strcpy ( (files_properties[i])->file_type , "regular file\n\0" );            break;
           		case S_IFSOCK: strcpy ( (files_properties[i])->file_type , "socket\n\0" );                  break;
          		default:       strcpy ( (files_properties[i])->file_type , "unknown?\n\0" );                break;
         	}
		(files_properties[i])->file_size = tmp_file_info -> st_size;
		tmp_perm = get_permissions (tmp_file_info -> st_mode);
		strcpy ( (files_properties[i])->file_permissions, tmp_perm );
		free (user_name);
		free (group_name);
		free (tmp_file_name);
		free (tmp_perm);
	}
	
	free ( tmp_file_info );
}

void increase_sem (int sem_id) {
	struct sembuf mybuf;	
	mybuf.sem_op = 1;
	mybuf.sem_flg = 0;
	mybuf.sem_num = 0;
	if (semop (sem_id, &mybuf, 1) < 0)
		my_error ("WTF?! Can not do an operation on semafor!\n\0");
	return;
}
void decrease_sem (int sem_id) {
	struct sembuf mybuf;
	mybuf.sem_op = -1;
	mybuf.sem_flg = 0;
	mybuf.sem_num = 0;
	if (semop (sem_id, &mybuf, 1) < 0)
		my_error ("WTF?! Can not do an operation on semafor!\n\0");
	return;
}

char * get_permissions (mode_t file_mode) {
	char* answer = NULL;
	mode_t ouner, group, others; 
	int i;

	ouner = file_mode & S_IRWXU;
	group = file_mode & S_IRWXG;
	others = file_mode & S_IRWXO;

	answer = (char*) malloc (sizeof (char) * PERMSIZE );
	for (i=0;i<PERMSIZE;++i)
		answer [i] = '-';
	answer [PERMSIZE-1] = '\0';

	if ((ouner & S_IRUSR)>0)
		answer [0] = 'r';
	if ((ouner & S_IWUSR)>0)
		answer [1] = 'w';
	if (ouner & S_IXUSR)
		answer [2] = 'x';
	if (group & S_IRGRP)
		answer [3] = 'r';
	if (group & S_IWGRP)
		answer [4] = 'w';
	if (group & S_IXGRP)
		answer [5] = 'x';
	if (others & S_IROTH)
		answer [6] = 'r';
	if (others & S_IWOTH)
		answer [7] = 'w';
	if (others & S_IXOTH)
		answer [8] = 'x';

	return answer;
}

int get_sem_id (char key_mod) {
	int sem_id;	
	key_t sem_key;

	sem_key = ftok (KEYFILE , key_mod);
	if (sem_key < 0)
		my_error ("FATALL ERROR! GENERAL FAILURE GETING KEY FOR SEMAFOR!\n\0");
	sem_id = semget (sem_key , 1 , IPC_CREAT | 0666);
	if (sem_id < 0)
		my_error ("ERROR semget! Exeting...\n\0");
	return sem_id;
}

void* get_mmap (char* path , size_t file_size) {
	int file_id;
	void* file_pointer = NULL;

	file_id = open ( path,  O_RDWR | O_CREAT | O_TRUNC , 0666 );
	if ( file_id < 0 )
		my_error ("Can not open file!\n\0");
	if ( ftruncate ( file_id , file_size ) < 0 )
		my_error ("Eroor! Can not change file size!\n\0");
	file_pointer = mmap ( NULL , file_size , PROT_READ | PROT_WRITE , MAP_SHARED , file_id , 0 );
	if ( file_pointer == MAP_FAILED )
		my_error ("Mapping failed!\n\0");
	if ( close (file_id) < 0 )
		my_error ("Error when closing file!\n\0");
	return file_pointer;

}
