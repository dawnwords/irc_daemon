#include "log.h"

extern u_long curr_nodeID;

//log file pointer for debug purpose
FILE *log_fp;

void init_log(){
	char log_buf[MAX_MSG_LEN];
    sprintf(log_buf,"node%lu.log",curr_nodeID);
    log_fp=fopen(log_buf,"w+");
}

void write_log(const char *format, ...){
    va_list args;
    va_start(args, format);
    fprintf(log_fp, format, args);
    va_end(args);
    fflush(log_fp);
}
 