#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct { int a,b,c,d,cap,prom; } M;
char B[8][8], S='w';
int drN[]={-2,-1,1,2,2,1,-1,-2}, dfN[]={1,2,2,1,-1,-2,-2,-1}, drB[]={-1,-1,1,1}, dfB[]={-1,1,1,-1}, drR[]={-1,1,0,0}, dfR[]={0,0,-1,1}, drQ[]={-1,-1,-1,0,1,1,1,0}, dfQ[]={-1,0,1,1,1,0,-1,-1}, uk = 100000;

int P(char p, char side, int opp) {
    return p != '.' && (isupper(p) ^ (side=='b') ^ opp);
}

void reset(void) {
    const char *setup = "rnbqkbnrpppppppp................................PPPPPPPPRNBQKBNR";
    for(int i=0;i<64;i++) B[i/8][i%8] = setup[i];
    S='w';
}

void mdo(M m,int u) {
    char pc = B[u?m.c:m.a][u?m.d:m.b];
    B[u?m.a:m.c][u?m.b:m.d] = m.prom ? (isupper(pc)?'P':'p') : pc;
    B[u?m.c:m.a][u?m.d:m.b] = u ? m.cap : '.';
    S ^= 'w' ^ 'b';
}

int pv(char c) {
    switch(c) {
        case 'P': return 10;
        case 'N':
        case 'B': return 30;
        case 'R': return 50;
        case 'Q': return 90;
        default: return 900;
    }
}

int eval(void) {
    int s=0;
    for(int i=0;i<64;i++){
        char c = B[i/8][i%8];
        if(c != '.') s += (isupper(c)?1:-1) * pv(toupper(c));
    }
    return s;
}

int gen_moves(char s, M*m) {
    int n=0;
    for(int r=0;r<8;r++){
        for(int f=0;f<8;f++){
            char p=B[r][f];
            if(!P(p,s,0)) continue;
            switch(toupper(p)){
                case 'P': {
                    int d = s=='w' ? -1 : 1;
                    if((r+d)>=0 && (r+d)<8 && B[r+d][f]=='.') m[n++] = (M){r,f,r+d,f,'.',0};
                    if((r==(s=='w'?6:1)) && B[r+d][f]=='.' && B[r+2*d][f]=='.') m[n++] = (M){r,f,r+2*d,f,'.',0};
                    for(int df=-1;df<=1;df+=2){
                        int nr=r+d,nf=f+df;
                        if(nr>=0 && nr<8 && nf>=0 && nf<8 && P(B[nr][nf],s,1)) m[n++] = (M){r,f,nr,nf,B[nr][nf],0};
                    }
                } break;
                case 'N':
                    for(int i=0;i<8;i++){
                        int nr=r+drN[i], nf=f+dfN[i];
                        if(nr>=0 && nr<8 && nf>=0 && nf<8 && !P(B[nr][nf],s,0))
                            m[n++] = (M){r,f,nr,nf,B[nr][nf],0};
                    } break;
                case 'B': case 'R': case 'Q': {
                    int *dr, *df, len;
                    if(toupper(p)=='B'){ dr=drB; df=dfB; len=4; }
                    else if(toupper(p)=='R'){ dr=drR; df=dfR; len=4; }
                    else { dr=drQ; df=dfQ; len=8; }
                    for(int i=0;i<len;i++){
                        for(int nr=r+dr[i], nf=f+df[i]; nr>=0 && nr<8 && nf>=0 && nf<8; nr+=dr[i], nf+=df[i]){
                            if(P(B[nr][nf],s,0)) break;
                            m[n++] = (M){r,f,nr,nf,B[nr][nf],0};
                            if(P(B[nr][nf],s,1)) break;
                        }
                    }
                } break;
                default:
                    for(int i=0;i<8;i++){
                        int nr=r+drQ[i], nf=f+dfQ[i];
                        if(nr>=0 && nr<8 && nf>=0 && nf<8 && !P(B[nr][nf],s,0))
                            m[n++] = (M){r,f,nr,nf,B[nr][nf],0};
                    } break;
            }
        }
    }
    return n;
}

int gen_legal(char s,M*o) {
    M m[256], v[256];
    int c=0;
    int i,j,k;
    for(i=0;i<gen_moves(s,m);i++){
        mdo(m[i],0);
        for(j=0;j<64;j++) if(B[j/8][j%8] == (s=='w'?'K':'k')) break;
        for(k=gen_moves(s^'w'^'b',v);k--;) if(v[k].c==j/8 && v[k].d==j%8) goto x;
        o[c++] = m[i];
        x:;
        mdo(m[i],1);
    }
    return c;
}

int minimax(int d,int a,int b,int m) {
    if(!d) return eval();
    M x[256];
    int n = gen_legal(S,x);
    if(!n) return eval();
    int v = m==S ? -uk : uk;
    for(int i=n;i--;){
        mdo(x[i],0);
        int e = minimax(d-1,a,b,m);
        mdo(x[i],1);
        if(m==S? e>v : e<v) v=e;
        if(m==S && v>a) a=v;
        if(m!=S && v<b) b=v;
        if(b<=a) break;
    }
    return v;
}

void best(void){
    M mv[256];
    int i,b=0,n=gen_legal(S,mv),e=S=='w'?-uk:uk;
    for(i=0;i<n;i++){
        mdo(mv[i],0);
        int v = minimax(3,-uk,uk,S);
        mdo(mv[i],1);
        if((v>e) ^ (S=='b')) { e=v; b=i; }
    }
    if(n){
        M *p = &mv[b];
        printf("bestmove %c%d%c%d\n",'a'+p->b,8-p->a,'a'+p->d,8-p->c);
        fflush(stdout);
    }
}

int main(void){
    char l[512];
    while(fgets(l,sizeof(l),stdin)){
        l[strcspn(l,"\r\n")] = 0;

        !strcmp(l,"uci")     ? (printf("id name bot\nid author s\nuciok\n"), fflush(stdout)) : 0;
        !strcmp(l,"isready") ? (printf("readyok\n"), fflush(stdout)) : 0;
        !strcmp(l,"ucinewgame") ? reset() : 0;

        if(!strncmp(l,"position",8)){
            strstr(l,"startpos") ? reset() : 0;
            char *ms=strstr(l,"moves");
            if(ms){
                ms+=6;
                char *t=strtok(ms," ");
                while(t){
                    int a=t[0]-'a', b='8'-t[1], c=t[2]-'a', d='8'-t[3];
                    char e=B[b][a];
                    B[d][c]=e;
                    B[b][a]='.';
                    if(strlen(t)==5) B[d][c]=isupper(e)?toupper(t[4]):tolower(t[4]);
                    S^='w'^'b';
                    t=strtok(NULL," ");
                }
            }
        }
        !strncmp(l,"go",2) ? best() : 0;
        if(!strcmp(l,"quit")) break;
    }
    return 0;
}
