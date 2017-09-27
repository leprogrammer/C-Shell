#include "sfish.h"

pid_t pid;
char *lastDirectory = NULL;
char *argCpy = NULL;
int stdinFD = -1;
int stdoutFD = -1;
int stderrFD = -1;
int specialFD = -1;
//int originalPGID = -1;
sigset_t prev_maskTop;

//struct ProcessDetails *listHeader = NULL;

volatile int alarmTime;

void helpBuiltin(){
	fprintf(stdout, "%s\n", "cd [-][.][..][path]");
	fprintf(stdout, "%s\n", "help");
	fprintf(stdout, "%s\n", "pwd");
	fprintf(stdout, "%s\n", "exit");
}

void pwdBuiltin(){
	int child_status;
	char *path = NULL;

	if((pid = fork()) == 0){
		//for(int i = 0; i < 1000000000; i++);
		path = malloc(256);
		getcwd(path, 256);
		strcat(path, "\n");
		if(write(STDOUT_FILENO, path, strlen(path) - 1) == -1){
			perror("write error");
		}
		free(path);
		cleanup();
		exit(3);
	}
	else if(pid > 0){
		restoreAllFD();
		setupSigHandlers();
		waitpid(pid, &child_status, 0);
	}
}

void pwdBuiltinNoFork(){
	char *path = malloc(256);
	getcwd(path, 256);
	strcat(path, "\n");
	if(write(STDOUT_FILENO, path, strlen(path) - 1) == -1){
		perror("write error");
	}
	free(path);
	cleanup();
	exit(3);
}

/*void pwdBuiltinBackground(){
	char *path = NULL;

	if((pid = fork()) == 0){
		setpgid(0, 0);
		//while(1);
		path = malloc(256);
		getcwd(path, 256);
		strcat(path, "\n");

		if(write(STDOUT_FILENO, path, strlen(path) - 1) == -1){
			perror("write error");
		}
		free(path);
		cleanup();
		cleanupList();
		exit(3);
	}
	else if(pid > 0){
		restoreAllFD();
		setupSigHandlers();
		sigset_t mask, prev_mask;
	    sigfillset(&mask);
	    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
		if(listHeader == NULL){
			//if((detailsHeader = (struct ProcessDetailsList*)malloc(sizeof(struct ProcessDetailsList*))) != NULL){
				if((listHeader = (struct ProcessDetails*)malloc(sizeof(struct ProcessDetails*))) != NULL){
					char *argv = "pwd";
					strncpy(listHeader->programAndArgs, argv, 4);
					listHeader->pid = pid;
					listHeader->done = 0;
					listHeader->startTime = clock();
					listHeader->next = NULL;
				}
			//}
		}
		else{
			//add to list
			struct ProcessDetails *ptr = listHeader;
			while(ptr != NULL){
				printf("list: %d\n", ptr->pid);
				ptr = ptr->next;
			}

			printf("add to list\n");
			if((ptr = (struct ProcessDetails*)malloc(sizeof(struct ProcessDetails*))) != NULL){
				char *argv = "pwd";
				strncpy(ptr->programAndArgs, argv, 4);
				ptr->pid = pid;
				ptr->done = 0;
				ptr->startTime = clock();
				ptr->next = NULL;
			}
			else{
				printf("couldnt malloc\n");
			}
		}
		sigprocmask(SIG_SETMASK, &prev_mask, NULL);
		printf("parent done\n");
	}
}

void fgBuiltin(int pid){
	sigset_t mask, prev_mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
	originalPGID = tcgetpgrp(STDIN_FILENO);
	if(listHeader != NULL){
		struct ProcessDetails *ptr = listHeader;
		while(ptr != NULL){
			if(ptr->pid == pid){
				pid_t processGroup = getpgid(pid);

				tcsetpgrp(STDIN_FILENO, processGroup);
				break;
			}
			ptr = ptr->next;
		}
		fprintf(stderr, "Process with ID %d doesn't exist.\n", pid);
	}

	sigprocmask(SIG_SETMASK, &prev_mask, NULL);
}*/

