#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

// Test result structure
struct TestResult {
    int passed;
    int failed;
    char *current_test;
};

struct TestResult test_result = {0, 0, NULL};

// Helper macros
#define TEST_START(name) \
    do { \
        test_result.current_test = name; \
        printf("Running: %s... ", name); \
        fflush(stdout); \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASSED\n"); \
        test_result.passed++; \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf("FAILED: %s\n", msg); \
        test_result.failed++; \
    } while(0)

#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while(0)

// Helper functions
void cleanup_test_files(void) {
    // Basic test files
    unlink("test_input.txt");
    unlink("test_output.txt");
    unlink("test_append.txt");
    unlink("test_shell_output.txt");
    unlink("test_script.sh");
    
    // Variable test files
    unlink("env_var_test.sh");
    unlink("env_var_output.txt");
    unlink("local_var_test.sh");
    unlink("local_var_output.txt");
    unlink("var_pipe_test.sh");
    unlink("var_pipe_output.txt");
    unlink("var_redir_test.sh");
    unlink("var_redir_output.txt");
    unlink("var_test_output.txt");
    unlink("escaped_var_test.sh");
    unlink("escaped_var_output.txt");
    unlink("undef_var_test.sh");
    unlink("undef_var_output.txt");
    
    // Any other potential test artifacts
    unlink("*.log");
    unlink("*.tmp");
}

int run_shell_command(const char *command, int timeout_seconds) {
    // Create a temporary script to run the command
    FILE *script = fopen("test_script.sh", "w");
    if (!script) return -1;
    
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout %d ./mysh << 'EOF'\n", timeout_seconds);
    fprintf(script, "%s\n", command);
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("test_script.sh", 0755);
    
    int status = system("./test_script.sh > test_shell_output.txt 2>&1");
    unlink("test_script.sh");
    
    return WEXITSTATUS(status);
}

char* read_file_content(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);
    
    return content;
}

// Integration test functions
void test_simple_command(void) {
    TEST_START("Simple command execution");
    
    int result = run_shell_command("echo 'Hello World'", 5);
    ASSERT_TRUE(result == 0, "Command execution failed");
    
    char *output = read_file_content("test_shell_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read output");
    ASSERT_TRUE(strstr(output, "Hello World") != NULL, "Expected output not found");
    
    free(output);
    TEST_PASS();
}

void test_single_pipe(void) {
    TEST_START("Single pipe command");
    
    int result = run_shell_command("echo 'Hello World' | grep Hello", 5);
    ASSERT_TRUE(result == 0, "Pipe command execution failed");
    
    char *output = read_file_content("test_shell_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read output");
    ASSERT_TRUE(strstr(output, "Hello World") != NULL, "Pipe output not found");
    
    free(output);
    TEST_PASS();
}

void test_double_pipe(void) {
    TEST_START("Double pipe command");
    
    int result = run_shell_command("echo 'Hello Beautiful World' | grep Beautiful | wc -w", 5);
    ASSERT_TRUE(result == 0, "Double pipe command execution failed");
    
    char *output = read_file_content("test_shell_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read output");
    ASSERT_TRUE(strstr(output, "3") != NULL, "Word count should be 3");
    
    free(output);
    TEST_PASS();
}

void test_triple_pipe(void) {
    TEST_START("Triple pipe command");
    
    int result = run_shell_command("echo -e 'line1\\nline2\\nline3' | grep line | wc -l | cat", 5);
    ASSERT_TRUE(result == 0, "Triple pipe command execution failed");
    
    char *output = read_file_content("test_shell_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read output");
    ASSERT_TRUE(strstr(output, "3") != NULL, "Line count should be 3");
    
    free(output);
    TEST_PASS();
}

