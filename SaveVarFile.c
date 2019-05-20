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
    fprintf(BufVar, "%d:%d:%d %d\n",timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, id);
    FileClose();
}

void FileSaveVarDouble(char * str, double id) {
    FileOpen(str);
    timeV();
    fprintf(BufVar, "%d:%d:%d %lf\n",timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, id);
    FileClose();
}

int FileClose() {
    fclose(BufVar);
    return 0;
}
