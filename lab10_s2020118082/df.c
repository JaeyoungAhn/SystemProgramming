#include <stdio.h>
#include <stdlib.h>
//#include <sys/vfs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <ctype.h>

int main(int argc,char* argv[])
{

	struct statfs buf;
	char opt = 'k'; // default option(kilobyte)
	char* location;
	for(int i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			for(char* pt = argv[i]++; pt !=NULL; pt++) {
				if( isupper(*pt) ) opt = *pt + 32; // convert to lowercase
				else opt = *pt;
			}
		}
		else {
			location=argv[i];
			printf("location:%s", argv[i]);
		}
	}

	if( opt != 'k' && opt != 'b' && opt != 'm' && opt != 'g' ) {
		printf("df: illegal option -- %c\n", opt);
		printf("usage: df [-b | -k | -m | -g] [filesystem ...]\n");
	}
	else {
		printf("%c\n", opt);
	}
	
	if( statfs(location, &buf) == -1 ) {
		printf("df: %s: No such file or directory", location);
		exit(1);
	}

	printf("%u\n", buf.f_bsize);
	printf("%llu\n", buf.f_blocks);
	printf("%llu\n", buf.f_bfree);

	return 0;
}


