#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <direct.h> // for _mkdir on Windows


#define BOARD_SIZE 8

static char board[BOARD_SIZE][BOARD_SIZE];
static char sideToMove = 'w';
static FILE *logFile = NULL;
static char botColor = 0; // 'w' or 'b'

// --- Logging ---
void log_msg(const char *fmt, ...) {
    if (!logFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(logFile, fmt, args);
    fprintf(logFile, "\n");
    fflush(logFile);
    va_end(args);
}

int ensure_dir_exists(const char *path) {
    return _mkdir(path); // returns 0 if created, -1 if already exists or error
}

void print_board_log() {
    log_msg("Current board:");
    for (int i = 0; i < BOARD_SIZE; i++) {
        char row[16] = {0};
        for (int j = 0; j < BOARD_SIZE; j++)
            row[j] = board[i][j];
        log_msg("%s", row);
    }
}

// --- Board setup ---
void reset_board() {
    const char *start[8] = {
        "rnbqkbnr",
        "pppppppp",
        "........",
        "........",
        "........",
        "........",
        "PPPPPPPP",
        "RNBQKBNR"
    };
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            board[i][j] = start[i][j];

    sideToMove = 'w';
    log_msg("Board reset");
    print_board_log();
}


// --- Helpers ---
bool on_board(int r, int f) {
    return r >= 0 && r < 8 && f >= 0 && f < 8;
}

bool is_my_piece(char piece, char side) {
    if (piece == '.') return false;
    return (side=='w') ? isupper(piece) : islower(piece);
}

bool can_capture(char piece, char side) {
    if (piece == '.') return true;
    return !is_my_piece(piece, side);
}

// --- Move generator ---
// Piece values for basic evaluation
int piece_value(char p){
    switch(toupper(p)){
        case 'P': return 1;
        case 'N': return 3;
        case 'B': return 3;
        case 'R': return 5;
        case 'Q': return 9;
        case 'K': return 1000; // king is invaluable
    }
    return 0;
}


#define BOARD_SIZE 8
#define MOVE_HISTORY_LEN 8  // number of previous moves to track

static char board[BOARD_SIZE][BOARD_SIZE];

struct MoveHistory {
    int from_r, from_f;
    int to_r, to_f;
};

static struct MoveHistory moveHistory[MOVE_HISTORY_LEN];
static int moveHistoryIndex = 0;


// --- Move History ---
bool repeats_history(int from_r, int from_f, int to_r, int to_f) {
    for(int i = 0; i < MOVE_HISTORY_LEN; i++){
        if(moveHistory[i].from_r == to_r && moveHistory[i].from_f == to_f &&
           moveHistory[i].to_r   == from_r && moveHistory[i].to_f   == from_f) {
            return true; // this move would undo a recent move
        }
    }
    return false;
}

// --- Apply move ---
void apply_move(const char *move) {
    if (strlen(move) < 4) return;

    int from_file = move[0] - 'a';
    int from_rank = '8' - move[1];
    int to_file   = move[2] - 'a';
    int to_rank   = '8' - move[3];

    char piece = board[from_rank][from_file];
    board[to_rank][to_file] = piece;
    board[from_rank][from_file] = '.';

    if (strlen(move) == 5) {
        char promo = move[4];
        if (sideToMove == 'w') promo = toupper(promo);
        else promo = tolower(promo);
        board[to_rank][to_file] = promo;
    }

    // Update move history
    moveHistory[moveHistoryIndex] = (struct MoveHistory){from_rank, from_file, to_rank, to_file};
    moveHistoryIndex = (moveHistoryIndex + 1) % MOVE_HISTORY_LEN;

    sideToMove = (sideToMove == 'w') ? 'b' : 'w';
}

// --- Generate one move ---
bool generate_one_move(char side) {
    struct Move { int from_r, from_f, to_r, to_f; char promo; int score; };
    struct Move bestMove = {-1,-1,-1,-1,0,-10000};

    for(int r=0;r<8;r++){
        for(int f=0;f<8;f++){
            char piece = board[r][f];
            if(!is_my_piece(piece,side)) continue;

            int dir = (side=='w')?-1:1;
            int start_row = (side=='w')?6:1;

            // --- PAWN ---
            if((piece=='P'&&side=='w')||(piece=='p'&&side=='b')){
                int to_r = r+dir;

                // Forward one
                if(on_board(to_r,f)&&board[to_r][f]=='.'){
                    char promo = ((side=='w'&&to_r==0)||(side=='b'&&to_r==7))?((side=='w')?'Q':'q'):0;
                    int score = promo ? 9 : 1;
                    if(score>bestMove.score){
                        bestMove=(struct Move){r,f,to_r,f,promo,score};
                    }
                }

                // Forward two
                if(r==start_row && board[r+dir][f]=='.' && board[r+2*dir][f]=='.'){
                    int to_r2=r+2*dir;
                    if(1>bestMove.score) bestMove=(struct Move){r,f,to_r2,f,0,1};
                }

                // Captures
                for(int df=-1;df<=1;df+=2){
                    int to_f=f+df;
                    if(on_board(to_r,to_f)&&can_capture(board[to_r][to_f],side)&&board[to_r][to_f]!='.'){
                        int score = piece_value(board[to_r][to_f]) - piece_value(piece);
                        if(score>bestMove.score) bestMove=(struct Move){r,f,to_r,to_f,((side=='w'&&to_r==0)||(side=='b'&&to_r==7))?((side=='w')?'Q':'q'):0,score};
                    }
                }
                continue;
            }

            // --- KNIGHT ---
            if(piece=='N'||piece=='n'){
                const int moves[8][2]={{2,1},{1,2},{-1,2},{-2,1},{-2,-1},{-1,-2},{1,-2},{2,-1}};
                for(int i=0;i<8;i++){
                    int to_r=r+moves[i][0], to_f=f+moves[i][1];
                    if(on_board(to_r,to_f)&&can_capture(board[to_r][to_f],side)){
                        int score = board[to_r][to_f]=='.'?3:piece_value(board[to_r][to_f])-3;
                        if(score>bestMove.score) bestMove=(struct Move){r,f,to_r,to_f,0,score};
                    }
                }
                continue;
            }

            // --- SLIDING PIECES ---
            int dirs[8][2]; int count=0; bool sliding=true;
            if(piece=='R'||piece=='r'){int tmp[4][2]={{1,0},{-1,0},{0,1},{0,-1}}; memcpy(dirs,tmp,sizeof(tmp)); count=4;}
            else if(piece=='B'||piece=='b'){int tmp[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}}; memcpy(dirs,tmp,sizeof(tmp)); count=4;}
            else if(piece=='Q'||piece=='q'){int tmp[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}}; memcpy(dirs,tmp,sizeof(tmp)); count=8;}
            else if(piece=='K'||piece=='k'){int tmp[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}}; memcpy(dirs,tmp,sizeof(tmp)); count=8; sliding=false;}
            else continue;

            for(int d=0; d<count; d++){
                int dr=dirs[d][0], df=dirs[d][1];
                int to_r=r+dr, to_f=f+df;
                while(on_board(to_r,to_f)&&can_capture(board[to_r][to_f],side)){
                    // Skip move if it undoes a recent move
                    if(repeats_history(r,f,to_r,to_f)) break;

                    int score = board[to_r][to_f]=='.'?piece_value(piece):piece_value(board[to_r][to_f])-piece_value(piece);
                    if(score>bestMove.score) bestMove=(struct Move){r,f,to_r,to_f,0,score};
                    if(board[to_r][to_f]!='.'||!sliding) break;
                    to_r+=dr; to_f+=df;
                }
            }
        }
    }

    if(bestMove.from_r!=-1){
        if(bestMove.promo)
            printf("bestmove %c%d%c%d%c\n",'a'+bestMove.from_f,8-bestMove.from_r,'a'+bestMove.to_f,8-bestMove.to_r,bestMove.promo);
        else
            printf("bestmove %c%d%c%d\n",'a'+bestMove.from_f,8-bestMove.from_r,'a'+bestMove.to_f,8-bestMove.to_r);
        fflush(stdout);
        return true;
    }

    return false;
}

