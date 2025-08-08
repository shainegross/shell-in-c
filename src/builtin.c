
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // for chdir, getcwd
#include "../include/shell.h"

// Define the command arrays
const char *built_in_commands[] = {"cd", "pwd", "help", "export", "set", "unset", "env", NULL};

// Checks and processes built-in commands 
// Returns: 0 = success (command found and executed)
//         -1 = error (command found but failed)  
//          1 = not a built-in command
int process_built_in_command(struct Command *cmd) {
    if (cmd == NULL || cmd->argv[0] == NULL) return 1;

    for (int i = 0; built_in_commands[i] != NULL; i++) {
        if (strcmp(cmd->argv[0], built_in_commands[i]) == 0) {
                //cd command
                if (strcmp(cmd->argv[0], "cd") == 0) {
                    if (cmd->argv[1] == NULL) {
                        char *home = get_variable(&var_store, "HOME");
                        if (home == NULL || chdir(home) != 0) {
                            fprintf(stderr, "cd: HOME not set\n");
                            return -1;
                        }
                    } else if (strcmp(cmd->argv[1], "-") == 0) {
                        char *oldpwd = get_variable(&var_store, "OLDPWD");
                        if (oldpwd == NULL || chdir(oldpwd) != 0) {
                            fprintf(stderr, "cd: OLDPWD not set\n");
                            return -1;
                        }
                    } else if (chdir(cmd->argv[1]) != 0) {
                        perror("cd failed");
                        return -1;
                    }

                    // Successfully changed directory - show current path
                    char cwd[MAX_INPUT_SIZE];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
                    else perror("getcwd failed");
                    return 0;
                }

                //pwd command
                if (strcmp(cmd->argv[0], "pwd") == 0) {
                    char cwd[MAX_INPUT_SIZE];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        printf("%s\n", cwd);
                        return 0;
                    } else {
                        perror("pwd failed");
                        return -1;
                    }
                }

                //export command
                if (strcmp(cmd->argv[0], "export") == 0) {
                    if (cmd->argv[1] == NULL) {
                        fprintf(stderr, "export: missing variable name\n");
                        return -1;
                    }
                    
                    //handles VAR=value
                    if (strchr(cmd->argv[1], '=')) {
                    char *arg_copy = strdup(cmd->argv[1]);  // Make a copy
                    char *var_name = strtok(arg_copy, "=");
                    char *var_value = strtok(NULL, "=");
                        if (var_name == NULL || var_value == NULL) {
                            fprintf(stderr, "export: invalid format, use VAR=value\n");
                            free(arg_copy);
                            return -1;
                        }
                        if (set_variable(&var_store, var_name, var_value, 1) != 0) {
                            fprintf(stderr, "export: failed to set variable\n");
                            free(arg_copy);
                            return -1;
                        }
                        printf("Variable %s exported successfully\n", var_name);
                        free(arg_copy);
                        return 0;
                    }

                    //handles export VAR
                    if (cmd->argv[1][0] == '\0') {
                        fprintf(stderr, "export: missing variable name\n");
                        return -1;
                    }
                    // Export the variable by name
                    if(export_variable(&var_store, cmd->argv[1]) != 0) {
                        fprintf(stderr, "export: failed to export variable\n");
                        return -1;
                    }
                    printf("Variable %s exported successfully\n", cmd->argv[1]);
                    return 0;
                }

                //set command
                if (strcmp(cmd->argv[0], "set") == 0) {
                    if (cmd->argv[1] == NULL) {
                        fprintf(stderr, "set: missing variable name\n");
                        return -1;
                    }
                    char *var = cmd->argv[1];
                    char *value = cmd->argv[2];
                    if (value == NULL) {
                        fprintf(stderr, "set: missing value\n");
                        return -1;
                    } 
                    if (set_variable(&var_store, var, value, 0) != 0) {
                        fprintf(stderr, "set: failed to set variable\n");
                        return -1;
                    }
                    printf("Variable %s set to %s\n", var, value);
                    return 0;
                }

                //unset command
                if (strcmp(cmd->argv[0], "unset") == 0) {
                    if (cmd->argv[1] == NULL) {
                        fprintf(stderr, "unset: missing variable name\n");
                        return -1;
                    }
                    if (unset_variable(&var_store, cmd->argv[1]) != 0) {
                        fprintf(stderr, "unset: variable not found\n");
                        return -1;
                    }
                    printf("Variable %s unset successfully\n", cmd->argv[1]);
                    return 0;
                }

                //env command to display environment variables
                if (strcmp(cmd->argv[0], "env") == 0) {
                    if (cmd->argv[1] != NULL) {
                        fprintf(stderr, "env: no arguments expected\n");
                        return -1;
                    }
                    display_variables(&var_store, DISPLAY_EXPORTED);
                    return 0;
                }

                //help command
                if (strcmp(cmd->argv[0], "help") == 0) {
                    printf("Available commands:\n");
                    printf("   cd <directory> - Change directory\n");
                    printf("   pwd - Print working directory\n");
                    printf("   exit - Exit the shell\n");
                    printf("   [other] Runs system command like ls, mkdir, echo, etc.\n");
                    return 0;
                }
            return 0;  // Should never reach here, but just in case
        }
    }
    return 1;  // Not a built-in command
}
