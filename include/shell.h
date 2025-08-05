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

// Built-in command names. Excludes "exit" since it's handled separately in the main loop.
extern const char *built_in_commands[];
extern const char *job_commands[];

// Piple-related structures
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

//Job related structures
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

// Function prototypes
void parse_input(char *input, struct Pipeline *pipeline, int *input_has_background_process); 
struct Command *initialze_Command(struct Command *cmd); 
int process_built_in_command(struct Command *cmd);
int process_job_command(struct Command *cmd, struct JobTable *job_table);
int createJob(struct JobTable *table, char *input, int *is_background, pid_t *pids, int pid_count);
int cleanup_single_job(struct Job *job);
void cleanup_finished_jobs(struct JobTable *table);
int find_finished_job(struct JobTable *table);
void sigchld_handler(int sig);
