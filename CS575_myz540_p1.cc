/*  
	CS575 Operating Systems
	Mike Zhong
	10/2/16
	Program 1
*/
// Header files needed 
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <list>
#include <fcntl.h>

// Define constants
#define MAXLINE 1024
#define MAXARGS 20
#define PROMPT "MZshell> "
#define HISTSIZE 10 // history size

using namespace std;

int parseCmd(const char *cmdline, char** arglist, int* flags);

int main(int argc, char *argv[])
{
	
	list<string> cmd_history;
	
	while(1){
		
		// declare local variables
		pid_t pid;	
		char cmd_buffer[MAXLINE];
		char* arglist[MAXARGS];
		char* args_passed[MAXARGS];
		int argcount = 0;
		int end_of_args = 0;
		int last_arg = 0;
		string temp_string;
		int flags[4] = {0,0,0,0}; // flags array for signalling &, <, >, |
		char* input = NULL; // input redirection
		char* output = NULL; // output redirection
		int pipefd[2];

		// prompt and attemp to read in command
		cout << "\n" << PROMPT << flush;
		cin.getline(cmd_buffer, MAXLINE);
		// if user enters nothing, keep prompting
		while(cmd_buffer[0] == '\0'){
			cout << "\n" << PROMPT << flush;
			cin.getline(cmd_buffer, MAXLINE);
		}

		temp_string = cmd_buffer; // convert to string object for history
		
		// cout << cmd_buffer << endl;
		
		// add command to history, limit to 10 items
		if(cmd_history.size() < HISTSIZE){
			cmd_history.push_front(temp_string);
		}
		else{
			cmd_history.pop_back();
			cmd_history.push_front(temp_string);
		} 		
	
		// parse command
		argcount = parseCmd(cmd_buffer, arglist, flags);
		
		last_arg = argcount - 1;

		for(int i = 0; i < argcount; ++i){
	
			// printf("arglist[%d]: %s ", i, arglist[i]); 
			// check input and output redirection
			// case of main < input.txt > output.txt
			if(!strcmp(arglist[i], "<")){
				if(!end_of_args){
					end_of_args = 1;
					last_arg = i - 1;
				}
				input = arglist[i + 1];
				// cout << "input redirect detected, setting flag\n";
				flags[1] = 1;
			}
			else if(!strcmp(arglist[i], ">")){
				if(!end_of_args){
					end_of_args = 1;
					last_arg = i - 1;
				}
				// cout << "output redirect detected, setting flag\n";
				output = arglist[i + 1];
				flags[2] = 1;
			}
		}
				
		// what to do now... set all arguments beyond the redirection operators to null
		for(int i = last_arg + 1; i < argcount; ++i){
			arglist[i] = NULL;
		}

		// not handling weird cases yet
		// input = strstr(arglist[0], '<');
		
		// check exit command
		if(!strcmp(arglist[0], "exit")){	
			exit(1);
		}
		// check cd command
		else if(!strcmp(arglist[0], "cd")){
			if(chdir(arglist[1]) < 0){
				cout << "Directory change failed\n";
			}
		}// check history command
		else if(!strcmp(arglist[0], "history")){
			cout << "displaying history\n";
			for(auto iter : cmd_history){
				cout << "command: " << iter << endl;
			}
		}
		else{ // no other known commands, time to execute
			// cout << "forking now\n";
			pid = fork();

			if(pid < 0){ // failed
				cout << "Fork failed\n";
			}	
			else if(pid == 0){ // child process
				// cout << "hello from child process\n";
				if(flags[1]){ // input redirection	
					// printf("About to redirect input to: %s\n", input);	
					int fd0 = open(input, O_RDONLY);
					if(fd0 < 0){
						cout << "open() failed\n";
					}
					if(dup2(fd0, STDIN_FILENO) < 0){
						cout << "dup2() failed\n";
					}
					close(fd0);
				}

				if(flags[2]){ // output redirection
					// printf("About to redirect output to: %s\n", output);
					int fd1 = creat(output, 0644);
					if(fd1 < 0){
						cout << "creat() failed\n";
					}
					int chk1 = dup2(fd1, STDOUT_FILENO);
					if(dup2(fd1, STDOUT_FILENO) < 0){
						cout << "dup2() failed\n";
					}
					close(fd1);
				}
				// cout << "bout ta execute!!!\n";
				execvp(arglist[0], arglist);
				cout << "Unrecognized command, execvp not successful\n";
				exit(0);
			}
			else{ // parent process
				
				// cout << "hello from parent process\n";
				if(!flags[0]){ // if it's not a background process, wait
					// cout << "parent gonna wait\n";
					waitpid(-1, NULL, 0);
				}
				
			}
		}
	}
  

  return 0;
}

/* 
 * parsecmd - Parse the command line and build the argv array.
 * Return the number of arguments.
 */
int parseCmd(const char *cmdline, char** arglist, int* flags) 
{
  static char array[MAXLINE]; /* holds local copy of command line */
  char *buf = array;          /* ptr that traverses command line */
  char *delim;                /* points to first space delimiter */
  int argc;                   /* number of args */

  strncpy(buf, cmdline, strlen(cmdline)+1);
  buf[strlen(buf)] = ' ';  /* replace trailing '\n' with space */
  while (*buf && (*buf == ' ')) /* ignore leading spaces */
    buf++;
  
  /* Build the arglist list */
  argc = 0;
  // based on delimiter " "(space), separate the commandline into arglist
  while ((delim = strchr(buf, ' ')) && (argc < MAXARGS - 1)) {
    arglist[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
		
		// check last char of arglist[0] and entirety of arglist[1] for &
		if(*(delim - 1) == '&'){
			flags[0] = 1;
			*(delim - 1) = '\0';
		}

    while (*buf && (*buf == ' ')) /* ignore spaces */
      buf++;
  }
  arglist[argc] = NULL;
  return argc;
  
}
