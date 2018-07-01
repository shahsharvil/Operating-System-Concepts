#ifndef _SHELL_H_
#define _SHELL_H_

#define MAX_COMMAND_LEN	100
#define MAX_ARGS 		 10
#define MY_COMMANDS 	  4

#define clear() printf("\033[H\033[J")

#define red()	printf("\033[1m\033[31m");
#define green() printf("\033[1m\033[32m");
#define blue()  printf("\033[1m\033[34m");
#define white() printf("\x1B[0m");

#endif	/* _SHELL_H_ */
