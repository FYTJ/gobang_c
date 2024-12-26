//
// Created by 竹彦博 on 2024/12/9.
//
#include "ai.h"

#include <limits.h>

#include "patterns.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include </usr/local/Cellar/libomp/19.1.6/include/omp.h>
#include <sys/time.h>


void AI_init(AI *ai, const char *name, int color) {
    ai->name = strdup(name);
    ai->color = color;
    ai->extension_threshold = 2000;
    ai->lmr_threshold = 10;
    ai->lmr_min_depth = 0;
    compile_patterns(ai->patterns);
    ai->openings_count = 0;
    int pattern_values[PATTERN_COUNT] = {50000, 4320, 720, 720, 120, 50, 20};
    for (int i = 0; i < PATTERN_COUNT; i++) {
        ai->pattern_weights[i] = pattern_values[i];
    }
}


void AI_read_openings(AI *ai, const char *filename) {
    /*
     * 从文件读取开局表
     */
    FILE *f = fopen(filename, "r");
    if (!f) {
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char tmp[256];
        int len = strlen(line);
        int idx = 0;
        for (int i = 0; i < len; i++) {
            char c = line[i];
            if (c != '[' && c != ']' && c != '(' && c != ')' && c != '\n' && c != ' ') {
                tmp[idx++] = c;
            }
        }
        tmp[idx] = '\0';
        int vals[10];
        int v_count = 0;
        char *p = strtok(tmp, ",");
        while (p && v_count < 10) {
            vals[v_count++] = atoi(p);
            p = strtok(NULL, ",");
        }
        if (v_count >= 10) {
            for (int i = 0; i < 10; i++) {
                ai->openings[ai->openings_count][i] = vals[i];
            }
            ai->openings_count++;
        }
    }
    fclose(f);
}

int AI_look_up_table(AI *ai, const int record_chess[225][2], int steps) {
    /*
     * 查开局表
     * :param record_chess: ([(int, int)]) 棋盘
     * :return: (int, int) | -1下一步移动
     */
    for (int i = 0; i < ai->openings_count; i++) {
        int r = rand() % ai->openings_count;
        if (r != i) {
            int tmp[10];
            memcpy(tmp, ai->openings[i], sizeof(tmp));
            memcpy(ai->openings[i], ai->openings[r], sizeof(tmp));
            memcpy(ai->openings[r], tmp, sizeof(tmp));
        }
    }

    for (int k = 0; k < ai->openings_count; k++) {
        int match = 1;
        for (int s = 0; s < steps; s++) {
            if (ai->openings[k][2 * s] != record_chess[s][0] || ai->openings[k][2 * s + 1] != record_chess[s][1]) {
                match = 0;
                break;
            }
        }
        if (match && steps < 5) {
            int nx = ai->openings[k][2 * steps];
            int ny = ai->openings[k][2 * steps + 1];
            return (nx << 8) | ny;
        }
    }
    return -1;
}

int AI_heuristic_alpha_beta_search(AI *ai, const Game *game, int depth, int is_first_move) {
    /*
     * 极小化极大计算最优移动，启发式alpha-beta剪枝。
     * 结合后期移动缩减(LMR)。
     * :param game: (Game) 游戏
     * :param depth: (int) 搜索深度
     * :param is_first_move: (0 | 1) 是否是第一个子
     * :return: (int, int) 落子位置
     */
    int move_x = -1, move_y = -1;
    if (is_first_move) {
        move_x = 7;
        move_y = 7;
    } else {
        if (game->record_count <= 4) {
            int code = AI_look_up_table(ai, game->record_chess, game->record_count);
            if (code != -1) {
                move_x = (code >> 8) & 0xFF;
                move_y = code & 0xFF;
            }
        }
        if (move_x < 0) {
            int state[BOARD_SIZE][BOARD_SIZE];
            for (int i = 0; i < BOARD_SIZE; i++) {
                for (int j = 0; j < BOARD_SIZE; j++) {
                    state[i][j] = game->board[i][j];
                }
            }
            int res = AI_max_value(ai, game, state, ai->color, -1e9, 1e9, depth);
            move_x = (res >> 8) & 0xFF;
            move_y = res & 0xFF;
        }
    }
    return (move_x << 8) | move_y;
}

