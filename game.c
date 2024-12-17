//
// Created by 竹彦博 on 2024/12/9.
//
#include "game.h"
#include <string.h>

// 定义方向移动函数
void up(int p[], int step) {
    p[0] -= step;
}

void right(int p[], int step) {
    p[1] += step;
}

void upper_right(int p[], int step) {
    p[0] -= step;
    p[1] += step;
}

void lower_right(int p[], int step) {
    p[0] += step;
    p[1] += step;
}


void Game_init(Game *game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            game->board[i][j] = 0;
        }
    }
    game->current_player = 1;
    game->record_count = 0;
}

int Game_to_move(Game *game) {
    return game->current_player;
}

void Game_display_board(Game *game, int last_x, int last_y, int have_last) {
    for (int x = BOARD_SIZE - 1; x >= 0; x--) {
        printf("%2d ", x + 1);
        for (int y = 0; y < BOARD_SIZE; y++) {
            int val = game->board[x][y];
            if (have_last && x == last_x && y == last_y) {
                val += 2;
            }
            printf("%2d ", val);
        }
        printf("\n");
    }
    printf("   ");
    for (int i = 1; i <= BOARD_SIZE; i++) {
        printf("%2d ", i);
    }
    printf("\n\n");
}

int Game_play(Game *game, int line, int col) {
    if (game->board[line][col] != 0) {
        return 0;
    }
    game->board[line][col] = game->current_player;
    game->current_player = 3 - game->current_player;
    game->record_chess[game->record_count][0] = line;
    game->record_chess[game->record_count][1] = col;
    game->record_count++;
    return 1;
}

int Game_check_winner(int state[BOARD_SIZE][BOARD_SIZE]) {
    // if (Game_check_ban(state, 1, -1, -1) != -1) {
    //     return 2;
    // }
    int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int d = 0; d < 4; d++) {
                char line[32];
                int idx = 0;
                for (int step = -4; step <= 4; step++) {
                    int nx = x + step * directions[d][0];
                    int ny = y + step * directions[d][1];
                    if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                        char c = state[nx][ny] + '0';
                        line[idx++] = c;
                    }
                }
                line[idx] = '\0';
                if (strstr(line, "11111") != NULL) return 1;
                if (strstr(line, "22222") != NULL) return 2;
            }
        }
    }

    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (state[i][j] == 0) return -1;

    return 0;
}

int Game_is_cutoff(Game *game, int state[BOARD_SIZE][BOARD_SIZE], int depth) {
    int w = Game_check_winner(state);
    return (w != -1 || depth <= 0);
}

void Game_actions(int state[BOARD_SIZE][BOARD_SIZE], Actions *act) {
    act->count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (state[i][j] == 0) {
                act->x[act->count] = i;
                act->y[act->count] = j;
                act->count++;
            }
        }
    }
}

void Game_result(int old_state[BOARD_SIZE][BOARD_SIZE], int new_state[BOARD_SIZE][BOARD_SIZE], int x, int y,
                 int player) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            new_state[i][j] = old_state[i][j];
        }
    }
    new_state[x][y] = player;
}

void Game_neighbors(int state[BOARD_SIZE][BOARD_SIZE], Actions *nbr) {
    nbr->count = 0;
    int directions[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
    int mark[BOARD_SIZE][BOARD_SIZE];
    memset(mark, 0, sizeof(mark));
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (state[x][y] == 0) {
                int add = 0;
                for (int d = 0; d < 8; d++) {
                    for (int r = 1; r <= 2; r++) {
                        int nx = x + r * directions[d][0];
                        int ny = y + r * directions[d][1];
                        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && state[nx][ny] != 0) {
                            add = 1;
                            break;
                        }
                    }
                    if (add) break;
                }
                if (add && !mark[x][y]) {
                    mark[x][y] = 1;
                }
            }
        }
    }
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (mark[i][j]) {
                nbr->x[nbr->count] = i;
                nbr->y[nbr->count] = j;
                nbr->count++;
            }
        }
    }
}


// 定义pos数组
static void (*pos[4])(int [], int) = {up, right, upper_right, lower_right};

int Game_check_ban(int state[BOARD_SIZE][BOARD_SIZE], int player, int last_x, int last_y) {
    // 禁手只对黑棋判断
    if (player != 1) {
        return -1;
    }
    int board[15][15];
    // 将state复制到board中
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            board[i][j] = state[i][j];
        }
    }
    int ban = -1;
    int last_pos[2];
    last_pos[0] = last_x;
    last_pos[1] = last_y;
    ban = check_ban(board, last_pos, player);
    return ban; // 返回1表示黑禁手，2表示白禁手，-1表示无禁手
}


typedef void (*FuncPtr)(int [], int);

