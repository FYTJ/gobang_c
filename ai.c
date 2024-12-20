//
// Created by 竹彦博 on 2024/12/9.
//
#include "ai.h"

#include <limits.h>

#include "patterns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include </usr/local/Cellar/libomp/19.1.6/include/omp.h>
#include <sys/time.h>

static long long AI_eval_pos(AI* ai, int state[BOARD_SIZE][BOARD_SIZE], int player, int x, int y, int direction[2]);
static void AI_get_line(int state[BOARD_SIZE][BOARD_SIZE], int player,int x,int y,int dx,int dy,char* line);
static void* AI_parallel_max_value(void* arg);
static long long AI_min_value(AI* ai, Game* game, int state[BOARD_SIZE][BOARD_SIZE], int player, double alpha, double beta,int depth);
static long long AI_max_value(AI* ai, Game* game, int state[BOARD_SIZE][BOARD_SIZE], int player, double alpha, double beta,int depth);

void AI_init(AI* ai, const char* name, int color) {
    ai->name = strdup(name);
    ai->color = color;
    ai->count = 0;
    ai->maxv = 0;
    ai->minv = 0;
    ai->extension_threshold = 2000;
    ai->lmr_threshold = 20;
    ai->lmr_min_depth = 0;
    compile_patterns(ai->patterns);
    ai->openings_count=0;
    int pattern_values[PATTERN_COUNT] = {50000, 4320, 720, 720, 120, 50, 20};
    for (int i = 0; i < PATTERN_COUNT; i++) {
        ai->pattern_weights[i] = pattern_values[i];
    }
}


void AI_read_openings(AI* ai, const char* filename) {
    FILE* f = fopen(filename,"r");
    if (!f) {
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char tmp[256];
        int len=strlen(line);
        int idx=0;
        for (int i=0;i<len;i++){
            char c=line[i];
            if (c!='[' && c!=']' && c!='(' && c!=')' && c!='\n' && c!=' ' ) {
                tmp[idx++]=c;
            }
        }
        tmp[idx]='\0';
        int vals[10];
        int v_count=0;
        char* p = strtok(tmp,",");
        while (p && v_count<10) {
            vals[v_count++]=atoi(p);
            p=strtok(NULL,",");
        }
        if (v_count>=10) {
            for (int i=0;i<10;i++){
                ai->openings[ai->openings_count][i]=vals[i];
            }
            ai->openings_count++;
        }
    }
    fclose(f);
}

int AI_look_up_table(AI* ai, int record_chess[225][2], int steps) {
    for (int i=0;i<ai->openings_count;i++){
        int r = rand()%ai->openings_count;
        if (r!=i){
            int tmp[10];
            memcpy(tmp,ai->openings[i],sizeof(tmp));
            memcpy(ai->openings[i],ai->openings[r],sizeof(tmp));
            memcpy(ai->openings[r],tmp,sizeof(tmp));
        }
    }

    for (int k=0;k<ai->openings_count;k++){
        int match=1;
        for (int s=0;s<steps;s++){
            if (ai->openings[k][2*s]!=record_chess[s][0] || ai->openings[k][2*s+1]!=record_chess[s][1]){
                match=0;
                break;
            }
        }
        if (match && steps<5) {
            int nx=ai->openings[k][2*steps];
            int ny=ai->openings[k][2*steps+1];
            return (nx<<8)|ny;
        }
    }
    return -1;
}

