/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   do32_pti.h
 * Author: rusin
 *
 * Created on 27 марта 2018 г., 10:44
 */

#ifndef DO32_PTI_H
#define DO32_PTI_H

#include "../typedef.h"
#include "../drvio.h"

#define DO32_PTI  0x02

void do32_ini(table_drv* drv);
void do32_dw(table_drv* drv);

typedef struct __attribute__((packed)) 
{
  ssbool data[32];
  ssbool sost[32];
  unsigned long old;
  unsigned char initok;
  char emptyspace[64];
} do32_data;

typedef struct __attribute__((packed)) 
{
  unsigned char init;
}do32_pti_inipar;

#endif /* DO32_PTI_H */
