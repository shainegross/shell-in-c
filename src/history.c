#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"


struct HistoryTable history;
char *history_filename = "./history.txt";

int is_empty_or_spaces(const char *s);


// Initialize the history table
int init_history_table(struct HistoryTable *history) {

    history->input = malloc(MAX_HISTORY * sizeof(char *));
    if (!history->input) return -1;

    for (int i = 0; i < MAX_HISTORY; i++)
        history->input[i] = NULL;
    history->count = 0;
    history->capacity = MAX_HISTORY;
    history->current_index = 0;
    history->navigate_index = 0;
    
    FILE *file = fopen(history_filename, "r");
    if (file) {
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // strip newline and add line to history struct
        if(add_to_history(history, line) == -1) {
            fprintf(stderr, "Failed to add to history\n");
        }
    }
    fclose(file);
    }
    return 0;
}

// Adds an individual input to the history table
int add_to_history(struct HistoryTable *history, const char *input) {
    if (!input || !history || input[0] == '\0' || (input[0] == '!' && input[1] == '!') || (input[0] == '!' && isalnum(input[1])) || is_empty_or_spaces(input)) return -1;
    struct HistoryTable *h = history;

    // check most recent 10 entries to see if input already exists
    for (int i = 1; i < 10 && i < h->count; i++) {
        int idx = (h->current_index - i) % h->capacity;
        if (strcmp(h->input[idx], input) == 0) 
            return 0; // Input already exists in history
    }

    if (h->count >= h->capacity)
        free(h->input[h->current_index]);
    
    // Allocate memory for the new input and copy it
    h->input[h->current_index] = malloc(strlen(input) + 1);
    if (!h->input[h->current_index]) return -1;
    strcpy(h->input[h->current_index], input);

    h->current_index = (h->current_index + 1) % h->capacity;
    if (h->count < h->capacity) h->count++;

    return 0;
}

// For '!!' and '!n', returns history entry corresponding to index (1 is most recent, not 0) 
char *fetch_history_by_index (struct HistoryTable *h, int index) {
     if (!h || index < 1 || index > h->count) return NULL;

    int pos = (h->current_index - index + h->capacity) % h->capacity;
    return h->input[pos];
}

// Saves History
void save_history(struct HistoryTable *history) {
    FILE *file = fopen(history_filename, "w");
    if (!file) {
        perror("Failed to open history file for writing");
        return;
    }
    for (int i = 0; i < history->count; i++) {
        int idx = (history->current_index + i) % history->capacity;
        if (history->input[idx]) {
            fprintf(file, "%s\n", history->input[idx]);
        }
    }
    fclose(file);
}

// Frees History Table and all individual entries
void free_history(struct HistoryTable *history) {
    if (!history || !history->input) return;
    
    for (int i = 0; i < history->capacity; i++) {
        if (history->input[i]) {
            free(history->input[i]);
            history->input[i] = NULL;
        }
    }
    free(history->input);
    history->input = NULL;
    
    // Reset counters just to be safe
    history->count = 0;
    history->capacity = 0;
    history->current_index = 0;
    history->navigate_index = 0;
}

// Prints all history entries 
void print_all_history(struct HistoryTable *history) {
    if (!history) return;

    printf("Command History:\n");
    printf("----------------\n");

    for (int i = 0; i < history->count; i++) {
        
        int idx = (history->current_index + i) % history->capacity;
        if (history->input[idx] != NULL) {
            printf("%d: %s\n", i + 1, history->input[idx]);
        }
    }
}

// Returns the input from a specific history index
char *print_history_entry(struct HistoryTable *history, int index) {
    if (!history || index < 0 || index >= history->count) return NULL;

    int idx = (history->current_index + index) % history->capacity;
    return history->input[idx];
}

// Helper function to flag empty or whitespace strings
// Returns 1 if empty or whitespace, 0 otherwise
int is_empty_or_spaces(const char *s) {
    if (s == NULL) return 1;   // Treat NULL as empty

    while (*s) {
        if (!isspace((unsigned char)*s)) {
            return 0;  // Return 0 for non-space character
        }
        s++;
    }
    return 1; // All spaces or empty string
}