/*void killBuiltin(int pid){
	if(detailsHeader != NULL){
		struct ProcessDetails *ptr = detailsHeader->list;
		while(ptr != NULL){
			if(ptr->pid == pid){
				if(kill(pid, SIGKILL) != 0){
					perror("sending SIGKILL did not work.");
				}
				break;
			}
			ptr = ptr->next;
		}
		fprintf(stderr, "Process with ID %d doesn't exist.\n", pid);
	}
}

void cloneBuiltin(int pid){
	if(detailsHeader != NULL){
		struct ProcessDetails *ptr = detailsHeader->list;
		while(ptr != NULL){
			if(ptr->pid == pid){
				if(strncmp(ptr->programAndArgs[0], "pwd", 3) == 0){
					pwdBuiltinBackground();
				}
				else{
					pid_t clonePid;
					if((clonePid = fork()) == 0){
						if((strchr(ptr->programAndArgs[0], '/')) != NULL){// / found, just stat and exec
							struct stat fileStat;
							char executableCpy[strlen(ptr->programAndArgs[0]) + 1];
							strcpy(executableCpy, ptr->programAndArgs[0]);

							char *pwd = malloc(128);
							getcwd(pwd, 128);
							char fullPath[strlen(pwd) + strlen(ptr->programAndArgs[0]) + 2];
							strcpy(fullPath, pwd);
							strcat(fullPath, "/");
							strcat(fullPath, executableCpy);
							free(pwd);

							int y = stat(fullPath, &fileStat);

							if(y == 0){
								restoreDefaultHandlers();
								sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
								setpgid(0, 0);
								execv(fullPath, ptr->programAndArgs);
							}
							else{
								fprintf(stderr, "%s: command not found\n", ptr->programAndArgs[0]);
								cleanup();
								exit(3);
							}
						}
						else{//need to search PATH
							char *pathEnv = getenv("PATH");
							char *path = strtok(pathEnv, ":");
							char executableCpy[strlen(ptr->programAndArgs[0]) + 1];
							strcpy(executableCpy, ptr->programAndArgs[0]);

							while(path != NULL){

								path = strtok(NULL, ":");

								if(path != NULL){
									char pathCpy[strlen(path) + strlen(ptr->programAndArgs[0]) + 1];
									strcpy(pathCpy, path);
									strcpy(executableCpy, ptr->programAndArgs[0]);

									strcat(pathCpy, "/");
									strcat(pathCpy, executableCpy);

									struct stat fileStat;
									int y = stat(pathCpy, &fileStat);
									if(y == 0){
										restoreDefaultHandlers();
										sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
										setpgid(0, 0);
										execv(pathCpy, ptr->programAndArgs);
									}
								}
							}
							fprintf(stderr, "%s: command not found\n", ptr->programAndArgs[0]);
						}
					}
					else{
						if(clonePid == -1){
							perror("fork error");
						}
						else{
							sigset_t mask, prev_mask;
						    sigemptyset(&mask);
						    sigfillset(&mask);
						    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
							restoreAllFD();
							if(detailsHeader == NULL){
								if((detailsHeader = (struct ProcessDetailsList*)malloc(sizeof(struct ProcessDetailsList*))) != NULL){
									if((detailsHeader->list = (struct ProcessDetails*)malloc(sizeof(struct ProcessDetails*))) != NULL){
										detailsHeader->list->programAndArgs = ptr->programAndArgs;
										detailsHeader->list->index = ptr->index;
										detailsHeader->list->pid = clonePid;
										detailsHeader->list->done = 0;
										detailsHeader->list->startTime = clock();
										detailsHeader->list->next = NULL;
									}
								}
							}
							else{
								//add to list
								struct ProcessDetails *ptr = detailsHeader->list;
								while(ptr->next != NULL){
									ptr = ptr->next;
								}

								if((ptr->next = (struct ProcessDetails*)malloc(sizeof(struct ProcessDetails*))) != NULL){
									ptr->next->programAndArgs = ptr->programAndArgs;
									ptr->next->index = ptr->index;
									ptr->next->pid = pid;
									ptr->next->done = 0;
									ptr->next->startTime = clock();
									ptr->next->next = NULL;
								}
							}
							sigprocmask(SIG_SETMASK, &prev_mask, NULL);
						}
					}
				}
				break;
			}
			ptr = ptr->next;
		}
		fprintf(stderr, "Process with ID %d doesn't exist.\n", pid);
	}
}*/

/*void jobsBuiltin(){
	sigset_t mask, prev_mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
	int printed = 0;
	printf("%15s\t%50s\t%30s\n", "PROCESS ID", "PROGRAM WITH ARGS", "HOW LONG ITS BEEN RUNNING");
	printf("%15s\t%50s\t%30s\n", "---------------", "------------------------------", "------------------------------");
	if(listHeader != NULL){
		struct ProcessDetails *ptr = listHeader;
		while(ptr != NULL){
			if(ptr->done != 1){
				printed = 1;
				clock_t endTime = clock();
				long x = ptr->startTime;
			    int cpuTime = ((double)(endTime - x) / (double)CLOCKS_PER_SEC) * 1000000;

				printf("%15d\t%50s\t%20d microseconds\n", ptr->pid, ptr->programAndArgs, cpuTime);
			}
			ptr = ptr->next;
		}
		if(printed == 0){
			fprintf(stderr, "There are no background processes.\n");
		}
	}
	else{
		fprintf(stderr, "There are no background processes.\n");
	}
	sigprocmask(SIG_SETMASK, &prev_mask, NULL);
}*/

