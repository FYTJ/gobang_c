//
// Created by 竹彦博 on 2024/12/9.
//

#ifndef AI_H
#define AI_H

#include "game.h"
#include "patterns.h"


typedef struct {
    char* name;
    int color;
    int count;
    int maxv;
    int minv;
    int extension_threshold;
    double p_threshold;
    int lmr_threshold;
    int lmr_min_depth;

    PatternEntry patterns[PATTERN_COUNT];
    int pattern_weights[PATTERN_COUNT];
    int openings_count;
    int openings[1000][10];
} AI;

typedef struct {
    int x;
    int y;
    long long score;
} Data;

void AI_init(AI* ai, const char* name, int color);
void AI_read_openings(AI* ai, const char* filename);
int AI_heuristic_alpha_beta_search(AI* ai, Game* game, int depth, int is_first_move);
void ai_play(Game* game, AI* ai, int first_move, int *x, int *y);
int AI_look_up_table(AI* ai, int record_chess[225][2], int steps);

int compare(const void *a, const void *b);
int compare_reverse(const void *a, const void *b);
void human_play(Game* game, int *x, int *y);

#endif