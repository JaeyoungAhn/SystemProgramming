/* splitline.c - command reading and parsing functions for smsh
 *
 * char **next_cmd(char *prompt, FILE *fp, int *return_nth) - get next command
 * char **splitline(char *str);	- parse a string
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smsh.h"

char **next_cmd(char *prompt, FILE *fp, int *return_nth)
/*
 * purpose: read next command line from fp
 * returns: dynamically allocated string holding command line
 *  errors: NULL at EOF (not really an error)
 *			calls fatal from emalloc()
 *	notes: allocates space in BUFSIZ chunks.
 */
{
	char **buf;	/* the buffer	*/
	int bufspace = 0;	/* total size	*/
	int pointerspace = 0;
	int pos = 0;	/* current position */
	int c;			/* input char	*/
	int nth=0;
	int flag=0;
	printf("%s", prompt);		/* prompt user	*/
	while( ( c = getc(fp)) != EOF ) {
		if(flag==1) {
			flag=0;
			nth++;
		}
		/* need space? */
		if( nth*4+1 >= pointerspace ) {	/* 1 for \0	*/
			if( pointerspace == 0 )
				buf = emalloc(BUFSIZ);
			else					/* or expand	*/
				buf = erealloc(buf,bufspace+BUFSIZ);
			pointerspace += BUFSIZ;		/* update size	*/
		}


		if( pos+1 >= bufspace ) {	/* 1 for \0	*/
			if( bufspace == 0 )
				buf[nth] = emalloc(BUFSIZ);
			else					/* or expand	*/
				buf[nth] = erealloc(buf[nth],bufspace+BUFSIZ);
			bufspace += BUFSIZ;		/* update size	*/
		}

		/* end of command? */
		if( c == '\n' )
			break;

		/* no, add to buffer */
		if(c != ';') {
			buf[nth][pos++] = c;
		}
		else {
			buf[nth][pos] = '\0';
			pos=0;
			bufspace=0;
			flag=1;
		}
	}
	
	if(pos!=0) {
		buf[nth][pos] = '\0';
		nth++;
	}

	if( c == EOF && pos == 0)		/* EOF and no input	*/
		return NULL;				/* say so		*/
	
	*return_nth=nth;
	return buf;
}

/**
 ** splitline(parse a line into an array of strings)
 **/
#define is_delim(x) ((x)==' '||(x)=='\t')

char ** splitline(char *line)
/*
 * purpose: split a line into array of white-space separated tokens
 * returns: a NULL-terminated array of pointers to copaies of the tokens
 *			or NULL if line if no tokens on the line
 *	action: traverse the array, locate strings, make copies
 *	  note: strtok() could work, but we may want to add quotes later
 */
{
	char *newstr();
	char **args;
	int spots = 0;	/* spots in table	*/
	int bufspace = 0;	/* bytes in table	*/
	int argnum = 0;		/* slots used	*/
	char *cp = line;	/* pos in string	*/
	char *start;
	int len;

	if ( line == NULL )	/* handle special case	*/
		return NULL;

	args = emalloc(BUFSIZ); /* initialize array	*/
	bufspace = BUFSIZ;
	spots = BUFSIZ/sizeof(char *);

	while( *cp != '\0' )
	{
		while ( is_delim(*cp) )	/* skip loading spaces	*/
			cp++;
		if ( *cp == '\0' )	/* quit at end-o-string	*/
			break;

		/* make sure the array has room (+1 for NULL) */
		if( argnum+1 >= spots ){
			args = erealloc(args,bufspace+BUFSIZ);
			bufspace += BUFSIZ;
			spots += (BUFSIZ/sizeof(char *));
		}

		/* mark start, then find end of word */
		start = cp;
		len = 1;
		while (*++cp != '\0' && !(is_delim(*cp)) )
			len++;
		args[argnum++] = newstr(start, len);
	}
	args[argnum] = NULL;
	return args;
}

/*
 * purpose: constructor for strings
 * returns: a string, never NULL
 */
char *newstr(char *s, int l)
{
	char *rv = emalloc(l+1);

	rv[l] = '\0';
	strncpy(rv, s, l);
	return rv;
}

void
freelist(char **list)
/*
 * purpose: free the list returned by splitline
 * returns: nothing
 *  action: free all strings in list and then free the list
 */
{
	char **cp = list;
	while( *cp )
		free(*cp++);
	free(list);
}

void * emalloc(size_t n) {
	void *rv;
	if( (rv = malloc(n)) == NULL )
		fatal("out of memory", "", 1);
	return rv;
}

void * erealloc(void *p, size_t n)
{
	void *rv;
	if ( (rv = realloc(p,n)) == NULL )
		fatal("realloc() failed", "", 1);
	return rv;
}
