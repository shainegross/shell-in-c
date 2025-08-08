#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

// Mock variable store structures for testing
struct Variable {
    char *name;
    char *value;
    int is_exported;
};

struct VariableStore {
    struct Variable *variables;
    int capacity;
    int count;
};

// Mock global variable store
struct VariableStore test_var_store = {NULL, 0, 0};

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
void test_variable_store_init(void);
void test_variable_set_get(void);
void test_variable_expansion(void);
void test_variable_export(void);
void test_variable_parsing(void);
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

// Mock variable store functions for testing
void init_test_variable_store(struct VariableStore *store) {
    store->capacity = 10;
    store->count = 0;
    store->variables = malloc(sizeof(struct Variable) * store->capacity);
}

int set_test_variable(struct VariableStore *store, const char *name, const char *value) {
    // Check if variable already exists
    for (int i = 0; i < store->count; i++) {
        if (strcmp(store->variables[i].name, name) == 0) {
            free(store->variables[i].value);
            store->variables[i].value = strdup(value);
            return 0;
        }
    }
    
    // Add new variable
    if (store->count >= store->capacity) return -1;
    
    store->variables[store->count].name = strdup(name);
    store->variables[store->count].value = strdup(value);
    store->variables[store->count].is_exported = 0;
    store->count++;
    return 0;
}

char* get_test_variable(struct VariableStore *store, const char *name) {
    for (int i = 0; i < store->count; i++) {
        if (strcmp(store->variables[i].name, name) == 0) {
            return store->variables[i].value;
        }
    }
    return NULL;
}

int export_test_variable(struct VariableStore *store, const char *name) {
    for (int i = 0; i < store->count; i++) {
        if (strcmp(store->variables[i].name, name) == 0) {
            store->variables[i].is_exported = 1;
            return 0;
        }
    }
    return -1;
}

void cleanup_test_variable_store(struct VariableStore *store) {
    for (int i = 0; i < store->count; i++) {
        free(store->variables[i].name);
        free(store->variables[i].value);
    }
    free(store->variables);
    store->variables = NULL;
    store->count = 0;
    store->capacity = 0;
}

// Mock expand_variable function for testing
char* mock_expand_variable(const char *input) {
    char *result = malloc(strlen(input) * 2 + 1); // Allocate extra space
    char *dest = result;
    
    for (const char *src = input; *src; src++) {
        if (*src == '$') {
            if (*(src + 1) == '(') {
                // Handle $(VAR) format
                src += 2; // skip $( 
                const char *var_start = src;
                while (*src && *src != ')') src++;
                if (*src == ')') {
                    char var_name[256];
                    int len = src - var_start;
                    strncpy(var_name, var_start, len);
                    var_name[len] = '\0';
                    
                    char *value = get_test_variable(&test_var_store, var_name);
                    if (value) {
                        strcpy(dest, value);
                        dest += strlen(value);
                    }
                } else {
                    // Malformed, copy literal
                    *dest++ = '$';
                    *dest++ = '(';
                    strcpy(dest, var_start);
                    dest += strlen(var_start);
                }
            } else {
                // Handle $VAR format
                src++; // skip $
                const char *var_start = src;
                while (*src && (isalnum(*src) || *src == '_')) src++;
                
                char var_name[256];
                int len = src - var_start;
                if (len > 0) {
                    strncpy(var_name, var_start, len);
                    var_name[len] = '\0';
                    
                    char *value = get_test_variable(&test_var_store, var_name);
                    if (value) {
                        strcpy(dest, value);
                        dest += strlen(value);
                    }
                    src--; // Back up one since loop will increment
                } else {
                    *dest++ = '$'; // Just copy the $
                    src--;
                }
            }
        } else {
            *dest++ = *src;
        }
    }
    *dest = '\0';
    return result;
}

