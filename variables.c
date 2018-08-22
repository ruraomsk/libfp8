/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   variables.c
 * Author: rusin
 * 
 * Created on 27 марта 2018 г., 10:14
 */

/*
 *  Функции по работе с переменными
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "variables.h"

char str[120];

static int lenVarCtrl = -1; //колличество элементов в буфере описаний переменных

/*
 * Сравнение по id описания переменных    
 */
int varCompare(const void *c1, const void *c2) {
    VarCtrl *vc1 = (VarCtrl *) c1;
    VarCtrl *vc2 = (VarCtrl *) c2;
    //    printf("%d %d\n",vc1->idVariable,vc2->idVariable);
    if (vc1->idVariable == vc2->idVariable) return 0;
    if (vc1->idVariable < vc2->idVariable) return -1;
    return 1;
}

/*
 * Возвращает длину в байтах в зависимости от формата 
 */
int varLen(VarCtrl *vc) {
    switch (vc->format) {
        case boolean:
        case char1b:
            return 1;
        case uint2b:
        case sint2b:
            return 2;
        case uint4b:
        case sint4b:
        case float4b:
            return 4;
        case sint8b:
        case float8b:
            return 8;
    }
    return 0;
}
static VarCtrl *vct;

/*
 * Функция инициализации массива структур описаний переменных 
 * вычисляет длину массива и сортирует его по возрастанию id 
 */
int initVariableTable(VarCtrl *varctrl) {
    int i = 0;
    lenVarCtrl = 0;
    while (varctrl[i++].idVariable >= 0) {
        lenVarCtrl++;
    }
    if (lenVarCtrl == 0) return 1;
    // Сортируем массив
    qsort(varctrl, lenVarCtrl, sizeof (VarCtrl), varCompare);
    vct = varctrl;
    return 0;
}

/*
 * Возвращает указатель  в буфере  описания если она найдена
 * NULL если не найдена     
 */
VarCtrl * findVariable(int idVar) {
    VarCtrl target, *result;
    target.idVariable = idVar;
    result = bsearch(&target, vct, lenVarCtrl, sizeof (VarCtrl), varCompare);
    return result;
}

/*
 * Создает и зачищает перемеенную общего вида (в которой могут хранится любые значения переменных
 * возвращает указатель на неею Если не нужна то удаляется фунцией destroyValue
 */
Value * newValue(VarCtrl *vc) {
    Value *val = malloc(sizeof (Value));
    if (val == NULL) return NULL;
    memset(val, sizeof (Value), sizeof (char));
    val->idVariable = vc->idVariable;
    return val;
}

/* Удаляет переменную общего вида
 */
void destroyValue(Value *val) {
    free(val);
}

/*
 * Копирует знчение переменной  
 */
int getDataValue(Value *val, void *value) {
    VarCtrl *vc = findVariable(val->idVariable);
    if (vc == NULL) return 0;
    int len = varLen(vc);
    if (len == 0) return 0;
    memcpy(val->value.b, value, len);
    return 1;
}

/* Устанавливает значение переменной плюс пишет байт качества
 */
int setDataValue(Value *val, void *value, char error) {
    VarCtrl *vc = findVariable(val->idVariable);
    if (vc == NULL) return 0;
    int len = varLen(vc);
    if (len == 0) return 0;
    memcpy( val->value.b,value, len);
    val->error = error;
    return 1;
}

/* создает символьное представление значения переменной общего вида 
 */
char *variableToString(short id) {
    VarCtrl *vc = findVariable(id);
    if (vc == NULL) return NULL;
    //    int len=varLen(vc);
    //    if(len==0) return 0;
    Convert *cv = (Convert *) vc->value;
    switch (vc->format) {
        case boolean:
            sprintf(str, "%s", cv->value.b[0] == 0 ? "false" : "true");
            return str;
        case char1b:
            sprintf(str, "%hhx", cv->value.b[0]);
            return str;
        case uint2b:
        case sint2b:
            sprintf(str, "%d", (int) cv->value.s[0]);
            return str;
        case uint4b:
        case sint4b:
            sprintf(str, "%d", cv->value.i[0]);
            return str;
        case float4b:
            sprintf(str, "%f", cv->value.f[0]);
            return str;
        case sint8b:
            sprintf(str, "%lld", cv->value.l);
            return str;
        case float8b:
            sprintf(str, "%lf", cv->value.d);
            return str;
    }
    sprintf(str, "error format %d", (int) vc->format);
    return str;
}

/* создает символьное представление значения переменной общего вида 
 */
char *valueToString(Value *val) {
    VarCtrl *vc = findVariable(val->idVariable);
    if (vc == NULL) return NULL;
    //    int len=varLen(vc);
    //    if(len==0) return 0;
    switch (vc->format) {
        case boolean:
            sprintf(str, "%s", val->value.b[0] == 0 ? "false" : "true");
            return str;
        case char1b:
            sprintf(str, "%hhx", val->value.b[0]);
            return str;
        case uint2b:
        case sint2b:
            sprintf(str, "%d", (int) val->value.s[0]);
            return str;
        case uint4b:
        case sint4b:
            sprintf(str, "%d", val->value.i[0]);
            return str;
        case float4b:
            sprintf(str, "%f", val->value.f[0]);
            return str;
        case sint8b:
            sprintf(str, "%lld", val->value.l);
            return str;
        case float8b:
            sprintf(str, "%lf", val->value.d);
            return str;
    }
    sprintf(str, "error format %d", (int) vc->format);
    return str;
}