void cdBuiltin(char *arg){
	if(arg == NULL){
		return;
	}

	if(lastDirectory == NULL){
		lastDirectory = malloc(256);
		getcwd(lastDirectory, 256);
	}

	if(strlen(arg) == 0){
		//home
		char *home = getenv("HOME");
		execChangeDirectory(arg, home);
	}

	else if(strncmp(arg, "..", 2) == 0){
		if(strlen(arg) == 2 || (strlen(arg) == 3 && *(arg + 2) == 47)){
			execChangeDirectory(arg, arg);
		}
		else if(strlen(arg) > 3){
			int y = checkArgForWhitespace(arg + 3);
            if(y == 1 || y == 0){
                execChangeDirectory(arg, "../");
            }
            else{
            	char originalArg[strlen(arg)];
				strcpy(originalArg, arg);
                execChangeDirectory(arg, originalArg);
            }
		}
		else{
			fprintf(stderr, "cd: %s: no such file or directory\n", arg);
		}
	}
	else if(strncmp(arg, ".", 1) == 0){
		if(strlen(arg) == 1 || (strlen(arg) == 2 && *(arg + 1) == 47) || (strlen(arg) == 2 && *(arg + 1) == 32)){
			execChangeDirectory(arg, arg);
		}
		else if(strlen(arg) > 2){
			int y = checkArgForWhitespace(arg + 2);
            if(y == 1 || y == 0){
                execChangeDirectory(arg, "./");
            }
            else{
            	/*if(*(arg + 2) == 32){
            		execChangeDirectory(arg, "./");
                }
                else{*/
                	char originalArg[strlen(arg)];
                	strcpy(originalArg, arg);

                	/*char *y = strchr(arg, ' ');
                	if(y != NULL){
                		*y = 0;
                	}*/

                    execChangeDirectory(arg, originalArg);
                //}
            }
		}
		else{
			fprintf(stderr, "cd: %s: no such file or directory\n", arg);
		}
	}

	else if(strncmp(arg, "-", 1) == 0){
		if(strlen(arg) == 1){
			execLastDirectory(arg);
		}
		else{
			int y = checkArgForWhitespace(arg + 1);
            if(y == 1 || y == 0){
                execLastDirectory(arg);
            }
            else{
            	fprintf(stderr, "cd: %s: invalid argument\n", arg);
            }
		}
	}
	else{
		execChangeDirectory(arg, arg);
	}
}

void execLastDirectory(char *arg){
	if(lastDirectory != NULL){
		char *temp = malloc(256);
		getcwd(temp, 256);
		int x = chdir(lastDirectory);
		if(x == 0){
			//printf("%s\n", lastDirectory);
			strncpy(lastDirectory, temp, 256);
		}
		else{
			fprintf(stderr, "cd: %s: no such file or directory\n", arg);
		}
		free(temp);
	}
	else{
		fprintf(stderr, "cd: %s: no other directory visited\n", arg);
	}
}

void execChangeDirectory(char *arg, char *directory){
	char *temp = malloc(256);
	getcwd(temp, 256);
	int x = chdir(directory);
	if(x == 0){
		//printf("%s\n", directory);
		strncpy(lastDirectory, temp, 256);
	}
	else{
		fprintf(stderr, "cd: %s: no such file or directory\n", arg);
	}
	free(temp);
}

void runProcess(char *arg){
	int child_status;

	if((pid = fork()) == 0){
		char *pathEnv = getenv("PATH");
		char *x;
		argCpy = strdup(arg);
		char *program = strtok(argCpy, " ");
		char *argv[strlen(arg)];
		char executable[strlen(program) + 1];
		strcpy(executable, program);

		argv[0] = executable;

		program = strtok(NULL, " ");

		for(int z = 1; program != NULL; z++){
			for(int i = 0; *(program) == 32; i++){
	            program = program + i;
	        }
			argv[z] = program;
			program = strtok(NULL, " ");
		}

		if((x = strchr(executable, '/')) != NULL){// / found, just stat and exec
			struct stat fileStat;

			char *pwd = malloc(128);
			getcwd(pwd, 128);
			char fullPath[strlen(pwd) + strlen(executable) + 2];
			strcpy(fullPath, pwd);
			strcat(fullPath, "/");
			strcat(fullPath, executable);
			free(pwd);

			int y = stat(fullPath, &fileStat);

			if(y == 0){
				restoreDefaultHandlers();
				sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
				execv(fullPath, argv);
			}
			else{
				fprintf(stderr, "%s: command not found\n", executable);
				cleanup();
				exit(3);
			}
		}
		else{//need to search PATH
			char *path = strtok(pathEnv, ":");
			char executableCpy[strlen(executable) + 1];
			strcpy(executableCpy, executable);

			while(path != NULL){

				path = strtok(NULL, ":");

				if(path != NULL){
					char pathCpy[strlen(path) + strlen(executable) + 1];
					strcpy(pathCpy, path);
					strcpy(executableCpy, executable);

					strcat(pathCpy, "/");
					strcat(pathCpy, executableCpy);

					struct stat fileStat;
					int y = stat(pathCpy, &fileStat);
					if(y == 0){
						restoreDefaultHandlers();
						sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
						execv(pathCpy, argv);
					}
				}
			}
			fprintf(stderr, "%s: command not found\n", executable);
			cleanup();
			exit(3);
		}
	}
	restoreAllFD();
	waitpid(pid, &child_status, 0);
}

