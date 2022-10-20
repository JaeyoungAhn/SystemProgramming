/* hello4.c
 * 	purpose show how to use erase, and draw for anomation 
 */

#include <stdio.h>
#include <unistd.h>
#include <curses.h>

int main() {
	int i;
	initscr();	/* turn on curses	*/
	clear();	/* clear screen	*/
	for(i=0; i<LINES; i++) {
		move( i, i+i );
		if (i%2==1)
			standout();
		addstr("Hello, world");
		if (i%2==1)
			standend();
		refresh();
		sleep(1);
		move(i,i+i);
		addstr("            ");
	}

	endwin();	/* turn offo curses	*/

	return 0;
}
