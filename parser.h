#pragma once

struct program *parse_program(char *program_file_path);

void write_compiled_program(struct program *program, char *filename);