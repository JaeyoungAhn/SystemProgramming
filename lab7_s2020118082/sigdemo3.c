/* sigdemo3.c - shows how a signal handler works.
 *			  - press Ctrl-c to check elapsed time.
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void SIG_HANDLER(int);

int elapsed = 0;

int main()
{

	signal( SIGINT, SIG_HANDLER );

	printf("you can't stop me!\n");
	while(1)
	{
		sleep(1);
		elapsed++;
		printf("haha\n");
	}

	return 0;
}

void SIG_HANDLER(int signum) {
	printf("Currently elapsed time: %d sec(s)\n", elapsed);
}