static long long AI_heuristic_eval(AI* ai, Game* game, int state[BOARD_SIZE][BOARD_SIZE], int player, int last_x, int last_y, int have_last, long long prev_eval, int prev_state[BOARD_SIZE][BOARD_SIZE]) {
    ai->count +=1;
    if (have_last && Game_check_ban(state, player, last_x, last_y) != -1) {
        return -100000000;
    }
    if (have_last) {
        int all_directions[4][2]={{1,0},{0,1},{1,1},{-1,1}};
        long long target_eval = 0, delta_eval = 0, delta_opponent_eval = 0;
        for (int i=0;i<4;i++) {
            int *direction = all_directions[i];
            target_eval += AI_eval_pos(ai, state, player, last_x, last_y, direction);
        }
        for (int i=0;i<4;i++) {
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
                    }
                    else if (state[x][y] == 3 - player) {
                        long long current_target_eval = AI_eval_pos(ai, state, 3 - player, x, y, direction);
                        long long prev_target_eval = AI_eval_pos(ai, prev_state, 3 - player, x, y, direction);
                        delta_opponent_eval += current_target_eval - prev_target_eval;
                    }
                }
            }
        }
        return prev_eval + target_eval + delta_eval - delta_opponent_eval * 2;
    }
    long long total_eval=0;
    for (int x=0;x<BOARD_SIZE;x++){
        for (int y=0;y<BOARD_SIZE;y++){
            int directions[4][2]={{1,0},{0,1},{1,1},{-1,1}};
            for (int i=0;i<4;i++) {
                int *direction = directions[i];
                int c=state[x][y];
                if (c==player) {
                    total_eval += AI_eval_pos(ai,state,player,x,y, direction);
                } else if (c==(3-player)) {
                    total_eval -= AI_eval_pos(ai,state,3-player,x,y, direction) * 2;
                }
            }
        }
    }
    return total_eval;
}

int AI_heuristic_alpha_beta_search(AI* ai, Game* game, int depth, int is_first_move) {
    ai->count=0; ai->maxv=0; ai->minv=0;
    int move_x=-1,move_y=-1;
    if (is_first_move) {
        move_x=7;move_y=7;
    } else {
        if (game->record_count<=4) {
            int code = AI_look_up_table(ai, game->record_chess, game->record_count);
            if (code!=-1) {
                move_x=(code>>8)&0xFF;
                move_y=code&0xFF;
            }
        }
        if (move_x<0) {
            int state[BOARD_SIZE][BOARD_SIZE];
            for (int i=0;i<BOARD_SIZE;i++){
                for (int j=0;j<BOARD_SIZE;j++){
                    state[i][j]=game->board[i][j];
                }
            }
            int res = AI_max_value(ai, game, state, ai->color, -1e9,1e9, depth);
            move_x = (res>>8)&0xFF;
            move_y = res&0xFF;
        }
    }
    printf("%d %d %d\n", ai->count, ai->maxv, ai->minv);
    return (move_x<<8)|move_y;
}

void ai_play(Game* game, AI* ai, int first_move, int *last_x, int *last_y) {
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    int code = AI_heuristic_alpha_beta_search(ai,game,5
    , first_move);
    int x=(code>>8)&0xFF;
    int y=code&0xFF;
    Game_play(game,x,y);
    gettimeofday(&end_time, NULL);
    *last_x=x;
    *last_y=y;
    double time_taken = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    printf("%s %c %d\n", ai->name, 'A' + y, x + 1);
    printf("time consumption: %f seconds\n", time_taken);
    Game_display_board(game,x,y,1);
}

void human_play(Game* game, int *x, int *y) {
    printf("your move (column, line): ");
    int line;
    char col_c;
    int col;
    char _;
    scanf("\n%c %d",&col_c, &line);
    line-=1;
    col = col_c - 'A';
    while(!Game_play(game,line,col) || line < 1 || line > BOARD_SIZE || col < 1 || col > BOARD_SIZE) {
        printf("try again\n");
        printf("your move should be like: H 8\n");
        printf("your move (column, line): ");
        scanf("\n%c %d",&col_c, &line);
        line-=1;
        col = col_c - 'A';
    }
    *x=line;
    *y=col;
    Game_display_board(game,line,col,1);
}

int compare(const void *a, const void *b) {
    Data *data1 = (Data*)a;
    Data *data2 = (Data*)b;
    // 根据score进行从大到小排序
    if (data1->score < data2->score) return 1;
    if (data1->score > data2->score) return -1;
    return 0;
}

int compare_reverse(const void *a, const void *b) {
    Data *data1 = (Data*)a;
    Data *data2 = (Data*)b;
    // 根据score进行从小到大排序
    if (data1->score < data2->score) return -1;
    if (data1->score > data2->score) return 1;
    return 0;
}

