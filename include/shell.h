#include <sys/types.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 64
#define MAX_COMMANDS 10
#define MAX_JOBS 32

// Redirection flags
#define REDIRECT_IN   0x01  // 0001
#define REDIRECT_OUT  0x02  // 0010
#define REDIRECT_APP  0x04  // 0100

#define VARS_EXCESS_CAPACITY 16 
#define INIT_HISTORY 25
#define MAX_HISTORY 100

// Supports display_variables function
#define DISPLAY_LOCAL 1
#define DISPLAY_EXPORTED 2
#define DISPLAY_ALL 3

// Built-in command names. Excludes "exit" since it's handled separately in the main loop.
extern const char *built_in_commands[];
extern const char *job_commands[];

// VARIABLE RELATED STRUCTS & VARIABLES
// Structure for a single variable (can be local or exported)
struct Variable {
    char *name;
    char *value;
    int is_exported;  // 1 if environment variable, 0 if local only
};

// Structure for managing all shell variables (both local and environment)
struct VariableStore {
    struct Variable *vars;  // Array of variables
    int count;              // Number of variables currently stored
    int capacity;           // Current capacity of the array
    char *PATH_PTR;         // Quick access pointer to PATH value

};

// Global variables for shell environment
extern char **environ;  // Original environment variables
extern struct VariableStore var_store;     



// PIPELINE RELATED STRUCTS
struct Redirection {
    char input_file[MAX_INPUT_SIZE];    
    char output_file[MAX_INPUT_SIZE];   
    char append_file[MAX_INPUT_SIZE];
};

struct Command{
    char *argv[MAX_TOKENS];
    struct Redirection redirects;
    int redirect_flags;
};

struct Pipeline {
    struct Command commands[MAX_COMMANDS];
    int pipe_count;
};

//JOB RELATED STRUCTS & VARIABLES
enum JobState {JOB_RUNNING, JOB_STOPPED, JOB_DONE };

struct Job {
    int job_id;                    // Job number (1, 2, 3...)
    pid_t pids[MAX_COMMANDS];      // All PIDs in this job (for pipelines)
    int pid_status[MAX_COMMANDS];  // 1=running, 0=finished
    int pid_count;                 // Number of processes in this job
    int is_background;             // Background or foreground
    char command_line[MAX_INPUT_SIZE]; // Original command for display
    enum JobState state;           // RUNNING, STOPPED, DONE
};

struct JobTable {
    struct Job jobs[MAX_JOBS];     // Array of jobs
    int job_count;                 // Number of active jobs
    int next_job_id;               // Next ID to assign; starts at 1
};

extern struct JobTable job_table;

// HISTORY RELATED STRUCT & VARIABLES
struct HistoryTable {
    char **input;            // Array of input strings
    int count;                  // Number of input in history
    int capacity;               // Capacity of the array
    int current_index;         // Current index for adding new commands (circular buffer)
    int navigate_index;  
};

extern struct HistoryTable history;

// FUNCTION PROTOTYPES
// built-ins.c
struct Command *initialze_Command(struct Command *cmd); 
int process_built_in_command(struct Command *cmd);

//history.c
int add_to_history(struct HistoryTable *history, const char *input);
char *fetch_history_by_index (struct HistoryTable *h, int index);
void free_history(struct HistoryTable *history);
int init_history_table(struct HistoryTable *history);
void print_all_history(struct HistoryTable *history);
char *print_history_entry(struct HistoryTable *history, int index);
void save_history(struct HistoryTable *history);

// jobs.c
int createJob(struct JobTable *table, char *input, int *is_background, pid_t *pids, int pid_count);
int cleanup_single_job(struct Job *job);
void cleanup_finished_jobs(struct JobTable *table);
int find_finished_job(struct JobTable *table);
int process_job_command(struct Command *cmd, struct JobTable *job_table);

// parser.c
void parse_input(char *input, struct Pipeline *pipeline, int *input_has_background_process); 

// vars.c - Variable management
char **build_environ_array(const struct VariableStore *vs);
void display_variables(const struct VariableStore *vs, int display_mode);
int export_variable(struct VariableStore *vs, const char *name);
char *find_executable_in_path(char* command, struct VariableStore *vs);
void free_environ_array(char **env_array);
void free_variable_store(struct VariableStore *vs);
char *get_variable(const struct VariableStore *vs, const char *name);
int init_variable_store(struct VariableStore *vs);
int set_variable(struct VariableStore *vs, const char *name, const char *value, int is_exported);
int unset_variable(struct VariableStore *vs, const char *name);

// signals.c
void save_and_exit(int signo);
void sigchld_handler(int sig);


