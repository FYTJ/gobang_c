#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "game.h"
#include "ai.h"

void tune_parameters(Game game);

int human_human(Game game);

int human_ai(Game game, AI ai, int human_color);

int ai_ai(Game game, AI ai1, AI ai2);

AI run_ai_vs_ai_matches(Game game, AI *ai1, AI *ai2, int game_count);

int main() {
    srand((unsigned) time(NULL));
    Game game;
    Game_init(&game);

    printf("1: human-human; 2: human-ai; 3: ai-ai\nchoose: ");
    int mode;
    scanf("%d", &mode);

    if (mode == 1)
        return human_human(game);
    if (mode == 2) {
        printf("Choose black or white.\n1: black; 2: white\nchoose: ");
        int human_color;
        scanf("%d", &human_color);
        AI ai;
        AI_init(&ai, "ai", 2 - human_color);
        return human_ai(game, ai, human_color);
    }
    if (mode == 3) {
        AI ai1, ai2;
        AI_init(&ai1, "ai1", 1);
        AI_init(&ai2, "ai2", 2);
        return ai_ai(game, ai1, ai2);
    }
    if (mode == 4) {
        tune_parameters(game);
    }
    if (mode == 5) {
        int existed_chess[][2] = {
            {7, 7}, {7, 8}, {8, 9}, {9, 9}, {6, 7}, {9, 8}, {6, 8}, {9, 7}, {9, 6}, {10, 8}, {8, 6}, {8, 8}, {11, 8},
            {5, 9}, {10, 4}, {9, 5}, {10, 6}, {7, 6}, {6, 5}, {6, 6}, {5, 7}, {4, 7}, {4, 6}, {7, 9}, {3, 5}, {2, 4},
            {12, 6}, {11, 6}, {5, 5}, {6, 10}, {5, 11}, {2, 5}, {3, 7}, {9, 10}, {9, 11}, {6, 4}, {3, 6}, {3, 8},
            {5, 4}, {5, 8}, {4, 5}, {7, 5}, {5, 6}
        };
        int num_existed = sizeof(existed_chess) / sizeof(existed_chess[0]);
        // 将预设棋子放置到棋盘上
        for (int i = 0; i < num_existed; i++) {
            int x = existed_chess[i][0];
            int y = existed_chess[i][1];
            Game_play(&game, x, y);
        }

        AI ai1, ai2;
        AI_init(&ai1, "ai1", 1);
        AI_init(&ai2, "ai2", 2);
        int last_x = existed_chess[num_existed - 1][0];
        int last_y = existed_chess[num_existed - 1][1];
        Game_display_board(&game, last_x, last_y, 1);
        AI ai = game.current_player == 1 ? ai1 : ai2;
        ai_play(&game, &ai, 0, &last_x, &last_y);
    }
    return 0;
}


int human_human(Game game) {
    Game_display_board(&game, -1, -1, 0);
    int x = 0, y = 0;
    while (Game_check_winner(game.board) == -1 && Game_check_ban(game.board, game.current_player, x, y) == -1) {
        human_play(&game, &x, &y);
        if (Game_check_winner(game.board) != -1 && Game_check_ban(game.board, game.current_player, x, y) == -1)
            break;
        human_play(&game, &x, &y);
    }
    for (int i = 0; i < game.record_count; i++) {
        printf("(%d,%d), ", game.record_chess[i][0], game.record_chess[i][1]);
    }
    printf("\n");
    int winner = Game_check_winner(game.board) != -1
                     ? Game_check_winner(game.board)
                     : Game_check_ban(game.board, game.current_player, x, y);
    return winner;
}


