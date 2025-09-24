#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct{int a,b,c,d;char cap,prom;}M;
char B[8][8],S='w';

#define O(r,f) ((r)>=0&&(r)<8&&(f)>=0&&(f)<8)
int P(char p,char side,int opp){return p!='.' && isupper(p) ^ side=='b' ^ opp;}

void reset(){char*s="rnbqkbnrpppppppp................................PPPPPPPPRNBQKBNR";for(int i=0;i<64;i++)B[i/8][i%8]=s[i];S='w';}

void am(M m){char pc=B[m.a][m.b];B[m.c][m.d]=m.prom?m.prom:pc;B[m.a][m.b]='.';S ^= 'w'^'b';}
void um(M m){char pc=B[m.c][m.d];B[m.a][m.b]= m.prom ? (isupper(pc)?'P':'p') : pc; B[m.c][m.d]=m.cap; S ^= 'w'^'b';}

int pv(c){
 return c=='P'?10:
        c=='N'||c=='B'?30:
        c=='R'?50:
        c=='Q'?90:900;
}

int eval(){int s=0;for(int i=0;i<64;i++){char c=B[i/8][i%8]; if(c!='.') s+=(isupper(c)?1:-1)*pv(toupper(c));}return s;}

int gen_p(int r,int f,char side,M*m){
 int d=(side=='w')?-1:1,cnt=0;
 if(O(r+d,f)&&B[r+d][f]=='.'){m[cnt++]=(M){r,f,r+d,f,'.',0}; if(r==(side=='w'?6:1)&&B[r+2*d][f]=='.')m[cnt++]=(M){r,f,r+2*d,f,'.',0};}
 for(int df=-1;df<=1;df+=2){int nr=r+d,nf=f+df;if(O(nr,nf)&&P(B[nr][nf],side,1))m[cnt++]=(M){r,f,nr,nf,B[nr][nf],0};}
 return cnt;
}

int gen_n(int r,int f,char side,M*m){
 char dr[]={-2,-1,1,2,2,1,-1,-2},df[]={1,2,2,1,-1,-2,-2,-1};int c=0;
 for(int i=0;i<8;i++){int nr=r+dr[i],nf=f+df[i]; if(O(nr,nf)&&!P(B[nr][nf],side,0)) m[c++]=(M){r,f,nr,nf,B[nr][nf],0};}
 return c;
}

int gen_slide(int r,int f,char side,M*m,char dr[],char df[],int n){
 int c=0;
 for(int i=0;i<n;i++){int nr=r+dr[i],nf=f+df[i];while(O(nr,nf)){ if(P(B[nr][nf],side,0)) break; m[c++]=(M){r,f,nr,nf,B[nr][nf],0}; if(P(B[nr][nf],side,1)) break; nr+=dr[i]; nf+=df[i];}}
 return c;
}

int gen_all(char side,M*m){
 int c=0;
 char dB[]={-1,-1,1,1},fB[]={-1,1,1,-1}, dR[]={-1,1,0,0},fR[]={0,0,-1,1}, dQ[]={-1,-1,-1,0,1,1,1,0}, fQ[]={-1,0,1,1,1,0,-1,-1}, dK[]={-1,-1,-1,0,1,1,1,0}, fK[]={-1,0,1,1,1,0,-1,-1};
 for(int r=0;r<8;r++)for(int f=0;f<8;f++){char p=B[r][f]; if(!P(p,side,0)) continue; int n=0;
  switch(toupper(p)){
   case'N': n=gen_n(r,f,side,m+c); break;
   case'B': n=gen_slide(r,f,side,m+c,dB,fB,4); break;
   case'R': n=gen_slide(r,f,side,m+c,dR,fR,4); break;
   case'Q': n=gen_slide(r,f,side,m+c,dQ,fQ,8); break;
   case'K': { for(int i=0;i<8;i++){int nr=r+dK[i],nf=f+fK[i]; if(O(nr,nf)&&!P(B[nr][nf],side,0)) m[c++]=(M){r,f,nr,nf,B[nr][nf],0}; } }
   default: n=gen_p(r,f,side,m+c); break;
  }
  c+=n;
 }
 return c;
}

void findp(char q,int *r,int *f){for(int i=0;i<64;i++) if(B[i/8][i%8]==q){*r=i/8;*f=i%8;return;}}

int attacked(r,f,by){
 M mv[256]; int n=gen_all(by,mv); for(int i=0;i<n;i++) if(mv[i].c==r && mv[i].d==f) return 1; return 0;
}
int inchk(side){int kr,kf; findp(side=='w'?'K':'k',&kr,&kf); return attacked(kr,kf,side^'w'^'b');}

int gen_legal(char side,M*out){
 M mv[256]; int n=gen_all(side,mv),cnt=0;
 for(int i=0;i<n;i++){ am(mv[i]); if(!inchk(side)) out[cnt++]=mv[i]; um(mv[i]); }
 return cnt;
}

int minimax(d,a,b,m){
 if(d==0) return eval();
 M mv[256]; int n=gen_legal(S,mv); if(!n) return eval();
 int val = m==S?-100000:100000;
 for(int i=0;i<n;i++){ am(mv[i]); int e=minimax(d-1,a,b,m); um(mv[i]); if(m==S? (e>val):(e<val)) val=e; if(m==S&&val>a) a=val; else if(m!=S&&val<b) b=val; if(b<=a) break; }
 return val;
}

void best(){
 int i,b=0,e=S=='w'?-100000:100000; M mv[256]; int n=gen_legal(S,mv);
 for(i=0;i<n;i++){ am(mv[i]); int v=minimax(3,-100000,100000,S); um(mv[i]); if(v>e^S=='b') e=v,b=i; }
 if(n){M*p=&mv[b]; printf("bestmove %c%d%c%d\n",'a'+p->b,8-p->a,'a'+p->d,8-p->c); fflush(stdout);}
}

int main(void){
 char line[512];
 while(fgets(line,512,stdin)){
  line[strcspn(line,"\r\n")]=0;
  if(strcmp(line,"uci")==0){printf("id name bot\nid author s\nuciok\n");fflush(stdout);}
  if(strcmp(line,"isready")==0){printf("readyok\n");fflush(stdout);}
  if(strcmp(line,"ucinewgame")==0) reset();
  if(strncmp(line,"position",8)==0){
    if(strstr(line,"startpos")) reset();
    char*ms=strstr(line,"moves");
    if(ms){ ms+=6; char*t=strtok(ms," "); while(t){ int a=t[0]-'a',b='8'-t[1],c=t[2]-'a',d='8'-t[3]; char e=B[b][a]; B[d][c]=e; B[b][a]='.'; if(strlen(t)==5) B[d][c]=isupper(e)?toupper(t[4]):tolower(t[4]); S = S=='w'?'b':'w'; t=strtok(NULL," "); } }
  }
  if(strncmp(line,"go",2)==0) best();
  if(strcmp(line,"quit")==0) break;
 }
}
