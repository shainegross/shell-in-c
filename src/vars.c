#include "../include/shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

// Initialize the variable store with environment variables
// All initial variables are marked as exported (since they come from environ)
int init_variable_store(struct VariableStore *vs) {
    // Count original environment variables
    int env_count = 0;
    while (environ[env_count] != NULL) env_count++;
    
    // Initialize the store
    vs->capacity = env_count + VARS_EXCESS_CAPACITY;
    vs->count = 0;  // Start with 0 variables, we'll add them one by one
    vs->vars = malloc(sizeof(struct Variable) * vs->capacity);
    if (vs->vars == NULL) {
        perror("malloc failed for variable store");
        return -1;
    }
    
    // Copy environment variables into the store
    for (int i = 0; i < env_count; i++) {
        char *env_var = environ[i];
        char *equals = strchr(env_var, '=');
        if (equals == NULL) continue; // Skip malformed entries
        
        // Extract name and value
        size_t name_len = equals - env_var;
        vs->vars[vs->count].name = malloc(name_len + 1);
        vs->vars[vs->count].value = malloc(strlen(equals + 1) + 1);
        
        if (vs->vars[vs->count].name == NULL || vs->vars[vs->count].value == NULL) {
            perror("malloc failed for variable name/value");
            return -1;
        }
        
        strncpy(vs->vars[vs->count].name, env_var, name_len);
        vs->vars[vs->count].name[name_len] = '\0';
        strcpy(vs->vars[vs->count].value, equals + 1);
        vs->vars[vs->count].is_exported = 1; // All initial vars are exported
        
        vs->count++;
    }
    
    for (int j = 0; j < vs->count; j++) {
        if (strcmp(vs->vars[j].name, "PATH") == 0) {
            vs->PATH_PTR = vs->vars[j].value;  
            break;  
        }
    }

    return vs->count;
}

// Resize the variable store to increase capacity
static int resize_variable_store(struct VariableStore *vs) {
    int new_capacity = vs->capacity += VARS_EXCESS_CAPACITY;
    struct Variable *new_vars = realloc(vs->vars, sizeof(struct Variable) * new_capacity);
    if (new_vars == NULL) {
        perror("realloc failed for variable store");
        return -1;
    }
    vs->vars = new_vars;
    vs->capacity = new_capacity;
    return 0;
}

