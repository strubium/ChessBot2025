#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define BOARD_SIZE 8
#define MAX_MOVES 256
#define MAX_DEPTH 3

// --- Board ---
char board[BOARD_SIZE][BOARD_SIZE];
char sideToMove = 'w'; // 'w' = white, 'b' = black

// --- Helper functions ---
bool on_board(int r, int f) { return r>=0 && r<BOARD_SIZE && f>=0 && f<BOARD_SIZE; }

bool is_piece_of(char piece, char side, bool opponent) {
    return piece != '.' && ((isupper(piece) ^ (side=='b')) ^ opponent);
}

// --- Reset board ---
void reset_board() {
    const char *start[BOARD_SIZE] = {
        "rnbqkbnr","pppppppp","........","........",
        "........","........","PPPPPPPP","RNBQKBNR"
    };
    for(int r=0;r<BOARD_SIZE;r++)
        for(int f=0;f<BOARD_SIZE;f++)
            board[r][f] = start[r][f];
    sideToMove = 'w';
}

// --- Move struct ---
typedef struct { int from_r, from_f, to_r, to_f; char captured; char promoted; } Move;

void apply_move_struct(Move m) {
    char piece = board[m.from_r][m.from_f];
    board[m.to_r][m.to_f] = (m.promoted) ? m.promoted : piece;
    board[m.from_r][m.from_f] = '.';
    sideToMove = (sideToMove=='w') ? 'b' : 'w';
}

void undo_move_struct(Move m) {
    char piece = board[m.to_r][m.to_f];
    board[m.from_r][m.from_f] = (m.promoted) ? ((isupper(piece))?'P':'p') : piece;
    board[m.to_r][m.to_f] = m.captured;
    sideToMove = (sideToMove=='w') ? 'b' : 'w';
}

// --- Board evaluation ---
int evaluate_board() {
    int piece_value[128] = { ['P']=10, ['N']=30, ['B']=30, ['R']=50, ['Q']=90, ['K']=900 };
    int score = 0;
    for(int r=0;r<BOARD_SIZE;r++)
        for(int f=0;f<BOARD_SIZE;f++){
            char c = board[r][f];
            if(c!='.')
                score += (isupper(c)?1:-1) * piece_value[(int)toupper(c)];
        }
    return score;
}

// --- Generate legal moves (simplified) ---
int generate_pawn_moves(int r, int f, char side, Move *moves) {
    int dir = (side=='w')?-1:1;
    int count=0;
    if(on_board(r+dir,f) && board[r+dir][f]=='.'){
        moves[count++] = (Move){r,f,r+dir,f,'.',0};
        if((r==(side=='w'?6:1)) && board[r+2*dir][f]=='.')
            moves[count++] = (Move){r,f,r+2*dir,f,'.',0};
    }
    for(int df=-1;df<=1;df+=2){
        int nf = f+df, nr = r+dir;
        if(on_board(nr,nf) && is_piece_of(board[nr][nf],side, true))
            moves[count++] = (Move){r,f,nr,nf,board[nr][nf],0};
    }
    return count;
}

int generate_knight_moves(int r,int f,char side,Move *moves){
    int count=0;
    int dr[BOARD_SIZE]={-2,-1,1,2,2,1,-1,-2};
    int df[BOARD_SIZE]={1,2,2,1,-1,-2,-2,-1};
    for(int i=0;i<BOARD_SIZE;i++){
        int nr=r+dr[i], nf=f+df[i];
        if(on_board(nr,nf) && !is_piece_of(board[nr][nf],side, false))
            moves[count++] = (Move){r,f,nr,nf,board[nr][nf],0};
    }
    return count;
}

int generate_sliding_moves(int r,int f,char side,Move *moves,int dr[],int df[],int n){
    int count=0;
    for(int i=0;i<n;i++){
        int nr=r+dr[i], nf=f+df[i];
        while(on_board(nr,nf)){
            if(is_piece_of(board[nr][nf],side, false)) break;
            moves[count++] = (Move){r,f,nr,nf,board[nr][nf],0};
            if(is_piece_of(board[nr][nf],side, true)) break;
            nr += dr[i]; nf += df[i];
        }
    }
    return count;
}