/*void runProcessBackground(char *arg){
	char *pathEnv = getenv("PATH");
	char *x;
	char *argv[strlen(arg)];
	argCpy = strdup(arg);
	char *program = strtok(argCpy, " ");
	char executable[strlen(program) + 1];
	strcpy(executable, program);

	argv[0] = executable;
	int z = 0;
	for(z = 1; program != NULL; z++){
		program = strtok(NULL, " ");
		for(int i = 0; *(arg + i) == 32; i++){
            program = program + i;
        }
		argv[z] = program;
	}

	if((pid = fork()) == 0){
		if((x = strchr(executable, '/')) != NULL){// / found, just stat and exec
			struct stat fileStat;

			char *pwd = malloc(128);
			getcwd(pwd, 128);
			char fullPath[strlen(pwd) + strlen(executable) + 2];
			strcpy(fullPath, pwd);
			strcat(fullPath, "/");
			strcat(fullPath, executable);
			free(pwd);

			int y = stat(fullPath, &fileStat);

			if(y == 0){
				setpgid(0, 0);
				restoreDefaultHandlers();
				sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
				execv(fullPath, argv);
			}
			else{
				fprintf(stderr, "%s: command not found\n", executable);
				cleanup();
				cleanupList();
				exit(3);
			}
		}
		else{//need to search PATH
			char *path = strtok(pathEnv, ":");
			char executableCpy[strlen(executable) + 1];
			strcpy(executableCpy, executable);

			while(path != NULL){

				path = strtok(NULL, ":");

				if(path != NULL){
					char pathCpy[strlen(path) + strlen(executable) + 1];
					strcpy(pathCpy, path);
					strcpy(executableCpy, executable);

					strcat(pathCpy, "/");
					strcat(pathCpy, executableCpy);

					struct stat fileStat;
					int y = stat(pathCpy, &fileStat);
					if(y == 0){
						setpgid(0, 0);
						restoreDefaultHandlers();
						sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
						execv(pathCpy, argv);
					}
				}
			}
			fprintf(stderr, "%s: command not found\n", executable);
			cleanup();
			cleanupList();
			exit(3);
		}
	}
	else{
		if(pid == -1){
			perror("fork error");
		}
		else{
			sigset_t mask, prev_mask;
		    sigfillset(&mask);
		    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
			restoreAllFD();
			setupSigHandlers();
			if(listHeader == NULL){
				//if((detailsHeader = (struct ProcessDetailsList*)malloc(sizeof(struct ProcessDetailsList*))) != NULL){
					if((listHeader = (struct ProcessDetails*)malloc(sizeof(struct ProcessDetails*))) != NULL){
						strcpy(listHeader->programAndArgs, arg);
						listHeader->pid = pid;
						listHeader->done = 0;
						listHeader->startTime = clock();
						listHeader->next = NULL;
					}
				//}
			}
			else{
				//add to list
				struct ProcessDetails *ptr = listHeader;
				while(ptr->next != NULL){
					ptr = ptr->next;
				}

				if((ptr->next = (struct ProcessDetails*)malloc(sizeof(struct ProcessDetails*))) != NULL){
					strcpy(ptr->next->programAndArgs, arg);
					ptr->next->pid = pid;
					ptr->next->done = 0;
					ptr->next->startTime = clock();
					ptr->next->next = NULL;
				}
			}
			sigprocmask(SIG_SETMASK, &prev_mask, NULL);
		}
	}
}*/