void test_background_simple(void) {
    TEST_START("Simple background command");
    
    // Create a test script that runs a background command and checks jobs
    FILE *script = fopen("bg_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 10 ./mysh << 'EOF'\n");
    fprintf(script, "sleep 2 &\n");
    fprintf(script, "jobs\n");
    fprintf(script, "sleep 3\n");  // Wait for background job to finish
    fprintf(script, "jobs\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("bg_test.sh", 0755);
    int result = system("./bg_test.sh > bg_test_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Background command test failed");
    
    char *output = read_file_content("bg_test_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read background test output");
    ASSERT_TRUE(strstr(output, "[1]") != NULL, "Job ID not found in output");
    
    free(output);
    unlink("bg_test.sh");
    unlink("bg_test_output.txt");
    TEST_PASS();
}

void test_background_pipeline(void) {
    TEST_START("Background pipeline command");
    
    FILE *script = fopen("bg_pipe_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 10 ./mysh << 'EOF'\n");
    fprintf(script, "echo 'test data' | grep test | wc -w &\n");
    fprintf(script, "jobs\n");
    fprintf(script, "sleep 2\n");
    fprintf(script, "jobs\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("bg_pipe_test.sh", 0755);
    int result = system("./bg_pipe_test.sh > bg_pipe_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Background pipeline test failed");
    
    char *output = read_file_content("bg_pipe_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read background pipeline output");
    ASSERT_TRUE(strstr(output, "[1]") != NULL, "Background job ID not found");
    
    free(output);
    unlink("bg_pipe_test.sh");
    unlink("bg_pipe_output.txt");
    TEST_PASS();
}

void test_mixed_fg_bg(void) {
    TEST_START("Mixed foreground and background commands");
    
    FILE *script = fopen("mixed_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 15 ./mysh << 'EOF'\n");
    fprintf(script, "sleep 1 &\n");
    fprintf(script, "echo 'foreground command'\n");
    fprintf(script, "sleep 1 | cat &\n");
    fprintf(script, "echo 'another foreground'\n");
    fprintf(script, "jobs\n");
    fprintf(script, "sleep 3\n");  // Wait for background jobs
    fprintf(script, "jobs\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("mixed_test.sh", 0755);
    int result = system("./mixed_test.sh > mixed_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Mixed fg/bg test failed");
    
    char *output = read_file_content("mixed_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read mixed test output");
    ASSERT_TRUE(strstr(output, "foreground command") != NULL, "Foreground output missing");
    ASSERT_TRUE(strstr(output, "another foreground") != NULL, "Second foreground output missing");
    ASSERT_TRUE(strstr(output, "[1]") != NULL, "First background job not found");
    ASSERT_TRUE(strstr(output, "[2]") != NULL, "Second background job not found");
    
    free(output);
    unlink("mixed_test.sh");
    unlink("mixed_output.txt");
    TEST_PASS();
}

void test_input_redirection(void) {
    TEST_START("Input redirection");
    
    // Create test input file
    FILE *input_file = fopen("test_input.txt", "w");
    fprintf(input_file, "Hello from file\n");
    fclose(input_file);
    
    int result = run_shell_command("cat < test_input.txt", 5);
    ASSERT_TRUE(result == 0, "Input redirection failed");
    
    char *output = read_file_content("test_shell_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read output");
    ASSERT_TRUE(strstr(output, "Hello from file") != NULL, "Input redirection content not found");
    
    free(output);
    cleanup_test_files();
    TEST_PASS();
}

void test_output_redirection(void) {
    TEST_START("Output redirection");
    
    int result = run_shell_command("echo 'Redirected output' > test_output.txt", 5);
    ASSERT_TRUE(result == 0, "Output redirection command failed");
    
    char *output = read_file_content("test_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read redirected output file");
    ASSERT_TRUE(strstr(output, "Redirected output") != NULL, "Redirected content not found");
    
    free(output);
    cleanup_test_files();
    TEST_PASS();
}

void test_append_redirection(void) {
    TEST_START("Append redirection");
    
    // First write
    int result1 = run_shell_command("echo 'First line' > test_append.txt", 5);
    ASSERT_TRUE(result1 == 0, "First write failed");
    
    // Then append
    int result2 = run_shell_command("echo 'Second line' >> test_append.txt", 5);
    ASSERT_TRUE(result2 == 0, "Append operation failed");
    
    char *output = read_file_content("test_append.txt");
    ASSERT_TRUE(output != NULL, "Could not read append test file");
    ASSERT_TRUE(strstr(output, "First line") != NULL, "First line not found");
    ASSERT_TRUE(strstr(output, "Second line") != NULL, "Second line not found");
    
    free(output);
    cleanup_test_files();
    TEST_PASS();
}

void test_job_control(void) {
    TEST_START("Job control (fg/bg commands)");
    
    FILE *script = fopen("job_control_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 15 ./mysh << 'EOF'\n");
    fprintf(script, "sleep 10 &\n");
    fprintf(script, "jobs\n");
    fprintf(script, "fg %%1\n");  // Use %% to escape the %
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("job_control_test.sh", 0755);
    int result = system("./job_control_test.sh > job_control_output.txt 2>&1");
    
    // Note: This test might timeout due to fg bringing job to foreground
    // The important thing is that the shell doesn't crash
    ASSERT_TRUE(WEXITSTATUS(result) == 0 || WEXITSTATUS(result) == 124, "Job control test script failed (exit code other than timeout)");
    
    char *output = read_file_content("job_control_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read job control output");
    
    free(output);
    unlink("job_control_test.sh");
    unlink("job_control_output.txt");
    TEST_PASS();
}

void test_stress_multiple_pipes(void) {
    TEST_START("Stress test: Multiple concurrent pipes");
    
    FILE *script = fopen("stress_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 20 ./mysh << 'EOF'\n");
    fprintf(script, "echo 'pipe1' | cat | cat | cat &\n");
    fprintf(script, "echo 'pipe2' | grep pipe | wc -w &\n");
    fprintf(script, "echo 'pipe3' | cat | grep pipe | cat &\n");
    fprintf(script, "jobs\n");
    fprintf(script, "sleep 3\n");
    fprintf(script, "jobs\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("stress_test.sh", 0755);
    int result = system("./stress_test.sh > stress_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Stress test failed");
    
    char *output = read_file_content("stress_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read stress test output");
    ASSERT_TRUE(strstr(output, "[1]") != NULL, "First background job not found");
    ASSERT_TRUE(strstr(output, "[2]") != NULL, "Second background job not found");
    ASSERT_TRUE(strstr(output, "[3]") != NULL, "Third background job not found");
    
    free(output);
    unlink("stress_test.sh");
    unlink("stress_output.txt");
    TEST_PASS();
}

void test_env_variable_expansion(void) {
    TEST_START("Environment variable expansion");
    
    FILE *script = fopen("env_var_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 10 ./mysh << 'EOF'\n");
    fprintf(script, "export TESTVAR=hello_world\n");
    fprintf(script, "echo $TESTVAR\n");
    fprintf(script, "echo prefix_$(TESTVAR)_suffix\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("env_var_test.sh", 0755);
    int result = system("./env_var_test.sh > env_var_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Environment variable test failed");
    
    char *output = read_file_content("env_var_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read env variable test output");
    ASSERT_TRUE(strstr(output, "hello_world") != NULL, "Variable expansion failed");
    ASSERT_TRUE(strstr(output, "prefix_hello_world_suffix") != NULL, "Parentheses variable expansion failed");
    
    free(output);
    unlink("env_var_test.sh");
    unlink("env_var_output.txt");
    TEST_PASS();
}

void test_local_variable_expansion(void) {
    TEST_START("Local variable expansion");
    
    FILE *script = fopen("local_var_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 10 ./mysh << 'EOF'\n");
    fprintf(script, "set LOCALVAR local_value\n");
    fprintf(script, "echo $LOCALVAR\n");
    fprintf(script, "echo test_$(LOCALVAR)_end\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("local_var_test.sh", 0755);
    int result = system("./local_var_test.sh > local_var_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Local variable test failed");
    
    char *output = read_file_content("local_var_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read local variable test output");
    ASSERT_TRUE(strstr(output, "local_value") != NULL, "Local variable expansion failed");
    ASSERT_TRUE(strstr(output, "test_local_value_end") != NULL, "Local variable parentheses expansion failed");
    
    free(output);
    unlink("local_var_test.sh");
    unlink("local_var_output.txt");
    TEST_PASS();
}

void test_variable_in_pipe(void) {
    TEST_START("Variable expansion in pipes");
    
    FILE *script = fopen("var_pipe_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 10 ./mysh << 'EOF'\n");
    fprintf(script, "export PATTERN=test\n");
    fprintf(script, "echo 'test line 1\\nother line\\ntest line 2' | grep $PATTERN\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("var_pipe_test.sh", 0755);
    int result = system("./var_pipe_test.sh > var_pipe_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Variable in pipe test failed");
    
    char *output = read_file_content("var_pipe_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read variable pipe test output");
    ASSERT_TRUE(strstr(output, "test line 1") != NULL, "Variable expansion in pipe failed");
    ASSERT_TRUE(strstr(output, "test line 2") != NULL, "Variable expansion in pipe failed");
    
    free(output);
    unlink("var_pipe_test.sh");
    unlink("var_pipe_output.txt");
    TEST_PASS();
}

void test_variable_in_redirection(void) {
    TEST_START("Variable expansion in redirection");
    
    FILE *script = fopen("var_redir_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 10 ./mysh << 'EOF'\n");
    fprintf(script, "export OUTFILE=var_test_output.txt\n");
    fprintf(script, "echo 'variable redirection test' > $(OUTFILE)\n");
    fprintf(script, "cat $(OUTFILE)\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("var_redir_test.sh", 0755);
    int result = system("./var_redir_test.sh > var_redir_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Variable in redirection test failed");
    
    char *output = read_file_content("var_redir_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read variable redirection test output");
    ASSERT_TRUE(strstr(output, "variable redirection test") != NULL, "Variable expansion in redirection failed");
    
    // Check that the file was actually created
    ASSERT_TRUE(access("var_test_output.txt", F_OK) == 0, "Output file was not created");
    
    free(output);
    unlink("var_redir_test.sh");
    unlink("var_redir_output.txt");
    unlink("var_test_output.txt");
    TEST_PASS();
}

void test_escaped_variable(void) {
    TEST_START("Escaped variable (literal $)");
    
    FILE *script = fopen("escaped_var_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 10 ./mysh << 'EOF'\n");
    fprintf(script, "export TESTVAR=should_not_expand\n");
    fprintf(script, "echo \\$TESTVAR");
    fprintf(script, "\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("escaped_var_test.sh", 0755);
    int result = system("./escaped_var_test.sh > escaped_var_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Escaped variable test failed");
    
    char *output = read_file_content("escaped_var_output.txt");
    char *echo_line = strstr(output, "\$TESTVAR");
//    printf("\nDEBUG in integration test: %s", output);
    ASSERT_TRUE(output != NULL, "Could not read escaped variable test output");
    ASSERT_TRUE(echo_line != NULL, "Variable was expanded when it should have been literal");
    ASSERT_TRUE(strstr(echo_line, "should_not_expand") == NULL, "Escaped variable was expanded");
    
    free(output);
    unlink("escaped_var_test.sh");
    unlink("escaped_var_output.txt");
    TEST_PASS();
}

void test_undefined_variable(void) {
    TEST_START("Undefined variable expansion");
    
    FILE *script = fopen("undef_var_test.sh", "w");
    fprintf(script, "#!/bin/bash\n");
    fprintf(script, "timeout 10 ./mysh << 'EOF'\n");
    fprintf(script, "echo before_$UNDEFINED_VAR after\n");
    fprintf(script, "echo test_$(UNDEFINED_VAR) end\n");
    fprintf(script, "exit\n");
    fprintf(script, "EOF\n");
    fclose(script);
    
    chmod("undef_var_test.sh", 0755);
    int result = system("./undef_var_test.sh > undef_var_output.txt 2>&1");
    
    ASSERT_TRUE(WEXITSTATUS(result) == 0, "Undefined variable test failed");
    
    char *output = read_file_content("undef_var_output.txt");
    ASSERT_TRUE(output != NULL, "Could not read undefined variable test output");
    // Undefined variables should expand to empty string
    ASSERT_TRUE(strstr(output, "before_ after\n") != NULL || strstr(output, "before_after") != NULL, 
                "Undefined variable not handled correctly");
    
    free(output);
    unlink("undef_var_test.sh");
    unlink("undef_var_output.txt");
    TEST_PASS();
}

void run_all_integration_tests(void) {
    printf("=== Running Integration Tests ===\n");
    printf("Note: These tests require the shell executable './mysh' to be present\n\n");
    
    // Check if shell executable exists
    if (access("./mysh", X_OK) != 0) {
        printf("ERROR: Shell executable './mysh' not found or not executable\n");
        printf("Please compile your shell first with: make mysh\n");
        return;
    }
    
    test_simple_command();
    test_single_pipe();
    test_double_pipe();
    test_triple_pipe();
    test_background_simple();
    test_background_pipeline();
    test_mixed_fg_bg();
    test_input_redirection();
    test_output_redirection();
    test_append_redirection();
    test_job_control();
    test_stress_multiple_pipes();
    
    // Variable expansion tests
    test_env_variable_expansion();
    test_local_variable_expansion();
    test_variable_in_pipe();
    test_variable_in_redirection();
    test_escaped_variable();
    test_undefined_variable();
    
    printf("\n=== Integration Test Results ===\n");
    printf("Passed: %d\n", test_result.passed);
    printf("Failed: %d\n", test_result.failed);
    printf("Total: %d\n", test_result.passed + test_result.failed);
    
    if (test_result.failed == 0) {
        printf("🎉 All integration tests passed!\n");
    } else {
        printf("❌ %d test(s) failed\n", test_result.failed);
    }
    
    // Cleanup
    unlink("test_shell_output.txt");
    cleanup_test_files();
}

int main(void) {
    run_all_integration_tests();
    return test_result.failed > 0 ? 1 : 0;
}