static void *AI_parallel_max_value(void *arg) {
    /*
     * 多线程辅助函数，功能为调用min_value()
     * :param arg: (Parallel_args*) 包含各参数的结构体指针
     * :return: NULL
     */
    Parallel_args *args = (Parallel_args *) arg;
    int *task_index = args->task_index;
    AI *ai = args->ai;
    const Game *game = args->game;
    int **nstate_arg = args->nstate_arg;
    int nstate[BOARD_SIZE][BOARD_SIZE];
    int player = args->player;
    double alpha = args->alpha;
    double beta = args->beta;
    int depth = args->depth;
    int x = args->x;
    int y = args->y;
    Parallel_returns *results = (Parallel_returns *) (args->results);
    void *mutex = args->result_mutex;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            nstate[i][j] = nstate_arg[i][j];
        }
    }
    long long res = AI_min_value(ai, game, nstate, player, alpha, beta, depth);
    pthread_mutex_lock(mutex);
    results[*task_index].v = res >> 16;
    results[*task_index].x = x;
    results[*task_index].y = y;
    pthread_mutex_unlock(mutex);
    free(task_index);
    for (int i = 0; i < BOARD_SIZE; i++) {
        free(nstate_arg[i]);
    }
    free(nstate_arg);
    free(args);
    return NULL;
}

int AI_compare(const void *a, const void *b) {
    // qsort辅助函数
    Data *data1 = (Data *) a;
    Data *data2 = (Data *) b;
    if (data1->score < data2->score) return 1;
    if (data1->score > data2->score) return -1;
    return 0;
}

int AI_compare_reverse(const void *a, const void *b) {
    // qsort辅助函数
    Data *data1 = (Data *) a;
    Data *data2 = (Data *) b;
    if (data1->score < data2->score) return -1;
    if (data1->score > data2->score) return 1;
    return 0;
}

