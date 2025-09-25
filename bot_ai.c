#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct{int a,b,c,d,cap,prom;} M;
char B[8][8],S='w';

#define O(r,f) ((r)>=0&&(r)<8&&(f)>=0&&(f)<8)
int P(p,side,opp){return p!='.' && isupper(p) ^ side=='b' ^ opp;}

reset(){for(int i=64;i--;)B[i/8][i%8]="rnbqkbnrpppppppp................................PPPPPPPPRNBQKBNR"[i];S='w';}

am(M m){char pc=B[m.a][m.b];B[m.c][m.d]=m.prom?m.prom:pc;B[m.a][m.b]='.';S^='w'^'b';}
um(M m){char pc=B[m.c][m.d];B[m.a][m.b]=m.prom?(isupper(pc)?'P':'p'):pc;B[m.c][m.d]=m.cap;S^='w'^'b';}

pv(c){return c=='P'?10:c=='N'||c=='B'?30:c=='R'?50:c=='Q'?90:900;}
eval(){int s=0;for(int i=0;i<64;i++){char c=B[i/8][i%8];if(c!='.')s+=(isupper(c)?1:-1)*pv(toupper(c));}return s;}

gen_moves(char s,M*m){
    int n=0, drN[]={-2,-1,1,2,2,1,-1,-2}, dfN[]={1,2,2,1,-1,-2,-2,-1}, drB[]={-1,-1,1,1}, dfB[]={-1,1,1,-1}, drR[]={-1,1,0,0}, dfR[]={0,0,-1,1}, drQ[]={-1,-1,-1,0,1,1,1,0}, dfQ[]={-1,0,1,1,1,0,-1,-1};
    for(int r=0;r<8;r++)for(int f=0;f<8;f++){
        char p=B[r][f]; if(!P(p,s,0)) continue;
        switch(toupper(p)){
            case 'P': {
                int d=s=='w'? -1:1;
                if(O(r+d,f)&&B[r+d][f]=='.') m[n++] = (M){r,f,r+d,f,'.',0};
                if(r==(s=='w'?6:1) && B[r+d][f]=='.' && B[r+2*d][f]=='.') m[n++] = (M){r,f,r+2*d,f,'.',0};
                for(int df=-1;df<=1;df+=2){int nr=r+d,nf=f+df;if(O(nr,nf)&&P(B[nr][nf],s,1)) m[n++] = (M){r,f,nr,nf,B[nr][nf],0};}
            } break;
            case 'N': for(int i=0;i<8;i++){int nr=r+drN[i],nf=f+dfN[i]; if(O(nr,nf)&&!P(B[nr][nf],s,0)) m[n++] = (M){r,f,nr,nf,B[nr][nf],0};} break;
            case 'B': case 'R': case 'Q': {
                int *dr, *df, len;
                if(toupper(p)=='B'){dr=drB; df=dfB; len=4;}
                else if(toupper(p)=='R'){dr=drR; df=dfR; len=4;}
                else{dr=drQ; df=dfQ; len=8;}
                for(int i=0;i<len;i++){
                    for(int nr=r+dr[i], nf=f+df[i]; O(nr,nf); nr+=dr[i], nf+=df[i]){
                        if(P(B[nr][nf],s,0)) break;
                        m[n++] = (M){r,f,nr,nf,B[nr][nf],0};
                        if(P(B[nr][nf],s,1)) break;
                    }
                }
            } break;
            default: for(int i=0;i<8;i++){int nr=r+drQ[i], nf=f+dfQ[i]; if(O(nr,nf)&&!P(B[nr][nf],s,0)) m[n++] = (M){r,f,nr,nf,B[nr][nf],0};} break;
        }
    }
    return n;
}

gen_legal(char s,M*o){
    M m[256],v[256];int n=gen_moves(s,m),c=0,i,j,k;
    for(i=0;i<n;i++){
        am(m[i]);
        for(j=0;j<64;j++) if(B[j/8][j%8]==(s=='w'?'K':'k')) break;
        for(k=gen_moves(s^'w'^'b',v);k--;) if(v[k].c==j/8&&v[k].d==j%8) goto x;
        o[c++]=m[i];
        x:;
        um(m[i]);
    }
    return c;
}

minimax(d,a,b,m){
    if(!d) return eval();
    M x[256]; int n=gen_legal(S,x); if(!n) return eval();
    int v=m==S?-100000:100000;
    for(int i=n;i--;){ am(x[i]); int e=minimax(d-1,a,b,m); um(x[i]);
        if(m==S?e>v:e<v)v=e;
        if(m==S&&v>a)a=v;
        if(m!=S&&v<b)b=v;
        if(b<=a)break;
    }
    return v;
}

best(){
    M mv[256]; int i,b=0,n=gen_legal(S,mv),e=S=='w'?-100000:100000;
    for(i=0;i<n;i++){ am(mv[i]); int v=minimax(3,-100000,100000,S); um(mv[i]); if(v>e^S=='b') e=v,b=i; }
    if(n){M*p=&mv[b]; printf("bestmove %c%d%c%d\n",'a'+p->b,8-p->a,'a'+p->d,8-p->c); fflush(stdout);}
}

main(){
    char l[512];
    while(fgets(l,512,stdin)){
        l[strcspn(l,"\r\n")]=0;
        if(!strcmp(l,"uci")) printf("id name bot\nid author s\nuciok\n"),fflush(stdout);
        if(!strcmp(l,"isready")) printf("readyok\n"),fflush(stdout);
        if(!strcmp(l,"ucinewgame")) reset();
        if(!strncmp(l,"position",8)){
            if(strstr(l,"startpos")) reset();
            char*ms=strstr(l,"moves");
            if(ms){
                ms+=6;
                char*t=strtok(ms," ");
                while(t){
                    int a=t[0]-'a',b='8'-t[1],c=t[2]-'a',d='8'-t[3];
                    char e=B[b][a]; B[d][c]=e; B[b][a]='.';
                    if(strlen(t)==5) B[d][c]=isupper(e)?toupper(t[4]):tolower(t[4]);
                    S^='w'^'b';
                    t=strtok(0," ");
                }
            }
        }
        if(!strncmp(l,"go",2)) best();
        if(!strcmp(l,"quit")) break;
    }
}
