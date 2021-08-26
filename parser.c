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
    program->alphabet_indexes = malloc(program->alphabet_size * sizeof(char*));

    // get the length of the individual alphabet symbols and put them then in a list
    line[line_length] = '\0';
    for (int current_element = 0; current_element < program->alphabet_size; ++current_element){
        // add symbol to the list
        program->alphabet[current_element] = strdup(strsep(&line, ","));
        program->alphabet_indexes[current_element] = current_element;
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
        printf("Program delta contains state or tape symbol which is not contained in the defined!");
        exit(-1);
    }
    return matched_index;
}

/*
 * Count all symbols in a line, which are seperated by a seperator char
 */
int get_element_count(char *line, char seperator) {
    int count = 1;
    while (*line != '\0') {
        if (*line == seperator)
            ++count;
        ++line;
    }
    return count;
}

/*
 * This function handles read and write makros with a '*' Wildcard.
 * If just '*' is used, insert all alphabet symbols.
 * If *-(...) is used, insert all alphabet symbols minus the listed ones.
 */
void handle_wildcard_symbol(struct program *program, struct alphabet_symbols *alphabet_symbols, char *symbols, int symbols_len) {
    if (symbols_len == 3) {
        alphabet_symbols->symbol_count = program->alphabet_size;
        alphabet_symbols->symbols = program->alphabet_indexes;
        return;
    }

    if (strncmp(symbols+1, "*-(", 3) != 0 || symbols[symbols_len - 2] != ')') {
        printf("Malformed Wildcard!");
        exit(-1);
    }

    // remove first parenthesis and "*-(", and last 2 parentheses
    symbols[symbols_len-2] = '\0';
    symbols += 4;
    symbols_len -= 6;
    
    int excluded_symbols_count = get_element_count(symbols, '|');
    int *excluded_symbols = malloc(excluded_symbols_count * sizeof(int));
    alphabet_symbols->symbol_count = program->alphabet_size - excluded_symbols_count;
    alphabet_symbols->symbols = malloc(alphabet_symbols->symbol_count * sizeof(int));

    for (int i = 0; i < excluded_symbols_count; ++i) {
        excluded_symbols[i] = search_matching_element(&symbols, program->alphabet, program->alphabet_size);
    }

    int is_excluded = 0;
    int index_counter = 0;
    for (int i = 0; i < program->alphabet_size; ++i) {
        // check if current alphabet symbol is in the excluded list and skip it
        for (int j = 0; j < excluded_symbols_count; ++j) {
            if (excluded_symbols[j] == i) {
                is_excluded = 1;
                break;
            }
        }
        if (is_excluded == 0)
            alphabet_symbols->symbols[index_counter++] = i;
        else
            is_excluded = 0;
    }

    free(excluded_symbols);
}

/*
 * Create a alphabet symbol helper struct.
 * The symbols contain a read/write symbol or a listing of those.
 * The helper struct contains how many and which symbols are used and if they should be matched 1 to 1 or 1 to n or None
 */
struct alphabet_symbols *get_alphabet_symbols(struct program *program, char *symbols) {
    struct alphabet_symbols *alphabet_symbols = malloc(sizeof(struct alphabet_symbols));
    int symbols_len = strlen(symbols);

