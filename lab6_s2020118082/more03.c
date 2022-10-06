/* more03.c - version 0.3 of more
 * 	read and print lines up to length of terminal row then pause for a few special commands
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#define LINELEN 512

void do_more(FILE *);
int see_more();

int main( int ac , char *av[] )
{
	FILE *fp;

	if ( ac == 1 )
		do_more( stdin );
	else
		while ( -- ac)
			if ( (fp = fopen( *++av , "r")) != NULL )
			{
				do_more( fp );
				fclose( fp );
			}
			else
				exit(1);
	return 0;
}

void do_more( FILE *fp)
/*
 * read lines up to length of rows in terminal, then call see_more() for further instructions
 */
{
	char line[LINELEN];
	int num_of_lines = 0;
	int see_more(), reply;
	struct winsize wbuf;
	int PAGELEN = 0;

	if( ioctl(0, TIOCGWINSZ, &wbuf) != -1 ) PAGELEN = wbuf.ws_row;
	
	while ( fgets( line, LINELEN, fp) ) {		/* more input	*/
		if ( num_of_lines == PAGELEN ) {	/* full screen? */
			reply = see_more(PAGELEN);		/* y: ask user */
			if ( reply == 0 )		/*	n: done	*/
				break;
			num_of_lines -= reply;		/* reset count	*/
		}
		if ( fputs( line, stdout ) == EOF )	/* show line 	*/
			exit(1);			/* or die	*/
		num_of_lines++;				/* count it	*/
	}
}

int see_more(int PAGELEN)
/*
 * print message, wait for response, return # of lines to advance
 * q means no, space means yes, CR means one line
 */
{
	int c;
	
	printf("\033[7m more? \033[m");		/* revser on a vt100	*/
	while( (c=getchar()) != EOF )		/* get reponse */
	{
		if ( c == 'q' )			/* q -> N	*/
			return 0;
		if ( c == ' ' )			/* ' '=> next page	*/
			return PAGELEN;		/* how many to show	*/
		if ( c == '\n' )		/* Enter key => 1 line	*/
			return 1;	
	}
	return 0;
}