/*
 * Записывает в буфер значений только значение по id переменной
 * 0 - нет такой переменной
 * 1 - операция успешна 
 */
int setDataVariable(int idVar, void* val, char error) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    int len = varLen(vc);
    if (len == 0) return 0;
    memcpy(vc->value, val, len);
    *(vc->value + varLen(vc)) = error;
    return 1;
}
int setData(int idVar, void* val){
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    int len = varLen(vc);
    if (len == 0) return 0;
    memcpy(vc->value, val, len+1);
    return 1;
}
/*
 * Читает из буфера значений только значение по id переменной
 * 0 - нет такой переменной
 * 1 - операция успешна 
 */
int getDataVariable(int idVar, void* val) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    int len = varLen(vc);
    if (len == 0) return 0;
    memcpy(val, vc->value, len);
    return 1;
}

/*
 * Читает из буфера значений только состояние байта достоверности по id переменной
 */
char getErrorVariable(int idVar) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0xFF;
    return (char) *(vc->value + varLen(vc));
}

/* 
 * Сравнение на равно двух разных значений данных по номеру Id
 */
int doCompare(int idVar, void *v1, void *v2) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    int len = varLen(vc);
    if (len == 0) return 0;
    return memcmp(v1, v2, len);
}

/* Возвращает перемееную как bool
 */
char getAsBool(int idVar) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.b[0];
}

/* Возвращает перемееную как short
 */
short getAsShort(int idVar) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.s[0];
}

/* Возвращает перемееную как int
 */
int getAsInt(int idVar) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.i[0];
}

/* Возвращает перемееную как float
 */
float getAsFloat(int idVar) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.f[0];
}

/* Возвращает перемееную как double
 */
double getAsDouble(int idVar) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.d;
}

/* Возвращает перемееную как long
 */
long long int getAsLong(int idVar) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.l;
}

/* Устанваливает  перемееную плюс байт качества
 */
char setAsBool(int idVar, char ch) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.b[0] = ch;
}

/* Устанваливает  перемееную плюс байт качества
 */
short setAsShort(int idVar, short sh) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.s[0] = sh;
}

/* Устанваливает  перемееную плюс байт качества
 */
int setAsInt(int idVar, int in) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.i[0] = in;
}

/* Устанваливает  перемееную плюс байт качества
 */
float setAsFloat(int idVar, float fl) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.f[0] = fl;
}

/* Устанваливает  перемееную плюс байт качества
 */
double setAsDouble(int idVar, double db) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.d = db;
}

/* Устанваливает  перемееную плюс байт качества
 */
long long int setAsLong(int idVar, long long int ll) {
    VarCtrl *vc = findVariable(idVar);
    if (vc == NULL) return 0;
    Convert *cv = (Convert *) vc->value;
    return cv->value.l = ll;
}

void swapBytes(const void *src, void *dst, int len) {
    if (len == 1) return;

    struct p {

        union {
            char b[sizeof (double)];
            short s[4];
        } value;
    };
    struct p *sr = (struct p *) src;
    struct p *ds = (struct p *) dst;
    for (int i = 0; i < len; i += 2) {
        ds->value.b[i + 1] = sr->value.b[i];
        ds->value.b[i] = sr->value.b[i+1];
    }
    if (len == 2) return;
    //    short t=ds->value.s[0];
    //    ds->value.s[0]=ds->value.s[1];
    //    ds->value.s[1]=t;
    //    if(len==4) return;
}

void stringToVariable(short id, char *value) {
    VarCtrl *vc = findVariable(id);
    short sh;
    int in;
    float fl;
    long long int lin;
    double db;
    if (vc == NULL) return;
    //    int len=varLen(vc);
    //    if(len==0) return 0;
    switch (vc->format) {
        case boolean:
            if ((value[0] == 'T') || (value[0] == 't') || (value[0] == '1')) *vc->value = 1;
            else *vc->value = 0;
            return;
        case char1b:
            *vc->value = value[0];
            return;
        case uint2b:
        case sint2b:
//            sscanf(value, "%d", &sh);
            sscanf(value, "%hu", &sh);
            setAsShort(id, sh);
            return;
        case uint4b:
        case sint4b:
            sscanf(value, "%d", &in);
            setAsInt(id, in);
            return;
        case float4b:
            sscanf(value, "%f", &fl);
            setAsFloat(id, fl);
            return;
        case sint8b:
            sscanf(value, "%lld", &lin);
            setAsLong(id, lin);
            return;
        case float8b:
            sscanf(value, "%lf", &db);
            setAsDouble(id, db);
            return;
    }
}