// --- Main ---
int gamestarted = 0;  // 0 = no game, 1 = game started

int main(void) {
    char line[1024];
    int moves_applied = 0;  // track how many moves we've applied

    ensure_dir_exists("C:/Users/hudso/CLionProjects/untitled/logs");

    logFile = fopen("C:/Users/hudso/CLionProjects/untitled/logs/bot.log","w");
    if (!logFile){
        fprintf(stderr,"Failed to open log file\n");
        return 1;
    }
    setvbuf(logFile, NULL, _IOLBF, 0); // line-buffered

    while(fgets(line,sizeof(line),stdin)){
        line[strcspn(line,"\r\n")]=0;
        log_msg("Received command: %s",line);

        if(strcmp(line,"uci")==0){
            printf("id name MyBot\nid author Me\nuciok\n"); fflush(stdout);
        }
        else if(strcmp(line,"isready")==0){
            printf("readyok\n"); fflush(stdout);
        }
        else if(strcmp(line,"ucinewgame")==0){
            gamestarted = 0;
            moves_applied = 0; // reset move counter
            log_msg("Starting new game");
        }
        else if(strcmp(line,"quit")==0){
            log_msg("Quitting");
            break;
        }
        else if(strncmp(line, "position", 8) == 0){
            char *moves = strstr(line, "moves");

            // Reset board only for new game
            if(!gamestarted && (strstr(line,"startpos") || strstr(line,"fen"))){
                reset_board();
                gamestarted = 1;

                if(strstr(line,"startpos")){
                    botColor = strstr(line," w ") ? 'w' : 'b';
                    log_msg("Bot is playing as %s",(botColor=='w')?"White":"Black");
                } else {
                    log_msg("FEN parsing not implemented, using default start position");
                }
            }

            // Apply moves if any
            if(moves){
                moves += 6; // skip "moves "
                char *tok = strtok(moves, " ");
                int move_index = 0;

                while(tok){
                    if(move_index >= moves_applied){
                        apply_move(tok);
                    }
                    tok = strtok(NULL, " ");
                    move_index++;
                }

                moves_applied = move_index; // update counter
            }
        }
        else if(strncmp(line,"go",2)==0){
            if(!gamestarted) log_msg("Warning: 'go' command received before game started!");

            log_msg("Bot to move: %s", (sideToMove=='w')?"White":"Black");
            if(!generate_one_move(sideToMove)){
                printf("bestmove a2a3\n"); fflush(stdout); // fallback
            }
        }
    }

    fclose(logFile);
    return 0;
}