void runProcessWithoutFork(char *arg){
	char *pathEnv = getenv("PATH");
	char *x;
	argCpy = strdup(arg);
	char *program = strtok(argCpy, " ");
	char *argv[strlen(arg)];
	char executable[strlen(program) + 1];
	strcpy(executable, program);

	argv[0] = executable;

	program = strtok(NULL, " ");

	for(int z = 1; program != NULL; z++){
		for(int i = 0; *(program) == 32; i++){
            program = program + i;
        }
		argv[z] = program;
		fprintf(stderr, "program:%s\n", program);
		program = strtok(NULL, " ");
	}

	if((x = strchr(executable, '/')) != NULL){// / found, just stat and exec
		struct stat fileStat;

		char *pwd = malloc(128);
		getcwd(pwd, 128);
		char fullPath[strlen(pwd) + strlen(executable) + 2];
		strcpy(fullPath, pwd);
		strcat(fullPath, "/");
		strcat(fullPath, executable);
		free(pwd);

		int y = stat(fullPath, &fileStat);

		if(y == 0){
			restoreDefaultHandlers();
			sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
			execv(fullPath, argv);
		}
		else{
			fprintf(stderr, "%s: command not found\n", executable);
			cleanup();
			exit(3);
		}
	}
	else{//need to search PATH
		char *path = strtok(pathEnv, ":");
		char executableCpy[strlen(executable) + 1];
		strcpy(executableCpy, executable);

		while(path != NULL){

			path = strtok(NULL, ":");

			if(path != NULL){
				char pathCpy[strlen(path) + strlen(executable) + 1];
				strcpy(pathCpy, path);
				strcpy(executableCpy, executable);

				strcat(pathCpy, "/");
				strcat(pathCpy, executableCpy);

				struct stat fileStat;
				int y = stat(pathCpy, &fileStat);
				if(y == 0){
					restoreDefaultHandlers();
					sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
					execv(pathCpy, argv);
				}
			}
		}
		fprintf(stderr, "%s: command not found\n", executable);
		cleanup();
		exit(3);
	}
}

void cleanup(){
	sigset_t mask, prev_mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
	if(lastDirectory != NULL){
		free(lastDirectory);
	}
	if(argCpy != NULL){
		free(argCpy);
	}
	sigprocmask(SIG_SETMASK, &prev_mask, NULL);
}

/*void cleanupList(){
	sigset_t mask, prev_mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
	if(listHeader != NULL){
		struct ProcessDetails *ptr = listHeader;
		cleanBGList(ptr);
	}
	sigprocmask(SIG_SETMASK, &prev_mask, NULL);
}

void cleanBGList(struct ProcessDetails *ptr){
	if(ptr != NULL)
    {
        cleanBGList(ptr->next);

        free(ptr);
    }
}*/

