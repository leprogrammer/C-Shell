#ifndef SFISH_H
#define SFISH_H
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "csapp.h"

/*struct ProcessDetails{
	char programAndArgs[128];
	long startTime;
	int pid;
	int done;

	struct ProcessDetails *next;
};*/

void helpBuiltin();
void pwdBuiltin();
void pwdBuiltinNoFork();
//void pwdBuiltinBackground();
//void fgBuiltin(int pid);
//void killBuiltin(int pid);
//void cloneBuiltin(int pid);
void jobsBuiltin();
void cdBuiltin(char *arg);
void execLastDirectory(char *arg);
void execChangeDirectory(char *arg, char *directory);
void cleanup();
//void cleanupList();

//void cleanBGList(struct ProcessDetails *ptr);

void runProcess(char *arg);
void runProcessBackground(char *arg);
void runProcessWithoutFork(char *arg);
void redirectFD(char redirectionChar, char *file);
void redirectSpecialSpecificFD(int fd, char *file);
void redirectSpecialBoth(char *file);
void redirectFDHereDoc(int fd);
void redirectAppend(char *file);
void restoreFD(char redirectionChar);
void restoreAllFD();
void setupPipe(char *program1, char *program2, char *cmd);
void setup3Pipe(char *program1, char *program2, char *program3, char *cmd);

void sigalrmHandler(int signal, siginfo_t *siginfo, void *context);
void sigchldHandler(int signal, siginfo_t *siginfo, void *context);
void sigusr2Handler(int signal);

void setupSigHandlers();
void restoreDefaultHandlers();

char *returnPwd();
char *shellHeader();
int checkArgForWhitespace(char *arg);

//struct ProcessDetails *listHeader;

//int originalPGID;

extern volatile int alarmTime;
extern sigset_t prev_maskTop;


#endif
