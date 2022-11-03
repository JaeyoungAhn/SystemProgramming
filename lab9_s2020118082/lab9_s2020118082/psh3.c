/** prompting shell version 3
 **
 **		Solves the 'one-shot' problem of version 1
 			Uses execvp(), but fork()s first so that the
			shell waits around to perform another command
		ends the program when pressed Ctrl+D(EOF) or typed "exit"
 **/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXARGS 20
#define ARGLEN 100

void execute( char ** );
char * makestring( char * );

int main() {
	char *arglist[MAXARGS+1];	/* an array of ptrs */
	int numargs;				/* index into array	*/
	char argbuf[ARGLEN];		/* read stuff here	*/
	char *makestring();			/* malloc etc		*/
	const char* finish = "exit";
	char* rtn_value;
	numargs = 0;


	while( numargs < MAXARGS ) {
		printf("Arg[%d]? ", numargs);
		if ( ((rtn_value = fgets(argbuf, ARGLEN, stdin))!=NULL) && *argbuf != '\n')
		{
			arglist[numargs++] = makestring(argbuf);
			if( !strcmp(arglist[numargs-1], finish) ) exit(1); // in case of exit
		}
		else {
			if(rtn_value==NULL) { // in case of EOF
				printf("\n");	
				exit(1);
			}
			else if(numargs > 0){	/* any args?	*/
				arglist[numargs]=NULL;	/* close list	*/
				execute( arglist );		/* do it		*/
				numargs = 0;			/* and reset	*/	
			}
		}
	}
	return 0;
}

void execute( char *arglist[] )
/*
 * use fork and execvp and wait to do it
 */
{
	int pid,exitstatus;	/* of child	*/

	pid = fork();		/* make new process	*/
	switch(pid) {
		case -1:
			perror("fork failed");
			exit(1);
		case 0:
			execvp(arglist[0], arglist);	/* do it */
			perror("execvp failed");
			exit(1);
		default:
			while( wait(&exitstatus) != pid )
				printf("child exited with status %d, %d\n",
						exitstatus>>8, exitstatus&0377);
	}
}

char * makestring( char *buf )
/*
 * trim off newline and create storage for the string
 */
{
	char *cp;

	buf[strlen(buf)-1] = '\0';	/* trim newline */
	cp = malloc( strlen(buf)+1 );	/* get memory	*/
	if ( cp == NULL ){
		fprintf(stderr, "no memory\n");
		exit(1);
	}
	strcpy(cp, buf);	/* copy chars	*/
	return cp;		/* return ptr	*/
}