void redirectFD(char redirectionChar, char *file){
	if(redirectionChar == '<'){
		fflush(stdin);
		stdinFD = dup(STDIN_FILENO);

		char *space = strrchr(file, ' ');
		if(space != NULL){
			*space = 0;
		}

		int newSTDIN = open(file, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if(newSTDIN == -1){
			newSTDIN = open(file, O_RDONLY);
		}

		int fd = dup2(newSTDIN, STDIN_FILENO);
		close(newSTDIN);
		if(fd == -1){
			perror("dup2 error");
		}
	}
	else{
		fflush(stdout);
		stdoutFD = dup(STDOUT_FILENO);
		//printf("New stdoutFD: %d\n", stdoutFD);

		char *space = strrchr(file, ' ');
		if(space != NULL){
			*space = 0;
		}

		int newSTDOUT = open(file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		//printf("newSTDOUT: %d\n", newSTDOUT);
		if(newSTDOUT == -1){
			newSTDOUT = open(file, O_WRONLY | O_TRUNC);
			if(newSTDOUT == -1){
				perror("open error");
			}
			//printf("newSTDOUT: %d\n", newSTDOUT);
		}

		//printf("about to dup2\n");

		int fd = dup2(newSTDOUT, STDOUT_FILENO);
		close(newSTDOUT);
		if(fd == -1){
			perror("dup2 error");
		}
	}
}

void redirectSpecialSpecificFD(int fd, char *file){
	if(fd == 1){
		fflush(stdout);
		stdoutFD = dup(fd);
	}
	else if(fd == 2){
		fflush(stderr);
		stderrFD = dup(fd);
	}

	char *space = strrchr(file, ' ');
	if(space != NULL){
		*space = 0;
	}

	int newSTDOUT = open(file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(newSTDOUT == -1){
		newSTDOUT = open(file, O_WRONLY | O_TRUNC);
	}

	int fdx = dup2(newSTDOUT, fd);
	close(newSTDOUT);
	if(fdx == -1){
		perror("dup2 error");
	}
}

void redirectSpecialBoth(char *file){
	fflush(stdout);
	stdoutFD = dup(STDOUT_FILENO);

	char *space = strrchr(file, ' ');
	if(space != NULL){
		*space = 0;
	}

	int newOUT = open(file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(newOUT == -1){
		newOUT = open(file, O_WRONLY | O_TRUNC);
	}

	int fd = dup2(newOUT, STDOUT_FILENO);
	if(fd == -1){
		perror("dup2 error");
	}

	fflush(stderr);
	stderrFD = dup(STDERR_FILENO);

	fd = dup2(newOUT, STDERR_FILENO);
	close(newOUT);
	if(fd == -1){
		perror("dup2 error");
	}
}

void redirectAppend(char *file){
	fflush(stdout);
	stdoutFD = dup(STDOUT_FILENO);

	char *space = strrchr(file, ' ');
	if(space != NULL){
		*space = 0;
	}

	int newSTDOUT = open(file, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(newSTDOUT == -1){
		newSTDOUT = open(file, O_WRONLY | O_APPEND);
	}

	int fd = dup2(newSTDOUT, STDOUT_FILENO);
	close(newSTDOUT);
	if(fd == -1){
		perror("dup2 error");
	}
}

void redirectFDHereDoc(int fd){
	fflush(stdin);
	stdinFD = dup(STDIN_FILENO);

	int dupReturn = dup2(fd, STDIN_FILENO);

	if(dupReturn == -1){
		perror("dup2 error");
	}
}

void restoreFD(char redirectionChar){
	if(redirectionChar == '<'){
		fflush(stdin);

		int fd = dup2(stdinFD, STDIN_FILENO);
		close(stdinFD);
		if(fd == -1){
			perror("dup2 error");
		}
		stdinFD = -1;
	}
	else{
		fflush(stdout);

		int fd = dup2(stdoutFD, STDOUT_FILENO);
		close(stdoutFD);
		if(fd == -1){
			perror("dup2 error");
		}
		stdoutFD = -1;
	}
}

void restoreAllFD(){
	if(stdinFD != -1){
		fflush(stdin);

		int fd = dup2(stdinFD, STDIN_FILENO);
		close(stdinFD);
		if(fd == -1){
			perror("dup2 error");
		}
		stdinFD = -1;
	}

	if(stdoutFD != -1){
		fflush(stdout);

		int fd = dup2(stdoutFD, STDOUT_FILENO);
		close(stdoutFD);
		if(fd == -1){
			perror("dup2 error");
		}
		stdoutFD = -1;
	}

	if(stderrFD != -1){
		fflush(stderr);

		int fd = dup2(stderrFD, STDERR_FILENO);
		close(stderrFD);
		if(fd == -1){
			perror("dup2 error");
		}
		stderrFD = -1;
	}
}

void setupPipe(char *program1, char *program2, char *cmd){
	int child_status;
	if(program2 == NULL){
        fprintf(stderr, "%s is not a valid command\n", cmd);
        return;
    }
    else if(strlen(program1) == 1 && *program1 == 32){
        fprintf(stderr, "%s is not a valid command\n", cmd);
        return;
    }
    else if(strlen(program2) == 1 && *program2 == 32){
        fprintf(stderr, "%s is not a valid command\n", cmd);
        return;
    }

	if(*program2 == 32){
        program2 = program2 + 1;
    }

    int pipeFD[2];
    pid_t pid2;

    if(pipe(pipeFD) == 0){//pipeFD[0] = read/stdin    pipeFD[1] = write/stdout
    	fflush(stdout);
		fflush(stdin);
		stdoutFD = dup(STDOUT_FILENO);
		stdinFD = dup(STDIN_FILENO);

		if((pid = fork()) == 0){//child
			if(dup2(pipeFD[1], STDOUT_FILENO) == -1){
				//printf("error\n");
				fprintf(stderr, "Error while changing STDIN\n");
				exit(3);
			}
			close(pipeFD[0]);

			if(strncmp(program1, "pwd", 3) == 0){
		        if(strlen(program1) > 3){
		            int x = checkArgForWhitespace(program1 + 3);
		            if(x == 1 || x == 0){
		                pwdBuiltinNoFork();
		            }
		            else{
		                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", program1);
		            }
		        }
		        else if(strlen(program1) == 3){
		            pwdBuiltinNoFork();
		        }
		        else{
		            fprintf(stderr, "%s is not a valid command.\n", program1);
		        }
		    }
		    else{
		        runProcessWithoutFork(program1);
		    }
		}
		else if(pid == -1){
			perror("fork error");
		}

		if((pid2 = fork()) == 0){
			if(dup2(pipeFD[0], STDIN_FILENO) == -1){
				fprintf(stderr, "Error while changing STDIN\n");
				exit(3);
			}
			close(pipeFD[1]);

			if(strncmp(program2, "pwd", 3) == 0){
		        if(strlen(program2) > 3){
		            int x = checkArgForWhitespace(program2 + 3);
		            if(x == 1 || x == 0){
		                pwdBuiltinNoFork();
		            }
		            else{
		                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", program2);
		            }
		        }
		        else if(strlen(program2) == 3){
		            pwdBuiltinNoFork();
		        }
		        else{
		            fprintf(stderr, "%s is not a valid command.\n", program2);
		        }
		    }
		    else{
		        runProcessWithoutFork(program2);
		    }
		}
		else if(pid2 == -1){
			perror("fork error");
		}

		close(pipeFD[0]);
		close(pipeFD[1]);
		int exit;
		restoreAllFD();
		while((exit = wait(&child_status)) != -1);
    }
    else{
    	perror("pipe error");
    }
}

void setup3Pipe(char *program1, char *program2, char *program3, char *cmd){
	int child_status;
	if(program2 == NULL){
        fprintf(stderr, "%s is not a valid command\n", cmd);
        return;
    }
    else if(strlen(program1) == 1 && *program1 == 32){
        fprintf(stderr, "%s is not a valid command\n", cmd);
        return;
    }
    else if(strlen(program2) == 1 && *program2 == 32){
        fprintf(stderr, "%s is not a valid command\n", cmd);
        return;
    }

	if(*program2 == 32){
        program2 = program2 + 1;
    }

    if(*program3 == 32){
        program3 = program3 + 1;
    }

    //printf("P1: %s\n", program1);
    //printf("P2: %s\n", program2);
    //printf("P3:%s\n", program3);


    int pipeFD1[2];
    int pipeFD2[2];
    pid_t pid2, pid3;

    if(pipe(pipeFD1) == 0 && pipe(pipeFD2) == 0){//pipeFD[0] = read/stdin    pipeFD[1] = write/stdout
    	fflush(stdout);
		fflush(stdin);
		stdoutFD = dup(STDOUT_FILENO);
		stdinFD = dup(STDIN_FILENO);

		if((pid = fork()) == 0){//child
			int fd = dup2(pipeFD1[1], STDOUT_FILENO);
			if(fd == -1){
				fprintf(stderr, "Error while changing STDIN\n");
				exit(3);
			}
			close(pipeFD1[0]);

			if(strncmp(program1, "pwd", 3) == 0){
		        if(strlen(program1) > 3){
		            int x = checkArgForWhitespace(program1 + 3);
		            if(x == 1 || x == 0){
		                pwdBuiltinNoFork();
		            }
		            else{
		                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
		            }
		        }
		        else if(strlen(cmd) == 3){
		            pwdBuiltinNoFork();
		        }
		        else{
		            fprintf(stderr, "%s is not a valid command.\n", cmd);
		        }
		    }
		    else{
		        runProcessWithoutFork(program1);
		    }
		}
		else if(pid == -1){
			perror("fork error");
		}

		if((pid2 = fork()) == 0){
			int fd = dup2(pipeFD1[0], STDIN_FILENO);
			if(fd == -1){
				fprintf(stderr, "Error while changing STDIN\n");
				exit(3);
			}
			close(pipeFD1[1]);

			fd = dup2(pipeFD2[1], STDOUT_FILENO);
			if(fd == -1){
				fprintf(stderr, "Error while changing STDIN\n");
				exit(3);
			}
			close(pipeFD2[0]);

			if(strncmp(program2, "pwd", 3) == 0){
		        if(strlen(program2) > 3){
		            int x = checkArgForWhitespace(program2 + 3);
		            if(x == 1 || x == 0){
		                pwdBuiltinNoFork();
		            }
		            else{
		                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
		            }
		        }
		        else if(strlen(cmd) == 3){
		            pwdBuiltinNoFork();
		        }
		        else{
		            fprintf(stderr, "%s is not a valid command.\n", cmd);
		        }
		    }
		    else{
		        runProcessWithoutFork(program2);
		    }
		}
		else if(pid2 == -1){
			perror("fork error");
		}
		close(pipeFD1[0]);
		close(pipeFD1[1]);

		if((pid3 = fork()) == 0){
			int fd = dup2(pipeFD2[0], STDIN_FILENO);
			if(fd == -1){
				fprintf(stderr, "Error while changing STDIN\n");
				exit(3);
			}
			close(pipeFD2[1]);

			if(strncmp(program3, "pwd", 3) == 0){
		        if(strlen(program3) > 3){
		            int x = checkArgForWhitespace(program3 + 3);
		            if(x == 1 || x == 0){
		                pwdBuiltinNoFork();
		            }
		            else{
		                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
		            }
		        }
		        else if(strlen(cmd) == 3){
		            pwdBuiltinNoFork();
		        }
		        else{
		            fprintf(stderr, "%s is not a valid command.\n", cmd);
		        }
		    }
		    else{
		        runProcessWithoutFork(program3);
		    }
		}
		else if(pid3 == -1){
			perror("fork error");
		}

		close(pipeFD2[0]);
		close(pipeFD2[1]);
		int exit;
		while((exit = wait(&child_status)) != -1);
		fflush(stdin);
		int fd = dup2(stdinFD, STDIN_FILENO);
		if(fd == -1){
			perror("dup2 error");
		}
		fflush(stdout);

		fd = dup2(stdoutFD, STDOUT_FILENO);
		if(fd == -1){
			fprintf(stderr, "Error while restoring STDOUT\n");
		}
    }
    else{
    	perror("pipe error");
    }
}

void setupSigHandlers(){
	struct sigaction alarmAction;
    sigemptyset(&alarmAction.sa_mask);
    alarmAction.sa_sigaction = sigalrmHandler;
    alarmAction.sa_flags = SA_SIGINFO;

    if(sigaction(SIGALRM, &alarmAction, NULL) == -1){
        perror("sigaction(SIGALRM) error");
        return;
    }

    struct sigaction childAction;
    sigemptyset(&childAction.sa_mask);
    childAction.sa_sigaction = sigchldHandler;
    childAction.sa_flags = SA_SIGINFO;

    if(sigaction(SIGCHLD, &childAction, NULL) == -1){
        perror("sigaction(SIGCHLD) error");
        return;
    }

    struct sigaction usr2Action;
    sigemptyset(&usr2Action.sa_mask);
    usr2Action.sa_handler = sigusr2Handler;
    usr2Action.sa_flags = 0;

    if(sigaction(SIGUSR2, &usr2Action, NULL) == -1){
        perror("sigaction(SIGUSR2) error");
        return;
    }
}
void restoreDefaultHandlers(){
    if(signal(SIGALRM, SIG_DFL) == SIG_ERR){
        perror("signal(SIGALRM) error");
        exit(3);
    }

    if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){
        perror("signal(SIGCHLD) error");
        exit(3);
    }

    if(signal(SIGUSR2, SIG_DFL) == SIG_ERR){
        perror("signal(SIGUSR2) error");
        exit(3);
    }
}

void sigalrmHandler(int signal, siginfo_t *siginfo, void *context){
	int oldErrno = errno;
	sigset_t mask, prev_mask;
    sigemptyset(&mask);
    sigfillset(&mask);

    sigprocmask(SIG_BLOCK, &mask, &prev_mask);

    Sio_puts("\nYour ");
    Sio_putl(alarmTime);
    Sio_puts(" second timer has finished!\n");

    char *prompt = shellHeader();
    Sio_puts(prompt);
    free(prompt);

	sigprocmask(SIG_SETMASK, &prev_mask, NULL);
	errno = oldErrno;
}

void sigchldHandler(int signal, siginfo_t *siginfo, void *context){
	int oldErrno = errno;
	sigset_t mask, prev_mask;
    sigfillset(&mask);

    sigprocmask(SIG_BLOCK, &mask, &prev_mask);
    long x = siginfo->si_utime;
    long y = siginfo->si_stime;
    double cpuTime = ((double)(x + y) / (double)CLOCKS_PER_SEC) * 1000;

    /*if(listHeader != NULL){
		struct ProcessDetails *ptr = listHeader;
    	while(ptr != NULL){
    		if(ptr->pid == siginfo->si_pid){
    			ptr->done = 1;
    			printf("done\n");
    			if(originalPGID != -1){
	    			tcsetpgrp(STDIN_FILENO, originalPGID);
    			}
    			break;
    		}
			ptr = ptr->next;
		}
	}*/

	/*if(WIFEXITED(siginfo->si_status)){
		Sio_puts("Child exited with exit status ");
		Sio_putl(WEXITSTATUS(siginfo->si_status));
	}*/

	Sio_puts("\nChild with PID ");
	Sio_putl(siginfo->si_pid);
	Sio_puts(" has died. It spent ");
	Sio_putl(cpuTime);
	Sio_puts(" microseconds utilizing the CPU.\n");

	sigprocmask(SIG_SETMASK, &prev_mask, NULL);
	errno = oldErrno;
}

void sigusr2Handler(int signal){
	int oldErrno = errno;
	sigset_t mask, prev_mask;
    sigemptyset(&mask);
    sigfillset(&mask);

    sigprocmask(SIG_BLOCK, &mask, &prev_mask);

	char *string = "\nWell that was easy.\n";
	write(STDOUT_FILENO, string, 25);

	sigprocmask(SIG_SETMASK, &prev_mask, NULL);
	errno = oldErrno;
}

char *returnPwd(){
	char *pwd = malloc(128);
	getcwd(pwd, 128);
	return pwd;
}

char *shellHeader(){
	char *prompt = malloc(256);
	*prompt = '\0';
    char *pwd = returnPwd();
    strcat(prompt, "<tprasad> : <");
    strcat(prompt, pwd);
    strcat(prompt, "> $ ");
    free(pwd);
	return prompt;
}

int checkArgForWhitespace(char *arg){//
	if(strlen(arg) > 0){
        int x = strlen(arg);
        int count = 0;

        for(int i = 0; i < x; i++){
        	if(*(arg + i) == 32){
        		count++;
        	}
        }
        if(count == x){
        	return 1;//arg is just whitespace
        }
        else return -1;//arg has other chars in it
    }
    return 0;//arg is length 0
}
