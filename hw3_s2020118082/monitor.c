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
 * CPU Usage + Memory Usage + Listening Sockets
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
 * 제가 만든 프로그램은 프로세스 별 CPU 사용량과 Memory 용량, 그리고 listening중인 socekt을 보여주는 프로그램입니다.
 * 제가 다른 리소스 중에서도 listening중인 tcp의 ip 및 포트를 보여줘야겠다고 생각한 이유는 평소에 웹 백엔드에 대해 관심이 많고
 * 서버를 구현하고 테스트하는 과정에서 서버를 종료하고 다시 가동시키는 작업이 반복되는 경우가 많은데,
 * 서버를 종료했더라도 사용중이던 포트가 정상적으로 풀리지 않는 경우가 많아서 같은 포트 번호를 사용하지 못하는 경우가 종종 있었습니다.
 * 그래서 서버를 가동 중일때 하나의 모니터 프로그램으로 CPU와 메모리 상태를 통해 시스템이 안정적인지 확인함과 동시에 현재 listening중인 포트들은 무엇들이 있는지,
 * 이러한 포트들이 종료되도록 의도되었으나 실제로 종료되지 않았는지 확인하기 위한 프로그램을 만들었습니다.
 *
 * 시스템프로그래밍 수업에서 배운 여러가지 개념들을 어떻게 프로그램에 녹여낼 수 있을까에 대한 고민을 먼저 했고,
 * 수업시간에 배운 내용들만으로도 충분히 생각하고 있는 프로그램을 구현할 수 있음을 깨달았습니다.
 * 각각의 프로세스를 하나의 파일로써 취급하여 다루는 것, tcgetattr, tcsetattr을 이용한 struct termios를 조작하는 것,
 * struct winsize를 이용하여 현재 터미널 사이즈에 맞게 출력하는 것, fcntl()을 이용하여 NONBLOCKING 모드로 작동시키는 것,
 * curses의 initscr(), move(), standout(), standout(), endwin() 등을 이용하여 화면에 보기 좋게 출력하는 것,
 * fork를 이용하여 다른 자식 프로세스와 부모 프로세스를 운용하여 다른 하나에서 명령어를 실행한 결과를 pipe를 이용해
 * 본래 프로그램에 입력 받는 것 등 많은 개념을 사용하여 프로그램을 구현했습니다.
 *
 * 컴파일 방법은 아래와 같습니다. (-lcurses 옵션을 주어야함)
 * 프로그램 작성 당시 시스템 환경인 CPU 4코어를 가정하고 프로그램을 작성하였습니다.
 * gcc -o monitor monitor.c -lcurses
 *
 * 사용법은 아래와 같습니다.
 * ./monitor로 현재 디렉토리에 있는 monitor 프로그램을 실행합니다.
 * 프로그램은 기본적으로 세가지 모드가 있고 이 모드들은 숫자키 1,2,3으로 전환이 가능합니다.
 * q를 눌러서 프로그램을 종료시킬 수 있습니다.
 * 기본 모드(1)는 PID-COMMAND(프로세스명)-CPU사용량-VIRT-RES-SHR-MEMORY사용량을 각 프로세스의 CPU 점유율 순으로 정렬하여 출력합니다.
 * 두번째 모드(2)는 (1)과 같은 내용을 MEMORY 점유율 순으로 정렬하여 출력합니다. 세번째 모드(3)는 listening socket들에 대한 내용을 출력합니다.
 *
 * 구현 이전에 알아야 하는 개념은 아래와 같습니다.
 * 각각의 프로세스가 얼마의 CPU 사용량을 차지하는지 계산하기 위해 제일 먼저 jiffies라는 전역 변수에 대해서 알고 있어야합니다.
 * jiffies는 시스템이 부팅한 이후에 작동된 tick수를 나타냅니다. 따라서 특정 시간 간격을 기준으로 잡고 그 기준 전후에 jiffies의 틱의 변화에 따라 CPU 사용량을 계산할 수 있습니다.
 * OS의 모드 중 user모드와 kernel모드라는 것이 있는데, 짧은 시간동안 특정 프로세스가 user모드와 kernel모드인 상태에서 상승시킨 jiffies의 합이 해당 프로세스가 사용한 jiffies 양의 되고,
 * 이를 전체 CPU가 사용한 jiffies와 비교하여 사용량 계산식을 얻을 수 있습니다.
 *
 * /proc/stat 경로의 있는 데이터들이 전체 CPU 축적 시간을 jiffies 단위로 알려주고 /proc/[pid]/stat에 있는 개별 프로세스의 데이터가
 * user mode에서 흘러간 utime과 kerel mode에서 흘러간 stime을 알려줍니다. 최종 식은 아래와 같습니다.
 *
 * 전체 흘러간 총 시간
 * total_time_after(/proc/stat 특정 시간 이후에 읽어들임) - total_time_before(/proc/stat파일 초기 측정)
 *
 * 특정 프로세스 유저모드 흘러간 총 시간
 * utime_after(/proc/[pid]/stat 특정 시간 이후 읽어들임) - utime_before(/proc/[pid]/stat 초기 측정)
 *
 * 특정 프로세스 커널모드 흘러간 총 시간
 * stime_after(/proc/[pid]/stat 특정 시간 이후 읽어들임) - stime_before(/proc/[pid]/stat 초기 측정)
 *
 * 한 프로세스의 사용량(%단위)
 * 100*(특정 프로세스 유저모드 흘러간 총 시간 + 특정 프로세스 커널모드 흘러간 총 시간)/전체 흘러간 총 시간
 *
 *
 * 프로그램에서 전역적으로 사용할 사용자정의 구조체 struct data는 아래와 같고 각각 프로세스 번호,
 * 프로세스명, 전체  메모리 사이즈 페이지 단위, resident 메모리 페이지 단위, shared 페이지입니다. 1블록 단위*페이지=용량이 나옵니다.
 * int pid, char filename[30], int total_memory, int resident_memory, int_shared_pages
 *
 * struct termios는 initial_settings와 new_settings두개를 두고 전자는 기본 셋팅으로 나중에 되돌아 오기위해 두고, 후자는 non-canonical 및 에코를 끄는 등 여러 셋팅을 해줍니다.
 * PAGELEN, PAGELEN2은 나중에 터미널 사이즈 높이를 구한 뒤 결과를 받을 변수입니다.
 *
 * 제일 우선 메인에서 cmpfunc,cmpfunc2 퀵소트 함수를 선언합니다.
 * 다음으로 여러 변수들을 선언하지만 너무 많아 아래 사용할때 어떤 변수를 쓰는지 언급 하겠습니다.
 *
 * 제일 처음 tcgetattr(0, &initial_settings);를 호출하여 기본 속성을 저장하고, new_setting에는 비트마스킹을 이용하여 ICANON,
 * ECHO, ISIG에 대하여 OFF한 것을 tcsetattr를 통해 적용합니다.
 *
 * 다음으로 fcntl을 두번 사용하여 O_NONBLOCK을 활성화시켜 블록되지 않게 합니다.
 *
 * 첫번째 바깥 while loop에 진입합니다. 1,2,3번 모드가 바깥 while loop 안에 있어서 동작을 반복합니다.
 * 첫번째 안쪽 while loop에 진입합니다. 이는 1번과 2번 모드를 반복하기 위한 loop입니다.
 *
 * 제일 먼저 curse의 initscr(), clear(), standout(), addstr(), standend()와 sprintf()를 이용하여
 * 첫번째 줄에 강조하여 예쁘게 표시할 수 있도록 효과를 넣어서 PID-COMMAND-CPU-VIRT-REs-SHR-MEM 헤더명을 출력합니다.
 *
 * 우선 /proc/stat의 파일에 접근하여 FILE* 타입의 total 변수에 받습니다. total변수를 fscanf로 파싱하여 첫번째값을 cpu_name으로 받고,
 * 두번째값부터 10번째 값까지 &total_values[0]부터 &total_values[8] 변수에 받습니다.
 *
 * for문을 돌려 total_time_before에 total_values 배열들을 전부 더해서 total cpu jiffies를 구합니다.
 * 그리고 4로 나누어 4코어인 경우를 가정하고 total 파일을 닫습니다. 다음으로 수회 반복하여 프로세스가 있는지 찾는 과정을 진행합니다.
 *
 * access() 함수를 이용하여 해당 프로세스가 접근이 불가한 경우 continue를 시키고 그 외에 경우에는 해당 i번을 기준으로 /proc/i/stat과
 * /proc/i/statm을 각각 FILE*타입의 eachProcess와 eachMemory에 엽니다.
 *
 * eachProcess는 fscanf로 파싱하여 첫번째 값으로 pid, 두번째 값으로 filename, 세번째 값으로 ppid, 나머지는 trash배열에 버리고,
 * 마지막 두번째로 usertime 값을, 마지막값으로 systemtime값을 받습니다.
 *
 * eachMemory도 fscanf로 파싱하여 첫번째 값(total memory), 두번째 째 값(resident memory), 세번째 값(shared pages)을
 * 각각 사전에 생성한 d라는 이름의 struct data에 할당합니다.
 *
 * 위에 논의한 계산식을 이용하여 사용률을 계산해주고 d에 할당합니다.
 *
 * 위의 내용은 하나의 프로세스에 대한 작업이고 이를 반복하여 for문을 빠져나오면, 1번 메뉴를 줬는지 2번 메뉴를 줬는지 int 변수를 flag 값으로 사용하여 qsort를 어떻게 할지 정하여 실행합니다.
 *
 * 정렬함수는 두 가지로, 아래에 따로 구현되어있습니다. 다음으로 터미널 크기에 따라 PAGELEN의 값을 받아오고, 해당 PAGELEN만큼 정렬한 것을 출력합니다.
 * 이때 프로세스명이 동일하는 등의 비정상적인 경우는 continue로 제외합니다. 깔끔하게 표기하기 위해 if문 4개를 두고 cpu나 메모리 사용량에 따라 약간씩 다르게 출력합니다.
 *
 * 여기서 메모리 점유율 15프로 이상이거나 또는 CPU점유율 30프로 이상인 경우에는 curses의 addch(문자|A_BOLD)를 이용하여 굵게 표시합니다.
 *
 * 이제 프로그램이 끝났으면 read()함수를 nonblocking으로 읽어들여서 ‘q’인 경우 바깥 while loop를 빠져나와 종료를,
 * ‘1’,’2’,’3’인 경우 각각 메뉴에 맞도록 플래그를 조정해줍니다. 만약 ‘1’,’2’인 경우 계속 while문을 반복하지만, ‘3’인 경우는 while을 빠져나옵니다.
 *
 * while을 빠져나온경우, 포크를 해주어서 하나는 be_receiver함수를, 다른 하나는 be_sender함수를 호출하게 합니다.
 * 둘 사이에 파이프를 두어서 결과를 주고 받을 수 있게 합니다. be_sender는 netstat 커맨드에 옵션을 준 것을 execlp로 실행하여 프로세스를 마치고,
 * 그 실행결과를 파이프를 통해 be_receiver에 전달합니다. be_receiver는 받은 데이터를 읽어서 curses를 이용해 출력합니다.
 *
 * listening socket에 대한 정보를 한번 출력한 후 또 다른 while문에 진입하여 nonblocking으로 메뉴 ‘q’,‘1’,’2’,’3’ 중 하나를 다시 선택하도록 합니다.
 * ‘q’를 선택한경우 바깥 while loop을 빠져나옵니다. 그 외의 선택의 경우 바깥 while loop를 다시 반복합니다.
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
