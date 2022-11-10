// smsh.h -- declares functino profiles for external references
#define YES 1
#define NO 0

/*
 * - Reads the next command from an input stream
 * - Calls malloc to accept command lines of any length
 */
char *next_cmd();
/*
 * - Splits a string into an array of words
 * - Returns that array
 */
 char **splitline(char*);
 // free the list returned by splitline
 void freelist(char**);
 // extension of malloc
 void *emalloc(size_t);
 // extension of realloc
 void *erealloc(void*, size_t);
 // run a program
 int execute(char**);
 // report an error
 void fatal(char*, char*, int);
 // process
 int process(char**);
