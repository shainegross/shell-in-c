#include "../include/shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>


const char *job_commands[] = {"jobs", "fg", "bg", NULL};


// Helper function to find job by ID
static struct Job* find_job_by_id(struct JobTable *job_table, int job_id) {
    for (int i = 0; i < job_table->job_count; i++) {
        if (job_table->jobs[i].job_id == job_id && job_table->jobs[i].state != JOB_DONE) {
            return &job_table->jobs[i];
        }
    }
    return NULL;
}

// Helper function to find current foreground job
static struct Job* find_foreground_job(struct JobTable *job_table) {
    for (int i = 0; i < job_table->job_count; i++) {
        struct Job *job = &job_table->jobs[i];
        if (!job->is_background && job->state == JOB_RUNNING) {
            return job;
        }
    }
    return NULL;
}

// Helper function to print jobs by state
static void print_jobs_by_state(struct JobTable *job_table, int target_state) {
    int found = 0;
    const char *section_header;
    const char *state_str;
    
    // Set header and state string based on target state
    switch (target_state) {
        case JOB_RUNNING:
            section_header = "=== RUNNING JOBS ===";
            state_str = "Running";
            break;
        case JOB_STOPPED:
            section_header = "=== STOPPED JOBS ===";
            state_str = "Stopped";
            break;
        case JOB_DONE:
            section_header = "=== DONE JOBS ===";
            state_str = "Done";
            break;
        default:
            section_header = "=== UNKNOWN JOBS ===";
            state_str = "Unknown";
            break;
    }
    
    // Print matching jobs
    for (int i = 0; i < job_table->job_count; i++) {
        struct Job *job = &job_table->jobs[i];
        if (job->state == target_state) {
            if (!found) {
                printf("%s\n", section_header);
                printf("---------------------------\n");
                found = 1;
            }
            printf("[%d]%c  %-20s %s %s\n", 
                   job->job_id,
                   (i == job_table->job_count - 1) ? '+' : '-',
                   state_str,
                   job->is_background ? "(bg)" : "(fg)",
                   job->command_line);
        }
    }
    if (found) printf("\n");
}

// Print complete job table for debugging
static void print_jobs_table(struct JobTable *job_table) {
    printf("=== COMPLETE JOB TABLE (count=%d, next_id=%d) ===\n", 
           job_table->job_count, job_table->next_job_id);
    
    print_jobs_by_state(job_table, JOB_RUNNING);
    print_jobs_by_state(job_table, JOB_STOPPED);
    print_jobs_by_state(job_table, JOB_DONE);
    
    printf("=== END JOB TABLE ===\n");
}

// Handle 'jobs' command
static int handle_jobs_command(struct JobTable *job_table) {
    print_jobs_table(job_table);
    return 1;
}

// Handle 'fg' command
static int handle_fg_command(struct Command *cmd, struct JobTable *job_table) {
    if (cmd->argv[1] == NULL) {
        fprintf(stderr, "fg: missing job ID\n");
        return 1;
    }

    int job_id = atoi(cmd->argv[1]);
    struct Job *target_job = find_job_by_id(job_table, job_id);
    
    if (target_job == NULL) {
        fprintf(stderr, "fg: job %d not found\n", job_id);
        return 1;
    }
    
    if (!target_job->is_background) {
        fprintf(stderr, "fg: job %d is already in foreground\n", job_id);
        return 1;
    }
    
    // Check if there's currently a foreground job
    struct Job *current_fg = find_foreground_job(job_table);
    if (current_fg != NULL) {
        printf("Moving current foreground job [%d] to background\n", current_fg->job_id);
        current_fg->is_background = 1;
        kill(-current_fg->pids[0], SIGTSTP);
        current_fg->state = JOB_STOPPED;
    }
    
    // Move target job to foreground
    printf("Bringing job [%d] to foreground: %s\n", target_job->job_id, target_job->command_line);
    target_job->is_background = 0;
    
    if (target_job->state == JOB_STOPPED) {
        target_job->state = JOB_RUNNING;
        kill(-target_job->pids[0], SIGCONT);
    }
    
    tcsetpgrp(STDIN_FILENO, target_job->pids[0]);
    
    for (int k = 0; k < target_job->pid_count; k++) {
        if (target_job->pid_status[k] == 1) {
            int status;
            waitpid(target_job->pids[k], &status, 0);
        }
    }
    
    tcsetpgrp(STDIN_FILENO, getpgrp());
    return 1;
}

