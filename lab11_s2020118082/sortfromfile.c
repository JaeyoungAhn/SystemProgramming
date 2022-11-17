/* sortfromfile.c
 * sorts the input data and outputs
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#define oops(m,x) {	perror(m); exit(x); }

int main(int ac, char **av) {
	int thepipe[2],		/* two file descriptos */
		newfd,			/* useful for pipes */
		pid;			/* and the pid	*/

	if ( ac != 2 ){
		fprintf(stderr, "usage: sortfromfile [.txt]\n");
		exit(1);
	}
	
	printf("About to run sort with input from %s\n", av[1]);

	if(pipe(thepipe) == -1 )	/* get a pipe	*/	
			oops("Cannot get a pipe", 1);	
	/* ---------------------------------------------------- */
	/*   now we have a pipe, now let's get two processes   */

	if( (pid = fork()) == -1 )	/* get a proc	*/
		oops("Cannot fork", 2);

	/* --------------------------------------------------- */
	/*		Right Here, there are two processes			   */
	/*			parent will read from pipe				   */
	if( pid > 0 ){	/* parent will exec sort	*/
		close(thepipe[1]);	/* parent doesn't wirte to pipe	*/
		
		if( (pid = fork()) == -1 ) /* get a proc */
			oops("Cannot fork", 2);

		if( pid == 0 ) {
			if( dup2(thepipe[0], 0) == -1 )
				oops("could not redirect stdin", 3);
			close(thepipe[0]);	/* stdin is duped, close pipe	*/
			execlp("sort", "sort", NULL);
			oops("sort", 4);
			exit(1);
		}
		else {
			close(thepipe[0]);
			wait(NULL);
			signal(SIGINT, SIG_DFL);
			printf("Done running sort < %s\n", av[1]);
			exit(1);
		}
	}
		/* child execs av[1] and writes into pipe		*/
	
	close(thepipe[0]);		/* child doesn't read from pipe */
	if( dup2(thepipe[1], 1) == -1 )
		oops("could not redirect stdout", 4);

	close(thepipe[1]);	/* stdout is duped, close pipe	*/
	
	execlp("cat", "cat", av[1], 0);
	oops(av[1], 5);

	return 0;
}
