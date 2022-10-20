/* hello1.c
 * 	purpose show the minimal calls needed to use curses
 * 	outline initialize, draw stuff, wait for input, quit
 */

#include <unistd.h>
#include <stdio.h>
#include <curses.h>

int main() {
	initscr();	/* turn on curses	*/
	int on_off = 0;

			/* send requests 	*/
	clear();	/* clear screen	*/
	while(1) {
		move((LINES-1)/2, ((COLS-1)/2)-6);	/* locate it at the center */
		if(on_off==0) {
			on_off=1;
			standout();
		}
		else if (on_off==1) {
			on_off=0;
			standend();
		}
		addstr("Hello, world");		/* add a string	*/
		move(LINES-1,0);		/* move to LL	*/
		refresh();	/* update the screen	*/
		sleep(1);
	}
	endwin();	/* turn off curses	*/

	return 0;
}
