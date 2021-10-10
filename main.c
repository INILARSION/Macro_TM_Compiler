#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

int main(int argc, char** argv) {
    char *program_file_path;
    char *output_file;

    if (argc == 3) {
        program_file_path = argv[1];
        output_file = argv[2];
    } else if (argc == 2) {
        program_file_path = argv[1];
        output_file = calloc(strlen(program_file_path) + 5, sizeof(char));
        sprintf(output_file, "%s.out", program_file_path);
    } else {
        printf("Usage: %s [Program file] [opt. output file]", argv[0]);
        exit(-1);
    }

    struct program *program = parse_program(program_file_path);

    write_compiled_program(program, output_file);

    return 0;
}
