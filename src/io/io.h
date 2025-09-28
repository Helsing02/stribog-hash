#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "../hash/stribog.h"

int process_input_output(const char *input_file, const char *output_file, int hash_size, int output_bytes_mode);

#endif // IO_H
