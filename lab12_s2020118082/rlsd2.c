/* rlsd.c - a remote ls server
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#define PORTNUM 15000 /* our remote ls server port */
#define HOSTLEN 256
#define oops(msg) { perror(msg); exit(1); }

void sanitize(char *);

int main(int ac, char *av[]) {
	struct sockaddr_in saddr; /* build our address here */
	struct hostent		 *hp; /* this is part of our */
	char hostname[HOSTLEN];   /* address			*/
	int sock_id, sock_fd;	  /* line id, file desc	*/
	FILE *sock_fpi, *sock_fpo; /* streams for in and out */
	FILE *pipe_fp;				/* use popen to run ls	*/
	char dirname[BUFSIZ];		/* from client			*/
	char command[BUFSIZ];		/* for popen()			*/
	char destination[BUFSIZ];
	int dirlen, c;
	int pipes[2];
	int pid;

	/** Step 1: ask kernel for a socket **/

	sock_id = socket(PF_INET, SOCK_STREAM, 0);	/* get a socket	*/
	if( sock_id == -1 )
		oops("socket");

	/** Step 2: bind address to socket. Address is host, port **/

	bzero((void *)&saddr, sizeof(saddr)); /* clear out struct	*/
	gethostname(hostname, HOSTLEN);	/* where am I ?	*/
	hp = gethostbyname(hostname);	/* get info about host */
	bcopy((void *)hp->h_addr, (void *)&saddr.sin_addr, hp->h_length);
	saddr.sin_port = htons(PORTNUM);		/* fill in socket port */
	saddr.sin_family = AF_INET;				/* fill in addr family	*/
	if(bind(sock_id, (struct sockaddr *)&saddr, sizeof(saddr)) != 0)
		oops("bind");

	/** Step 3:	allow incoming calls with Qsize=1 on socket **/

	if( listen(sock_id, 1) != 0 )
		oops("listen");

	/*
	 * main loop: accept(), write(), close()
	 */

	while(1) {
		sock_fd = accept(sock_id, NULL, NULL); /* wait for call */
		if(sock_fd == -1)
			oops("accept");

		/* open reading direction as buffered stream */
		if( (sock_fpi = fdopen(sock_fd, "r")) == NULL )
			oops("fdopen reading");

		if(fgets(dirname, BUFSIZ-5, sock_fpi) == NULL)
			oops("reading dirname");
		sanitize(dirname);
		/* open writing direction as buffered stream */
		if( (sock_fpo = fdopen(sock_fd, "w")) == NULL )
			oops("fdopen writing");

		sprintf(command, "ls %s", dirname);
		if(pipe(pipes) == -1)
			oops("pipe failed");

		if( (pid = fork()) == -1 )
			oops("fork");

		if(pid >0) { // parent
			close(pipes[1]);
			pipe_fp = fdopen(pipes[0], "r");
			if(pipe_fp == NULL)
				oops("Error converting pipes to streams");
			while( (c = getc(pipe_fp)) != EOF )
				putc(c, sock_fpo);
		}
		else { // child
			if(dup2(pipes[1],1) == -1)
			close(pipes[0]);
			close(pipes[1]);
			strcpy(destination, dirname);
			execlp("ls", "ls", destination, NULL);
			oops("Cannot run ls");
		}
	//	if( (pipe_fp = popen(command,"r")) == NULL )
	//		oops("popen");

		/* transfer data from ls to socket */
		pclose(pipe_fp);
		fclose(sock_fpo);
		fclose(sock_fpi);
	}
}

void sanitize(char *str)
/*
 * it would be very bad if someone passed us an dirname like
 * "; rm *" and we naively created a command "ls; rm *"
 *
 * so.. we remove everything but slashes and alphanumerics
 * There are nicer solutions, see exercises
 */
{
	char *src, *dest;

	for(src = dest = str; *src; src++)
		if( *src == '/' || isalnum(*src) )
			*dest++ = *src;
	*dest = '\0';
}