int human_ai(Game game, AI ai, int human_color) {
    AI_read_openings(&ai, "/Users/zhuyanbo/PycharmProjects/人工智能：现代方法/gobang/openings.txt");
    Game_display_board(&game, -1, -1, 0);
    int x = 0, y = 0;
    if (human_color == 2)
        ai_play(&game, &ai, 1, &x, &y);
    while (Game_check_winner(game.board) == -1 && Game_check_ban(game.board, game.current_player, x, y) == -1) {
        human_play(&game, &x, &y);
        if (Game_check_winner(game.board) != -1 && Game_check_ban(game.board, game.current_player, x, y) == -1)
            break;
        ai_play(&game, &ai, 0, &x, &y);
    }
    for (int i = 0; i < game.record_count; i++) {
        printf("(%d,%d), ", game.record_chess[i][0], game.record_chess[i][1]);
    }
    printf("\n");
    int winner = Game_check_winner(game.board) != -1
                     ? Game_check_winner(game.board)
                     : Game_check_ban(game.board, game.current_player, x, y);
    return winner;
}


int ai_ai(Game game, AI ai1, AI ai2) {
    AI_read_openings(&ai1, "/Users/zhuyanbo/PycharmProjects/人工智能：现代方法/gobang/openings.txt");
    AI_read_openings(&ai2, "/Users/zhuyanbo/PycharmProjects/人工智能：现代方法/gobang/openings.txt");
    Game_display_board(&game, -1, -1, 0);
    int x = 0, y = 0;
    ai_play(&game, &ai1, 1, &x, &y);
    while (Game_check_winner(game.board) == -1) {
        ai_play(&game, &ai2, 0, &x, &y);
        if (Game_check_winner(game.board) != -1)
            break;
        ai_play(&game, &ai1, 0, &x, &y);
    }
    for (int i = 0; i < game.record_count; i++) {
        printf("(%d,%d), ", game.record_chess[i][0], game.record_chess[i][1]);
    }
    printf("\n");
    int winner = Game_check_winner(game.board) != -1
                     ? Game_check_winner(game.board)
                     : Game_check_ban(game.board, game.current_player, x, y);
    for (int i = 0; i < game.record_count; i++) {
        printf("{%d,%d}, ", game.record_chess[i][0], game.record_chess[i][1]);
    }
    return winner;
}


void tune_parameters(Game game) {
    AI ai1, ai2;
    AI_init(&ai1, "ai1", 1);
    AI_init(&ai2, "ai2", 2);

    int best_weights[PATTERN_COUNT] = {50000, 4320, 720, 720, 120, 50, 20};

    for (int trial = 1; trial <= 1; trial++) {
        // 随机参数：例如在原值附近随机微调
        printf("trial: %d\n", trial);
        for (int i = 0; i < PATTERN_COUNT; i++) {
            ai2.pattern_weights[i] = best_weights[i];
            ai1.pattern_weights[i] = best_weights[i];
        }
        for (int i = 0; i < PATTERN_COUNT; i++) {
            ai1.pattern_weights[i] = ai1.pattern_weights[i] + (
                                         rand() % ai1.pattern_weights[i] - ai1.pattern_weights[i] / 2);
            if (ai1.pattern_weights[i] < 0) ai1.pattern_weights[i] = 0; // 保持非负
        }
        // 对弈N局
        AI winner = run_ai_vs_ai_matches(game, &ai1, &ai2, 1);

        for (int i = 0; i < PATTERN_COUNT; i++) {
            best_weights[i] = winner.pattern_weights[i];
        }
        printf("Best weights:");
        for (int i = 0; i < PATTERN_COUNT; i++) {
            printf(" %d", best_weights[i]);
        }
        printf("\n");
    }
}


AI run_ai_vs_ai_matches(Game game, AI *ai1, AI *ai2, int game_count) {
    int win_1 = 0;
    for (int i = 0; i < game_count; i++) {
        printf("game_count: %d\n", i + 1);
        if (ai_ai(game, *ai1, *ai2) == 1) {
            win_1++;
        }
    }
    if (win_1 >= game_count / 2)
        return *ai1;
    return *ai2;
}
