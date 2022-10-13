/* play_again1.c
 *	purpose: ask if user wants another transaction
 *	method: set tty into char-by-char mode, read char, return result
 *	returns: 0=>yes, 1=>no
 *	better: do no echo inappropriate input
*/

#include <stdio.h>
#include <termios.h>

#define QUESTION	"Do you want another transaction"

int get_response( char * );
void set_crmode();
int tty_mode(int);

int main() {
	int response;

	response = get_response(QUESTION);	/* get some answer */
	return response;
}

int get_response(char *question)
/*
 * purpose: ask a question and wait for a y/n answer
 * method: use getchar and ignore non y/n answers
 * returns: 0=>yes, 1=>no
 */
{
	printf("%s (y/n)?", question);
	while(1) {
		switch( getchar() ) {
			case 'y':
			case 'Y': return 0;
			case 'n':
			case 'N':
			case EOF: return 1;
		}
	}
}
