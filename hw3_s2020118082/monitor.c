#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <math.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/wait.h>
/*
 * Jaeyoung Ahn 2020118082
 * SCSE Kyungpook National Universiy
 *
 * System Programming HW3
 * 
 * Monitoring System Resources
 * 		|
 * 		|
 * 		|
 * CPU usage + Memory Usage + Listening Sockets
 *
 * How to Use: Press 1,2,3 to change mode and press q to quit
 * 
 * mode 1 : show cpu usage and mem usage order by cpu usage descending
 * mode 2 : show cpu usage and mem usage order by mem usage descending
 * mode 3 : show listening sockets
 *
 * assume the system running the program has 4 core cpu
 *
 *
 * How to Calculate Usage:
 *
 * In order to find how much CPU usage of each process takes up,
 * we first need to know what the global variable 'jiffies' is.
 * 
 * The global variable 'jiffies' holds the number of ticks that have occurred since the system booted.
 *
 * In /proc/stat, the data tells us total cumulative CPU time in units of jiffies, and
 * in /proc/[pid]/stat, the data tells us that of each process in units of jiffies as utime and stime.
 * 
 * utime is a user mode jiffies and stime is a kernel mode jiffies.
 *
 * We first need to measure jiffies for /proc/stat and /proc/[pid]/stat
 * and sleep for certain period of time. Then we need to measure it again.
 *
 * This can be expressed as the formula below.
 *
 * user_usage = 100 * (utime_after - utime_before) / (total_time_after - total_time_before)
 * sys_usage = 100 * (stime_after - stime_before) / (total_time_after - total_time_before)
 *
 * sum of user_usage and sys_usage equals to percentage of cpu usage each process takes up.
 *
 */
struct data {
	int pid; // proncess number
	char filename[30]; // process name
	double usage; // CPU usage in percentage
	int total_memory; // VIRT
	int resident_memory; // RES
	int shared_pages; // SHR
};


struct termios initial_settings, new_settings; // for disabling canonical and echo mode

int PAGELEN2=0; // for assining terminal height

