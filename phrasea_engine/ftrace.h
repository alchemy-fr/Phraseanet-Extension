#ifndef FTRACE_H
#define FTRACE_H 1

#define ftrace(...) realftrace(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

void realftrace(char *file, int line, const char *function, const char *fmt, ...);

/*
 utility to log on file when zend_printf is not a solution
 */


#endif	/* FTRACE_H */
