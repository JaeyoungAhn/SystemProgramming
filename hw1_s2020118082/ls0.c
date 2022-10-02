/** ls0.c
 ** implementation of ls -R
 ** list of files must be sorted in alphabetical order
 ** when going recursive, relative directory path need to be
 ** shown from initial directory to current directory
 **/

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


void do_ls(char [], char path[]);

bool recursive = false;

int main(int ac, char *av[]) {
	
	int deep_ac = ac;
	char** deep_av = av;
	char path[1000]=".";
	DIR *dir_ptr;
	bool parent=false;
	int streak=0;
	bool flag=false;

	if(deep_ac>=3)  flag=true;
	while(--deep_ac) {
		if( *(++deep_av)[0] == '-') recursive = true;
		if(!strcmp(*deep_av, "..")) streak+=1;
	}
	if (recursive == true && ac == 2 || recursive == false && ac == 1 )
		do_ls(*av, path);
	else
		while ( --ac ) {
			memset(path, 0, 1000*sizeof(char));
			++av;
			if(*av[0] != '-' && strcmp(*av,"..")) {
				if( (dir_ptr = opendir( *av) ) == NULL ) {
					fprintf(stderr, "ls0: cannot access '%s': No such file or directory\n", *av);
					break;
				}

				strcat(path, ".");
				if(!strcmp(*av,"..")) strcat(path, ".");
				if(*av[0] != '.') {
					strcat(path,"/");
					strcat(path, *av);
				}		
				chdir(*av);
				do_ls( *av, path);
			}
		}
		if ( streak>0 ) {
			if( recursive == true && flag == true ) printf("\n");
			memset(path, 0, 1000*sizeof(char));
			strcat(path, "..");
			chdir("..");
		       	do_ls("..", path);
		}
	return 0;
}


void do_ls ( char dirname[], char path[] )
/*
 * list files in directory called dirname
 */
{
	int name_count = 0;
   	struct dirent** name_list = NULL;

	if(recursive==true) printf("%s:\n", path);

       	name_count = scandir(".", &name_list, NULL, alphasort);

	for (int i = 0; i < name_count; i++) {
		if( name_list[i]->d_name[0] != '.' ) printf("%s  ", name_list[i]->d_name );
	}
	
	printf("\n");

	if(recursive == true) {
		
		struct stat info;
		char path2[1000];

		for (int i = 0; i< name_count; i++) {
			if (stat( name_list[i]->d_name, &info ) == -1 ) perror( name_list[i]->d_name );
			else {
				memset(path2, 0, 1000*sizeof(char));
				path2 == strdup(path);
				strcat(path2, path);
				if( S_ISDIR(info.st_mode) && name_list[i]->d_name[0] != '.' ) {
					printf("\n");
					strcat(path2, "/");
					strcat(path2, name_list[i]->d_name);
					chdir(name_list[i]->d_name);	
					do_ls(name_list[i]->d_name, path2);
				}
			}
		}
	}
	if(strcmp(path,".")) chdir("..");
}
