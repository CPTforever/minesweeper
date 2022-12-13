#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ncurses.h>
#include <time.h>

#define OPTIONS "h:w:m:f:"

typedef struct GameStateObj {
    size_t width;
    size_t height; 
    size_t **board;
    bool **discovered;
    bool **flag;
    size_t **canidates;
    size_t mines;

    size_t cursor_x;
    size_t cursor_y;
} GameStateObj;

typedef struct GameStateObj *GameState;

int digits(size_t i) {
    int j = 0;
    while(i > 0) {
        i/=10;
        j++;
    }

    return j;
}

typedef enum {NUM_1 = 1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_COUNT, MINE, CURSOR, EMPTY, FLAG, CLEARED} node;

GameState board_create(size_t width, size_t height, size_t mines) {
    GameState gs = malloc(sizeof(GameStateObj));
    gs->board = calloc(sizeof(size_t *), height);
    gs->canidates = calloc(sizeof(size_t *), height * width);
    gs->discovered = calloc(sizeof(bool *), height);
    gs->flag = calloc(sizeof(bool *), height);

    for (size_t y = 0; y < height; y++) {
        gs->board[y] = calloc(sizeof(size_t), width);
        for (size_t x = 0; x < width; x++) {
            gs->board[y][x] = EMPTY;
        }
    } 
    
    for (size_t y = 0; y < height; y++) {
        gs->discovered[y] = calloc(sizeof(bool), width);
        for (size_t x = 0; x < width; x++) {
            gs->discovered[y][x] = false;
        }
    } 
    
    for (size_t y = 0; y < height; y++) {
        gs->flag[y] = calloc(sizeof(bool), width);
        for (size_t x = 0; x < width; x++) {
            gs->flag[y][x] = false;
        }
    } 

    gs->width = width;
    gs->height = height;
    gs->mines = mines;

    return gs;
}

void board_delete(GameState *pgs) {
    if (pgs && *pgs) {
        for (size_t i = 0; i < (*pgs)->height; i++) {
            free((*pgs)->board[i]);
        }
        free((*pgs)->board);
        free(*pgs);

        *pgs = NULL;
    }
}


#define CROSS           (110 | A_ALTCHARSET)
#define LEFT_TREE       (117 | A_ALTCHARSET)
#define RIGHT_TREE      (116 | A_ALTCHARSET)
#define UP_TREE         (118 | A_ALTCHARSET)
#define DOWN_TREE       (119 | A_ALTCHARSET)
#define BOTTOM_LEFT     (109 | A_ALTCHARSET) 
#define TOP_LEFT        (108 | A_ALTCHARSET) 
#define TOP_RIGHT       (107 | A_ALTCHARSET) 
#define BOTTOM_RIGHT    (106 | A_ALTCHARSET) 
#define HORIZANTAL      (113 | A_ALTCHARSET)
#define VERTICAL        (120 | A_ALTCHARSET)

void print_board(GameState gs) {
    size_t board_width = gs->width;
    size_t board_height = gs->height;
    
    size_t max_height, max_width;
    getmaxyx(stdscr, max_height, max_width);

    if (board_width > max_width || board_height > max_height) {
        printw("ERROR! TERMINAL WINDOW IS NOT BIG ENOUGH\n");
        return;
    }

    int curr_width = (max_width) / 2 - board_width / 2, curr_height = (max_height) / 2 - board_height / 2;
    
    move(curr_height, curr_width); 
    for (size_t y = 0; y < gs->height; y++) {
        for (size_t x = 0; x < gs->width; x++) {
            size_t n = gs->board[y][x];
            size_t d = gs->discovered[y][x];
            size_t f = gs->flag[y][x];
            chtype pair = COLOR_PAIR(n);
            
            if (x == gs->cursor_x && y == gs->cursor_y) {
                pair = COLOR_PAIR(CURSOR);
            } 
            else if (f) {
                pair = COLOR_PAIR(FLAG);
            }
            else if (!d) {
                pair = COLOR_PAIR(EMPTY);
            }
            
            attron(pair); 

            if (f) {
                printw("F");
            }
            else if (!d) {
                printw(".");
            }
            else if (n == CLEARED) {
                printw("O");
            }
            else if (n == MINE) {
                printw("X");
            }
            else if (n == EMPTY) {
                printw(" ");
            }
            else {
                printw("%zu", n);
            }
            attroff(pair); 
        }
        move(++curr_height, curr_width); 
    }

    refresh();
}

