// Tests utility functions. Compiled with your utils.c
// gcc -I../include test_shell.c ../src/utils.c -o test_shell

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "../include/utils.h"

// Test counter
int tests_passed = 0;
int tests_total = 0;

// Test helper macros
#define TEST_START(name) \
    printf("Testing %s... ", name); \
    tests_total++;

#define TEST_PASS() \
    printf("PASS\n"); \
    tests_passed++;

#define TEST_FAIL(msg) \
    printf("FAIL: %s\n", msg); \
    exit(1);

#define ASSERT_EQ(expected, actual, msg) \
    if ((expected) != (actual)) { \
        printf("FAIL: %s (expected %ld, got %ld)\n", msg, (long)(expected), (long)(actual)); \
        exit(1); \
    }

#define ASSERT_STR_EQ(expected, actual, msg) \
    if (strcmp(expected, actual) != 0) { \
        printf("FAIL: %s (expected '%s', got '%s')\n", msg, expected, actual); \
        exit(1); \
    }

// Test initialze_Command function
void test_initialize_command() {
    TEST_START("initialze_Command");
    
    struct Command cmd;
    initialze_Command(&cmd);
    
    // Check that argv is null-terminated
    ASSERT_EQ(0, (intptr_t)cmd.argv[0], "argv[0] should be NULL");
    ASSERT_EQ(0, (intptr_t)cmd.argv[MAX_TOKENS-1], "argv[MAX_TOKENS-1] should be NULL");
    
    // Check redirect flags are cleared
    ASSERT_EQ(0, cmd.redirect_flags, "redirect_flags should be 0");
    
    // Check redirect filenames are empty
    ASSERT_STR_EQ("", cmd.redirects.input_file, "input_file should be empty");
    ASSERT_STR_EQ("", cmd.redirects.output_file, "output_file should be empty");
    ASSERT_STR_EQ("", cmd.redirects.append_file, "append_file should be empty");
    
    TEST_PASS();
}

// Test simple command parsing
void test_parse_simple_command() {
    TEST_START("parse_input - simple command");
    
    struct Pipeline pipeline;
    pipeline.command_count = 0;
    initialze_Command(&pipeline.commands[0]);
    
    char input[] = "ls -la /tmp";
    parse_input(input, &pipeline);
    
    // Should have 1 command (command_count = 0 means first command)
    ASSERT_EQ(0, pipeline.command_count, "should have command_count = 0 (first command is index 0)");
    
    // Check parsed arguments
    ASSERT_STR_EQ("ls", pipeline.commands[0].argv[0], "argv[0] should be 'ls'");
    ASSERT_STR_EQ("-la", pipeline.commands[0].argv[1], "argv[1] should be '-la'");
    ASSERT_STR_EQ("/tmp", pipeline.commands[0].argv[2], "argv[2] should be '/tmp'");
    ASSERT_EQ(0, (intptr_t)pipeline.commands[0].argv[3], "argv[3] should be NULL");
    
    // Should have no redirections
    ASSERT_EQ(0, pipeline.commands[0].redirect_flags, "should have no redirect flags");
    
    TEST_PASS();
}

// Test input redirection parsing
void test_parse_input_redirect() {
    TEST_START("parse_input - input redirection");
    
    struct Pipeline pipeline;
    pipeline.command_count = 0;
    initialze_Command(&pipeline.commands[0]);
    
    char input[] = "cat < input.txt";
    parse_input(input, &pipeline);
    
    // Check command
    ASSERT_STR_EQ("cat", pipeline.commands[0].argv[0], "argv[0] should be 'cat'");
    ASSERT_EQ(0, (intptr_t)pipeline.commands[0].argv[1], "argv[1] should be NULL");
    
    // Check redirection
    ASSERT_EQ(REDIRECT_IN, pipeline.commands[0].redirect_flags, "should have REDIRECT_IN flag");
    ASSERT_STR_EQ("input.txt", pipeline.commands[0].redirects.input_file, "input_file should be 'input.txt'");
    
    TEST_PASS();
}

