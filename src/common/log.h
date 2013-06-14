#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>
#include "util.h"
#include "csapp.h"
#include "rtlib.h"

void init_log();
void write_log(const char *format, ...);
#endif