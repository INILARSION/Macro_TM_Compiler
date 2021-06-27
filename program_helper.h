#pragma once

/*
 * Struct contains one delta/transition of the program
 */
struct deltas {
    // index of state name in state_names in program struct
    char *state;
    // index of subsequent state name in state_names in program struct
    char *subsequent_state;
    // index of read alphabet element in alphabet in program struct
    int read_symbol;
    // index of write alphabet element in alphabet in program struct
    int write_symbol;
    // Move direction of head. Can be '<', '>' and '-'.
    char movement;
};

/*
 * Struct contains all information of the TM-Program
 */
struct program {
    // Array of all state names.
    char **state_names;
    // Number of different states.
    int state_count;
    // Array of all elements of the alphabet. Elements are strings.
    char **alphabet;
    // Array of alphabet indexes (from 0,1,...,alphabet_size)
    // Helper for * Wildcards
    int *alphabet_indexes;
    // Number of different alphabet elements
    int alphabet_size;
    // Array of all deltas/transitions of the Program
    struct deltas **deltas;
    //Number of all deltas/transitions
    int deltas_count;
};

struct alphabet_symbols {
    // how many symbols
    int symbol_count;
    // list of symbols
    int *symbols;
    // type of read/write symbol matching
    //possible types are '1' for 1 to 1 mapping and 'n' for 1 to n mapping or '0' if it is just one symbol
    char type;
};

struct state_helper {
    // this flag shows if and which symbols should be substituted in the state name between prefix and postfix
    // possibilities: r: read symbol, w: write symbol, 0: None (just use the prefix as a standalone state name)
    char substitution_type;
    char *state_prefix;
    char *state_postfix;
};