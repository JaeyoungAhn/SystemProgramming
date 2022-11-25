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
	char dirname[BUFSIZ];		/* from client			*/
	strcpy(dirname, "/hello;ls;");
	printf("%s\n", dirname);
	sanitize(dirname);
	printf("%s\n", dirname);
	
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

