#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define BOARD_SIZE 8

// --- Board ---
static char board[BOARD_SIZE][BOARD_SIZE];
static char sideToMove = 'w'; // 'w' = white, 'b' = black

// --- Helpers ---
bool on_board(int r, int f) { return r>=0 && r<8 && f>=0 && f<8; }
bool is_my_piece(char piece, char side) {
    if(piece=='.') return false;
    return (side=='w') ? isupper(piece) : islower(piece);
}

// --- Setup ---
void reset_board() {
    const char *start[8] = {
        "rnbqkbnr","pppppppp","........","........",
        "........","........","PPPPPPPP","RNBQKBNR"
    };
    for(int r=0;r<8;r++)
        for(int f=0;f<8;f++)
            board[r][f] = start[r][f];
    sideToMove = 'w';
}

// --- Apply move (e2e4 format) ---
void apply_move(const char *move) {
    int from_f = move[0]-'a', from_r='8'-move[1];
    int to_f   = move[2]-'a', to_r  ='8'-move[3];
    char piece = board[from_r][from_f];
    board[to_r][to_f] = piece;
    board[from_r][from_f] = '.';
    if(strlen(move)==5) {
        char promo = move[4];
        board[to_r][to_f] = (sideToMove=='w') ? toupper(promo) : tolower(promo);
    }
    sideToMove = (sideToMove=='w') ? 'b' : 'w';
}

// --- Generate one simple move ---
bool generate_one_move(char side) {
    for(int r=0;r<8;r++){
        for(int f=0;f<8;f++){
            char piece = board[r][f];
            if(!is_my_piece(piece,side)) continue;

            // Simple pawn move
            int dir = (side=='w')?-1:1;
            if((piece=='P' && side=='w')||(piece=='p' && side=='b')){
                int to_r = r+dir;
                if(on_board(to_r,f) && board[to_r][f]=='.'){
                    printf("bestmove %c%d%c%d\n",'a'+f,8-r,'a'+f,8-to_r);
                    fflush(stdout);
                    return true;
                }
            }
        }
    }
    // fallback
    printf("bestmove a2a3\n"); fflush(stdout);
    return false;
}

// --- Main UCI loop ---
int main(void) {
    char line[1024];

    while(fgets(line,sizeof(line),stdin)){
        line[strcspn(line,"\r\n")] = 0;

        if(strcmp(line,"uci")==0){
            printf("id name MinimalBot\nid author You\nuciok\n"); fflush(stdout);
        }
        else if(strcmp(line,"isready")==0){
            printf("readyok\n"); fflush(stdout);
        }
        else if(strcmp(line,"ucinewgame")==0){
            reset_board();
        }
        else if(strncmp(line,"position",8)==0){
            if(strstr(line,"startpos")) reset_board();
            char *moves = strstr(line,"moves");
            if(moves){
                moves+=6;
                char *tok = strtok(moves," ");
                while(tok){ apply_move(tok); tok=strtok(NULL," "); }
            }
            // determine side
            sideToMove = (strstr(line," w ")!=NULL) ? 'w':'b';
        }
        else if(strncmp(line,"go",2)==0){
            generate_one_move(sideToMove);
        }
        else if(strcmp(line,"quit")==0){
            break;
        }
    }

    return 0;
}