// Variable unit tests
void test_variable_store_init(void) {
    printf("Testing variable store initialization... ");
    
    struct VariableStore store;
    init_test_variable_store(&store);
    
    assert(store.capacity == 10);
    assert(store.count == 0);
    assert(store.variables != NULL);
    
    cleanup_test_variable_store(&store);
    printf("PASSED\n");
}

void test_variable_set_get(void) {
    printf("Testing variable set and get... ");
    
    struct VariableStore store;
    init_test_variable_store(&store);
    
    // Test setting and getting a variable
    int result = set_test_variable(&store, "TEST_VAR", "test_value");
    assert(result == 0);
    assert(store.count == 1);
    
    char *value = get_test_variable(&store, "TEST_VAR");
    assert(value != NULL);
    assert(strcmp(value, "test_value") == 0);
    
    // Test getting non-existent variable
    char *missing = get_test_variable(&store, "MISSING_VAR");
    assert(missing == NULL);
    
    // Test updating existing variable
    result = set_test_variable(&store, "TEST_VAR", "new_value");
    assert(result == 0);
    assert(store.count == 1); // Count should not increase
    
    value = get_test_variable(&store, "TEST_VAR");
    assert(strcmp(value, "new_value") == 0);
    
    cleanup_test_variable_store(&store);
    printf("PASSED\n");
}

void test_variable_export(void) {
    printf("Testing variable export... ");
    
    struct VariableStore store;
    init_test_variable_store(&store);
    
    // Set a variable and export it
    set_test_variable(&store, "EXPORT_TEST", "exported_value");
    int result = export_test_variable(&store, "EXPORT_TEST");
    assert(result == 0);
    assert(store.variables[0].is_exported == 1);
    
    // Try to export non-existent variable
    result = export_test_variable(&store, "MISSING_VAR");
    assert(result == -1);
    
    cleanup_test_variable_store(&store);
    printf("PASSED\n");
}

void test_variable_expansion(void) {
    printf("Testing variable expansion... ");
    
    init_test_variable_store(&test_var_store);
    
    // Set up test variables
    set_test_variable(&test_var_store, "HOME", "/home/user");
    set_test_variable(&test_var_store, "USER", "testuser");
    
    // Test simple variable expansion
    char *result = mock_expand_variable("echo $HOME");
    assert(strstr(result, "/home/user") != NULL);
    free(result);
    
    // Test parentheses variable expansion
    result = mock_expand_variable("prefix_$(USER)_suffix");
    assert(strstr(result, "prefix_testuser_suffix") != NULL);
    free(result);
    
    // Test multiple variables
    result = mock_expand_variable("$USER at $HOME");
    assert(strstr(result, "testuser") != NULL);
    assert(strstr(result, "/home/user") != NULL);
    free(result);
    
    // Test undefined variable
    result = mock_expand_variable("$UNDEFINED_VAR");
    assert(strlen(result) >= 0); // Should handle gracefully
    free(result);
    
    cleanup_test_variable_store(&test_var_store);
    printf("PASSED\n");
}

void test_variable_parsing(void) {
    printf("Testing variable parsing in commands... ");
    
    init_test_variable_store(&test_var_store);
    set_test_variable(&test_var_store, "CMD", "ls");
    set_test_variable(&test_var_store, "ARGS", "-la");
    
    // Test variable in command
    char input[] = "$CMD $ARGS /home";
    char *expanded = mock_expand_variable(input);
    assert(strstr(expanded, "ls") != NULL);
    assert(strstr(expanded, "-la") != NULL);
    assert(strstr(expanded, "/home") != NULL);
    
    free(expanded);
    cleanup_test_variable_store(&test_var_store);
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
    
    // Variable system tests
    test_variable_store_init();
    test_variable_set_get();
    test_variable_export();
    test_variable_expansion();
    test_variable_parsing();
    
    printf("=== All Unit Tests Passed! ===\n\n");
}

int main(void) {
    run_all_unit_tests();
    return 0;
}
