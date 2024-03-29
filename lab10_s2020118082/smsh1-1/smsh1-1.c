/** smsh1-1.c small-shell version 1-1
 **				first really useful version after prompting shell
 **				this one parses the command line into strings
 **				uses fork, exec, wait, and ignores signals
 **				supports parsing multiple commands in a line
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "smsh.h"

#define DFL_PROMPT	"> "

int main() {
	char **cmdline, *prompt, **arglist;
	int result;
	void setup();
	int length;
	int nth;
	prompt = DFL_PROMPT;
	setup();

	while( (cmdline = next_cmd(prompt, stdin, &nth)) != NULL ){
		length = nth;
		for(int i=0; i<length; i++) {
			if( (arglist = splitline(cmdline[i])) != NULL ) {
			result = execute(arglist);
			freelist(arglist);
			}
			free(cmdline[i]);
		}
	}
	free(cmdline);
	return 0;
}

void setup()
/*
 * purpose: initialize shell
 * returns: nothing, calls fatal() if trouble
 */
{
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
}

void fatal(char *s1, char *s2, int n) {
	fprintf(stderr, "Error: %s,%s\n", s1, s2);
	exit(n);
}
