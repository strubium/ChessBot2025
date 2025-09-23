#include <stdio.h>
#include <ctype.h>
#include <string.h>

int is_keyword(const char *s) {
    const char *kw[] = {
        "auto","break","case","char","const","continue","default","do",
        "double","else","enum","extern","float","for","goto","if","inline",
        "int","long","register","restrict","return","short","signed","sizeof",
        "static","struct","switch","typedef","union","unsigned","void","volatile","while","_Bool","_Complex","_Imaginary"
    };
    for(int i=0;i<sizeof(kw)/sizeof(kw[0]);i++) if(strcmp(s,kw[i])==0) return 1;
    return 0;
}

int main(int argc, char **argv) {
    if(argc<2){printf("Usage: %s file.c\n",argv[0]);return 1;}
    FILE *f=fopen(argv[1],"r");if(!f){perror("fopen");return 1;}
    int c, tokens=0;
    char buf[128]; int pos=0;
    while((c=fgetc(f))!=EOF){
        if(isspace(c)) continue;
        if(isalpha(c)||c=='_'){ // identifier or keyword
            buf[pos=0]=c;
            while((c=fgetc(f))!=EOF&&(isalnum(c)||c=='_')) buf[++pos]=c;
            buf[++pos]=0;
            tokens++;
        } else if(isdigit(c)){ // number literal
            while((c=fgetc(f))!=EOF&&(isalnum(c)||c=='.')){}
            tokens++;
            if(c==EOF) break; else ungetc(c,f);
        } else if(c=='"'||c=='\''){ // string or char literal
            int quote=c; tokens++;
            while((c=fgetc(f))!=EOF && c!=quote){
                if(c=='\\') fgetc(f);
            }
        } else { // operator/punctuator
            tokens++;
        }
    }
    fclose(f);
    printf("Token count: %d\n",tokens);
    return 0;
}
