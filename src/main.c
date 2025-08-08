#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <errno.h>
#include "../include/shell.h"

struct JobTable job_table;

// Manage Variables (both local and environment)
extern char **environ;          // Original environment variables
struct VariableStore var_store; // Store for shell variables

int main() {
    // Initialize variable store (includes environment variables)
    if (init_variable_store(&var_store) < 0) {
        fprintf(stderr, "Failed to initialize variable store\n");
        return 1;
    }
    
    // Initialize JobTable
    job_table.job_count = 0;
    job_table.next_job_id = 1;

    // Signal handling
        // handle SIGINT and SIGTSTP
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;  
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTSTP, &sa, NULL);

        // handle SIGCHILD
        sa.sa_handler = sigchld_handler;
        sa.sa_flags = SA_RESTART; 
        sigaction(SIGCHLD, &sa, NULL); 

        // Prevent background processes from writing and reading to terminal
        sigaction(SIGTTOU, &sa, NULL);  
        sigaction(SIGTTIN, &sa, NULL); 

    while(1){
        // Cleanup finished jobs before processing new input
        cleanup_finished_jobs(&job_table);

        // Initialize pipeline
        struct Pipeline *pipeline = malloc(sizeof(struct Pipeline));
        pipeline->pipe_count = 0;
        initialze_Command(&pipeline->commands[0]);

        int pipes[MAX_COMMANDS - 1][2];  
        pid_t child_pids[MAX_COMMANDS];  // Store child PIDs

        char *input = NULL;
        size_t len = 0;

        int input_has_background_process = 0;

        printf("mysh> ");
        fflush(stdout);

        // Read a line of input; SA_RESTART should handle EINTR automatically
        if(getline(&input, &len, stdin) == -1 || input[0] == '\n') {
            printf("\n");
            break;
        }
        input[strcspn(input, "\n")] = 0;

        printf("DEBUG: Raw input after getline: '%s'\n", input);
        parse_input(input, pipeline, &input_has_background_process);        
        
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
                should_exit = 1; //set flag for outer loop
                break;
            }

            // handle built-in commands
            int builtin_result = process_built_in_command(cmd);
            if (builtin_result == 0 || builtin_result == -1) continue;  // Built-in found (success or error)

            // check and handle job commands
            if (process_job_command(cmd, &job_table) == 1) continue;

            // it's a regular command. fork
            pid_t pid = fork();
            if (pid < 0) perror("fork failed");
            
            //Child process
            if (pid == 0) { 
                // Restore default signal handling for children
                struct sigaction sa_default;
                sa_default.sa_handler = SIG_DFL;
                sigemptyset(&sa_default.sa_mask);
                sa_default.sa_flags = 0;
                sigaction(SIGINT, &sa_default, NULL);
                sigaction(SIGTSTP, &sa_default, NULL);

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

                // Handle pipes
                if(pipeline->pipe_count > 0) {
                    if (i > 0)
                        dup2(pipes[i - 1][0], STDIN_FILENO); // Read end of previous pipe
                    if (i < pipeline->pipe_count) 
                        dup2(pipes[i][1], STDOUT_FILENO); // Write end of current pipe
                    
                    // Close all pipe descriptors in child
                    for (int j = 0; j < pipeline->pipe_count; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                }

                if (pipeline->pipe_count > 0 || input_has_background_process) 
                    if (i == 0) setpgid(0, 0);  // Create new process group
                
                // Build environment array for child process
                char **child_env = build_environ_array(&var_store);
                if (child_env == NULL) {
                    fprintf(stderr, "Failed to build environment for child process\n");
                    exit(1);
                }
                //char **child_env = environ;  // Use original environment for testing
                                
                // Identify path to executable
                printf("DEBUG: Looking for command: '%s'\n", cmd->argv[0]);
                char *full_path = find_executable_in_path(cmd->argv[0], &var_store);
                if (full_path == NULL) {
                    fprintf(stderr, "%s: command not found\n", cmd->argv[0]);
                    exit(127);
                }
                printf("DEBUG: Found full path: '%s'\n", full_path);
                execve(full_path, cmd->argv, child_env);
    
                fprintf(stderr, "%s: command not found\n", cmd->argv[0]);
                exit(127);  // Standard exit code for "command not found"

            } else {
                // Parent Process
                // Set pgid for job control only when needed
                if (pipeline->pipe_count > 0 || input_has_background_process) {
                    if (i == 0) {
                        setpgid(pid, pid);  
                    } else {
                        setpgid(pid, child_pids[0]); 
                    }
                }

                //store child PIDs
                child_pids[child_count++] = pid;
            }
        }
        
        // handle exit in outer loop
        if (should_exit) {
            free(input);
            free(pipeline);
            break;
        }
    
        // Close all pipes in parent process
        if(pipeline->pipe_count > 0) {
            for (int i = 0; i < pipeline->pipe_count; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
        }

        if (child_count > 0) {
            if(createJob(&job_table, input, &input_has_background_process, child_pids, child_count) == -1) {
                for (int i = 0; i < child_count; i++) {
                    int status;
                    waitpid(child_pids[i], &status, 0);
                }
                break;
            }

            struct Job *job = &job_table.jobs[job_table.job_count - 1];

            if (job->is_background) {
                // Background job (simple or pipeline) - print info, don't wait
                printf("[%d] %ld\n", job->job_id, (long)job->pids[0]);
                fflush(stdout);
            } 
            // Simple foreground command - no process group change needed; must wait for it
            else if (pipeline->pipe_count == 0) {
                for (int i = 0; i < child_count; i++) {
                    int status;
                    waitpid(child_pids[i], &status, 0);
                    job->pid_status[i] = 0; 
                }
                job->state = JOB_DONE;

            } else {
                // Foreground pipeline - use process group and wait
                tcsetpgrp(STDIN_FILENO, job->pids[0]);
            
                // Block SIGCHLD to prevent race condition with signal handler
                sigset_t mask, oldmask;
                sigemptyset(&mask);
                sigaddset(&mask, SIGCHLD);
                sigprocmask(SIG_BLOCK, &mask, &oldmask);
            
                // Pipeline - wait for process group 
                int status;
                waitpid(-job->pids[0], &status, 0);  // Wait for process group
            
                // Restore signal mask
                sigprocmask(SIG_SETMASK, &oldmask, NULL);
                
                // Only restore terminal control if we changed it
                // Small delay to ensure process group is fully cleaned up
                struct timespec ts = {0, 1000000};  // 1 millisecond
                nanosleep(&ts, NULL);                
                
                pid_t shell_pg = getpgrp();
                
                // Block SIGTTOU temporarily
                sigset_t tto_mask, old_tto_mask;
                sigemptyset(&tto_mask);
                sigaddset(&tto_mask, SIGTTOU);
                sigprocmask(SIG_BLOCK, &tto_mask, &old_tto_mask);

                // Attempt to set the terminal's foreground process group
                tcsetpgrp(STDIN_FILENO, shell_pg);

                // Restore the original signal mask
                sigprocmask(SIG_SETMASK, &old_tto_mask, NULL);
                
                job->state = JOB_DONE;
                for (int i = 0; i < child_count; i++) 
                    job->pid_status[i] = 0;  
            }
        }

        free(input);
        
        for (int i = 0; i <= pipeline->pipe_count; i++)
            initialze_Command(&pipeline->commands[i]);
        pipeline->pipe_count = 0;
        
        free(pipeline);
    }
    
    free_variable_store(&var_store);
}