int pattern3_1[6] = {0, 1, 1, 1, 0, 0};
int pattern3_2[6] = {0, 0, 1, 1, 1, 0};
int pattern3_3[6] = {0, 1, 0, 1, 1, 0};
int pattern3_4[6] = {0, 1, 1, 0, 1, 0};
int non_pattern3_5[8] = {2, 0, 1, 1, 1, 0, 0, 1};
int non_pattern3_6[8] = {1, 0, 0, 1, 1, 1, 0, 2};
int pattern4_1[6] = {0, 1, 1, 1, 1, 0};
int pattern4_2[5] = {1, 1, 1, 0, 1};
int pattern4_3[5] = {1, 1, 0, 1, 1};
int pattern4_4[5] = {1, 0, 1, 1, 1};
int single_pattern4_5[7] = {1, 0, 1, 1, 1, 0, 1};
int single_pattern4_6[8] = {1, 0, 1, 1, 1, 1, 0, 1};
int single_pattern4_7[9] = {1, 1, 1, 0, 1, 0, 1, 1, 1};
int single_pattern4_8[8] = {1, 1, 0, 1, 1, 0, 1, 1};


int double_three(int board[15][15], int last_pos[2]);

int double_three(int board[15][15], int last_pos[2]) {
    int counter = 0;
    for (int i = 0; i < 4; i++) {
        int series[11]; // 以落子处为中心，向正负方向分别搜索4个步长
        for (int j = 0; j < 11; j++) {
            series[j] = 0;
        }
        series[5] = 1;
        int search_pos[] = {last_pos[0], last_pos[1]};
        for (int j = 1; j <= 5; j++) {
            pos[i](search_pos, 1);
            if (search_pos[0] > 14 || search_pos[1] > 14)
                break;
            series[5 + j] = board[search_pos[0]][search_pos[1]] == 3 ? 1 : board[search_pos[0]][search_pos[1]];
        }
        search_pos[0] = last_pos[0];
        search_pos[1] = last_pos[1];
        for (int j = 1; j <= 5; j++) {
            pos[i](search_pos, -1);
            if (search_pos[0] < 0 || search_pos[1] < 0)
                break;
            series[5 - j] = board[search_pos[0]][search_pos[1]] == 3 ? 1 : board[search_pos[0]][search_pos[1]];
        }
        // 如果包含活三型但不包含活四型，计数+1
        if (find(pattern3_1, series, 6, 11) ||
            find(pattern3_2, series, 6, 11) ||
            find(pattern3_3, series, 6, 11) ||
            find(pattern3_4, series, 6, 11) &&
            !find(pattern4_1, series, 6, 11) &&
            !find(pattern4_2, series, 5, 11) &&
            !find(pattern4_3, series, 5, 11) &&
            !find(pattern4_4, series, 5, 11) &&
            !find(non_pattern3_5, series, 8, 11) &&
            !find(non_pattern3_6, series, 8, 11))
            counter++;
        if (counter == 2) {
            // printf("黑双三禁手\n");
            return 1;
        }
    }
    return 0;
}


int double_four(int board[15][15], int last_pos[2]);

int double_four(int board[15][15], int last_pos[2]) {
    int counter = 0;
    for (int i = 0; i < 4; i++) {
        int series[9]; // 以落子处为中心，向正负方向分别搜索3个步长
        for (int j = 0; j < 9; j++) {
            series[j] = 0;
        }
        series[4] = 1;
        int search_pos[] = {last_pos[0], last_pos[1]};
        for (int j = 1; j <= 4; j++) {
            pos[i](search_pos, 1);
            if (search_pos[0] > 14 || search_pos[1] > 14)
                break;
            series[4 + j] = board[search_pos[0]][search_pos[1]] == 3 ? 1 : board[search_pos[0]][search_pos[1]];
        }
        search_pos[0] = last_pos[0];
        search_pos[1] = last_pos[1];
        for (int j = 1; j <= 4; j++) {
            pos[i](search_pos, -1);
            if (search_pos[0] < 0 || search_pos[1] < 0)
                break;
            series[4 - j] = board[search_pos[0]][search_pos[1]] == 3 ? 1 : board[search_pos[0]][search_pos[1]];
        }
        if (find(pattern4_1, series, 6, 9) ||
            find(pattern4_2, series, 5, 9) ||
            find(pattern4_3, series, 5, 9) ||
            find(pattern4_4, series, 5, 9))
            counter++;
        if (find(single_pattern4_5, series, 7, 9) ||
            find(single_pattern4_6, series, 8, 9) ||
            find(single_pattern4_7, series, 9, 9) ||
            find(single_pattern4_8, series, 8, 9))
            return 1;
        if (counter == 2) {
            // printf("黑双四禁手\n");
            return 1;
        }
    }
    return 0;
}

int check_ban(int board[15][15], int last_pos[], int player) {
    /*
     *判断禁手
     *Args:
     *  board(int [][]): 数字棋盘
     *  last_pos(int [x, y]): 上次落子的坐标x, y
     *  player({1: black, 2: white}): 正在落子的玩家
     *Return:
     *  is_ban(int): 0 / 1
     */

    if (player != 1)
        return 0;
    int start[] = {last_pos[0], last_pos[1]};
    if (double_three(board, start))
        return 1;
    if (double_four(board, start))
        return 1;
    return -1;
}


int find(int pattern[], int list[], int pattern_len, int list_len) {
    for (int start = 0; start <= list_len - pattern_len; start++) {
        int result = 0;
        for (int i = 0; i < pattern_len; i++) {
            if (list[start + i] == pattern[i])
                result++;
        }
        if (result == pattern_len)
            return 1;
    }
    return 0;
}