// Min/Max search

static void* AI_parallel_max_value(void* arg) {
    Parallel_args *args = (Parallel_args*)arg;
    int* task_index = args -> task_index;
    AI* ai = args -> ai;
    Game* game = args -> game;
    int **nstate_arg = args -> nstate_arg;
    int nstate[BOARD_SIZE][BOARD_SIZE];
    int player = args -> player;
    double alpha = args -> alpha;
    double beta = args -> beta;
    int depth = args -> depth;
    int x = args -> x;
    int y = args -> y;
    Parallel_returns *results = (Parallel_returns*)(args -> results);
    void* mutex = args -> result_mutex;
    for (int i=0;i<BOARD_SIZE;i++) {
        for (int j=0;j<BOARD_SIZE;j++) {
            nstate[i][j] = nstate_arg[i][j];
        }
    }
    long long res = AI_min_value(ai, game, nstate, player, alpha, beta, depth);
    pthread_mutex_lock(mutex);
    results[*task_index].v = res >> 16;
    results[*task_index].x = x;
    results[*task_index].y = y;
    pthread_mutex_unlock(mutex);
    return NULL;
}

static long long AI_max_value(AI* ai, Game* game, int state[BOARD_SIZE][BOARD_SIZE], int player, double alpha, double beta,int depth) {
    long long current_eval = AI_heuristic_eval(ai,game,state,player,-1,-1,0,0, NULL);
    if (Game_is_cutoff(game,state,depth)) {
        long long val = current_eval;
        return (val<<16)| (0xFF<<8)|0xFF;
    }

    ai->lmr_threshold=10;
    ai->lmr_min_depth=0;

    Actions act;
    Game_actions(state,&act);

    Data arr[act.count];
    int arr_count=0;
    for (int i=0;i<act.count;i++){
        int nstate[BOARD_SIZE][BOARD_SIZE];
        Game_result(state,nstate,act.x[i],act.y[i],player);
        long long val = AI_heuristic_eval(ai,game,nstate,player,act.x[i],act.y[i],1,current_eval, state);
        arr[arr_count].x=act.x[i];
        arr[arr_count].y=act.y[i];
        arr[arr_count].score=val;
        arr_count++;
    }
    qsort(arr, arr_count, sizeof(Data), compare);

    ai->maxv += 1;

    Actions nbr;
    Game_neighbors(state,&nbr);

    long long best_val = -100000000;
    int best_x=255,best_y=255;

    if (depth == 5) {
        Parallel_returns results[10];
        pthread_t threads[10];
        pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

        for (int i=0;i<arr_count;i++) {
            int* task_index = malloc(sizeof(int));
            *task_index = i;

            int x=arr[i].x;
            int y=arr[i].y;

            int nstate[15][15];
            Game_result(state,nstate,x,y,player);
            int **nstate_arg = malloc(BOARD_SIZE * sizeof(int *));
            for (int j=0;j<BOARD_SIZE;j++) {
                nstate_arg[j] = malloc(BOARD_SIZE * sizeof(int));
            }
            for (int j=0;j<BOARD_SIZE;j++) {
                for (int k=0;k<BOARD_SIZE;k++) {
                    nstate_arg[j][k]=nstate[j][k];
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

            int in_nbr=0;
            for (int k=0;k<nbr.count;k++){
                if (nbr.x[k]==x && nbr.y[k]==y) {
                    in_nbr=1;
                    break;
                }
            }
            if (!in_nbr) continue;

            if (i >= ai->lmr_threshold && depth>=ai->lmr_min_depth) {
                continue;
            }


            pthread_create(&threads[i], NULL, AI_parallel_max_value, myArgs);
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

    // #pragma omp parallel for shared(ai, game, arr, nbr, state, player, alpha, beta, depth) reduction(max:best_val) reduction(max:best_x) reduction(max:best_y)
    for (int i=0;i<arr_count;i++){
        int x=arr[i].x;
        int y=arr[i].y;

        int in_nbr=0;
        for (int k=0;k<nbr.count;k++){
            if (nbr.x[k]==x && nbr.y[k]==y) {
                in_nbr=1;
                break;
            }
        }
        if (!in_nbr) continue;

        if (i >= ai->lmr_threshold && depth>=ai->lmr_min_depth) {
            continue;
        }

        int nstate[BOARD_SIZE][BOARD_SIZE];
        Game_result(state,nstate,x,y,player);

        long long res = AI_min_value(ai, game, nstate, player, alpha,beta, depth-1);
        long long v = res>>16;
        if (v > best_val) {
            best_val=v;best_x=x;best_y=y;
        }
        if (v>=beta) {
            return (v<<16)|(best_x<<8)|best_y;
        }
        if (v>alpha) alpha=v;
    }

    return (best_val<<16)|(best_x<<8)|best_y;
}

static long long AI_min_value(AI* ai, Game* game, int state[BOARD_SIZE][BOARD_SIZE], int player, double alpha, double beta,int depth) {
    long long current_eval = AI_heuristic_eval(ai,game,state,player,-1,-1,0,0, state);
    if (Game_is_cutoff(game,state,depth)) {
        long long val = current_eval;
        return (val<<16)|(0xFF<<8)|0xFF;
    }

    ai->lmr_threshold=10;
    ai->lmr_min_depth=0;

    Actions act;
    Game_actions(state,&act);

    Data arr[act.count];
    int arr_count=0;
    for (int i=0;i<act.count;i++){
        int nstate[BOARD_SIZE][BOARD_SIZE];
        Game_result(state,nstate,act.x[i],act.y[i],3-player);
        long long val = AI_heuristic_eval(ai,game,nstate,player,act.x[i],act.y[i],1,current_eval, state);
        arr[arr_count].x=act.x[i];
        arr[arr_count].y=act.y[i];
        arr[arr_count].score=val;
        arr_count++;
    }
    qsort(arr, arr_count, sizeof(Data), compare_reverse);
    ai->minv += 1;

    Actions nbr;
    Game_neighbors(state,&nbr);

    long long best_val = 100000000;
    int best_x=255,best_y=255;

// #pragma omp parallel for shared(ai, game, arr, nbr, state, player, alpha, beta, depth) reduction(min:best_val) reduction(min:best_x) reduction(min:best_y)
    for (int i=0;i<arr_count;i++){
        int x=arr[i].x;int y=arr[i].y;
        int in_nbr=0;
        for (int k=0;k<nbr.count;k++){
            if (nbr.x[k]==x && nbr.y[k]==y){in_nbr=1;break;}
        }
        if (!in_nbr) continue;

        if (i>=ai->lmr_threshold && depth>=ai->lmr_min_depth) {
            continue;
        }

        int nstate[BOARD_SIZE][BOARD_SIZE];
        Game_result(state,nstate,x,y,3-player);
        long long res = AI_max_value(ai,game,nstate,player,alpha,beta,depth-1);
        long long v=res>>16;
        if (v<best_val) {
            best_val=v;best_x=x;best_y=y;
        }
        if (v<=alpha) {
            return (v<<16)|(best_x<<8)|best_y;
        }
        if (v<beta) beta=v;
    }

    return (best_val<<16)|(best_x<<8)|best_y;
}

static long long AI_eval_pos(AI* ai, int state[BOARD_SIZE][BOARD_SIZE], int player, int x, int y, int direction[2]) {
    long long total_eval=0;
    char line[64];
    AI_get_line(state,player,x,y,direction[0],direction[1],line);
    total_eval+= match_patterns(ai->patterns, line, ai->pattern_weights);
    return total_eval;
}

static void AI_get_line(int state[BOARD_SIZE][BOARD_SIZE], int player,int x,int y,int dx,int dy,char* line) {
    int idx=0;
    for (int step=-4; step<=4; step++){
        int nx=x+step*dx;int ny=y+step*dy;
        char c='2';
        if (nx>=0 && nx<BOARD_SIZE && ny>=0 && ny<BOARD_SIZE) {
            int val=state[nx][ny];
            if (val==player) c='1';
            else if (val==0) c='0';
            else c='2';
        }
        line[idx++]=c;
    }
    line[idx]='\0';
}
