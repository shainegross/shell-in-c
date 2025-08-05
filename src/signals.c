#include "../include/shell.h"
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    // Reap all finished children and mark them as done
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        for (int i = 0; i < job_table.job_count; i++) {
            for (int j = 0; j < job_table.jobs[i].pid_count; j++) {
                if (job_table.jobs[i].pids[j] == pid) {
                    // Mark this specific PID as finished
                    job_table.jobs[i].pid_status[j] = 0;

                    if (WIFSTOPPED(status)) {
                        job_table.jobs[i].state = JOB_STOPPED;
                        job_table.jobs[i].is_background = 1;
                    } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                        job_table.jobs[i].state = JOB_DONE;
                    }
                    goto next_pid;
                }
            }
        }
    next_pid:;
    }
}
