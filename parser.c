#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "program_helper.h"

/*
 * Get the line count of a file.
 */
int get_file_line_count(FILE *file_ptr) {
    char *line = NULL;
    size_t buffer_size = 0;
    int line_count = 0;
    //count lines and reset to beginning of file
    while (getline(&line, &buffer_size, file_ptr) != -1) {
        ++line_count;
    }
    fseek(file_ptr, 0, 0);
    return line_count;
}

/*
 * Parse state names and store it in the struct.
 * Sets start, halt and reject states.
 */
void parse_states(struct program *program, char *line, size_t line_length) {
    // check that at least one alphabet is present and that a program follows
    if (line_length < 4 || line[0] != 'S' || line[line_length-1] != '\n'){
        exit(-1);
    }
    // check some formatting
    if (line_length < 4 || line[0] != 'S') {
        printf("Wrong format for states!\n");
        exit(-1);
    }

    // exclude G: and \n
    line_length -= 4;
    // skip "S: "
    line += 3;

    // count number of states
    program->state_count = 1;
    for (int i = 0; i < line_length; ++i) {
        if (line[i] == ',')
            ++program->state_count;
    }

    program->state_names = malloc(program->state_count * sizeof(char*));

    // remove newline '\n' at end of line
    line[line_length] = '\0';

    for (int current_state = 0; current_state < program->state_count; ++current_state){
        program->state_names[current_state] = strdup(strsep(&line, ","));
    }
}

/*
 * Parse alphabet part of the program file. Add its parts to the program struct.
 */
void parse_alphabet(struct program *program, char *line, size_t line_length) {
    // check that at least one alphabet is present and that a program follows
    if (line_length < 4 || line[0] != 'G' || line[line_length-1] != '\n'){
        exit(-1);
    }
    // exclude G: and \n
    line_length -= 4;
    // skip "G: "
    line += 3;

    // get the number of alphabet symbols
    program->alphabet_size = 1;
    for (int i = 0; i < line_length; ++i) {
        if (line[i] == ',')
            ++program->alphabet_size;
    }

    program->alphabet = malloc(program->alphabet_size * sizeof(char*));

    // get the length of the individual alphabet symbols and put them then in a list
    line[line_length] = '\0';
    for (int current_element = 0; current_element < program->alphabet_size; ++current_element){
        // add symbol to the list
        program->alphabet[current_element] = strdup(strsep(&line, ","));
    }
}

/*
 * Get index of matching state name of first substring in the line.
 */
int search_matching_element(char **line, char **elements, int element_count) {
    char *tmp_str = strsep(line, "|");
    int found_match = -1;
    int matched_index;
    // compare substring with the list of elements and get the index if found
    for (matched_index = 0; matched_index < element_count; ++matched_index) {
        if(strcmp(tmp_str, elements[matched_index]) == 0) {
            found_match = 0;
            break;
        }
    }
    if (found_match == -1) {
        printf("Program delta contains state which is not contained in the defined states!");
        exit(-1);
    }
    return matched_index;
}

int get_element_count(char *symbol, char seperator) {
    int count = 1;
    while (*symbol != '\0') {
        if (*symbol == seperator)
            ++count;
        ++symbol;
    }
    return count;
}

struct alphabet_symbols *get_alphabet_symbols(struct program *program, char *symbols) {
    struct alphabet_symbols *alphabet_symbols = malloc(sizeof(struct alphabet_symbols));
    int symbols_len = strlen(symbols);
    if (symbols[0] == '(') {
        if (symbols[symbols_len - 1] != ')') {
            printf("Malformed delta, ) ist missing!");
            exit(-1);
        }
        symbols[symbols_len - 1] = '\0';
        ++symbols;
        alphabet_symbols->type = '1';
    } else if (symbols[0] == '[') {
        if (symbols[symbols_len - 1] != ']') {
            printf("Malformed delta, ] is missing!");
            exit(-1);
        }
        symbols[symbols_len - 1] = '\0';
        ++symbols;
        alphabet_symbols->type = 'n';
    } else {
        alphabet_symbols->type = '0';
    }
    alphabet_symbols->symbol_count = get_element_count(symbols, '|');
    alphabet_symbols->symbols = malloc(alphabet_symbols->symbol_count * sizeof(int));
    for (int i = 0; i < alphabet_symbols->symbol_count; ++i) {
        alphabet_symbols->symbols[i] = search_matching_element(&symbols, program->alphabet, program->alphabet_size);
    }

    return alphabet_symbols;
}