static long long AI_max_value(AI *ai, const Game *game, int state[BOARD_SIZE][BOARD_SIZE], int player, double alpha,
                              double beta, int depth) {
    /*
     * 计算最大值的辅助函数
     * :param game: (Game) 游戏对象
     * :param state: (int[][]) 棋盘
     * :param player: (int) 当前玩家
     * :param alpha: (int) 当前最大下界
     * :param beta: (int) 当前最小上界
     * :param depth: (int) 搜索深度
     * :return: (int, int, int) (效用值, 最佳行动(x, y))
     * :var ai.lmr_threshold: (int) 后期移动缩减动作序数阈值，该值是depth的函数
     * :var ai.lmr_min_depth: (int) 在达到一定深度后进行LMR
     */
    long long current_eval = AI_heuristic_eval(ai, game, state, player, -1, -1, 0, 0,
                                               NULL);
    if (Game_is_cutoff(game, state, depth)) {
        long long val = current_eval;
        return (val << 16) | (0xFF << 8) | 0xFF;
    }

    // ai->lmr_threshold = 10;
    // ai->lmr_min_depth = 0;
    Actions act;
    Actions nbr;
    Game_actions(state, &act);
    Game_neighbors(state, &nbr);
    Data arr[act.count];
    int arr_count = 0;
    for (int i = 0; i < act.count; i++) {
        int nstate[BOARD_SIZE][BOARD_SIZE];
        Game_result(state, nstate, act.x[i], act.y[i], player);
        long long val = AI_heuristic_eval(ai, game, nstate, player, act.x[i], act.y[i], 1, current_eval, state);
        arr[arr_count].x = act.x[i];
        arr[arr_count].y = act.y[i];
        arr[arr_count].score = val;
        arr_count++;
    }
    qsort(arr, arr_count, sizeof(Data), AI_compare);
    long long best_val = LONG_LONG_MIN;
    int best_x = 255, best_y = 255;

    // 首次进入该函数时执行多线程
    if (depth == 5) {
        Parallel_returns results[10];
        pthread_t threads[10];
        pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

        for (int i = 0; i < arr_count; i++) {
            int *task_index = malloc(sizeof(int));
            *task_index = i;
            int x = arr[i].x;
            int y = arr[i].y;
            int nstate[15][15];
            Game_result(state, nstate, x, y, player);
            int **nstate_arg = malloc(BOARD_SIZE * sizeof(int *));
            for (int j = 0; j < BOARD_SIZE; j++) {
                nstate_arg[j] = malloc(BOARD_SIZE * sizeof(int));
            }
            for (int j = 0; j < BOARD_SIZE; j++) {
                for (int k = 0; k < BOARD_SIZE; k++) {
                    nstate_arg[j][k] = nstate[j][k];
                }
            }

            Parallel_args *myArgs = malloc(sizeof(Parallel_args));
            myArgs->task_index = task_index;
            myArgs->ai = ai;
            myArgs->game = game;
            myArgs->nstate_arg = nstate_arg;
            myArgs->player = player;
            myArgs->alpha = alpha;
            myArgs->beta = beta;
            myArgs->depth = depth - 1;
            myArgs->x = x;
            myArgs->y = y;
            myArgs->results = results;
            myArgs->result_mutex = &result_mutex;

            int in_nbr = 0;
            for (int k = 0; k < nbr.count; k++) {
                if (nbr.x[k] == x && nbr.y[k] == y) {
                    in_nbr = 1;
                    break;
                }
            }
            if (!in_nbr)
                continue;
            if (i >= ai->lmr_threshold && depth >= ai->lmr_min_depth)
                continue;

            pthread_create(&threads[i], NULL, AI_parallel_max_value, (void*)myArgs);
        }
        for (int i = 0; i < ai->lmr_threshold; i++) {
            pthread_join(threads[i], NULL);
        }
        pthread_mutex_destroy(&result_mutex);
        long long max_result = LONG_LONG_MIN;
        int max_index = -1;
        for (int i = 0; i < ai->lmr_threshold; i++) {
            if (results[i].v > max_result) {
                max_result = results[i].v;
                max_index = i;
            }
        }
        return (max_result << 16) | (results[max_index].x << 8) | results[max_index].y;
    }

    for (int i = 0; i < arr_count; i++) {
        int x = arr[i].x;
        int y = arr[i].y;
        int in_nbr = 0;
        for (int k = 0; k < nbr.count; k++) {
            if (nbr.x[k] == x && nbr.y[k] == y) {
                in_nbr = 1;
                break;
            }
        }
        if (!in_nbr)
            continue;
        if (i >= ai->lmr_threshold && depth >= ai->lmr_min_depth)
            continue;

        int nstate[BOARD_SIZE][BOARD_SIZE];
        Game_result(state, nstate, x, y, player);
        long long res = AI_min_value(ai, game, nstate, player, alpha, beta, depth - 1);
        long long v = res >> 16;
        if (v > best_val) {
            best_val = v;
            best_x = x;
            best_y = y;
        }
        if (v >= beta)
            return (v << 16) | (best_x << 8) | best_y;
        if (v > alpha)
            alpha = v;
    }
    return (best_val << 16) | (best_x << 8) | best_y;
}

