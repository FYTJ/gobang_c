//
// Created by 竹彦博 on 2024/12/9.
//

#ifndef GAME_H
#define GAME_H

#include <stdio.h>

#define BOARD_SIZE 15

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player;
    int record_count;
    int record_chess[225][2];
} Game;

typedef struct {
    int x[BOARD_SIZE*BOARD_SIZE];
    int y[BOARD_SIZE*BOARD_SIZE];
    int count;
} Actions;

void Game_init(Game* game);
int Game_to_move(Game* game);
void Game_display_board(Game* game, int last_x, int last_y, int have_last);
int Game_play(Game* game, int line, int col);
int Game_check_winner(int state[BOARD_SIZE][BOARD_SIZE]);
int Game_is_cutoff(Game* game, int state[BOARD_SIZE][BOARD_SIZE], int depth);
void Game_actions(int state[BOARD_SIZE][BOARD_SIZE], Actions* act);
void Game_result(int old_state[BOARD_SIZE][BOARD_SIZE], int new_state[BOARD_SIZE][BOARD_SIZE], int x, int y, int player);
void Game_neighbors(int state[BOARD_SIZE][BOARD_SIZE], Actions* nbr);
int Game_check_ban(int state[BOARD_SIZE][BOARD_SIZE], int player, int last_x, int last_y);
void up(int [], int);
void right(int [], int);
void upper_right(int [], int);
void lower_right(int [], int);
int find(int pattern[], int list[], int pattern_len, int list_len);
int check_ban(int board[15][15], int last_pos[2], int player);

#endif