struct state_helper *get_state_helper(char* state) {
    struct state_helper *state_helper = malloc(sizeof(struct state_helper));
    int state_len = strlen(state);
    for (int i = 0; i < state_len; ++i) {
        if (state[i] == '(' && state[i + 1] == '*' && (state[i + 2] == 'r' || state[i + 2] == 'w')) {
            state_helper->substitution_type = state[i + 2];
            state_helper->state_prefix = strndup(state, i);
            state_helper->state_postfix = strndup(state + i + 4, state_len - ( i + 3));
            return state_helper;
        }
    }
    state_helper->substitution_type = '0';
    state_helper->state_prefix = state;
    return state_helper;
}

void add_state_to_program(struct program *program, char *state) {
    // check if state is yet contained
    for (int i = 0; i < program->state_count; ++i) {
        if (strcmp(program->state_names[i], state) == 0)
            return;
    }
    ++program->state_count;
    program->state_names = realloc(program->state_names, program->state_count * sizeof(struct deltas*));
    program->state_names[program->state_count - 1] = state;
}

char *generate_state_str(struct program *program, struct state_helper *state_helper, int read_symbol, int write_symbol){
    if (state_helper->substitution_type == '0')
            return state_helper->state_prefix;
    
    char *state;
    int symbol;
    int state_len = strlen(state_helper->state_prefix) + strlen(state_helper->state_postfix);
    if (state_helper->substitution_type == 'r')
        symbol = read_symbol;
    else
        symbol = write_symbol;

    state = calloc( state_len + strlen(program->alphabet[symbol]) + 1, sizeof(char));
    strcat(state, state_helper->state_prefix);
    strcat(state, program->alphabet[symbol]);
    strcat(state, state_helper->state_postfix);
    add_state_to_program(program, state);
    return state;
}

void generate_1t1_deltas(struct program *program, struct state_helper *state, struct state_helper *subsequent_state, struct alphabet_symbols *read_symbols, struct alphabet_symbols *write_symbols, char movement) {
    if (read_symbols->symbol_count != write_symbols->symbol_count) {
        printf("Number of read symbols are not equal the number of write symbols!\n");
        exit(-1);
    }
    
    if (program->deltas_count == 0) {
        program->deltas = malloc(read_symbols->symbol_count * sizeof(struct deltas*));
    } else {
        program->deltas = realloc(program->deltas, (program->deltas_count + read_symbols->symbol_count) * sizeof(struct deltas*));
    }

    for (int i = 0; i < read_symbols->symbol_count; ++i) {
        struct deltas *delta = malloc(sizeof(struct deltas));
        delta->read_symbol = read_symbols->symbols[i];
        delta->write_symbol = write_symbols->symbols[i];
        delta->state = generate_state_str(program, state, delta->read_symbol, delta->write_symbol);
        delta->subsequent_state = generate_state_str(program, subsequent_state, delta->read_symbol, delta->write_symbol);
        delta->movement = movement;
        program->deltas[program->deltas_count + i] = delta;
    }
    program->deltas_count += read_symbols->symbol_count;
}

void generate_1tn_deltas(struct program *program, struct state_helper *state, struct state_helper *subsequent_state, struct alphabet_symbols *read_symbols, struct alphabet_symbols *write_symbols, char movement) {
    int new_deltas = read_symbols->symbol_count * write_symbols->symbol_count;
    if (program->deltas_count == 0) {
        program->deltas = malloc(new_deltas * sizeof(struct deltas*));
    } else {
        program->deltas = realloc(program->deltas, (program->deltas_count + new_deltas) * sizeof(struct deltas*));
    }

    for (int i = 0; i < read_symbols->symbol_count; ++i) {
        for (int j = 0; j < write_symbols->symbol_count; ++j) {
            struct deltas *delta = malloc(sizeof(struct deltas));
            delta->read_symbol = read_symbols->symbols[i];
            delta->write_symbol = write_symbols->symbols[j];
            delta->state = generate_state_str(program, state, delta->read_symbol, delta->write_symbol);
            delta->subsequent_state = generate_state_str(program, subsequent_state, delta->read_symbol, delta->write_symbol);
            delta->movement = movement;
            program->deltas[program->deltas_count + (i * write_symbols->symbol_count) + j] = delta;
        }
    }
    program->deltas_count += new_deltas;
}

void generate_1_delta(struct program *program, struct state_helper *state, struct state_helper *subsequent_state, struct alphabet_symbols *read_symbols, struct alphabet_symbols *write_symbols, char movement) {
    struct deltas *delta = malloc(sizeof(struct deltas));
    delta->read_symbol = *read_symbols->symbols;
    delta->write_symbol = *write_symbols->symbols;
    delta->state = generate_state_str(program, state, delta->read_symbol, delta->write_symbol);
    delta->subsequent_state = generate_state_str(program, subsequent_state, delta->read_symbol, delta->write_symbol);
    delta->movement = movement;

    if (program->deltas_count == 0) {
        program->deltas_count = 1;
        program->deltas = malloc(sizeof(struct deltas*));
        program->deltas[0] = delta;
    } else {
        ++program->deltas_count;
        program->deltas = realloc(program->deltas, program->deltas_count * sizeof(struct deltas*));
        program->deltas[program->deltas_count - 1] = delta;
    }
}