static long long AI_min_value(AI *ai, const Game *game, int state[BOARD_SIZE][BOARD_SIZE], int player, double alpha,
                              double beta, int depth) {
    /*
     * 计算最小值的辅助函数
     * :param game: (Game) 游戏对象
     * :param state: (int[][]) 棋盘
     * :param player: (int) 当前玩家
     * :param alpha: (int) 当前最大下界
     * :param beta: (int) 当前最小上界
     * :param depth: (int) 搜索深度
     * :return: (int, int, int) (效用值, 最佳行动(x, y))
     * :var ai.lmr_threshold: (int) 后期移动缩减动作序数阈值，该值是depth的函数
     * :var ai.lmr_min_depth: (int) 在达到一定深度后进行LMR
     */
    long long current_eval = AI_heuristic_eval(ai, game, state, player, -1, -1, 0, 0, state);
    if (Game_is_cutoff(game, state, depth)) {
        long long val = current_eval;
        return (val << 16) | (0xFF << 8) | 0xFF;
    }
    // ai->lmr_threshold = 10;
    // ai->lmr_min_depth = 0;
    Actions act;
    Actions nbr;
    Game_actions(state, &act);
    Game_neighbors(state, &nbr);

    Data arr[act.count];
    int arr_count = 0;
    for (int i = 0; i < act.count; i++) {
        int nstate[BOARD_SIZE][BOARD_SIZE];
        Game_result(state, nstate, act.x[i], act.y[i], 3 - player);
        long long val = AI_heuristic_eval(ai, game, nstate, player, act.x[i], act.y[i], 1, current_eval, state);
        arr[arr_count].x = act.x[i];
        arr[arr_count].y = act.y[i];
        arr[arr_count].score = val;
        arr_count++;
    }
    qsort(arr, arr_count, sizeof(Data), AI_compare_reverse);
    long long best_val =LONG_LONG_MAX;
    int best_x = 255, best_y = 255;
    for (int i = 0; i < arr_count; i++) {
        int x = arr[i].x;
        int y = arr[i].y;
        int in_nbr = 0;
        for (int k = 0; k < nbr.count; k++) {
            if (nbr.x[k] == x && nbr.y[k] == y) {
                in_nbr = 1;
                break;
            }
        }
        if (!in_nbr)
            continue;
        if (i >= ai->lmr_threshold && depth >= ai->lmr_min_depth)
            continue;

        int nstate[BOARD_SIZE][BOARD_SIZE];
        Game_result(state, nstate, x, y, 3 - player);
        long long res = AI_max_value(ai, game, nstate, player, alpha, beta, depth - 1);
        long long v = res >> 16;
        if (v < best_val) {
            best_val = v;
            best_x = x;
            best_y = y;
        }
        if (v <= alpha)
            return (v << 16) | (best_x << 8) | best_y;
        if (v < beta)
            beta = v;
    }
    return (best_val << 16) | (best_x << 8) | best_y;
}

static long long AI_heuristic_eval(AI *ai, const Game *game, int state[BOARD_SIZE][BOARD_SIZE], int player, int last_x,
                                   int last_y, int have_last, long long prev_eval,
                                   int prev_state[BOARD_SIZE][BOARD_SIZE]) {
    /*
     * 计算当前状态对指定玩家的评价
     * :param game: (Game) 游戏对象
     * :param state: (np.ndarray) 棋盘
     * :param player: (int) 当前玩家
     * :param last_mov: (int, int) 上一次落子位置，只需要评估该位置附近的区域。
     * :param prev_eval: (int) 上一次的评估得分。如果为None，则为第一次评估，需要对整个棋盘进行评估。
     * :param prev_state: (np.ndarray) 未在last_mov处落子时的棋盘，默认值为None，用于增量更新
     * :return: (int) 评估得分
     * :note: 增量更新条件：last_mov, prev_eval, prev_state 同时有值
     * :note: 增量更新逻辑如下
     *     current_eval = prev_eval + target_eval + delta_eval - delta_opponent_eval
     *     delta_eval, delta_opponent 初始化为0
     *     target_eval针对last_mov处的子进行评价
     *     确定last_mov的作用域为：以last_mov为中心四个方向range(-4, 5){0}的棋盘点
     *     对作用域上的一个方向direction上的一个子:
     *     delta_target = _eval_pos(state, player, target_pos, direction)
     *     - _eval_pos(prev_state, player, target_pos, direction)
     *     若为player，则delta_eval += delta_target，3 - player亦然
     */
    if (have_last && Game_check_ban(state, player, last_x, last_y) != -1)
        return -100000000;

    if (have_last) {
        int all_directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {-1, 1}};
        long long target_eval = 0, delta_eval = 0, delta_opponent_eval = 0;
        for (int i = 0; i < 4; i++) {
            int *direction = all_directions[i];
            target_eval += AI_eval_pos(ai, state, player, last_x, last_y, direction);
        }
        for (int i = 0; i < 4; i++) {
            int *direction = all_directions[i];
            for (int step = -4; step < 5; step++) {
                if (step == 0)
                    continue;
                int x = last_x + step * direction[0];
                int y = last_y + step * direction[1];
                if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
                    if (state[x][y] == player) {
                        long long current_target_eval = AI_eval_pos(ai, state, player, x, y, direction);
                        long long prev_target_eval = AI_eval_pos(ai, prev_state, player, x, y, direction);
                        delta_eval += current_target_eval - prev_target_eval;
                    } else if (state[x][y] == 3 - player) {
                        long long current_target_eval = AI_eval_pos(ai, state, 3 - player, x, y, direction);
                        long long prev_target_eval = AI_eval_pos(ai, prev_state, 3 - player, x, y, direction);
                        delta_opponent_eval += current_target_eval - prev_target_eval;
                    }
                }
            }
        }
        return floor(prev_eval + target_eval + delta_eval - delta_opponent_eval * 1.2);
    }
    long long total_eval = 0;
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {-1, 1}};
            for (int i = 0; i < 4; i++) {
                int *direction = directions[i];
                int c = state[x][y];
                if (c == player) {
                    total_eval += AI_eval_pos(ai, state, player, x, y, direction);
                } else if (c == (3 - player)) {
                    total_eval -= floor(AI_eval_pos(ai, state, 3 - player, x, y, direction) * 1.2);
                }
            }
        }
    }
    return total_eval;
}

