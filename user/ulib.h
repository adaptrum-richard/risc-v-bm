#ifndef __ULIB_H__
#define __ULIB_H__
#include "kernel/types.h"

#define stdin 0
#define stdout 1
#define stderr 2

#define	STDIN_FILENO	0	/* Standard input.  */
#define	STDOUT_FILENO	1	/* Standard output.  */

int fputs(const char *s, int stream);
int getline(char **lineptr, size_t *n, int stream);

#endif
