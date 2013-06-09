#include "util.h"
#include "csapp.h"
#include <stdlib.h>
#include "rtlib.h"

/* Global variables */
u_long curr_nodeID;
rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */
rt_args_t args;

/*
 * void init_node( int argc, char *argv[] )
 *
 * Takes care of initializing a node for an IRC server
 * from the given command line arguments
 */
void init_node( int argc, char *argv[] ){
    int i;

    if( argc < 3 ) {
        printf( "%s <nodeID> <config file>\n", argv[0] );
        exit( 0 );
    }

    /* Parse nodeID */
    curr_nodeID = atol( argv[1] );

    /* Store  */
    rt_parse_config_file(argv[0], &curr_node_config_file, argv[2] );

    /* Get config file for this node */
    for( i = 0; i < curr_node_config_file.size; ++i )
        if( curr_node_config_file.entries[i].nodeID == curr_nodeID )
             curr_node_config_entry = &curr_node_config_file.entries[i];

    /* Check to see if nodeID is valid */
    if( !curr_node_config_entry ) {
        printf( "Invalid NodeID\n" );
        exit(1);
    }
}

void init_daemon( int argc, char *argv[] ){
    int i;

    rt_parse_command_line(&args, argc, argv);
    /* Parse nodeID */
    curr_nodeID = args.nodeID;
    curr_node_config_file = args.config_file;

    /* Get config file for this node */
    for( i = 0; i < curr_node_config_file.size; ++i )
        if( curr_node_config_file.entries[i].nodeID == curr_nodeID )
             curr_node_config_entry = &curr_node_config_file.entries[i];

    /* Check to see if nodeID is valid */
    if( !curr_node_config_entry ) {
        printf( "Invalid NodeID\n" );
        exit(1);
    }
}


/*
 * size_t get_msg( char *buf, char *msg )
 *
 * char *buf : the buffer containing the text to be parsed
 * char *msg : a user Malloc'ed buffer to which get_msg will copy the message
 *
 * Copies all the characters from buf[0] up to and including the first instance
 * of the IRC endline characters "\r\n" into msg.  msg should be at least as
 * large as buf to prevent overflow.
 *
 * Returns the size of the message copied to msg.
 */
size_t get_msg(char *buf, char *msg) {
    char *end;
    int  len;

    /* Find end of message */
    end = strstr(buf, "\r\n");

    if( end ) {
        len = end - buf + 2;
    } else {
        /* Could not find \r\n, try searching only for \n */
        end = strstr(buf, "\n");
        if( end )
            len = end - buf + 1;
        else
            return -1;
    }

    /* found a complete message */
    memcpy(msg, buf, len);
    msg[end-buf] = '\0';

    return len;
}


/*
 * int tokenize( char const *in_buf, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1] )
 *
 * A strtok() variant.  If in_buf is a space-separated list of words,
 * then on return tokens[X] will contain the Xth word in in_buf.
 *
 * Note: You might want to look at the first word in tokens to
 * determine what action to take next.
 *
 * Returns the number of tokens parsed.
 */
int tokenize( char const *in_buf, char tokens[MAX_MSG_TOKENS][MAX_MSG_LEN+1] ) {
    int i = 0;
    const char *current = in_buf;
    int done = 0;

    /* Possible Bug: handling of too many args */
    while (!done && (i<MAX_MSG_TOKENS)) {
        char *next = strchr(current, ' ');

        if (next) {
            int temp = next-current;
            if(temp>LIMIT_MSG_LEG)
                temp = LIMIT_MSG_LEG;
            memcpy(tokens[i], current, temp);
            tokens[i][temp] = '\0';
            current = next + 1;   /* move over the space */
            ++i;

            /* trailing token */
            if (*current == ':') {
                ++current;
                strncpy(tokens[i], current,LIMIT_MSG_LEG);
                ++i;
                done = 1;
            }
        } else {
            strncpy(tokens[i], current,LIMIT_MSG_LEG);
            ++i;
            done = 1;
        }
    }
    return i - 1;
}