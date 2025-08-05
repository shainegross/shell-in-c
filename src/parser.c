#include "../include/shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Initialize a Command structure
struct Command *initialze_Command(struct Command *cmd) {
    for (int i = 0; i < MAX_TOKENS; i++) {
        cmd->argv[i] = NULL;
    }
    cmd->redirects = (struct Redirection){ .input_file = "", .output_file = "", .append_file = "" };
    cmd->redirect_flags = 0;
    return cmd;
}

void parse_input(char *input, struct Pipeline *pipeline, int *input_has_background_process) {
    int argc = 0;
    char *token = strtok(input, " \t\n");
    struct Pipeline *p = pipeline;

    while (token != NULL && argc < MAX_TOKENS - 1) {
        if(strcmp(token, "|") == 0) {
            if (p->pipe_count < MAX_COMMANDS - 1) {
                // Null-terminate current command; Initialize next command and restart argument count
                p->commands[p->pipe_count].argv[argc] = NULL; 
                p->pipe_count++;
                initialze_Command(&p->commands[p->pipe_count]);
                argc = 0; 
            } else {
                fprintf(stderr, "Error: Too many commands in pipeline\n");
                return;
            }
        } else if (strcmp(token, "<") == 0) {
            p->commands[p->pipe_count].redirect_flags |= REDIRECT_IN;
            token = strtok(NULL, " \t\n"); // Get the filename
            if (token) {
                strcpy(p->commands[p->pipe_count].redirects.input_file, token);
            }
        } else if (strcmp(token, ">") == 0) {
            p->commands[p->pipe_count].redirect_flags |= REDIRECT_OUT;
            token = strtok(NULL, " \t\n"); // Get the filename
            if (token) {
                strcpy(p->commands[p->pipe_count].redirects.output_file, token);
            }
        } else if (strcmp(token, ">>") == 0) {
            p->commands[p->pipe_count].redirect_flags |= REDIRECT_APP;
            token = strtok(NULL, " \t\n"); // Get the filename
            if (token) {
                strcpy(p->commands[p->pipe_count].redirects.append_file, token);
            }
        } else {
            p->commands[p->pipe_count].argv[argc++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    // Check for background job and null-terminate
    if (argc > 0 && strcmp(p->commands[p->pipe_count].argv[argc - 1], "&") == 0) {
        *input_has_background_process = 1; // Set background flag if last token is '&'
        p->commands[p->pipe_count].argv[--argc] = NULL; // Remove '&' from argv
    }
    // Null-terminate the last command's argv array
    p->commands[p->pipe_count].argv[argc] = NULL;
}