int generate_all_moves(char s,Move*m){
    int c=0;
    int drB[]={-1,-1,1,1}, dfB[]={-1,1,1,-1};
    int drR[]={-1,1,0,0}, dfR[]={0,0,-1,1};
    int drQ[]={-1,-1,-1,0,1,1,1,0}, dfQ[]={-1,0,1,1,1,0,-1,-1};
    for(int r=0;r<BOARD_SIZE;r++)
        for(int f=0;f<BOARD_SIZE;f++){
            char p=board[r][f]; if(!is_piece_of(p,s,0)) continue;
            int n=0;
            switch(toupper(p)){
                case 'P': n=generate_pawn_moves(r,f,s,m+c); break;
                case 'N': n=generate_knight_moves(r,f,s,m+c); break;
                case 'B': n=generate_sliding_moves(r,f,s,m+c,drB,dfB,4); break;
                case 'R': n=generate_sliding_moves(r,f,s,m+c,drR,dfR,4); break;
                case 'Q': n=generate_sliding_moves(r,f,s,m+c,drQ,dfQ,8); break;
                case 'K': { int drK[]={-1,-1,-1,0,1,1,1,0}, dfK[]={-1,0,1,1,1,0,-1,-1};
                    for(int i=0;i<8;i++){
                        int nr=r+drK[i], nf=f+dfK[i];
                        if(on_board(nr,nf) && !is_piece_of(board[nr][nf],s,0))
                            m[c++] = (Move){r,f,nr,nf,board[nr][nf],0};
                    }
                }
            }
            c+=n;
        }
    return c;
}

// --- King position and check detection ---
void find_piece(char p,int *r,int *f){
    for(int i=0;i<64;i++)
        if(board[i/8][i%8]==p){*r=i/8;*f=i%8;return;}
}


bool square_attacked(int r, int f, char by_side){
    Move moves[MAX_MOVES];
    int n = generate_all_moves(by_side,moves);
    for(int i=0;i<n;i++)
        if(moves[i].to_r==r && moves[i].to_f==f)
            return true;
    return false;
}

bool in_check(char side){
    int kr,kf;
    find_piece(side=='w'?'K':'k', &kr, &kf);
    return square_attacked(kr,kf,(side=='w')?'b':'w');
}

// --- Legal move generation ---
int generate_legal_moves(char side, Move *legal_moves){
    Move moves[MAX_MOVES];
    int n = generate_all_moves(side,moves);
    int count=0;
    for(int i=0;i<n;i++){
        apply_move_struct(moves[i]);
        if(!in_check(side))
            legal_moves[count++] = moves[i];
        undo_move_struct(moves[i]);
    }
    return count;
}

// --- Minimax ---
int minimax(int d,int a,int b,char m){
    if(d==0) return evaluate_board();
    Move mv[MAX_MOVES];
    int n=generate_legal_moves(sideToMove,mv);
    if(!n) return evaluate_board();
    int val=m==sideToMove?-100000:100000;
    for(int i=0;i<n;i++){
        apply_move_struct(mv[i]);
        int e=minimax(d-1,a,b,m);
        undo_move_struct(mv[i]);
        if(m==sideToMove?(e>val):(e<val)) val=e;
        if(m==sideToMove && val>a) a=val; else if(m!=sideToMove && val<b) b=val;
        if(b<=a) break;
    }
    return val;
}

// --- Generate best move ---
void generate_best_move(){
    Move moves[MAX_MOVES];
    int n = generate_legal_moves(sideToMove,moves);
    int bestEval = (sideToMove=='w')?-100000:100000;
    int bestIdx = 0;
    for(int i=0;i<n;i++){
        apply_move_struct(moves[i]);
        int eval = minimax(MAX_DEPTH,-100000,100000,sideToMove);
        undo_move_struct(moves[i]);
        if((sideToMove=='w' && eval>bestEval) || (sideToMove=='b' && eval<bestEval)){
            bestEval=eval;
            bestIdx=i;
        }
    }
    if(n>0){
        Move m = moves[bestIdx];
        printf("bestmove %c%d%c%d\n",'a'+m.from_f,8-m.from_r,'a'+m.to_f,8-m.to_r);
        fflush(stdout);
    }
}

// --- Main UCI loop ---
int main(void){
    char line[1024];

    while(fgets(line,sizeof(line),stdin)){
        line[strcspn(line,"\r\n")]=0;

        if(strcmp(line,"uci")==0){
            printf("id name bot\nid author s\nuciok\n"); fflush(stdout);
        }
        else if(strcmp(line,"isready")==0){
            printf("readyok\n"); fflush(stdout);
        }
        else if(strcmp(line,"ucinewgame")==0){
            reset_board();
        }
        else if(strncmp(line,"position",8)==0){
            if(strstr(line,"startpos")) reset_board();
            char *moves_str = strstr(line,"moves");
            if(moves_str){
                moves_str+=6;
                char *tok=strtok(moves_str," ");
                int a,b,c,d; char e;
                while(tok)
                    a=tok[0]-'a', b='8'-tok[1],
                    c=tok[2]-'a', d='8'-tok[3],
                    e=board[b][a],
                    board[d][c]=e, board[b][a]='.',
                    strlen(tok)==5?(board[d][c]=(isupper(e)?toupper(tok[4]):tolower(tok[4]))):0,
                    sideToMove=sideToMove=='w'?'b':'w',
                    tok=strtok(NULL," ");
            }
        }
        else if(strncmp(line,"go",2)==0){
            generate_best_move();
        }
        else if(strcmp(line,"quit")==0){
            break;
        }
    }
    return 0;
}
