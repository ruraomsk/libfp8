/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   vds32_pti.h
 * Author: rusin
 *
 * Created on 27 марта 2018 г., 12:08
 */

#ifndef VDS32_PTI_H
#define VDS32_PTI_H
#define VDS32_PTI 0x03
#include "../typedef.h"
#include "../drvio.h"

void vds32pti_ini(table_drv* drv);
void vds32pti_dr(table_drv* drv);

typedef struct __attribute__((packed))
{ 
  unsigned char tadr; 
} vds32pti_inimod;

typedef struct __attribute__((packed))
{
  ssbool data[32];
  unsigned char workok;
} vds32pti_data;

#endif /* VDS32_PTI_H */
