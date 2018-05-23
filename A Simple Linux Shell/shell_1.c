#include <stdio.h>	// getenv()
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_ARGS 10
#define MY_COMMANDS 3
#define clear() printf("\033[H\033[J")


void init_shell()
{
	clear();
	
	puts("\n\n\t\tWelcome!\n\n");
	puts("\tShell_Author:  Sharvil Shah");
	puts("\tShell_Version: v1.01");
	
	char *value = getenv("USER");
	printf("\n\tUser: @%s\n", value);
	
	sleep(1);	
	clear();
}


void print_directory()
{
	char dir[1024];
	getcwd(dir, sizeof(dir));
	printf("%s", dir);
}


int take_input(char *str)
{	
	size_t cmd_len = 100;
	char *buf = NULL;
	
	printf(":$ ");
	ssize_t ori_len = getline(&buf, &cmd_len, stdin);
	
	if(ori_len > 1)
	{
		strcpy(str, buf);
		return 0;
	}
	else
	{
		return 1;	
	}	
}


int parse_cmd(char *str, char **parsed)
{	
	char *token;
	token = strtok(str, " '\n'");

	int arg_ind = 0;
	while(token != NULL)
	{
		if(arg_ind == MAX_ARGS)
		{	
			printf("Too many arguments\n");
			return -1;
		}

		parsed[arg_ind] = (char *)malloc(strlen(token) + 1);
		strcpy(parsed[arg_ind], token);
		
		arg_ind++;		
		
		token = strtok(NULL, " '\n'");
	}

	return 1;
}


int exec_cmd(char **parsed)
{	
	char *my_commands[MY_COMMANDS];
	my_commands[0] = "exit";
	my_commands[1] = "clear";
	my_commands[2] = "cd";

	int command_no = 0;
	for(int i = 0; i < MY_COMMANDS; i++)
	{	
		if(strcmp(my_commands[i], parsed[0]) == 0)
		{	
			command_no = i + 1;
			break;
		}
	}

	switch(command_no)
	{
		case 1:
			printf("Bye!\n");
			exit(0);
		case 2:
			clear();
			return 1;
		case 3:
			chdir(parsed[1]);
			return 1;
		default:
			printf("%s command not found!\n", parsed[0]);
			break;			
	}

	return 0;
}


int main()
{
	char user_command[100], *parsed_args[MAX_ARGS] = {0};

	int ret = 0;
	
	init_shell();

	while(1)
	{
		print_directory();
		
		if(take_input(user_command))
			continue;

		ret = parse_cmd(user_command, parsed_args);	

		if(ret > 0)
		{	
			exec_cmd(parsed_args);
		}
		
		// free parsed_args
		for(int i = 0; i < MAX_ARGS && parsed_args[i] != NULL; i++)
		{
			free(parsed_args[i]);
			parsed_args[i] = 0;
		}			
	}

	return 0;
}