     if (symbols[0] == '[') {
        if (symbols[symbols_len - 1] != ']') {
            printf("Malformed delta, ] ist missing!\n");
            printf("Line: %s\n", symbols);
            exit(-1);
        }

        alphabet_symbols->type = '1';

        // check if "[*] wildcard is used, then use all alphabet symbols
        if (strncmp(symbols, "[*", 2) == 0) {
            handle_wildcard_symbol(program, alphabet_symbols, symbols, symbols_len);
            return alphabet_symbols;
        }

        // remove '[' and ']' from the string to parse the different alphabet symbols later
        symbols[symbols_len - 1] = '\0';
        ++symbols;
    } else if (symbols[0] == '{') {
        if (symbols[symbols_len - 1] != '}') {
            printf("Malformed delta, } is missing!");
            exit(-1);
        }

        alphabet_symbols->type = 'n';

        // check if "{*} wildcard is used, then use all alphabet symbols
         if (strncmp(symbols, "{*", 2) == 0) {
             handle_wildcard_symbol(program, alphabet_symbols, symbols, symbols_len);
             return alphabet_symbols;
         }

        // remove '{' and '}' from the string to parse later
        symbols[symbols_len - 1] = '\0';
        ++symbols;
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

/*
 * Create a state name helper struct.
 * Since a state name can have a part substituted by a read or write symbol it needs to be parsed.
 * The Struct contains the prefix, postfix and if a read or write symbol or no symbol should be placed between pre/postfix.
 */
struct state_helper *get_state_helper(char* state) {
    struct state_helper *state_helper = malloc(sizeof(struct state_helper));
    int state_len = strlen(state);
    for (int i = 0; i < state_len; ++i) {
        // check if a read/write symbol should be substituted
        if (state[i] == '(' && state[i + 1] == '*' && (state[i + 2] == 'r' || state[i + 2] == 'w')) {
            state_helper->substitution_type = state[i + 2];
            state_helper->state_prefix = strndup(state, i);
            state_helper->state_postfix = strndup(state + i + 4, state_len - ( i + 3));
            return state_helper;
        }
    }
    // if no substitution, just use the whole state name as prefix
    state_helper->substitution_type = '0';
    state_helper->state_prefix = state;
    return state_helper;
}

/*
 * This function checks if a state name is contained in the state name list and adds it if not.
 */
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

/*
 * This function creates the state name with a state helper struct and the read/write symbols.
 */
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

/*
 * This function generates deltas which have makros for read and write symbols
 * Example: read symbols: (a|b|c) write symbols: (x|y|) will generate deltas with: (a,x), (b,y), (c,z) as (read, write) symbols
 * number of read and write symbols have to be the same
 */
void generate_1t1_deltas(struct program *program, struct state_helper *state, struct state_helper *subsequent_state, struct alphabet_symbols *read_symbols, struct alphabet_symbols *write_symbols, char movement) {
    if (read_symbols->symbol_count != write_symbols->symbol_count) {
        printf("Number of read symbols are not equal the number of write symbols!\n");
        printf("Read symbols: %d, write symbols: %d\n", read_symbols->symbol_count, write_symbols->symbol_count);
        exit(-1);
    }

    // If array empty create new one, else reallocate more memory
    if (program->deltas_count == 0) {
        program->deltas = malloc(read_symbols->symbol_count * sizeof(struct deltas*));
    } else {
        program->deltas = realloc(program->deltas, (program->deltas_count + read_symbols->symbol_count) * sizeof(struct deltas*));
    }

    // create the different deltas and save them in the program struct
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

/*
 * This function generates deltas which have makros for read and write symbols
 * Example: read symbols: (a|b|c) write symbols: (x|y) will generate deltas with: (a,x) (a,y), (b,x), (b,y), (c,x), (c,y) as (read, write) symbols
 */
void generate_1tn_deltas(struct program *program, struct state_helper *state, struct state_helper *subsequent_state, struct alphabet_symbols *read_symbols, struct alphabet_symbols *write_symbols, char movement) {
    int new_deltas = read_symbols->symbol_count * write_symbols->symbol_count;

    // If array empty create new one, else reallocate more memory
    if (program->deltas_count == 0) {
        program->deltas = malloc(new_deltas * sizeof(struct deltas*));
    } else {
        program->deltas = realloc(program->deltas, (program->deltas_count + new_deltas) * sizeof(struct deltas*));
    }

    // create the different deltas and save them in the program struct
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

/*
 * This function generates deltas which have no makros for read and write symbols
 */
void generate_1_delta(struct program *program, struct state_helper *state, struct state_helper *subsequent_state, struct alphabet_symbols *read_symbols, struct alphabet_symbols *write_symbols, char movement) {
    struct deltas *delta = malloc(sizeof(struct deltas));
    // create the different delta and save it in the program struct
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

        // create the deltas
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

/*
 * Write the compiled program to a file.
 */
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

    // write deltas
    for (int i = 0; i < program->deltas_count; ++i) {
        struct deltas *delta = program->deltas[i];
        fprintf(file_ptr, "D: %s,%s,%s,%s,%c\n", delta->state, program->alphabet[delta->read_symbol], delta->subsequent_state, program->alphabet[delta->write_symbol], delta->movement);
    }
}