int main(void) {
	int cmpfunc(const void *a, const void *b); // quick sorting order by CPU usage
	int cmpfunc2(const void *a, const void *b); // quick sorting order by MEM usage

	struct data *d;
	struct data d_init[100000];
	double user_usage;
	double sys_usage;
	long long utime_before[100000];
	long long stime_before[100000];
	unsigned long total_time_after;
	unsigned long total_time_before;
	
	int pid;
	char filename[100];
	char state;
	int ppid;
	int trash1[4];
	unsigned int trash2;
	unsigned long trash3[4];
	unsigned long usertime;
	unsigned long systemtime;

	unsigned long total_memory;
	unsigned long resident_memory;
	unsigned long shared_pages;
	unsigned long entire_memory_size;

	char str[1000];
	char str_memory[1000];
	char str2[300];
	char number[8];
	char cpu_name[5];
	int idx;
	int idx2;
	int idx3;
	char temp[30];
	double temp2; 
	unsigned long temp3;

	FILE* eachProcess;
	FILE* total;
	FILE* eachMemory;
	FILE* memory_entire;
	unsigned long total_values[9];
	int j;
	int out_flag=0;
	double memory_percentage;

	struct winsize wbuf;
	int PAGELEN = 0;
	int sort_flag = 0;
	int n;
	int on_switch=1;

	unsigned char key;

	/* disabling canonical and echo mode and make the program nonblcoking */
	new_settings = initial_settings;

	tcgetattr(0, &initial_settings);
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 0;
	new_settings.c_cc[VTIME] = 0;

	tcsetattr(0, TCSANOW, &new_settings);
	

	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);


	while(1) { // start of outer while
	while(1) { // start of innre while


		if(on_switch==0) break; // if not working as intended, just break the program
		char buf[20];
		int numRead;
		initscr(); // start curses
		idx=0;
		idx2=0;
		idx3=0;
		j=1;
		temp2=0;
		clear(); // curses erase
		move(0, 0);
		standout(); // curses make letters fancy
		sprintf(str2, "  PID\t\tCOMMAND\t   %%CPU\t    VIRT     RES     SHR     %%MEM  \n");	
	
		addstr(str2); // curses show string onto screen
		standend(); // curses off making letters fancy

		total = fopen("/proc/stat", "r"); // where we can get total CPU jiffies info
						  
		/* parse it */
		fscanf(total, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu", cpu_name,
				&total_values[0], &total_values[1], &total_values[2],
				&total_values[3], &total_values[4], &total_values[5],
				&total_values[6], &total_values[7], &total_values[8]);
		total_time_before=0;
		for(int i=0; i<9; i++) {
			total_time_before+=total_values[i]; // this is the current total jiffies
		}
		total_time_before/=4; // assuming that we are running on 4 core cpu
		fclose(total);

		for(int i=0; i<100000; i++) { // to find out valid process id

			memset(str, 0, 1000*sizeof(char));
			memset(number, 0, 8*sizeof(char));
			strcpy(str, "/proc/");
			sprintf(number, "%d", i);
			strcat(str, number);
			strcat(str, "/stat");
	
			if( access(str, F_OK) != 0 ) // if /proc/[i]/stat isn't accessible
				continue;

			eachProcess = fopen(str, "r"); // if accessible, open it
			/* just throw away some variables that won't be used in the future */
			fscanf(eachProcess, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu",
					&pid, filename, &state, &ppid, &trash1[0], &trash1[1], &trash1[2],
					&trash1[3], &trash2, &trash3[0], &trash3[1], &trash3[2], &trash3[3],
					&usertime, &systemtime); 
			d_init[idx].pid = pid;

			utime_before[idx] = usertime;
			stime_before[idx++] = systemtime;
			fclose(eachProcess);	
		}
		d = (struct data*)malloc(idx * sizeof(struct data));
		
		sleep(1); // new measurement below after one second!
		
		/* the procedure are similar */
		total = fopen("/proc/stat", "r");
		
		fscanf(total, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu", cpu_name,
			&total_values[0], &total_values[1], &total_values[2],
			&total_values[3], &total_values[4], &total_values[5],
			&total_values[6], &total_values[7], &total_values[8]);
		
		total_time_after=0;
		for(int i=0; i<9; i++) {
			total_time_after+=total_values[i];
		}
		total_time_after/=4;

		memory_entire = fopen("/proc/meminfo", "r");
		fscanf(memory_entire, "%s %lu", filename, &entire_memory_size);

		fclose(memory_entire);
		fclose(total);
		for(int i=0; i<idx; i++) {
			memset(str, 0, 1000*sizeof(char));
			memset(number, 0, 8*sizeof(char));
			memset(str_memory, 0, 1000*sizeof(char)); 
			strcpy(str, "/proc/");
			sprintf(number, "%d", d_init[i].pid);
			strcat(str, number);
			strcpy(str_memory, str);
			strcat(str, "/stat");
			strcat(str_memory, "/statm");
	
			if( access(str, F_OK) != 0 )
				continue;

			eachMemory = fopen(str_memory, "r");

			eachProcess = fopen(str, "r");
			fscanf(eachProcess, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu",
					&pid, filename, &state, &ppid, &trash1[0], &trash1[1], &trash1[2],
					&trash1[3], &trash2, &trash3[0], &trash3[1], &trash3[2], &trash3[3],
					&usertime, &systemtime);

			fscanf(eachMemory, "%d %d %d", &d[idx2].total_memory, &d[idx2].resident_memory, &d[idx2].shared_pages);			

			/* doing calculation */
			user_usage=100*(double)(usertime - utime_before[idx2]) / (double)(total_time_after - total_time_before);
			sys_usage=100*(double)(systemtime - stime_before[idx2]) / (double)(total_time_after - total_time_before);

			d[idx2].pid = pid;
			strcpy(d[idx2].filename, filename);
			d[idx2++].usage = user_usage+sys_usage;
			
			fclose(eachMemory);
			fclose(eachProcess);		
		}
			
			if(sort_flag==0) {
				qsort((struct data*)d, idx2, sizeof(d[0]), cmpfunc);
			} else if(sort_flag==1) {
				qsort((struct data*)d, idx2, sizeof(d[0]), cmpfunc2);
			}

			/* show the reuslt on screen neatly */
			if (ioctl(0, TIOCGWINSZ, &wbuf) != -1 ) PAGELEN = wbuf.ws_row-2;
			PAGELEN2 = PAGELEN;
			while(PAGELEN) {
				if(d[idx3].usage > 100) {
					idx3++;       
					continue;
				}
				if(temp != NULL && !strncmp(temp, d[idx3].filename, 5)) {
					idx3++;
				       	continue;
				}
				if(temp2 != 0 && fabs(temp2-d[idx3].usage) < 0.001) {
					idx3++;
					continue;
				}
				if(temp3 != 0 && fabs(temp3-d[idx3].resident_memory) < 0.001) {
					idx3++;
					continue;	
				}

				PAGELEN--;
				move(j++ , 0);

				memory_percentage = 100*4*(d[idx3].resident_memory)/(double)entire_memory_size;
				if(d[idx3].usage <= 10.0 && memory_percentage <= 10.0) {
					sprintf(str2, "%5d	%15.15s  0%.2lf%% %8d %7d %7d   0%.2lf%%\n", d[idx3].pid, d[idx3].filename ,d[idx3].usage, (d[idx3].total_memory)*4, (d[idx3].resident_memory)*4, (d[idx3].shared_pages)*4, memory_percentage);
				}
				else if(d[idx3].usage <= 10.0) {
					sprintf(str2, "%5d	%15.15s  0%.2lf%% %8d %7d %7d   %.2lf%%\n", d[idx3].pid, d[idx3].filename ,d[idx3].usage, (d[idx3].total_memory)*4, (d[idx3].resident_memory)*4, (d[idx3].shared_pages)*4, memory_percentage);
	
				}
				else if(memory_percentage <= 10.0) {
					sprintf(str2, "%5d	%15.15s  %.2lf%% %8d %7d %7d   0%.2lf%%\n", d[idx3].pid, d[idx3].filename ,d[idx3].usage, (d[idx3].total_memory)*4, (d[idx3].resident_memory)*4, (d[idx3].shared_pages)*4, memory_percentage);
				}
				else {
					sprintf(str2, "%5d	%15.15s  %.2lf%% %8d %7d %7d   %.2lf%%\n", d[idx3].pid, d[idx3].filename ,d[idx3].usage, (d[idx3].total_memory)*4, (d[idx3].resident_memory)*4, (d[idx3].shared_pages)*4, memory_percentage);
				}

				if( memory_percentage >= 15.0 || d[idx3].usage >= 30.0) {
					for(int i=0; i<strlen(str2); i++)
						addch(str2[i]|A_BOLD);
				}
				else{
					addstr(str2);
				}

				refresh();
				strcpy(temp, d[idx3].filename);	
				temp2 = d[idx3].usage;	
				temp3 = d[idx3].resident_memory;
				idx3++;
						
			}
			free(d);

	memset(buf, 0, 20*sizeof(char));
	numRead = read(0, buf, 4);
	if (numRead > 0) { // nonblocking input
		if(buf[0] == 'q') {
			out_flag=1;
			break;
		}
		else if(buf[0] == '1') {
			sort_flag = 0;
		}
		else if(buf[0] == '2') {
			sort_flag = 1;
		}
		else if(buf[0] == '3') {
			break;
		}

	}
	 

	} // end of inner while
	on_switch=0;
	move(0,0);

	if(out_flag==1) break;
	char buf;
	void be_receiver(int *);
	void be_sender(int *);
	int from_sender[2];

	pipe(from_sender);

	pid = fork();

	if(pid != 0) {
		be_receiver(from_sender); // receive char* from sender
		wait(NULL);
	}
	else {
		be_sender(from_sender); // send to receiver the string obtained from execlp
	}

	while(1) { // nonblocking input
		buf = getch();
		if(buf == 'q') {
			out_flag=1;
			on_switch=1;
			break;
		}
		else if(buf == '1') {
			sort_flag = 0;
			on_switch=1;
			break;
		}
		else if(buf == '2') {
			sort_flag = 1;
			on_switch=1;
			break;
		}
	}
		
	if(out_flag==1) break;

	} // end of outer while 
	endwin();
	tcsetattr(0,TCSANOW, &initial_settings); // back to original setting
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | ~O_NONBLOCK); // back to original setting
	return 0;
} // end of main

int cmpfunc(const void *a, const void *b) {
	return ((struct data*)b)->usage - ((struct data*)a)->usage;
}

int cmpfunc2(const void *a, const void *b) {
	return ((struct data*)b)->resident_memory - ((struct data*)a)->resident_memory;
}

void be_receiver(int from_sender[2]) {
	int i=0;
	clear();
	int num1, num2;
	char operation[BUFSIZ], message[BUFSIZ], *fgets();
	FILE *fpin, *fdopen();

	close(from_sender[1]);
	fpin = fdopen(from_sender[0], "r");
	
	char message2[BUFSIZ]="temp";
	while(PAGELEN2--) {		
		fgets(message,BUFSIZ,fpin);
		if(!strcmp(message, message2))
			break;
		move(i++,0);
		addstr(message);
		refresh();
		strcpy(message2, message);
	}


	fclose(fpin);
}

void be_sender(int out[2]) {
	dup2(out[1],1);
	close(out[1]);
	close(out[0]);

	execlp("netstat", "netstat", "-tnl", NULL);

}
