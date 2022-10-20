/* hello2.c
 * 	purpose show how to use curses funcions with a loop
 * 	outline initialize, draw stuff, wrap up
 */

#include <unistd.h>
#include <curses.h>

int main() {
	int i;
	initscr();	/* turn on curses	*/
	
			/* send requests 	*/
	clear();	/* clear screen	*/
	for(i=0; i<LINES; i++) {
		move( i, i+i );
		if (i%2==1)
			standout();
		addstr("Hello, world");
		if (i%2==1)
			standend();
	}

	refresh();	/* update the screen	*/
	sleep(3);
	endwin();	/* turn offo curses	*/

	return 0;
}
