#define _GNU_SOURCE
#include <stdio.h>	
#include <string.h>
#include <errno.h>
#include <stdlib.h>			/* getenv() */
#include <unistd.h>			/* getcwd(), chdir() */
#include <inttypes.h>		/* uint8_t */
#include <sys/wait.h>		/* wait() */

#include "shell.h"


/*
 *	init_shell
 *	
 *	Details:
 *		- prints welcome message and details about the shell
 *	
 */
void init_shell()
{
	clear();
#ifdef COLOR	
	green();
#endif	
	char *value = getenv("USER");
	printf("\n\n\t\tWelcome! @%s\n\n", value);
	puts("\tShell_Author:  Sharvil Shah");
	puts("\tShell_Version: v1.1");

	sleep(2);	
	clear();
}


/*
 *	print_cwd()
 *
 *	Details:
 *		- prints user name and the current working directory
 */
void print_cwd()
{	
	char *value = getenv("USER");
#ifdef COLOR		
	green();
#endif		
	printf("%s", value);
#ifdef COLOR	
	white();
#endif	
	printf(":");
#ifdef COLOR
	blue();
#endif
	char *cur_dir_path = get_current_dir_name();
	printf("%s", cur_dir_path);
#ifdef COLOR	
	white();
#endif
	printf(">> ");

	free(cur_dir_path);
}


/*
 *	take_input
 *
 *	Details:
 *		- reads an entire line from stdin
 *
 *	Return value
 *		- 0, if command read from stdin
 *		- 1, else
 */
int take_input(char *str)
{	
	char *buffer = NULL;
	size_t len = 0;
	
	ssize_t ori_len = getline(&buffer, &len, stdin);
	if(ori_len > 1)
	{
		strcpy(str, buffer);
		return 0;
	}
	else
	{
		return 1;	
	}	
}


/*
 *	parse_cmd
 *
 *	Details:
 *		- parses string read by 'take_input' command
 *		- if pipe encountered, args after pipe stored separately
 *
 * 	Return Value:
 *		-  1, if pipe present
 *		-  0, if no pipe present
 *		- -1, if argument count exceeds MAX_ARGS
 */
int parse_cmd(char *str, char **parsed_args, char **parsed_args_after_pipe)
{	
	char *token;
	uint8_t pipe_flag = 0;
	int arg_ind = 0;
	
	token = strtok(str, " '\n'");

	while(token != NULL)
	{
		if(arg_ind == MAX_ARGS)
		{	
			printf("Too many arguments\n");
			return -1;
		}

		if(*token == '|')
		{	
			pipe_flag = 1;
			arg_ind = 0;
			
			token = strtok(NULL, " '\n'");
			while(token != NULL)
			{
				if(arg_ind == MAX_ARGS)
				{	
					printf("Too many arguments\n");
					return -1;
				}
				parsed_args_after_pipe[arg_ind] = (char *)malloc(strlen(token) + 1);
				strcpy(parsed_args_after_pipe[arg_ind], token);
				arg_ind++;		
				token = strtok(NULL, " '\n'");
			}

			break;
		}
		else
		{
			parsed_args[arg_ind] = (char *)malloc(strlen(token) + 1);
			strcpy(parsed_args[arg_ind], token);
			arg_ind++;		
			token = strtok(NULL, " '\n'");
		}
	}

	return pipe_flag;
}


/*
 *	exec_cmd - handler for "my own" as well as "non-piped" commands
 *
 *	Details:
 *		- executes custom defined commands with switch statement
 *		- every other command not in the list is executed using execvp
 */
