#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s [Program file] [opt. output file]", argv[0]);
    }
    char *program_file_path = argv[1];
    char *output_file;

    if (argc >= 3)
        output_file = argv[2];
    else {
        output_file = calloc(strlen(program_file_path) + 5, sizeof(char));
        strcpy(output_file, program_file_path);
        strcat(output_file, ".out");
    }

    struct program *program = parse_program(program_file_path);

    write_compiled_program(program, output_file);

    return 0;
}