// Test output redirection parsing
void test_parse_output_redirect() {
    TEST_START("parse_input - output redirection");
    
    struct Pipeline pipeline;
    pipeline.command_count = 0;
    initialze_Command(&pipeline.commands[0]);
    
    char input[] = "echo hello > output.txt";
    parse_input(input, &pipeline);
    
    // Check command
    ASSERT_STR_EQ("echo", pipeline.commands[0].argv[0], "argv[0] should be 'echo'");
    ASSERT_STR_EQ("hello", pipeline.commands[0].argv[1], "argv[1] should be 'hello'");
    ASSERT_EQ(0, (intptr_t)pipeline.commands[0].argv[2], "argv[2] should be NULL");
    
    // Check redirection
    ASSERT_EQ(REDIRECT_OUT, pipeline.commands[0].redirect_flags, "should have REDIRECT_OUT flag");
    ASSERT_STR_EQ("output.txt", pipeline.commands[0].redirects.output_file, "output_file should be 'output.txt'");
    
    TEST_PASS();
}

// Test pipe parsing
void test_parse_pipe() {
    TEST_START("parse_input - pipe");
    
    struct Pipeline pipeline;
    pipeline.command_count = 0;
    initialze_Command(&pipeline.commands[0]);
    
    char input[] = "ls -la | grep txt";
    parse_input(input, &pipeline);
    
    // Should have 2 commands (command_count = 1 means commands 0 and 1)
    ASSERT_EQ(1, pipeline.command_count, "should have command_count = 1 (two commands: 0 and 1)");
    
    // Check first command
    ASSERT_STR_EQ("ls", pipeline.commands[0].argv[0], "first command argv[0] should be 'ls'");
    ASSERT_STR_EQ("-la", pipeline.commands[0].argv[1], "first command argv[1] should be '-la'");
    ASSERT_EQ(0, (intptr_t)pipeline.commands[0].argv[2], "first command argv[2] should be NULL");
    
    // Check second command
    ASSERT_STR_EQ("grep", pipeline.commands[1].argv[0], "second command argv[0] should be 'grep'");
    ASSERT_STR_EQ("txt", pipeline.commands[1].argv[1], "second command argv[1] should be 'txt'");
    ASSERT_EQ(0, (intptr_t)pipeline.commands[1].argv[2], "second command argv[2] should be NULL");
    
    TEST_PASS();
}

// Test empty input
void test_parse_empty() {
    TEST_START("parse_input - empty input");
    
    struct Pipeline pipeline;
    pipeline.command_count = 0;
    initialze_Command(&pipeline.commands[0]);
    
    char input[] = "";
    parse_input(input, &pipeline);
    
    // Should have no valid commands
    ASSERT_EQ(0, pipeline.command_count, "should have command_count = 0");
    ASSERT_EQ(0, (intptr_t)pipeline.commands[0].argv[0], "argv[0] should be NULL");
    
    TEST_PASS();
}

int main() {
    printf("Shell Parsing Unit Tests\n");
    printf("========================\n\n");
    
    // Run all tests
    test_initialize_command();
    test_parse_simple_command();
    test_parse_input_redirect();
    test_parse_output_redirect();
    test_parse_pipe();
    test_parse_empty();
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Tests passed: %d/%d\n", tests_passed, tests_total);
    
    if (tests_passed == tests_total) {
        printf("All tests PASSED! 🎉\n");
        printf("\nNext steps:\n");
        printf("1. Test your actual shell binary with real commands\n");
        printf("2. Test edge cases (empty input, invalid commands, etc.)\n");
        printf("3. Test complex pipelines and redirections\n");
        return 0;
    } else {
        printf("Some tests FAILED! 😞\n");
        return 1;
    }
}
