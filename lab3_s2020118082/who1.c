/* who1.c - read /etc/utmp and list info therein
 *		  - does not suppress empty records
 *		  - does not format time nicely
 */
#include <stdio.h>
#include <unistd.h>
#include <utmp.h>
#include <fcntl.h>
#include <stdlib.h>

#define SHOWHOST

void show_info(struct utmp *);

int main() {
	struct utmp current_record;		/* read info into here */
	int	utmpfd;		/* read from this descriptor */
	int reclen = sizeof(current_record);
	
	if( (utmpfd = open(UTMP_FILE, O_RDONLY)) == -1) {
		perror(UTMP_FILE);
		exit(1);
	}

	while( read(utmpfd, &current_record, reclen) == reclen )
		show_info( &current_record );
	close(utmpfd);
	return 0;
}

/*
 *		show info()
 *						displays the contents of the utmp struct
 *						in human readable form
 *						* displays nothing if records has no user name
 */
void show_info( struct utmp *utbufp ) {

	printf("%-8.8s", utbufp->ut_name);		/* the longname */
	printf(" ");							/* a space		*/
	printf("%-8.8s", utbufp->ut_line);		/* the tty		*/
	printf(" ");							/* a space		*/
	printf("%10d", utbufp->ut_time);
	printf(" ");
	#ifdef SHOWHOST
	printf("(%s)", utbufp->ut_host);		/* the host	*/
	#endif
	printf("\n");						/* newline	*/
}

