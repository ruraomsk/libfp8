#include <stdio.h>
#include <stdlib.h>
#include "SaveVarFile.h"
#include <sys/time.h>
#include <time.h>

FILE *BufVar;
struct tm * timeinfo;
   
void timeV(){
    time_t rawtime;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
}

int FileOpen(char * name) {
    BufVar = fopen(name, "a");
    if (BufVar == NULL) {
        return EXIT_FAILURE;
    }
    return 0;
}

void FileSaveVarInt(char * str, int id) {
    FileOpen(str);
    fprintf(BufVar, "%d\n", id);
    FileClose();
}

void FileSaveVarDouble(char * str, double id) {
    FileOpen(str);
    timeV();
    fprintf(BufVar, "%lf\n", id);
    FileClose();
}

int FileClose() {
    fclose(BufVar);
    return 0;
}
