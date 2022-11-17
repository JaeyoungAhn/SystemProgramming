/* whotofile2.c
 *	purpose: show how to redirect output for another program
 *	   idea: fork, then in the child, redirect output, then exec
 *	   note: it will append the output instead of overriding.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

int main() {
	int pid;
	int fd;

	printf("About to run who into a file\n");

	/* create a new process or quit	*/
	if( (pid = fork() ) == -1 ) {
		perror("fork"); exit(1);
	}
	/* child does the work */
	if ( pid == 0 ) {
		close(1);					/* close, */
		if( access( "userlist", F_OK ) != -1) { // file exists
			fd = open("userlist", O_WRONLY|O_APPEND);
		} else { // file doesn't exist
			fd = creat("userlist", 0644);
		}
		execlp("who", "who", NULL);		/* and run	*/
		perror("execlp");
		exit(1);
	} 
	/* parent waits then reports */
	if (pid != 0 ){
		wait(NULL);
		printf("Done running who. results in userlist\n");
	}
	
	return 0;
}
