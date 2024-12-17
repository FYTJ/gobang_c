//
// Created by 竹彦博 on 2024/12/9.
//
#include "patterns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void compile_patterns(PatternEntry patterns[PATTERN_COUNT]) {
    const char* pattern_str[PATTERN_COUNT] = {
        "11111",
        "011110",
        "011112|211110",
        "011100|001110|011010|010110|11011|10111|11101",
        "001100|001010|010100",
        "000112|211000|001012|210100|010012|210010|10001",
        "000100|001000"
    };
    // int pattern_val[PATTERN_COUNT] = {50000,4320,720,720,120,50,20};

    for (int i=0;i<PATTERN_COUNT;i++){
        strcpy(patterns[i].pattern,pattern_str[i]);
        // patterns[i].value=pattern_val[i];
        if (regcomp(&patterns[i].regex, patterns[i].pattern, REG_EXTENDED)) {
            fprintf(stderr,"Regex compile error: %s\n", patterns[i].pattern);
            exit(1);
        }
    }
}

long long match_patterns(PatternEntry patterns[PATTERN_COUNT], const char* line, const int pattern_weights[PATTERN_COUNT]) {
    long long score=0;
    for (int i=0;i<PATTERN_COUNT;i++){
        const char* str=line;
        regmatch_t pmatch[1];
        while (regexec(&patterns[i].regex, str, 1, pmatch, 0)==0) {
            score += pattern_weights[i];
            str += pmatch[0].rm_eo;
            if (*str=='\0') break;
        }
    }
    return score;
}