#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

// Mock shell structures for testing
struct Command {
    char *argv[16];
    int redirect_flags;
    struct {
        char *input_file;
        char *output_file;
        char *append_file;
    } redirects;
};

struct Pipeline {
    struct Command commands[8];
    int pipe_count;
};

struct Job {
    int job_id;
    pid_t pids[8];
    int pid_count;
    int is_background;
    int state;
    int pid_status[8];
    char *command;
};

struct JobTable {
    struct Job jobs[64];
    int job_count;
    int next_job_id;
};

// Test function prototypes
void test_command_initialization(void);
void test_pipeline_parsing_simple(void);
void test_pipeline_parsing_single_pipe(void);
void test_pipeline_parsing_multiple_pipes(void);
void test_background_detection(void);
void test_job_creation(void);
void test_job_management(void);
void test_redirection_parsing(void);
void run_all_unit_tests(void);

// Mock implementations for testing
void initialize_command(struct Command *cmd) {
    for (int i = 0; i < 16; i++) {
        cmd->argv[i] = NULL;
    }
    cmd->redirect_flags = 0;
    cmd->redirects.input_file = NULL;
    cmd->redirects.output_file = NULL;
    cmd->redirects.append_file = NULL;
}

void initialize_pipeline(struct Pipeline *pipeline) {
    pipeline->pipe_count = 0;
    for (int i = 0; i < 8; i++) {
        initialize_command(&pipeline->commands[i]);
    }
}

// Simple tokenizer for testing
void tokenize_input(char *input, char **tokens, int *token_count) {
    *token_count = 0;
    char *token = strtok(input, " \t\n");
    while (token != NULL && *token_count < 32) {
        tokens[*token_count] = strdup(token);
        (*token_count)++;
        token = strtok(NULL, " \t\n");
    }
}

// Test implementations
void test_command_initialization(void) {
    printf("Testing command initialization... ");
    
    struct Command cmd;
    initialize_command(&cmd);
    
    assert(cmd.argv[0] == NULL);
    assert(cmd.redirect_flags == 0);
    assert(cmd.redirects.input_file == NULL);
    assert(cmd.redirects.output_file == NULL);
    assert(cmd.redirects.append_file == NULL);
    
    printf("PASSED\n");
}

void test_pipeline_parsing_simple(void) {
    printf("Testing simple command parsing... ");
    
    char input[] = "ls -l";
    char *tokens[32];
    int token_count;
    
    tokenize_input(input, tokens, &token_count);
    
    assert(token_count == 2);
    assert(strcmp(tokens[0], "ls") == 0);
    assert(strcmp(tokens[1], "-l") == 0);
    
    // Cleanup
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    
    printf("PASSED\n");
}

void test_pipeline_parsing_single_pipe(void) {
    printf("Testing single pipe parsing... ");
    
    char input[] = "echo hello | grep hello";
    char *tokens[32];
    int token_count;
    
    tokenize_input(input, tokens, &token_count);
    
    assert(token_count == 5); // echo, hello, |, grep, hello
    assert(strcmp(tokens[0], "echo") == 0);
    assert(strcmp(tokens[1], "hello") == 0);
    assert(strcmp(tokens[2], "|") == 0);
    assert(strcmp(tokens[3], "grep") == 0);
    assert(strcmp(tokens[4], "hello") == 0);
    
    // Cleanup
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    
    printf("PASSED\n");
}

void test_pipeline_parsing_multiple_pipes(void) {
    printf("Testing multiple pipe parsing... ");
    
    char input[] = "echo hello | grep hello | wc -w";
    char *tokens[32];
    int token_count;
    
    tokenize_input(input, tokens, &token_count);
    
    assert(token_count == 8); // echo, hello, |, grep, hello, |, wc, -w
    assert(strcmp(tokens[0], "echo") == 0);
    assert(strcmp(tokens[2], "|") == 0);
    assert(strcmp(tokens[5], "|") == 0);
    assert(strcmp(tokens[6], "wc") == 0);
    
    // Cleanup
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    
    printf("PASSED\n");
}

