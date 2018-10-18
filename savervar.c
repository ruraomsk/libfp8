/*
 * Библиотека работа с сохраненными константами 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "variables.h"

void initSaver(char *filename, short * arrayVarNom) {
    short *t = arrayVarNom;
    filenamesaver = filename;
    countSaveBuffer = 0;
    while (*t++ >= 0) {
        countSaveBuffer++;
    }
    if (countSaveBuffer == 0) return;
    //  создаем буфер для хранения переменных для записи
    sizeFile = countSaveBuffer * sizeof (Value);
    valueBuffer = (Value *) malloc(sizeFile);
    if (valueBuffer == NULL) {
        countSaveBuffer = 0;
        return;
    }
    memset(valueBuffer, 0, sizeFile);
    saveIdArray = arrayVarNom;
    // Заполняем оба буфера
    t = arrayVarNom;
    Value *val = valueBuffer;
    int id;
    while ((id = *t++) >= 0) {
        VarCtrl *var = findVariable(id);
        if (var == NULL) {
            countSaveBuffer = 0;
            free(valueBuffer);
            return;
        }
        //        memcpy(vr, var, sizeof (VarCtrl));
        val->idVariable = var->idVariable;
        setDataValue(val, var->value, 0);
        val++;
    }
    statusopen = 0;
    saver = fopen(filenamesaver, "rb");
    val = valueBuffer;
    if (saver == NULL) {
        // файла нет отметим это
        statusopen = -1;
    } else {
        statusopen = 0;
    }
    if (statusopen == 0) {
        Value temp;
        Value *val = valueBuffer;
        for (int i = 0; i < countSaveBuffer; i++) {
            VarCtrl *vr=findVariable(saveIdArray[i]);
            readValue(i, &temp);
            memcpy(vr->value, &temp.value, varLen(vr));
            memcpy(val->value.b, &temp.value, varLen(vr));
            val->error = 0;
            val++;
        }
    }
    if (statusopen == 0 || statusopen == 1) fclose(saver);
    saver = fopen(filenamesaver, "wb");
    Value *value = valueBuffer;
    for (int i = 0; i < countSaveBuffer; i++) {
        value->error = 0;
        writeRecord(i, value);
        value++;
    }
    fflush(saver);
}

int readValue(int recno, Value *value) {
    if (fseek(saver, recno * sizeof (Value), SEEK_SET) != 0) return 1;
    if (fread(value, sizeof (Value), 1, saver) != sizeof (Value)) return 1;
    return 0;
}

int writeRecord(int recno, Value *value) {
    if (fseek(saver, recno * sizeof (Value), SEEK_SET) != 0) return 1;
    if (fwrite(value, sizeof (Value), 1, saver) != sizeof (Value)) return 1;
    return 0;
}

void updateDataSaver() {
    if (countSaveBuffer == 0) return;
    Value *value = valueBuffer;

    for (int i = 0; i < countSaveBuffer; i++) {
        VarCtrl *vctrl = findVariable(saveIdArray[i]);
        if (memcmp(vctrl->value, value->value.b, varLen(vctrl))) {
            memcpy(value->value.b, vctrl->value, varLen(vctrl));
            //            value->error = 1;
            writeRecord(i, value);
            needFlush = 1;
            //            printf("->%d\n",value->idVariable);
        }
        value++;
    }
    if (needFlush) {
        fflush(saver);
        needFlush = 0;
    }
}
