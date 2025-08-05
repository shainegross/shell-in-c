#include "../include/shell.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>  // for chdir, getcwd

// Define the command arrays
const char *built_in_commands[] = {"cd", "pwd", "help", NULL};

// Checks and processes built-in commands 
// Returns 1 if a built-in command was found, 0 otherwise
int process_built_in_command(struct Command *cmd) {
    if (cmd == NULL || cmd->argv[0] == NULL) return 0;

    for (int i = 0; built_in_commands[i] != NULL; i++) {
        if (strcmp(cmd->argv[0], built_in_commands[i]) == 0) {
                //cd command
                if (strcmp(cmd->argv[0], "cd") == 0) {
                    if (cmd->argv[1] == NULL) fprintf(stderr, "cd: missing argument\n");
                    else if (chdir(cmd->argv[1]) != 0) perror("cd failed");

                    // Successfully changed directory - show current path
                    else {
                        char cwd[MAX_INPUT_SIZE];
                        if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
                        else perror("getcwd failed");
                    }
                }

                //pwd command
                if (strcmp(cmd->argv[0], "pwd") == 0) {
                    char cwd[MAX_INPUT_SIZE];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
                    else perror("pwd failed");
                }

                //help command
                if (strcmp(cmd->argv[0], "help") == 0) {
                    printf("Available commands:\n");
                    printf("   cd <directory> - Change directory\n");
                    printf("   pwd - Print working directory\n");
                    printf("   exit - Exit the shell\n");
                    printf("   [other] Runs system command like ls, mkdir, echo, etc.\n");
                }
            return 1;
        }
    }
    return 0;
}
