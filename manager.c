#include <sys/ipc.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int count_of_alocated_pointers = 0;
void ** array_of_alocated_pointers = NULL;

void my_error (char* message);

void add_pointer (void* new_pointer);

void free_all_pointers ();

int main (int argc, char **argv) {
	int fork_result;

	umask (0);	
	
	if (argc<2)
		my_error ("You should enter a directory!\n\0");	

	
	fork_result=fork();
	if (fork_result < 0)
		my_error ("Can not run fork!\n\0");
	else if (fork_result == 0) {
		execl ("./part2.out","./part2.out" ,  NULL);
		my_error ("Can not run application!\n\0");
	}
		
	

	free_all_pointers();
	
	execl ("./part1.out","./part1.out", argv [1], NULL);

	my_error ("Can not run part1!\n\0");
	
	return -1;
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
