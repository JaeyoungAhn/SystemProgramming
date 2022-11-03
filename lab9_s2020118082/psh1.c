/*		prompting shell version 1
			Prompts for the command and its arguments.
			Builds the argument vector for the call to execvp.
			Uses execvp(), and never returns.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define MAXARGS	20		/* cmdline args */
#define ARGLEN 100		/* token length */

int execute( char ** );
char * makestring( char * );

int main() {
	char *arglist[MAXARGS+1];	/* an array of ptrs */
	int numargs;				/* index into array	*/
	char argbuf[ARGLEN];		/* read stuff here	*/
	char *makestring();			/* malloc etc		*/

	numargs = 0;
	while( numargs < MAXARGS ) {
		printf("Args[%d]? ", numargs);
		if ( fgets(argbuf, ARGLEN, stdin) && *argbuf != '\n')
			arglist[numargs++] = makestring(argbuf);
		else {
			if( numargs > 0){	/* any args?	*/
				arglist[numargs]=NULL;	/* close list	*/
				execute( arglist );		/* do it		*/
				numargs = 0;			/* and reset	*/	
			}
		}
	}
	return 0;
}