int exec_cmd(char **parsed_args)
{	
	char *my_commands[MY_COMMANDS];
	my_commands[0] = "exit";
	my_commands[1] = "clear";
	my_commands[2] = "cd";
	my_commands[3] = "help";

	pid_t execvp_pid;
	
	int command_no = 0;
	for(int i = 0; i < MY_COMMANDS; i++)
	{	
		if(strcmp(my_commands[i], parsed_args[0]) == 0)
		{	
			command_no = i + 1;
			break;
		}
	}

	switch(command_no)
	{
		case 1:
			printf("exiting shell...\n");
			exit(0);

		case 2:
			clear();
			return 1;

		case 3:
			chdir(parsed_args[1]);
			return 1;

		case 4:
			printf("available commands:\n");
			for(int i = 0; i < MY_COMMANDS; i++)
			{
				printf("\t %d. %s\n", i + 1, my_commands[i]);
			}
			return 1;

		default:
			execvp_pid = fork();			;
			if(!execvp_pid)
			{
				execvp(parsed_args[0], parsed_args);
				printf("%s: command not found\n", parsed_args[0]);
				exit(0);
			}
			wait(NULL);
			break;			
	}

	return 0;
}


/*
 *	exec_pipe_cmd - handler for "piped" commands
 *	
 *	Details:
 *		- forks 2 child processes with pids child_1 and child_2
 *		- child_1 executes commands before pipe and child_2 executes
 *		  commands after pipe
 *		- the parent process waits until both child processes terminate
 */
int exec_pipe_cmd(char** parsed_args, char** parsed_args_after_pipe)
{
	int pipefd[2]; 
    pid_t child_1, child_2;
 
    if (pipe2(pipefd, 0) < 0) {
        printf("cannot create pipe, errno: %d\n", errno);
        return -1;
    }
    child_1 = fork();
    if (child_1 < 0) {
        printf("unable to fork!\n");
        return -1;
    }
 	
    if (child_1 == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
 
        if (execvp(parsed_args[0], parsed_args) < 0)
        {
#ifdef COLOR
        	red();
#endif
        	printf("Error: ");
#ifdef COLOR
        	white();
#endif 
            printf("cannot execute command %s\n", parsed_args[0]);
            exit(0);
        }
    }
    else
    {	
        child_2 = fork();
 
        if (child_2 < 0) {
            printf("unable to fork!\n");
            return -1;
        }
 
        if (child_2 == 0)
        {	
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsed_args_after_pipe[0], parsed_args_after_pipe) < 0)
            {
#ifdef COLOR
        	red();
#endif
        	printf("Error: ");
#ifdef COLOR
        	white();
#endif    	
            printf("cannot execute command %s\n", parsed_args_after_pipe[0]);
                exit(0);
            }
        }
        else
        {	
        	close(pipefd[0]);
        	close(pipefd[1]);

            wait(NULL);
            wait(NULL);
        }
    }

    return 0;
}


/*
 *	free_args_mem
 *
 *	Details:
 *		- frees previously allocated memory by 'parse_cmd' function
 */
void free_args_mem(char** parsed_args, char** parsed_args_after_pipe, int pipe_flag)
{
	for(int i = 0; i < MAX_ARGS && parsed_args[i] != NULL; i++)
	{	
		free(parsed_args[i]);
		parsed_args[i] = 0;
	}

	if(pipe_flag)
	{
		for(int i = 0; i < MAX_ARGS && parsed_args_after_pipe[i] != NULL; i++)
		{	
			free(parsed_args_after_pipe[i]);
			parsed_args_after_pipe[i] = 0;
		}
	}
}


/*
 *	main
 *	
 *	Details:
 *		- declares arrays to store and parse user commands
 *		- runs an infinite loop that parses and executes input commands
 */
int main()
{
	char user_command[MAX_COMMAND_LEN];
	char *parsed_args[MAX_ARGS] = {0};
	char *parsed_args_after_pipe[MAX_ARGS] = {0};

	int piped = 0;
	
	init_shell();

	while(1)
	{	
		print_cwd();
		
		if(take_input(user_command))
		{
			continue;
		}

		piped = parse_cmd(user_command, parsed_args, parsed_args_after_pipe);
		if(piped == 0)
		{	
			exec_cmd(parsed_args);
		}
		else if(piped == 1)
		{
			exec_pipe_cmd(parsed_args, parsed_args_after_pipe);
		}

		free_args_mem(parsed_args, parsed_args_after_pipe, piped);
	}

	return 0;
}