/*
 * Parse delta part of the program file. Add an array of delta structs to the program struct.
 */
void parse_deltas(struct program *program, FILE *file_ptr, int line_count) {
    size_t line_length;
    size_t buffer_size = 0;
    char *line;
    char *tmp_line;
    char *state_str;
    char *subsequent_state_str;
    char *read_symbol_str;
    char *write_symbol_str;
    char movement;
    program->deltas_count = 0;
    program->deltas = NULL;

    for (int i = 0; i < line_count; ++i) {
        line_length = getline(&line, &buffer_size, file_ptr);
        tmp_line = strdup(line);

        // check formatting and size
        if (line_length < 12 || tmp_line[0] != 'D'){
            printf("Delta has wrong formatting!");
            exit(-1);
        }

        // skip "D: "
        tmp_line += 3;

        state_str = strsep(&tmp_line, ",");
        read_symbol_str = strsep(&tmp_line, ",");
        subsequent_state_str = strsep(&tmp_line, ",");
        write_symbol_str = strsep(&tmp_line, ",");
        movement = *tmp_line;

        struct alphabet_symbols *read_symbols = get_alphabet_symbols(program, read_symbol_str);
        struct alphabet_symbols *write_symbols = get_alphabet_symbols(program, write_symbol_str);
        struct state_helper *state = get_state_helper(state_str);
        struct state_helper *subsequent_state = get_state_helper(subsequent_state_str);

        if (read_symbols->type != write_symbols->type) {
            printf("Delta is malformed, read/write symbol makros don't match!\n");
            exit(-1);
        }

        switch (read_symbols->type) {
            case '0':
                generate_1_delta(program, state, subsequent_state, read_symbols, write_symbols, movement);
                break;
            case '1':
                generate_1t1_deltas(program, state, subsequent_state, read_symbols, write_symbols, movement);
                break;
            case 'n':
                generate_1tn_deltas(program, state, subsequent_state, read_symbols, write_symbols, movement);
                break;
        }

    }
}

/*
 * Parse the file containing the TM-Program and produce a struct containing its information.
 */
struct program *parse_program(char *program_file_path) {
    FILE *file_ptr = fopen(program_file_path, "r");
    size_t line_length;
    char *line = NULL;
    size_t buffer_size = 0;
    struct program *program = malloc(sizeof(struct program));

    // check that file is found
    if(file_ptr == NULL) {
        printf("File %s not found!", program_file_path);
        exit(1);
    }

    //count lines and reset to beginning of file
    int line_count = get_file_line_count(file_ptr);

    // check if enough lines are contained, line for states, alphabet and at least one transition has to be present
    if(line_count < 3) {
        printf("File does not contain enough lines!");
        exit(1);
    }

    // parse states from next line
    line_length = getline(&line, &buffer_size, file_ptr);
    parse_states(program, line, line_length);
    --line_count;

    // parse alphabet from next line
    line_length = getline(&line, &buffer_size, file_ptr);
    parse_alphabet(program, line, line_length);
    --line_count;

    // get all deltas/transitions of the program
    parse_deltas(program, file_ptr, line_count);

    if (line)
        free(line);
    fclose(file_ptr);

    return program;
}

void write_compiled_program(struct program *program, char *filename) {
    FILE *file_ptr = fopen(filename, "w");

    // write states
    fprintf(file_ptr, "S: ");
    for (int i = 0; i < program->state_count - 1; ++i) {
        fprintf(file_ptr, "%s,", program->state_names[i]);
    }
    fprintf(file_ptr, "%s\n", program->state_names[program->state_count - 1]);

    //write alphabet
    fprintf(file_ptr, "G: ");
    for (int i = 0; i < program->alphabet_size - 1; ++i) {
        fprintf(file_ptr, "%s,", program->alphabet[i]);
    }
    fprintf(file_ptr, "%s\n", program->alphabet[program->alphabet_size - 1]);

    for (int i = 0; i < program->deltas_count; ++i) {
        struct deltas *delta = program->deltas[i];
        fprintf(file_ptr, "D: %s,%s,%s,%s,%c\n", delta->state, program->alphabet[delta->read_symbol], delta->subsequent_state, program->alphabet[delta->write_symbol], delta->movement);
    }
}