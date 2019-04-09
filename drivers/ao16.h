/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ao16.h
 * Author: rusin
 *
 * Created on 27 марта 2018 г., 12:17
 */

#ifndef AO16_H
#define AO16_H
#include "../typedef.h"
#include "../drvio.h"
#define AO16FRDY  0x80
#define AO16FAOR  0x20
#define AO16FCHSI 0x80
#define AO16_PTI   0x04
#define AO16FERRNORDY    0x01
#define AO16FERRNOAOR    0x02
#define AO16FERRNOAOR2   0x04
#define AO16FNOCONTROL   0xff

typedef struct __attribute__ ((packed))
{ unsigned short workmsk; // маска рабочих каналов
  unsigned char aoeto;    // макс. допустимое время ожидания условий для выдачи сигнала (RDY && !AOR)
  unsigned char ao16id[2]; //идентификатор конфигурации модуля AO16 (['C','V'],[8,16])
  unsigned short ao161stout[16]; // первоначально выдаваемый код
} ao16_inimod;

typedef struct __attribute__ ((packed))
{ ssint adata[16];
  sschar asost[16];
  unsigned short aold[16];
} ao16_data;

void ao16_ini(table_drv* drv);
void ao16_dw(table_drv* drv);

#endif /* AO16_H */
