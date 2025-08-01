
#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 64
#define MAX_COMMANDS 10

// Redirection flags
#define REDIRECT_IN   0x01  // 0001
#define REDIRECT_OUT  0x02  // 0010
#define REDIRECT_APP  0x04  // 0100


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


void parse_input(char *input, struct Pipeline *pipeline); 
struct Command *initialze_Command(struct Command *cmd); 