static long long AI_eval_pos(AI *ai, int state[BOARD_SIZE][BOARD_SIZE], int player, int x, int y, int direction[2]) {
    /*
     * 对指定玩家和指定位置的棋子计算评价
     * :param state: (int[][]) 棋盘
     * :param player: (int) 玩家
     * :param pos: (int, int) 目标评价位置
     * :param direction: (int, int) 方向
     */
    long long total_eval = 0;
    char line[64];
    AI_get_line(state, player, x, y, direction[0], direction[1], line);
    total_eval += match_patterns(ai->patterns, line, ai->pattern_weights);
    return total_eval;
}

static void AI_get_line(int state[BOARD_SIZE][BOARD_SIZE], int player, int x, int y, int dx, int dy, char *line) {
    /*
     * 获取一个方向上的棋盘
     * :param state: (int[][]) 棋盘
     * :param player: (int) 玩家
     * :param x: (int) 当前棋子的x坐标
     * :param y: (int) 当前棋子的y坐标
     * :param dx: (int) x的变化步长
     * :param dy: (int) y的变化步长
     * :return: (char*) dx, dy方向上的棋盘
     */
    int idx = 0;
    for (int step = -4; step <= 4; step++) {
        int nx = x + step * dx;
        int ny = y + step * dy;
        char c = '2';
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
            int val = state[nx][ny];
            if (val == player) c = '1';
            else if (val == 0) c = '0';
            else c = '2';
        }
        line[idx++] = c;
    }
    line[idx] = '\0';
}


void ai_play(Game *game, AI *ai, int first_move, int *last_x, int *last_y) {
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    int code = AI_heuristic_alpha_beta_search(ai, game, 5, first_move);
    int x = (code >> 8) & 0xFF;
    int y = code & 0xFF;
    Game_play(game, x, y);
    gettimeofday(&end_time, NULL);
    *last_x = x;
    *last_y = y;
    double time_taken = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    printf("%s %c %d\n", ai->name, 'A' + y, x + 1);
    printf("time consumption: %f seconds\n", time_taken);
    Game_display_board(game, x, y, 1);
}

void human_play(Game *game, int *x, int *y) {
    printf("your move (column, line): ");
    int line;
    char col_c;
    int col;
    char _;
    scanf("\n%c %d", &col_c, &line);
    line -= 1;
    col = col_c - 'A';
    while (!Game_play(game, line, col) || line < 1 || line > BOARD_SIZE || col < 1 || col > BOARD_SIZE) {
        printf("try again\n");
        printf("your move should be like: H 8\n");
        printf("your move (column, line): ");
        scanf("\n%c %d", &col_c, &line);
        line -= 1;
        col = col_c - 'A';
    }
    *x = line;
    *y = col;
    Game_display_board(game, line, col, 1);
}