void test_background_detection(void) {
    printf("Testing background process detection... ");
    
    char input1[] = "sleep 5 &";
    char input2[] = "echo hello";
    char *tokens[32];
    int token_count;
    
    // Test background command
    strcpy(input1, "sleep 5 &");
    tokenize_input(input1, tokens, &token_count);
    assert(token_count == 3);
    assert(strcmp(tokens[2], "&") == 0);
    
    // Cleanup
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    
    // Test foreground command
    strcpy(input2, "echo hello");
    tokenize_input(input2, tokens, &token_count);
    assert(token_count == 2);
    // No & should be found
    
    // Cleanup
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    
    printf("PASSED\n");
}

void test_job_creation(void) {
    printf("Testing job creation... ");
    
    struct JobTable job_table;
    job_table.job_count = 0;
    job_table.next_job_id = 1;
    
    // Create a mock job
    struct Job *job = &job_table.jobs[job_table.job_count];
    job->job_id = job_table.next_job_id++;
    job->pids[0] = 12345;
    job->pid_count = 1;
    job->is_background = 1;
    job->state = 0; // RUNNING
    job->command = strdup("sleep 5 &");
    job_table.job_count++;
    
    assert(job_table.job_count == 1);
    assert(job->job_id == 1);
    assert(job->pids[0] == 12345);
    assert(job->is_background == 1);
    assert(strcmp(job->command, "sleep 5 &") == 0);
    
    free(job->command);
    printf("PASSED\n");
}

void test_job_management(void) {
    printf("Testing job management... ");
    
    struct JobTable job_table;
    job_table.job_count = 0;
    job_table.next_job_id = 1;
    
    // Add multiple jobs
    for (int i = 0; i < 3; i++) {
        struct Job *job = &job_table.jobs[job_table.job_count];
        job->job_id = job_table.next_job_id++;
        job->pids[0] = 1000 + i;
        job->pid_count = 1;
        job->is_background = 1;
        job->state = 0;
        job->command = strdup("test command");
        job_table.job_count++;
    }
    
    assert(job_table.job_count == 3);
    assert(job_table.next_job_id == 4);
    
    // Test job lookup
    struct Job *found_job = NULL;
    for (int i = 0; i < job_table.job_count; i++) {
        if (job_table.jobs[i].job_id == 2) {
            found_job = &job_table.jobs[i];
            break;
        }
    }
    assert(found_job != NULL);
    assert(found_job->pids[0] == 1001);
    
    // Cleanup
    for (int i = 0; i < job_table.job_count; i++) {
        free(job_table.jobs[i].command);
    }
    
    printf("PASSED\n");
}

void test_redirection_parsing(void) {
    printf("Testing redirection parsing... ");
    
    char input[] = "cat < input.txt > output.txt";
    char *tokens[32];
    int token_count;
    
    tokenize_input(input, tokens, &token_count);
    
    assert(token_count == 5); // cat, <, input.txt, >, output.txt
    assert(strcmp(tokens[0], "cat") == 0);
    assert(strcmp(tokens[1], "<") == 0);
    assert(strcmp(tokens[2], "input.txt") == 0);
    assert(strcmp(tokens[3], ">") == 0);
    assert(strcmp(tokens[4], "output.txt") == 0);
    
    // Cleanup
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    
    printf("PASSED\n");
}

void run_all_unit_tests(void) {
    printf("=== Running Unit Tests ===\n");
    
    test_command_initialization();
    test_pipeline_parsing_simple();
    test_pipeline_parsing_single_pipe();
    test_pipeline_parsing_multiple_pipes();
    test_background_detection();
    test_job_creation();
    test_job_management();
    test_redirection_parsing();
    
    printf("=== All Unit Tests Passed! ===\n\n");
}

int main(void) {
    run_all_unit_tests();
    return 0;
}