size_t distance(size_t x, size_t y) {
    size_t r = (x > y) ? x - y : y - x;

    return r * r;
}

bool insert_mines(GameState gs) {
    size_t num_canidates = 0;
    for (size_t y = 0; y < gs->height; y++) {
        for (size_t x = 0; x < gs->width; x++) {
            size_t circle = distance(y, gs->cursor_y) + distance(x, gs->cursor_x);
            if ((gs->width > 4 || gs->height > 4 ? 4 : 1) > circle) continue;
            if (gs->board[y][x] != MINE) {
                gs->canidates[num_canidates++] = gs->board[y] + x;
            }
        }
    }

    if (num_canidates == 0) {
        return false;
    }

    size_t canidate = rand() % num_canidates;
    
    *(gs->canidates[canidate]) = MINE;

    return true;
}

size_t bounds(GameState gs, size_t y, size_t x) {
    if (y >= gs->height || x >= gs->width) return false;

    return true;
}

size_t check_surrounding(GameState gs, size_t y, size_t x) {
    size_t count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (!bounds(gs, y + i, x + j)) continue;
            
            if (gs->board[y + i][x + j] == MINE) ++count;
        }
    }

    return count;
}

bool generate_mine(GameState gs) {
    for (size_t i = 0; i < gs->mines; i++) {
        insert_mines(gs);
    }

    for (size_t y = 0; y < gs->height; y++) {
        for (size_t x = 0; x < gs->width; x++) {
            size_t *n = gs->board[y] + x;
            if (*n == MINE) continue;
            else {
                *n = check_surrounding(gs, y, x);
                if (*n == 0) *n = EMPTY;
            }
        }
    }

    return true;
}

typedef enum {LEFT, RIGHT, UP, DOWN, DIRECTION_NUM} DIRECTION;
#define X_COORD 0
#define Y_COORD 1
static const int dir_off[DIRECTION_NUM][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

bool cursor_move(GameState gs, DIRECTION dir) {
    size_t cursor_x = gs->cursor_x + dir_off[dir][X_COORD];
    size_t cursor_y = gs->cursor_y + dir_off[dir][Y_COORD];
    
    if (cursor_x > gs->width || cursor_y > gs->height) return false;

    gs->cursor_x = cursor_x;
    gs->cursor_y = cursor_y;
    
    return true;
}

void discover(GameState gs, size_t y, size_t x) {
    if (x >= gs->width || y >= gs->height) return;

    size_t *node = gs->board[y] + x;
    bool *discovered = gs->discovered[y] + x;
    
    gs->flag[gs->cursor_y][gs->cursor_x] = false;
    if (*discovered) return;
    
    *discovered = true;

    if (*node == MINE) {
        return;
    }

    if (*node < NUM_COUNT) {
        return;
    }
    if (*node == EMPTY) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue;
                discover(gs, y + i, x + j);
            }
        }

    }

}

bool cursor_discover(GameState gs) {
    size_t node = gs->board[gs->cursor_y][gs->cursor_x];
    if (node == MINE) {
        gs->discovered[gs->cursor_y][gs->cursor_x] = true;

        return false;
    }
    discover(gs, gs->cursor_y, gs->cursor_x);

    return true;
}

