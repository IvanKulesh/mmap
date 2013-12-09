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
#include "commonmmap.h"



int count_of_alocated_pointers = 0;
void** array_of_alocated_pointers = NULL;



int main () {
	int  msg_id , i;
	key_t msg_key;
	size_t file_size;
	void * mmapped_file = NULL; 
	void *tmp_mmapped_file = NULL;
	int number_of_files;
	//file_info * files_properties = NULL;
	struct first_msgbuf {
		long mtype;
		size_t file_lenght;
	} first_msg;
	
	umask (0);
	msg_key = ftok (KEYFILE , 1);
	if (msg_key < 0)
		my_error ("ERROR WHEN CALL FTOK!\n\0");
	
	msg_id = msgget (msg_key, IPC_CREAT | 0666);
	if (msg_id < 0)
		my_error ("ERROR WHEN CALL MSGGET!\n\0");

	if ( msgrcv (msg_id , &first_msg, sizeof (size_t) , HELLO , 0) < 0)
		my_error ("Can't recieve a message!\n\0");
	
	file_size = first_msg.file_lenght ;

	mmapped_file = get_mmap (RESULTFILE , file_size );
	tmp_mmapped_file = mmapped_file;
	number_of_files = * ((int * ) tmp_mmapped_file );
	tmp_mmapped_file += sizeof (int);
	//add_pointer ( files_properties );
	//iles_properties = (file_info *) malloc (sizeof (file_info) * number_of_files);
	
	printf ("total %i files \n", number_of_files);

	for ( i=0 ; i < number_of_files ; ++i) {
		//printf ("Still here!\n");
		//printf ("In cycle!\n");
		//files_properties [i] = * ( (file_info *) tmp_mmapped_file);
		printf ("\nfile name\tuser owner\tgroup owner\n");
		printf ("%s\n",((file_info *)tmp_mmapped_file)->file_name_user_group);
		printf ("Create date\t%s",((file_info *)tmp_mmapped_file)->create_date);
		printf ("Last mod date\t%s",((file_info *)tmp_mmapped_file)->last_change_date );
		printf ("Permissions\t%s\n",((file_info *)tmp_mmapped_file)->file_permissions);
		printf ("File type\t%s",((file_info *)tmp_mmapped_file)->file_type );
		printf ("File size\t%i\n",(int)((file_info *)tmp_mmapped_file)->file_size );
		tmp_mmapped_file += ((file_info *)tmp_mmapped_file)->name_lenght ;
		
	}
	if ( munmap ( mmapped_file , file_size  ) < 0 )
		my_error ("Error when unmapping!\n\0");	

	first_msg.mtype = READY;
	if (msgsnd (msg_id, &first_msg , 0 , 0) < 0) {
		msgctl (msg_id , IPC_RMID , NULL );
		my_error ("Can not send a message!\n\0");
	}

	//increase_sem (sem_id);
	free_all_pointers ();
	return 0;
}


