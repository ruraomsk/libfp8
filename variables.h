/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   variables.h
 * Author: rusin
 *
 * Created on 27 марта 2018 г., 10:14
 */

#ifndef VARIABLES_H
#define VARIABLES_H
#include <stdio.h>
#include <stdlib.h>

#define boolean 1   //1 byte
#define uint2b  2   //2 byte unsined short
#define sint2b  3   //2 byte short
#define uint4b  4   //4 byte unsigned int
#define sint4b  5   //4 byte int
#define float4b 8   //4 byte float
#define sint8b  11  //8 byte long long int
#define float8b 14  //8 byte double 
#define char1b  18  // 1 byte not logical

typedef struct __attribute__((packed)){
    int idVariable;
    union {
        char b[sizeof (double)];
        short s[4];
        int i[2];
        float f[2];
        long long int l;
        double d;
    } value;
    char error;
} Value;
typedef struct __attribute__((packed)){
    int idVariable;
    char format;
    short size;
    char * value;
} VarCtrl;
typedef struct __attribute__((packed)){
    union {
        char b[sizeof (double)];
        short s[4];
        int i[2];
        float f[2];
        long long int l;
        double d;
    } value;
} Convert;

static char *filenamesaver;
static short *saveIdArray;
static Value *valueBuffer;
static VarCtrl *varCtrlBuffer;
static int countSaveBuffer;
static int sizeFile;
static FILE *saver;
static char statusopen;
static char needFlush=0;


/* Функция инициализации массива структур описаний переменных 
 * вычисляет длину массива и сортирует его по возрастанию id 
 */
int varCompare(const void *c1,const void *c2);
int initVariableTable(VarCtrl *vct);
/* Возвращает указатель  в буфере  описания если она найдена
 * NULL если не найдена     
 */
VarCtrl * findVariable(int idVar);
/*
 * Создает и зачищает перемеенную общего вида (в которой могут хранится любые значения переменных
 * возвращает указатель на неею Если не нужна то удаляется фунцией destroyValue
 */
Value * newValue(VarCtrl *vc);
/* Удаляет переменную общего вида
 */
void destroyValue(Value *val);
int getDataValue(Value *val, void *value);
int setDataValue(Value *val, void *value, char error);
int setData(int idVar, void* val);
int moveData(int idVar, int idVarSrc);
/* создает символьное представление значения переменной общего вида 
 */
char *valueToString(Value *val);
char *variableToString(short id);
char *variableArrayToString(short id,int index);
/*
 * Записывает в буфер значений только значение по id переменной
 * 0 - нет такой переменной
 * 1 - операция успешна 
 */
int setDataVariable(int idVar, void* val, char error);
int setArrayDataVariable(int idVar, void* val, char error,int index);
/*
 * Записывает в буфер значений только состояние байта достоверности по id переменной 
 * 0 - нет такой переменной
 * 1 - операция успешна 
 */
int getDataVariable(int idVar, void* val);
/*
 * Читает из буфера значений только состояние байта достоверности по id переменной
 */
char getErrorVariable(int idVar);
/* Сравнение на равно двух разных значений данных по номеру Id
 * 0 - равны
 * не 0 в противном случае
 */
//int doCompare(int idVar, void *v1, void *v2);

char getAsBool(int idVar);
short getAsShort(int idVar);
int getAsInt(int idVar);
float getAsFloat(int idVar);
double getAsDouble(int idVar);
long long int getAsLong(int idVar);
char setAsBool(int idVar,char ch);
short setAsShort(int idVar,short sh);
int setAsInt(int idVar,int in);
float setAsFloat(int idVar,float fl);
double setAsDouble(int idVar,double db);
long long int setAsLong(int idVar,long long int ll);

void initSaver(char *filename, short * arrayVarNom);
void updateDataSaver(void);
int readValue(int recno, Value *value);
int writeRecord(int recno, Value *value);


int varLen(VarCtrl *vc);
int byfferLen(VarCtrl *vc);
void swapBytes(const void *src, void *dst,int len);
void stringToVariable(short id, char *value);
#endif /* VARIABLES_H */
