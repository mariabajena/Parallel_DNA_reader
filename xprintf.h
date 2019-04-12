#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define URED  "\x1B[4m\x1B[31m"
#define UGRN  "\x1B[4m\x1B[32m"
#define UYEL  "\x1B[4m\x1B[33m"
#define UBLU  "\x1B[4m\x1B[34m"
#define MAG   "\x1B[1m\x1B[35m"
#define CYN   "\x1B[1m\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#include <unistd.h>

void xprintf(char* dna){

    int i,j,inprimer=0;
    for(i=0; i<strlen(dna); i++){
      usleep(10000);
        switch(dna[i]){
            case 'T': inprimer?printf(UGRN "T" RESET):printf(GRN "T" RESET); break;
            case 'C': inprimer?printf(UYEL "C" RESET):printf(YEL "C" RESET); break;
            case 'G': inprimer?printf(UBLU "G" RESET):printf(BLU "G" RESET); break;
            case 'A': if(&dna[i] == strstr(&dna[i], "AAAAA")){
                        for(j=0; j<5; j++,i++)
                            printf(CYN "A" RESET);
                        if(inprimer)
                             inprimer=0;
                        else inprimer=1;
                      }else
                        inprimer ? printf(URED "A" RESET) : printf(RED "A" RESET);
                      break;
            default:  printf(MAG "%c" RESET, dna[i]);
        }
        fflush(stdout);
      }
}
