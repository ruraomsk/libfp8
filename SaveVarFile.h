#ifndef SaveVarFile_H
#define SaveVarFile_H

int FileOpen();
int FileClose();
void FileSaveVarInt(char * str, int id);
void FileSaveVarDouble(char * str, double id);


#endif /* SaveVarFile_H */

