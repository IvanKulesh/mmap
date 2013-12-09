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

int count_of_alocated_pointers;
void** array_of_alocated_pointers;

void add_pointer (void* new_pointer);

void my_error (char* message);

void free_all_pointers ();

void increase_sem (int sem_id);
	
void decrease_sem (int sem_id);

int get_sem_id (char key_mod);

void* get_mmap (char* path , size_t file_size);

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

	file_id = open ( path,  O_RDWR , 0666 );
	if ( file_id < 0 )
		my_error ("Can not open file!\n\0");
	file_pointer = mmap ( NULL , file_size , PROT_READ | PROT_WRITE , MAP_SHARED , file_id , 0 );
	if ( file_pointer == MAP_FAILED )
		my_error ("Mapping failed!\n\0");
	if ( close (file_id) < 0 )
		my_error ("Error when closing file!\n\0");
	return file_pointer;

}


