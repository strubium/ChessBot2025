#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct{int a,b,c,d,cap,prom;}M;
char B[8][8],S='w';

#define O(r,f) ((r)>=0&&(r)<8&&(f)>=0&&(f)<8)
int P(p,side,opp){return p!='.' && isupper(p) ^ side=='b' ^ opp;}

reset(){for(int i=64;i--;)B[i/8][i%8]="rnbqkbnrpppppppp................................PPPPPPPPRNBQKBNR"[i];S='w';}

am(M m){char pc=B[m.a][m.b];B[m.c][m.d]=m.prom?m.prom:pc;B[m.a][m.b]='.';S^='w'^'b';}
um(M m){char pc=B[m.c][m.d];B[m.a][m.b]=m.prom?(isupper(pc)?'P':'p'):pc;B[m.c][m.d]=m.cap;S^='w'^'b';}

pv(c){
 return c=='P'?10:
        c=='N'||c=='B'?30:
        c=='R'?50:
        c=='Q'?90:900;
}

eval(){int s=0;for(int i=0;i<64;i++){char c=B[i/8][i%8]; if(c!='.') s+=(isupper(c)?1:-1)*pv(toupper(c));}return s;}

gen_p(int r,int f,char side,M*m){
 int d=side=='w'?-1:1,cnt=0;
 if(O(r+d,f)&&B[r+d][f]=='.'){m[cnt++]=(M){r,f,r+d,f,'.',0}; if(r==(side=='w'?6:1)&&B[r+2*d][f]=='.')m[cnt++]=(M){r,f,r+2*d,f,'.',0};}
 for(int df=-1;df<=1;df+=2){int nr=r+d,nf=f+df;if(O(nr,nf)&&P(B[nr][nf],side,1))m[cnt++]=(M){r,f,nr,nf,B[nr][nf],0};}
 return cnt;
}

gen_n(int r,int f,char side,M*m){
 int dr[]={-2,-1,1,2,2,1,-1,-2},df[]={1,2,2,1,-1,-2,-2,-1},c=0;
 for(int i=8;i--;){int nr=r+dr[i],nf=f+df[i]; if(O(nr,nf)&&!P(B[nr][nf],side,0)) m[c++]=(M){r,f,nr,nf,B[nr][nf],0};}
 return c;
}

gen_slide(int r,int f,char s,M*m,char dr[],char df[],int n){int c=0;for(int i=n;i--;)for(int nr=r+dr[i],nf=f+df[i];O(nr,nf);nr+=dr[i],nf+=df[i]){if(P(B[nr][nf],s,0))break;m[c++]=(M){r,f,nr,nf,B[nr][nf],0};if(P(B[nr][nf],s,1))break;}return c;}


int n;
gen_all(char s,M*m){
 n=0;
 char dB[]={-1,-1,1,1},fB[]={-1,1,1,-1},dR[]={-1,1,0,0},fR[]={0,0,-1,1},
 dQ[]={-1,-1,-1,0,1,1,1,0},fQ[]={-1,0,1,1,1,0,-1,-1};
 for(int r=0;r<8;r++)for(int f=0;f<8;f++){char p=B[r][f];if(P(p,s,0)){
  switch(p&~32){
   case'N':n+=gen_n(r,f,s,m+n);break;
   case'B':n+=gen_slide(r,f,s,m+n,dB,fB,4);break;
   case'R':n+=gen_slide(r,f,s,m+n,dR,fR,4);break;
   case'Q':n+=gen_slide(r,f,s,m+n,dQ,fQ,8);break;
   case'K':for(int i=0;i<8;i++){int nr=r+dQ[i],nf=f+fQ[i];
    if(O(nr,nf)&&!P(B[nr][nf],s,0))m[n++]=(M){r,f,nr,nf,B[nr][nf],0};}
   default:n+=gen_p(r,f,s,m+n);
  }}}
 return n;
}

attacked(r,f,by){
 M mv[256];
 for(n=gen_all(by,mv);n--;)if(mv[n].c==r&&mv[n].d==f)return 1;
 return 0;
}

findp(q,r,f)int *r,*f;{int i;for(i=0;i<64;i++)if(B[i/8][i%8]==q){*r=i/8;*f=i%8;return;}}

inchk(side){int kr,kf;findp(side=='w'?'K':'k',&kr,&kf);return attacked(kr,kf,side^'w'^'b');}


gen_legal(char side,M*out){
 M mv[256]; int n=gen_all(side,mv),cnt=0;
 for(int i=0;i<n;i++){ am(mv[i]); if(!inchk(side)) out[cnt++]=mv[i]; um(mv[i]); }
 return cnt;
}

minimax(d, a, b, m){
 if(!d)return eval();
 M x[256];int n=gen_legal(S,x);if(!n)return eval();
 int v=m==S?-100000:100000;
 for(int i=n;i--;){am(x[i]);int e=minimax(d-1,a,b,m);um(x[i]);
  if(m==S?e>v:e<v)v=e;
  if(m==S&&v>a)a=v;
  if(m!=S&&v<b)b=v;
  if(b<=a)break;
 }
 return v;
}

best(){
 M mv[256];int i,b=0,n=gen_legal(S,mv),e=S=='w'?-100000:100000;
 for(i=0;i<n;i++){ am(mv[i]); int v=minimax(3,-100000,100000,S); um(mv[i]); if(v>e^S=='b') e=v,b=i; }
 if(n){M*p=&mv[b]; printf("bestmove %c%d%c%d\n",'a'+p->b,8-p->a,'a'+p->d,8-p->c); fflush(stdout);}
}

main(){char l[512];while(fgets(l,512,stdin)){l[strcspn(l,"\r\n")]=0;
 if(!strcmp(l,"uci"))printf("id name bot\nid author s\nuciok\n"),fflush(stdout);
 if(!strcmp(l,"isready"))printf("readyok\n"),fflush(stdout);
 if(!strcmp(l,"ucinewgame"))reset();
 if(!strncmp(l,"position",8)){if(strstr(l,"startpos"))reset();char*ms=strstr(l,"moves");if(ms){ms+=6;char*t=strtok(ms," ");while(t){int a=t[0]-'a',b='8'-t[1],c=t[2]-'a',d='8'-t[3];char e=B[b][a];B[d][c]=e;B[b][a]='.';if(strlen(t)==5)B[d][c]=isupper(e)?toupper(t[4]):tolower(t[4]);S^='w'^'b';t=strtok(0," ");}}}
 if(!strncmp(l,"go",2))best();
 if(!strcmp(l,"quit"))break;}}
