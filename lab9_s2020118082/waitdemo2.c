/* waitdemo2.c - shows how parent pauses until child finishes
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define DELAY 10

int main()
{
	int newpid;
	void child_code(), parent_code();

	printf("before: mypid is %d\n", getpid());

	if( (newpid = fork()) == -1 )
		perror("fork");
	else if ( newpid == 0 )
		child_code(DELAY);
	else
		parent_code(newpid);

	return 0;
}

/*
 * new process takes a nap and then exits
 */

void child_code(int delay) {
	printf("chlid %d here. will sleep for %d seconds\n", getpid(), delay);
	sleep(delay);
	printf("child done. about to exit\n");
	exit(17);
}
/*
 * parent waits for child then prints a message
 */
void parent_code(int childpid)
{
	int wait_rv;		/* return value from wait() */
	int child_status;
	int high_8, low_7, bit_7;

	wait_rv = wait(&child_status);
	printf("done waiting for %d. Wait returned: %d\n", childpid, wait_rv);

	int mask;
	printf("Child status: ");
	for(int i=15; i>=0; i--)
	{
		mask = 1 << i;
		printf("%d", child_status & mask ? 1 : 0);
		
		if ( i % 8 == 0 )
			printf(" ");
	}
	printf("\n");

	high_8 = child_status >> 8;	/* 1111 1111 0000 0000 */
	low_7 = child_status & 0x7F;	/* 0000 0000 0111 1111 */
	bit_7 = child_status & 0x80;	/* 0000 0000 1000 0000 */
	printf("status: exit=%d, sig=%d, core=%d\n", high_8, low_7, bit_7);
}