// Handle 'bg' command
static int handle_bg_command(struct Command *cmd, struct JobTable *job_table) {
    if (cmd->argv[1] == NULL) {
        fprintf(stderr, "bg: missing job ID\n");
        return 1;
    }

    int job_id = atoi(cmd->argv[1]);
    struct Job *target_job = find_job_by_id(job_table, job_id);
    
    if (target_job == NULL) {
        fprintf(stderr, "bg: job %d not found\n", job_id);
        return 1;
    }
    
    if (target_job->state != JOB_STOPPED) {
        fprintf(stderr, "bg: job %d is not stopped\n", job_id);
        return 1;
    }
    
    printf("[%d]+ %s &\n", target_job->job_id, target_job->command_line);
    target_job->is_background = 1;
    target_job->state = JOB_RUNNING;
    
    kill(-target_job->pids[0], SIGCONT);
    return 1;
}

// Checks and processes job commands 
int process_job_command(struct Command *cmd, struct JobTable *job_table) {
    if (cmd == NULL || cmd->argv[0] == NULL) return 0;

    for (int i = 0; job_commands[i] != NULL; i++) {
        if (strcmp(cmd->argv[0], job_commands[i]) == 0) {
            
            if (strcmp(cmd->argv[0], "jobs") == 0) {
                return handle_jobs_command(job_table);
            }
            else if (strcmp(cmd->argv[0], "fg") == 0) {
                return handle_fg_command(cmd, job_table);
            }
            else if (strcmp(cmd->argv[0], "bg") == 0) {
                return handle_bg_command(cmd, job_table);
            }
            
            return 1; // Found a job command
        }
    }
    return 0; // Not a job command
}

// Initialize a new job
int createJob(struct JobTable *table, char *input, int *is_background, pid_t *pids, int pid_count) {
    int slot_index;
    
    // Find an available slot (either new or reuse finished job slot)
    if (table->job_count < MAX_JOBS) {
        // Use next available slot
        slot_index = table->job_count;
        table->job_count++;  // Increment job count for new slot
    } else {
        // Array is full, try to find a finished job slot to reuse
        slot_index = find_finished_job(table);
        if (slot_index == -1) {
            fprintf(stderr, "Error: Maximum number of jobs reached\n");
            return -1;
        }
    }

    struct Job *new_job = &table->jobs[slot_index];
    new_job->job_id = table->next_job_id++;  // Always increment - never reuse job IDs
    new_job->pid_count = pid_count;
    new_job->is_background = *is_background;
    new_job->state = JOB_RUNNING;

    // Copy the command line for display
    strncpy(new_job->command_line, input, MAX_INPUT_SIZE - 1);
    new_job->command_line[MAX_INPUT_SIZE - 1] = '\0'; // Ensure null-termination

    // Copy all the PIDs and initialize their status
    for (int i = 0; i < pid_count; i++) {
        new_job->pids[i] = pids[i];
        new_job->pid_status[i] = 1;  // Initialize as running
    }

    *is_background = 0; // Reset for next command

    return 0;
}

// Check and update status of a single job
// Returns: 1 if job completed, 0 if still running, -1 on error
int cleanup_single_job(struct Job *job) {
    if (job->state != JOB_RUNNING) {
        return (job->state == JOB_DONE) ? 1 : 0;  
    }
    
    // For foreground jobs that just completed, don't call waitpid() again
    // The main loop already reaped them
    if (!job->is_background) {
        job->state = JOB_DONE;
        for (int j = 0; j < job->pid_count; j++) {
            job->pid_status[j] = 0;
        }
        return 1;
    }
    
    // Only do waitpid() for background jobs
    int running_count = 0;
    for (int j = 0; j < job->pid_count; j++) {
        if (job->pid_status[j] == 1) {  
            int status;
            pid_t result = waitpid(job->pids[j], &status, WNOHANG);
            if (result > 0) {
                // Process finished
                job->pid_status[j] = 0;
            } else if (result == 0) {
                running_count++;
            } else if (result == -1) {
                // Error - treat as finished
                if (errno == ECHILD) {
                    job->pid_status[j] = 0; 
                } else {
                    perror("waitpid failed");
                    return -1;
                }
            }
        }
    }
    
    if (running_count == 0) {
        job->state = JOB_DONE;
        if (job->is_background) {
            printf("[%d]+  Done                    %s\n", 
                   job->job_id, job->command_line);
        }
        return 1;
    }
    
    return 0;
}

// Cleanup all finished jobs (wrapper function)
void cleanup_finished_jobs(struct JobTable *table) {
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask); // Block SIGCHLD during cleanup

    for (int i = 0; i < table->job_count; i++) {
        cleanup_single_job(&table->jobs[i]);
    }

    sigprocmask(SIG_SETMASK, &oldmask, NULL); // Restore signal mask
}

// Find first available slot with a finished job
// Returns: array index of reusable slot, or -1 if no slots available
int find_finished_job(struct JobTable *table) {
    for (int i = 0; i < table->job_count; i++) {
        if (table->jobs[i].state == JOB_DONE) {
            return i;  
        }
    }
    return -1; 
}
