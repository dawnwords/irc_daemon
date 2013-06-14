#include "log.h"

extern u_long curr_nodeID;

//log file pointer for debug purpose
FILE *log_fp;

void init_log(){
	char log_buf[MAX_MSG_LEN];
    sprintf(log_buf,"node%lu.log",curr_nodeID);
    log_fp=fopen(log_buf,"w+");
}

void write_log(const char *form, ...){
    va_list arg;   
    char pbString[MAX_MSG_LEN];
    va_start(arg,form);  
    vsprintf(pbString,form,arg);
    fprintf(log_fp,"%s",pbString);
    va_end(arg);   
    fflush(log_fp);
}
 