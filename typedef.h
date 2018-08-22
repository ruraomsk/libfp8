/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   typedef.h
 * Author: rusin
 *
 * Created on 27 марта 2018 г., 10:47
 */

#ifndef TYPEDEF_H
#define TYPEDEF_H
typedef unsigned char uspaint8;
typedef signed char spaint8;

typedef struct __attribute__ ((packed)) {
    uspaint8 c;
    uspaint8 error;
}
sschar;

typedef struct __attribute__ ((packed)) {
    uspaint8 b;
    uspaint8 error;
}
ssbool;

typedef struct __attribute__ ((packed)) {
    short i;
    uspaint8 error;
}
ssint;

typedef struct __attribute__ ((packed)) {
    float f;
    uspaint8 error;
}
ssfloat;

typedef struct __attribute__((packed)){
  int l;
  uspaint8 error;
}sslong;

typedef sschar *pschar; 
typedef ssbool *psbool; 
typedef ssint *psint; 
typedef ssfloat *psfloat; 
typedef sslong *pslong; 

#endif /* TYPEDEF_H */

