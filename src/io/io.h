#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "../hash/stribog.h"

int process_input_output(const char *input_file, const char *output_file,
						 int hash_size, int hex_input, int hex_output);
#endif // IO_H
