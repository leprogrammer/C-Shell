#include "sfish.h"
#include "debug.h"

/*
 * As in previous hws the main function must be in its own file!
 */
volatile int alarmTime = 0;

int main(int argc, char const *argv[], char* envp[]){
    /* DO NOT MODIFY THIS. If you do you will get a ZERO. */
    rl_catch_signals = 0;
    /* This is disable readline's default signal handlers, since you are going to install your own.*/
    char *cmd;
    char *cmdHereDoc;

    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);

    sigprocmask(SIG_BLOCK, &mask, &prev_maskTop);

    char *shellLine = shellHeader();

    setupSigHandlers();


    while((cmd = readline(shellLine)) != NULL) {
        free(shellLine);
        if(strlen(cmd) == 0){

        }
        else{
            if (strncmp(cmd, "exit", 4) == 0){
                if(strlen(cmd) > 4){
                    int x = checkArgForWhitespace(cmd + 4);
                    if(x == 1 || x == 0){
                        cleanup();
                        free(cmd);
                        sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
                        return EXIT_SUCCESS;
                    }
                    else{
                        fprintf(stdout, "%s is not a valid command.\n", cmd);
                    }
                }
                else if(strlen(cmd) == 4){
                    cleanup();
                    free(cmd);
                    sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);
                    return EXIT_SUCCESS;
                }
                else{
                    fprintf(stdout, "%s is not a valid command.\n", cmd);
                }
            }

            /*else if (strncmp(cmd, "jobs", 4)==0){
                if(strlen(cmd) > 4){
                    int x = checkArgForWhitespace(cmd + 4);
                    if(x == 1 || x == 0){
                        jobsBuiltin();
                    }
                    else{
                        fprintf(stdout, "%s is not a valid command.\n", cmd);
                    }
                }
                else if(strlen(cmd) == 4){
                    jobsBuiltin();
                }
                else{
                    fprintf(stdout, "%s is not a valid command.\n", cmd);
                }
            }*/

            else if(strncmp(cmd, "cd", 2) == 0){
                char *arg = NULL;
                int x = checkArgForWhitespace(cmd + 2);
                if(x == 1 || x == 0){
                    cdBuiltin("");
                }
                else{
                    if(*(cmd + 2) == 32){
                        arg = strdup(cmd + 3);
                        char *argTemp = arg;
                        for(int i = 0; *(argTemp) == 32; i++){
                            argTemp = argTemp + i;
                        }
                        cdBuiltin(argTemp);
                        free(arg);
                    }
                    else{
                        fprintf(stderr, "%s is not a valid command.\n", cmd);
                    }
                    /*if(*(cmd + 2) == 32){
                        arg = strdup(cmd + 3);
                        cdBuiltin(arg);
                        free(arg);
                    }
                    else{
                        fprintf(stderr, "%s is not a valid command.\n", cmd);
                    }*/
                }
            }
            else if(strncmp(cmd, "help", 4) == 0){
                if(strlen(cmd) > 4){
                    int x = checkArgForWhitespace(cmd + 4);
                    if(x == 1 || x == 0){
                        helpBuiltin();
                    }
                    else{
                        fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                    }
                }
                else if(strlen(cmd) == 4){
                    helpBuiltin();
                }
                else{
                    fprintf(stderr, "%s is not a valid command.\n", cmd);
                }
            }
            else if(strncmp(cmd, "alarm", 5) == 0){
                char *arg = NULL;
                int x = checkArgForWhitespace(cmd + 5);
                if(x == 1 || x == 0){
                    fprintf(stderr, "%s has no arguments.\n", cmd);
                }
                else{
                    if(*(cmd + 5) == 32){
                        arg = strdup(cmd + 6);

                        int i = 0;
                        for(i = 0; isdigit(*(arg + i)); i++){}

                        if(i == strlen(arg)){
                            int x = atoi(arg);
                            if(x <= 0){
                                fprintf(stderr, "%s is not a valid command.\n", cmd);
                            }
                            else{
                                alarmTime = x;
                                alarm(x);
                            }
                        }
                        free(arg);
                    }
                    else{
                        fprintf(stderr, "%s is not a valid command.\n", cmd);
                    }
                }
            }
            else{
                char *specialRedirect1 = "1>";
                char *specialRedirect2 = "2>";
                char *specialRedirectAnd = "&>";
                char *specialRedirectAppend = ">>";
                char *specialHereDoc = "<<";
                char *cmdDup = strdup(cmd);
                if(strstr(cmdDup, specialRedirect1) != NULL){
                    strcpy(cmdDup, cmd);
                    char *programAndArgs = strtok(cmdDup, "1>");
                    char *file = strtok(NULL, "1>");

                    /*printf("programAndArgs: %s\n", programAndArgs);
                    printf("File: %s\n", file);*/

                    if(*file == 32){
                        redirectSpecialSpecificFD(1, file + 1);
                    }
                    else{
                        redirectSpecialSpecificFD(1, file);
                    }

                    if(strncmp(programAndArgs, "pwd", 3) == 0){
                        if(strlen(programAndArgs) > 3){
                            int x = checkArgForWhitespace(programAndArgs + 3);
                            if(x == 1 || x == 0){
                                pwdBuiltin();
                            }
                            else{
                                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                            }
                        }
                        else if(strlen(cmd) == 3){
                            pwdBuiltin();
                        }
                        else{
                            fprintf(stderr, "%s is not a valid command.\n", cmd);
                        }
                    }
                    else{
                        runProcess(programAndArgs);
                    }
                    //restoreAllFD();
                }
                else if(strstr(cmdDup, specialRedirect2) != NULL){
                    strcpy(cmdDup, cmd);
                    char *programAndArgs = strtok(cmdDup, "2>");
                    char *file = strtok(NULL, "2>");

                    /*printf("programAndArgs: %s\n", programAndArgs);
                    printf("File: %s\n", file);*/

                    if(*file == 32){
                        redirectSpecialSpecificFD(2, file + 1);
                    }
                    else{
                        redirectSpecialSpecificFD(2, file);
                    }

                    if(strncmp(programAndArgs, "pwd", 3) == 0){
                        if(strlen(programAndArgs) > 3){
                            int x = checkArgForWhitespace(programAndArgs + 3);
                            if(x == 1 || x == 0){
                                pwdBuiltin();
                            }
                            else{
                                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                            }
                        }
                        else if(strlen(cmd) == 3){
                            pwdBuiltin();
                        }
                        else{
                            fprintf(stderr, "%s is not a valid command.\n", cmd);
                        }
                    }
                    else{
                        runProcess(programAndArgs);
                    }
                    //restoreAllFD();
                }
                else if(strstr(cmdDup, specialRedirectAnd) != NULL){
                    strcpy(cmdDup, cmd);
                    char *programAndArgs = strtok(cmdDup, "&>");
                    char *file = strtok(NULL, "&>");

                    /*printf("programAndArgs: %s\n", programAndArgs);
                    printf("File: %s\n", file);*/

                    if(*file == 32){
                        redirectSpecialBoth(file + 1);
                    }
                    else{
                        redirectSpecialBoth(file);
                    }

                    if(strncmp(programAndArgs, "pwd", 3) == 0){
                        if(strlen(programAndArgs) > 3){
                            int x = checkArgForWhitespace(programAndArgs + 3);
                            if(x == 1 || x == 0){
                                pwdBuiltin();
                            }
                            else{
                                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                            }
                        }
                        else if(strlen(cmd) == 3){
                            pwdBuiltin();
                        }
                        else{
                            fprintf(stderr, "%s is not a valid command.\n", cmd);
                        }
                    }
                    else{
                        runProcess(programAndArgs);
                    }
                    //restoreAllFD();
                }
                else if(strstr(cmdDup, specialRedirectAppend) != NULL){
                    strcpy(cmdDup, cmd);
                    char *programAndArgs = strtok(cmdDup, ">>");
                    char *file = strtok(NULL, ">>");

                    /*printf("programAndArgs: %s\n", programAndArgs);
                    printf("File: %s\n", file);*/

                    if(*file == 32){
                        redirectAppend(file + 1);
                    }
                    else{
                        redirectAppend(file);
                    }

                    if(strncmp(programAndArgs, "pwd", 3) == 0){
                        if(strlen(programAndArgs) > 3){
                            int x = checkArgForWhitespace(programAndArgs + 3);
                            if(x == 1 || x == 0){
                                pwdBuiltin();
                            }
                            else{
                                fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                            }
                        }
                        else if(strlen(cmd) == 3){
                            pwdBuiltin();
                        }
                        else{
                            fprintf(stderr, "%s is not a valid command.\n", cmd);
                        }
                    }
                    else{
                        runProcess(programAndArgs);
                    }
                    //restoreAllFD();
                }
                else if(strstr(cmdDup, specialHereDoc) != NULL){
                    char *programAndArgs = strtok(cmdDup, "<<");
                    char *delimiter = strtok(NULL, "<<");

                    if(*delimiter == 32)
                        delimiter = delimiter + 1;

                    int fd[2];
                    if(pipe(fd) == -1){
                        perror("pipe error");
                    }
                    else{
                        while((cmdHereDoc = readline(">")) != NULL) {//create pipe, write to stdout of pipe, when fork, set input of pipe to fork
                            if(strncmp(cmdHereDoc, delimiter, strlen(delimiter)) == 0){
                                //run cmd with fd[0] for stdin
                                redirectFDHereDoc(fd[0]);
                                close(fd[1]);
                                break;
                            }
                            else{
                                //printf("cmdHereDoc: %s\n", cmdHereDoc);
                                write(fd[1], cmdHereDoc, strlen(cmdHereDoc));
                            }
                        }
                        free(cmdHereDoc);

                        if(strncmp(programAndArgs, "pwd", 3) == 0){
                            if(strlen(programAndArgs) > 3){
                                int x = checkArgForWhitespace(programAndArgs + 3);
                                if(x == 1 || x == 0){
                                    pwdBuiltin();
                                }
                                else{
                                    fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                                }
                            }
                            else if(strlen(cmd) == 3){
                                pwdBuiltin();
                            }
                            else{
                                fprintf(stderr, "%s is not a valid command.\n", cmd);
                            }
                        }
                        else{
                            runProcess(programAndArgs);
                        }
                        close(fd[0]);
                        restoreAllFD();
                    }
                }
                else{
                    char *y;
                    if((y = strchr(cmd, '<')) != NULL){
                        if((y = strchr(cmd, '>')) != NULL){
                            char *programAndArgs = strtok(cmd, "<");
                            char *firstFile = strtok(NULL, ">");

                            if(*firstFile == 32){
                                redirectFD('<', firstFile + 1);
                            }
                            else{
                                redirectFD('<', firstFile);
                            }

                            char *secondFile = strtok(NULL, ">");
                            if(*secondFile == 32){
                                redirectFD('>', secondFile + 1);
                            }
                            else{
                                redirectFD('>', secondFile);
                            }

                            if(strncmp(programAndArgs, "pwd", 3) == 0){
                                if(strlen(programAndArgs) > 3){
                                    int x = checkArgForWhitespace(programAndArgs + 3);
                                    if(x == 1 || x == 0){
                                        pwdBuiltin();
                                    }
                                    else{
                                        fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                                    }
                                }
                                else if(strlen(cmd) == 3){
                                    pwdBuiltin();
                                }
                                else{
                                    fprintf(stderr, "%s is not a valid command.\n", cmd);
                                }
                            }
                            else{
                                runProcess(programAndArgs);
                            }
                            //restoreFD('<');
                            //restoreFD('>');
                        }
                        else{
                            char *programAndArgs = strtok(cmd, "<");
                            char *file = strtok(NULL, "<");

                            if(*file == 32){
                                redirectFD('<', file + 1);
                            }
                            else{
                                redirectFD('<', file);
                            }

                            if(strncmp(programAndArgs, "pwd", 3) == 0){
                                if(strlen(programAndArgs) > 3){
                                    int x = checkArgForWhitespace(programAndArgs + 3);
                                    if(x == 1 || x == 0){
                                        pwdBuiltin();
                                    }
                                    else{
                                        fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                                    }
                                }
                                else if(strlen(cmd) == 3){
                                    pwdBuiltin();
                                }
                                else{
                                    fprintf(stderr, "%s is not a valid command.\n", cmd);
                                }
                            }
                            else{
                                runProcess(programAndArgs);
                            }
                            //restoreFD('<');
                        }
                    }
                    else if((y = strchr(cmd, '>')) != NULL){
                        char *programAndArgs = strtok(cmd, ">");
                        char *file = strtok(NULL, ">");

                        if(*file == 32){
                            redirectFD('>', file + 1);
                        }
                        else{
                            redirectFD('>', file);
                        }

                        if(strncmp(programAndArgs, "pwd", 3) == 0){
                            if(strlen(programAndArgs) > 3){
                                int x = checkArgForWhitespace(programAndArgs + 3);
                                if(x == 1 || x == 0){
                                    pwdBuiltin();
                                }
                                else{
                                    fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                                }
                            }
                            else if(strlen(cmd) == 3){
                                pwdBuiltin();
                            }
                            else{
                                fprintf(stderr, "%s is not a valid command.\n", cmd);
                            }
                        }
                        else{
                            runProcess(programAndArgs);
                        }
                        //printf("second restore\n");
                        //restoreFD('>');
                    }
                    else if((y = strchr(cmd, '|')) != NULL){
                        char *firstProgram = strtok(cmd, "|");
                        char *secondProgram = strtok(NULL, "|");
                        char *thirdProgram;

                        if((thirdProgram = strtok(NULL, "|")) != NULL){
                            setup3Pipe(firstProgram, secondProgram, thirdProgram, cmd);
                        }
                        else{
                            setupPipe(firstProgram, secondProgram, cmd);
                        }
                    }
                    else{
                        if(strncmp(cmd, "pwd", 3) == 0){
                            if(strlen(cmd) > 3){
                                int x = checkArgForWhitespace(cmd + 3);
                                if(x == 1 || x == 0){
                                    pwdBuiltin();
                                }
                                else{
                                    /*if(*(cmd + strlen(cmd) - 1) == 38){
                                        pwdBuiltinBackground();
                                    }
                                    else{*/
                                        fprintf(stderr, "%s is not a valid command as it has too many arguments.\n", cmd);
                                    //}
                                }
                            }
                            else if(strlen(cmd) == 3){
                                pwdBuiltin();
                            }
                            else{
                                fprintf(stderr, "%s is not a valid command.\n", cmd);
                            }
                        }
                        else{
                            /*if(*(cmd + strlen(cmd) - 1) == 38){
                                *(cmd + strlen(cmd) - 1) = 0;
                                printf("runProcessBackground\n");
                                runProcessBackground(cmd);
                            }
                            else{*/
                                runProcess(cmd);
                            //}
                        }
                    }
                }
            free(cmdDup);
            }
        }


        //int exit;
        //while((exit = wait(&child_status)) != -1);
        shellLine = shellHeader();
        //printf("%s\n",cmd);
        /* All your debug print statements should use the macros found in debu.h */
        /* Use the `make debug` target in the makefile to run with these enabled. */
        //info("Length of command entered: %ld\n", strlen(cmd));
        /* You WILL lose points if your shell prints out garbage values. */


    }

    /* Don't forget to free allocated memory, and close file descriptors. */
    free(cmd);
    free(shellLine);
    sigprocmask(SIG_SETMASK, &prev_maskTop, NULL);

    return EXIT_SUCCESS;
}
