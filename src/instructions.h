#ifndef INSTRUCTIONS_H_
#define INSTRUCTIONS_H_

enum instructions {
    PUSH       = 0,
    PUSH_TRUE  = 1,
    PUSH_FALSE = 2,
    POP        = 3,
    ADD        = 4,
    MIN        = 5,
    MUL        = 6,
    DIV        = 7,
    NOT        = 8,
    DEQ        = 9,
    NEQ        = 10,
    GRE        = 11,
    GRQ        = 12,
    LES        = 13,
    LEQ        = 14,
    AND        = 15,
    OR         = 16,
    DUP        = 17,
    DUP_LOC    = 18,
    PUSH_ADDR  = 19,
    CHANGE     = 20,
    PUSH_BASE  = 21,
    CHANGE_LOC = 22,
    JMP_NOT    = 23,
    JMP        = 24,
    CALL       = 25,
    RET        = 26,
    PUSH_NUM   = 27,
    GET_FIELD  = 28,
    SET_FIELD  = 29
};

#endif
