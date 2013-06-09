#ifndef __UTIL_H__
#define __UTIL_H__

#include "csapp.h"
/* Macros */
#define MAX_MSG_TOKENS 10
#define MAX_MSG_LEN 512
#define LIMIT_MSG_LEG 480
#define MAX_NAME_LENGTH 9

/* Function prototypes */
void init_node( int argc, char *argv[] );
void init_daemon( int argc, char *argv[] );
size_t get_msg( char *buf, char *msg );
int tokenize( char const *in_buf, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1] );


#endif /*__UTIL_H__*/