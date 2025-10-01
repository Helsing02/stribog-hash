#ifndef CLI_H
#define CLI_H

typedef struct {
    const char *input_file;
    const char *output_file;
    int hex_input;
    int hex_output;
    int hash_size;
    int help_requested;
} Config;

int parse_arguments(int argc, char *argv[], Config *config);
void print_help(const char *program_name);

#endif // CLI_H
