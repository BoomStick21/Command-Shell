#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

#define OUTPUT 21
#define INPUT 22
#define PIPELINE 23
#define NORMAL 24

typedef void (*sighandler_t)(int);

using namespace std;

void chop(char* srcPtr)
{
	while(*srcPtr != ' ' && *srcPtr != '\t' && *srcPtr != '\n')
	{
		srcPtr++;
	}
	*srcPtr = '\0';
}

int Parse(char* inputString, char* cmdArgv[], char** supplementPtr, int* modePtr) {
	int cmdArgc = 0, terminate = 0;
	char* srcptr = inputString;
	while(*srcptr != '\0' && terminate == 0) {
		*cmdArgv = srcptr;
		cmdArgc++;
		while (*srcptr != ' ' && *srcptr != '\t' && *srcptr != '\n' && *srcptr != '\0' && terminate == 0) {
			switch (*srcptr) {
				case '>' :
					*modePtr = OUTPUT;
					*cmdArgv = '\0';
					srcptr++;
					while(*srcptr == ' ' || *srcptr == '\t')
						srcptr++;
					*supplementPtr = srcptr;
					chop(*supplementPtr);
					terminate = 1;
					break;
				case '<' :
					*modePtr = INPUT;
					*cmdArgv = '\0';
					srcptr++;
					while(*srcptr == ' ' || *srcptr == '\t')
						srcptr++;
					*supplementPtr = srcptr;
					chop(*supplementPtr);
					terminate = 1;
					break;
				case '|' :
					*modePtr = PIPELINE;
					*cmdArgv = '\0';
					srcptr++;
					while(*srcptr == ' ' || *srcptr == '\t')
						srcptr++;
					*supplementPtr = srcptr;
					//chop(*supplementPtr);
					terminate = 1;
					break;
			}
			srcptr++;
		}
		while ((*srcptr == ' ' || *srcptr == '\t' || *srcptr == '\n') && terminate == 0) {
			*srcptr = '\0';
			srcptr++;
		}
		*cmdArgv++;
	}
	*cmdArgv = '\0';
	return cmdArgc;
}

void execute(char **cmdArgv, int mode, char **supplementPtr)
{
	pid_t pid, pid2;
	FILE *fp;
	int mode2 = NORMAL, cmdArgc, status1, status2;
	char *cmdArgv2[100], *supplement2 = NULL;
	int myPipe[2];
	if(mode == PIPELINE)
	{
		if(pipe(myPipe))
		{
			fprintf(stderr, "Pipe failed!");
			exit(-1);
		}
		Parse(*supplementPtr, cmdArgv2, &supplement2, &mode2);
	}
	pid = fork();
	if( pid < 0)
	{
		printf("Error occured");
		exit(-1);
	}
	else if(pid == 0)
	{
		switch(mode)
		{
			case OUTPUT:
				fp = fopen(*supplementPtr, "w+");
				dup2(fileno(fp), 1);
				break;
			case INPUT:
				fp = fopen(*supplementPtr, "r");
				dup2(fileno(fp), 0);
				break;
			case PIPELINE:
				close(myPipe[0]);
				dup2(myPipe[1], fileno(stdout));
				close(myPipe[1]);
				break;
		}
		execvp(*cmdArgv, cmdArgv);
	}
	else
	{
		if(mode == PIPELINE)
		{
			waitpid(pid, &status1, 0);
			pid2 = fork();
			if(pid2 < 0)
			{
				printf("Error");
				exit(-1);
			}
			else if(pid2 == 0)
			{
				close(myPipe[1]);
				dup2(myPipe[0], fileno(stdin));
				close(myPipe[0]);
				execvp(*cmdArgv2, cmdArgv2);
			}
			else
			{
				close(myPipe[0]);
				close(myPipe[1]);
			}
		}
		else
			waitpid(pid, &status1, 0);
	}
}


int main(int argc, char **argv) {
    int mode = NORMAL, cmdArgc;
    char currdir[100];
    char *line, *cmdArgv[100], *supplement;
    line = (char*)malloc(sizeof(char)*100);
    size_t len = 100;
    
    while(1) {
  		getcwd(currdir, 100);
  		cout << currdir;
  		cout << "$ ";
  		getline(&line, &len, stdin);
  		if (strcmp(line, "exit\n") == 0) {
  			exit(0);
  		}
  		else {
  			cmdArgc = Parse(line, cmdArgv, &supplement, &mode);
  			if (strcmp(*cmdArgv, "cd") == 0) {
  				chdir(cmdArgv[1]);
  			}
  			else {
  				execute(cmdArgv, mode, &supplement);
  			}
  		}
    }
    return 0;
 }
