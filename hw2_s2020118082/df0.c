#include <stdio.h>
#include <stdlib.h>
#include <sys/vfs.h>
#include <ctype.h>
#include <string.h>
int main(int argc,char* argv[])
{

	struct statfs buf;
	char opt = 'K'; // default option(kilobyte)
	char* location="."; // default location
	int digit_count(); // digit is needed to print neatly

	/*
	 * ./df0 . -option (how original df works)
	 * ./df0 -option . (as demonstrated on the lms)
	 * both way are possible */
	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			int length = strlen(argv[i]);
			char* pt = argv[i];
			for(int j=1; j<length; j++) {
				if( islower(pt[j]) ) opt = pt[j] - 32; // convert to lowercase
				else opt = pt[j];
				/* in case of -bkmG, G which is last char
				 * will be assigned just as original df works */
			}
		}
		else location=argv[i];
	}

	if( opt != 'K' && opt != 'B' && opt != 'M' && opt != 'G' ) {
		printf("df: illegal option -- %c\n", opt);
		printf("usage: df [-b | -k | -m | -g] [filesystem ...]\n");
	}
	
	if( statfs(location, &buf) == -1 ) {
		printf("df: %s: No such file or directory", location);
		exit(1);
	}
	
	double block_size = buf.f_bsize; // default(4096bytes)
	long long total_space;
	long long used_space;
	long long free_space;
	int percentage;
	int total_digit;
	int used_digit;
	int free_digit;
	switch(opt){ // block unit conversion
		case 'K':
			block_size/=1024;
			break;
		case 'M':
			block_size/=1024;
			block_size/=1024;
			break;
		case 'G':
			block_size/=1024;
			block_size/=1024;
			block_size/=1024;
			break;
	}
	total_space = (long long)(block_size*buf.f_blocks);
	free_space = (long long)(block_size*buf.f_bfree);
	used_space = total_space-free_space;
	percentage = ((double)used_space/total_space)*100;
	total_digit = digit_count(total_space);
	free_digit = digit_count(free_space)+1;
	used_digit = digit_count(used_space)+1;

	if(opt=='B' || opt=='K') { // if it's B or K, priting either M or K is not required.
		free_digit-=1;
		used_digit-=1;
	}
	
	/* printing the result neatly */
	char block_name[10];
	sprintf(block_name, "1%c-blocks", opt);
	char* used_name="Used";
	char* free_name="Available";
	if(total_digit>9) printf("%*s ", total_digit, block_name);
	else printf("%s ", block_name);
	if(used_digit>4) printf("%*s ", used_digit,used_name);
	else printf("%s ", used_name);
	if(free_digit>9) printf("%*s ", free_digit, free_name);
	else printf("%s ", free_name);
	printf("MyUse%%\n");
	
	if(total_digit<9) printf("%9llu ", total_space);
	else printf("%llu ", total_space);

	if(used_digit<4) {
		if(opt=='M' || opt=='G') printf("%3llu%c ", used_space, opt);
		else printf("%4llu ", used_space);
	}
	else {
		if(opt=='M' || opt=='G') printf("%llu%c ", used_space, opt);
		else printf("%llu ", used_space);
	}

	if(free_digit<9) {
		if(opt == 'M' || opt=='G') printf("%8llu%c ", free_space, opt);
		else printf("%9llu ", free_space);

	}
	else {
		if(opt=='M' || opt=='G') printf("%llu%c ", free_space, opt);
		else printf("%llu ", free_space);
	}	
	printf("%5d%%\n", percentage);
	return 0;
}

int digit_count(unsigned long long number) { // return length of digits
	int repeat=0;
	while(number!=0) {
		repeat++;
		number/=10;
	}
	return repeat;
}
