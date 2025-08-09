#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/shell.h"

// FUNCTION PROTOTYPES
char *expand_var(const char *input, struct VariableStore *var_store);
int var_name_end(const char *s);



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

    char *input_expanded = expand_var(input, &var_store);

    int argc = 0;
    char *token = strtok(input_expanded, " \t\n");
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
        }  else if (strcmp(token, "<") == 0) {
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

// Takes user's full input and expands variables
// If variable not found, returns user's input 
char *expand_var(const char *input, struct VariableStore *var_store){

    size_t cap = 1024;
    char *out = malloc(cap);
    if (!out) return NULL;
    char *marker = out;

    for (size_t i = 0; input[i] != '\0'; i++) {
        if (input[i] == '\\' && input[i+1] == '$') {
            // Escape sequence, just copy the next character
            *marker++ = '$';
            i++;
        } else if (input[i] == '$') {
            if (input[i+1] == '(') {
                //handle $(VAR) structure
                char* end_brace = strchr(input + i, ')');
                if (!end_brace){
                    fprintf(stderr, "Error: Unmatched parenthesis in variable expansion\n");
                    free(out);
                    return NULL;
                }
                               size_t var_name_len = end_brace - (input + i + 2);
                char var_name[256];
                strncpy(var_name, input + i + 2, var_name_len);
                var_name[var_name_len] = '\0';

                char *val = get_variable(var_store, var_name);
                if (val) {
                    size_t len = strlen(val);
                    memcpy(marker, val, len);
                    marker += len;
                }
                i = end_brace - input;
            } else {
                //handle $VAR structure
                int name_len = var_name_end(input + i + 1);
                if (name_len > 0) {
                    char var_name[256];
                    strncpy(var_name, input + i + 1, name_len);
                    var_name[name_len] = '\0';

                    char *val = get_variable(var_store, var_name);
                    if (val) {
                        size_t len = strlen(val);
                        memcpy(marker, val, len);
                        marker += len;
                    }
                    i += name_len; // skip past var
                } else {
                    *marker++ = '$';
                }
            }
        } else {
            *marker++ = input[i];
        }
    }
    *marker = '\0';
    return out;
}

// Returns first non-name char index after start
int var_name_end(const char *s) {
    int i = 0;

    // Consume letters, digits, underscores
    while (isalnum((unsigned char)s[i]) || s[i] == '_') i++;
    return i; // index of first char after var name
}