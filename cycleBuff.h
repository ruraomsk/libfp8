/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   cycleBuff.h
 * Author: rusin
 *
 * Created on 13 апреля 2018 г., 10:04
 */

#ifndef CYCLEBUFF_H
#define CYCLEBUFF_H

#include <sys/time.h>
#include <string.h>
#include "variables.h"

typedef struct __attribute__((packed)){
    int  idVariable;    // id
    int  idApertura;    // id
    int  size;          // кол-во значений в буфере
    char *name;         // имя переменной
} BufVar;

typedef struct __attribute__((packed)){
    int  idVariable;    // id
    void * head;        //первое значение
    void * start;       // начало буфера
    void *end;          // конец буфера
    void *tail;         // за последним значением
    int el_size;        // размер элемента смещение времени + значение + достоверность
    struct timeval tstart; // полное время записи первого значения
    char tvalue[sizeof (double)]; // текущее значение записанное в буфер
    char *name;         // имя переменной 
    char format;        // формат значений
    void * value;       // указатель на значение переменной 
    void * apertura;    // указатель на занчение апертуры
} CtrlVarBuf;

int initCycleBuffers( BufVar *);
void doCycleBuffers(void);
#endif /* CYCLEBUFF_H */
