//
// Created by 竹彦博 on 2024/12/9.
//

#ifndef PATTERNS_H
#define PATTERNS_H

#include <regex.h>

typedef struct {
    char pattern[128];
    // 2024/12/10 11:11 int value;
    regex_t regex;
} PatternEntry;

#define PATTERN_COUNT 7

void compile_patterns(PatternEntry patterns[PATTERN_COUNT]);
long long match_patterns(PatternEntry patterns[PATTERN_COUNT], const char* line, const int pattern_weights[PATTERN_COUNT]);

#endif