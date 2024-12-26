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


typedef struct {
    long long v;
    int x;
    int y;
} Parallel_returns;


typedef struct {
    int *task_index;
    AI* ai;
    const Game* game;
    int **nstate_arg;
    int player;
    double alpha;
    double beta;
    int depth;
    int x;
    int y;
    Parallel_returns* results;
    void* result_mutex;
} Parallel_args;


void AI_init(AI* ai, const char* name, int color);
void AI_read_openings(AI* ai, const char* filename);
int AI_look_up_table(AI* ai, const int record_chess[225][2], int steps);
int AI_heuristic_alpha_beta_search(AI* ai, const Game* game, int depth, int is_first_move);
static long long AI_eval_pos(AI* ai, int state[BOARD_SIZE][BOARD_SIZE], int player, int x, int y, int direction[2]);
static void AI_get_line(int state[BOARD_SIZE][BOARD_SIZE], int player,int x,int y,int dx,int dy,char* line);
static void* AI_parallel_max_value(void* arg);
static long long AI_min_value(AI* ai, const Game* game, int state[BOARD_SIZE][BOARD_SIZE], int player, double alpha, double beta,int depth);
static long long AI_max_value(AI* ai, const Game* game, int state[BOARD_SIZE][BOARD_SIZE], int player, double alpha, double beta,int depth);
static long long AI_heuristic_eval(AI* ai, const Game* game, int state[BOARD_SIZE][BOARD_SIZE], int player, int last_x, int last_y, int have_last, long long prev_eval, int prev_state[BOARD_SIZE][BOARD_SIZE]);

int compare(const void *a, const void *b);
int compare_reverse(const void *a, const void *b);
void human_play(Game* game, int *x, int *y);
void ai_play(Game* game, AI* ai, int first_move, int *x, int *y);
#endif