bool set_flag(GameState gs) {
    bool *flag = gs->flag[gs->cursor_y] + gs->cursor_x;
    bool discovered = gs->discovered[gs->cursor_y][gs->cursor_x];
    
    if (discovered) {
        return false;
    }

    *flag = !(*flag);

    return true;
}

bool display_mines(GameState gs, bool won) { 
    for (size_t y = 0; y < gs->height; y++) {
        for (size_t x = 0; x < gs->width; x++) {
            if (gs->board[y][x] == MINE) gs->discovered[y][x] = true;
            if (won && gs->flag[y][x] && gs->board[y][x] == MINE) {
                gs->flag[y][x] = false;
                gs->board[y][x] = CLEARED;
            }
            print_board(gs); 
            refresh();
            usleep(2000);
        }
    }
    return true;
}

bool check_win(GameState gs) {
    for (size_t y = 0; y < gs->height; y++) {
        for (size_t x = 0; x < gs->width; x++) {
            if (gs->flag[y][x] && gs->board[y][x] != MINE) return false;
            if (gs->board[y][x] == MINE && !gs->flag[y][x]) return false;
            if (gs->board[y][x] != MINE && !gs->discovered[y][x]) return false;
        }
    }

    return true;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    int opt; 
    
    size_t height = 24, width = 24, mines = 99;

    while((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch(opt) {
            case 'h':
                height = strtoul(optarg, NULL, 10);
                if (height > 32) height = 32;
                if (height < 2) height = 2;
                break;
            case 'w':
                width = strtoul(optarg, NULL, 10);
                if (width > 32) width = 32;
                if (width < 2) width = 2;
                break;
            case 'm':
                mines = strtoul(optarg, NULL, 10);
                break;
        }
    }

    initscr();
    raw(); 
    noecho();
	start_color();
    curs_set(0); 

    init_pair(EMPTY, COLOR_WHITE, COLOR_BLACK);
    init_pair(NUM_1, COLOR_BLUE, COLOR_BLACK);
    init_pair(NUM_2, COLOR_GREEN, COLOR_BLACK);
    init_pair(NUM_3, COLOR_RED, COLOR_BLACK);
    init_pair(NUM_4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(NUM_5, COLOR_CYAN, COLOR_BLACK);
    init_pair(NUM_6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(NUM_7, COLOR_WHITE, COLOR_BLACK);
    init_pair(NUM_8, COLOR_BLACK, COLOR_BLUE);
    init_pair(MINE, COLOR_BLACK, COLOR_RED);
    init_pair(CURSOR, COLOR_BLACK, COLOR_WHITE);
    init_pair(FLAG, COLOR_BLACK, COLOR_RED); 
    init_pair(CLEARED, COLOR_BLACK, COLOR_GREEN); 

    GameState gs = board_create(width, height, mines);
    print_board(gs); 
    refresh();
    
    int c = '\0';
    bool playing = true;
    bool won = false;
    bool mines_laid = false;
    while ((c = getch()) != 27) {
        DIRECTION dir = DIRECTION_NUM;
        
        if (c == 'a') dir = LEFT;
        else if (c == 'w') dir = UP;
        else if (c == 's') dir = DOWN;
        else if (c == 'd') dir = RIGHT;
     
        if (c == 10 || c == KEY_ENTER) {
            if (!mines_laid) {
                generate_mine(gs);
                mines_laid = true;
            }
            playing = cursor_discover(gs);

        }
        else if (c == 'f') {
            set_flag(gs);
        }
        if (dir != DIRECTION_NUM) {
            cursor_move(gs, dir);
        }
        clear();

        print_board(gs); 
        refresh();
        
        if (mines_laid && check_win(gs)) {
            won = true;
            display_mines(gs, true);
            sleep(1);
            break;
        }
        if (playing == false) {
            display_mines(gs, false);
            sleep(1);
            break;
        }
    }

    endwin();
    
    if (won == true) {
        printf("You won!\n");
    }
    else {
        printf("You lost!\n");
    }
    


    board_delete(&gs);

    return 0;
}