// Find a variable by name, return index or -1 if not found
static int find_variable(const struct VariableStore *vs, const char *name) {
    for (int i = 0; i < vs->count; i++) {
        if (strcmp(vs->vars[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Set a variable (local or exported)
// If is_exported is 1, it's an environment variable
// If is_exported is 0, it's a local variable
// Returns 0 on success, -1 on failure
int set_variable(struct VariableStore *vs, const char *name, const char *value, int is_exported) {
    int index = find_variable(vs, name);
    int update_PATH = ((strcmp(name, "PATH") == 0) ? 1 : 0);

    if (index >= 0) {
        // Variable exists, update it
        free(vs->vars[index].value);
        vs->vars[index].value = malloc(strlen(value) + 1);
        if (vs->vars[index].value == NULL) {
            perror("malloc failed for variable value");
            return -1;
        }
        strcpy(vs->vars[index].value, value);
        vs->vars[index].is_exported = is_exported;
        if (update_PATH) {
            vs->PATH_PTR = vs->vars[index].value; 
        }
        return 0;
    }
    
    // Variable doesn't exist, add new one
    if (vs->count >= vs->capacity) {
        if (resize_variable_store(vs) < 0) return -1;
    }
    
    vs->vars[vs->count].name = malloc(strlen(name) + 1);
    vs->vars[vs->count].value = malloc(strlen(value) + 1);
    
    if (vs->vars[vs->count].name == NULL || vs->vars[vs->count].value == NULL) {
        perror("malloc failed for new variable");
        return -1;
    }
    
    strcpy(vs->vars[vs->count].name, name);
    strcpy(vs->vars[vs->count].value, value);
    vs->vars[vs->count].is_exported = is_exported;
    
    vs->count++;
    return 0;
}

// Get a variable's value by name
// Returns pointer to value or NULL if not found
char *get_variable(const struct VariableStore *vs, const char *name) {
    int index = find_variable(vs, name);
    if (index >= 0) {
        return vs->vars[index].value;
    }
    return NULL;
}

// Export a variable (promote from local to environment)
// Returns 0 on success, -1 if variable not found
int export_variable(struct VariableStore *vs, const char *name) {
    int index = find_variable(vs, name);
    if (index >= 0) {
        vs->vars[index].is_exported = 1;
        return 0;
    }
    return -1; // Variable not found
}

// Remove a variable from the store
// Returns 0 on success, -1 if variable not found
int unset_variable(struct VariableStore *vs, const char *name) {
    int index = find_variable(vs, name);
    if (index < 0) return -1; // Not found
    
    // Free the variable
    free(vs->vars[index].name);
    free(vs->vars[index].value);
    
    // Move the last variable to fill the gap
    if (index < vs->count - 1) {
        vs->vars[index] = vs->vars[vs->count - 1];
        vs->vars[vs->count - 1] = (struct Variable){0}; // Clear the last variable
    }
    
    vs->count--;
    return 0;
}

// Build an environment array for passing to child processes
// Returns a NULL-terminated array of "name=value" strings
// Caller is responsible for freeing the returned array (but not the strings inside)
char **build_environ_array(const struct VariableStore *vs) {
    // Count exported variables
    int exported_count = 0;
    for (int i = 0; i < vs->count; i++) {
        if (vs->vars[i].is_exported) {
            exported_count++;
        }
    }
    
    // Allocate array (+ 1 for NULL terminator)
    char **env_array = malloc(sizeof(char *) * (exported_count + 1));
    if (env_array == NULL) {
        perror("malloc failed for environment array");
        return NULL;
    }
    
    // Fill array with exported variables
    int env_index = 0;
    for (int i = 0; i < vs->count; i++) {
        if (vs->vars[i].is_exported) {
            // Allocate space for "name=value"
            size_t total_len = strlen(vs->vars[i].name) + strlen(vs->vars[i].value) + 2;
            env_array[env_index] = malloc(total_len);
            if (env_array[env_index] == NULL) {
                perror("malloc failed for environment string");
                // Free what we've allocated so far
                for (int j = 0; j < env_index; j++) {
                    free(env_array[j]);
                }
                free(env_array);
                return NULL;
            }
            sprintf(env_array[env_index], "%s=%s", vs->vars[i].name, vs->vars[i].value);
            env_index++;
        }
    }
    
    env_array[exported_count] = NULL; // NULL terminate
    return env_array;
}

void display_variables(const struct VariableStore *vs, int display_mode) {
    for (int i = 0; i < vs->count; i++) {
        if (display_mode == DISPLAY_LOCAL && vs->vars[i].is_exported) continue;
        if (display_mode == DISPLAY_EXPORTED && !vs->vars[i].is_exported) continue;
        printf("%s=%s\n", vs->vars[i].name, vs->vars[i].value);
    }
}


// Free an environment array created by build_environ_array
void free_environ_array(char **env_array) {
    if (env_array == NULL) return;
    
    for (int i = 0; env_array[i] != NULL; i++) {
        free(env_array[i]);
    }
    free(env_array);
}

// Clean up the variable store
void free_variable_store(struct VariableStore *vs) {
    for (int i = 0; i < vs->count; i++) {
        free(vs->vars[i].name);
        free(vs->vars[i].value);
    }
    free(vs->vars);
    vs->vars = NULL;
    vs->count = 0;
    vs->capacity = 0;
}

char *find_executable_in_path(char* command, struct VariableStore *vs){
    // If command contains '/', it's already a path - just check if it exists
    const char *path = vs->PATH_PTR;

    if (strchr(command, '/') != NULL) {
        if (access(command, X_OK) == 0) {
            return strdup(command);
        }
        return NULL;
    }
    
    // Use provided PATH
    if (path == NULL) return NULL;
    
    // Make a copy of PATH to tokenize
    char *path_copy = strdup(path);
    char *dir = strtok(path_copy, ":");
    
    while (dir != NULL) {
        // Build full path: dir + "/" + command
        size_t full_path_len = strlen(dir) + strlen(command) + 2;
        char *full_path = malloc(full_path_len);
        snprintf(full_path, full_path_len, "%s/%s", dir, command);
        
        // Check if executable exists
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;  
        }
        
        free(full_path);
        dir = strtok(NULL, ":");
    }
    
    free(path_copy);
    return NULL;  // Not found
}
