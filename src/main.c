#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/utils.h"


int main() {

    struct Pipeline *pipeline = malloc(sizeof(struct Pipeline));
    pipeline->pipe_count = 0;
    initialze_Command(&pipeline->commands[0]);
    int pipes[MAX_COMMANDS - 1][2];  
    pid_t child_pids[MAX_COMMANDS];  // Store child PIDs

    while(1){
        char *input = NULL;
        size_t len = 0;

        printf("mysh> ");
        fflush(stdout);

        // Read a line of input
        if(getline(&input, &len, stdin) == -1) {
            printf("\n");
            break;
        }
        input[strcspn(input, "\n")] = 0;

        parse_input(input, pipeline);
        
        // Create pipes if needed (for pipe_count > 0, we need pipe_count pipes)
        if(pipeline->pipe_count > 0) {
            for (int i = 0; i < pipeline->pipe_count; i++) {
                if (pipe(pipes[i]) < 0) {
                    perror("pipe failed");
                    exit(1);
                }
            }
        }

        //iterate through commands in the pipeline (pipe_count + 1 total commands)
        int child_count = 0;
        int should_exit = 0;
        for (int i = 0; i <= pipeline->pipe_count; i++) {
            struct Command *cmd = &pipeline->commands[i];
            if (cmd->argv[0] == NULL) continue; 
            if (strncmp(cmd->argv[0], "exit", 4) == 0) {
                should_exit = 1;
                break;
            }

            // handle built-in commands
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
                continue;
            }

            //pwd command
            if (strcmp(cmd->argv[0], "pwd") == 0) {
                char cwd[MAX_INPUT_SIZE];
                if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
                else perror("pwd failed");
                continue;
            }

            //help command
            if (strcmp(cmd->argv[0], "help") == 0) {
                printf("Available commands:\n");
                printf("   cd <directory> - Change directory\n");
                printf("   pwd - Print working directory\n");
                printf("   exit - Exit the shell\n");
                printf("   [other] Runs system command like ls, mkdir, echo, etc.\n");
                continue;
            }


            // fork and execute commands
            pid_t pid = fork();
            if (pid < 0) perror("fork failed");
            
            //Child process
            if (pid == 0) { 
                // Handle input redirection
                if (cmd->redirect_flags & REDIRECT_IN) {
                    int fd = open(cmd->redirects.input_file, O_RDONLY);
                    if (fd == -1) { perror("Input redirection failed"); exit(1); }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                if (cmd->redirect_flags & REDIRECT_OUT) {
                    int fd = open(cmd->redirects.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) { perror("Output redirection failed"); exit(1); }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                if (cmd->redirect_flags & REDIRECT_APP) {
                    int fd = open(cmd->redirects.append_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd == -1) { perror("Append redirection failed"); exit(1); }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                if(pipeline->pipe_count > 0) {
                    // Handle pipes
                    if (i > 0)
                        dup2(pipes[i - 1][0], STDIN_FILENO); // Read end of previous pipe
                    if (i < pipeline->pipe_count) 
                        dup2(pipes[i][1], STDOUT_FILENO); // Write end of current pipe
                    
                    // Close ALL pipe descriptors in child
                    for (int j = 0; j < pipeline->pipe_count; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                }
                execvp(cmd->argv[0], cmd->argv);
                perror("exec failed");
                exit(1);
            } else {
                // Parent process - store child PID
                child_pids[child_count++] = pid;
            }
        }
        
        // Check if we should exit the shell
        if (should_exit) {
            free(input);
            break;
        }
    
        // Close all pipes in parent process
        if(pipeline->pipe_count > 0) {
            for (int i = 0; i < pipeline->pipe_count; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
        }
        
        // Wait for all child processes
        for (int i = 0; i < child_count; i++) {
            int status;
            waitpid(child_pids[i], &status, 0);
        }

        free(input);
        for (int i = 0; i <= pipeline->pipe_count; i++)
            initialze_Command(&pipeline->commands[i]);
        pipeline->pipe_count = 0;

    }
    free(pipeline);
}
