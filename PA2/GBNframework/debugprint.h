#ifndef _DEBUG_PRINT_H_
#define _DEBUG_PRINT_H_

#include <stdio.h>
#include <string.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
//Color codes:
//Karlphillip, http://stackoverflow.com/questions/3585846/color-text-in-terminal-aplications-in-unix

#define _DEBUG_STMT_PRINT_(color, num, format, ...) do {fprintf(stderr, "%s%s:%s %s[%d]%s ", KGRN, __FILE__, KNRM, color, num, KNRM); fprintf(stderr, format,##__VA_ARGS__);} while (0);

#ifdef DEBUG4
#define debug4(...)	_DEBUG_STMT_PRINT_(KRED, 4, __VA_ARGS__);
#define DEBUG3
#else
#define debug4(...)
#endif

#ifdef DEBUG3
#define debug3(...)	_DEBUG_STMT_PRINT_(KYEL, 3, __VA_ARGS__);
#define DEBUG2
#else
#define debug3(...)
#endif

#ifdef DEBUG2
#define debug2(...)	_DEBUG_STMT_PRINT_(KBLU, 2, __VA_ARGS__);
#define DEBUG1
#else
#define debug2(...)
#endif

#ifdef DEBUG1
#define debug1(...)	_DEBUG_STMT_PRINT_(KWHT, 1, __VA_ARGS__);
#else
#define debug1(...)
#endif


#endif
