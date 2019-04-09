/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   do32_pti.c
 * Author: rusin
 * 
 * Created on 27 марта 2018 г., 10:44
 */
#include <sys/io.h>
#include "do32_pti.h"
#define devaddr (drv->address)
#define devdata ((do32_data *)(drv->data))

extern char flag_ini;

void do32_ini(table_drv *drv) {
    unsigned char k;

    if (!flag_ini)
        devdata->old = 0ul;

    for (k = 0; k < 32; k++) {
        devdata->sost[k].b = (devdata->old & (1ul << k)) != 0;
        devdata->sost[k].error = 0;
    }

    outw((unsigned short) (devdata->old),devaddr );
    outw((unsigned short) (devdata->old >> 16),devaddr + 2);

    drv->error = 0;
}

int f2iz3(unsigned int *rd){
    if(rd[0] == rd[1]) return 0;
    if(rd[1] == rd[2]) return 1;
    if(rd[0] == rd[2]) return 0;
    return -1;
}

void do32_dw(table_drv *drv) {
    unsigned char k;
    unsigned long l;

    union _twoints {
        unsigned int l;
        unsigned short i[2];
    } rd[3], tmp;

    tmp.l = devdata->old;

    drv->error = 0;
    for (k = 0; k < 32; k++) {
        devdata->sost[k].b = 0;
        devdata->sost[k].error = 0;

//        if (devdata->data[k].error == 0) {
            if (devdata->data[k].b)
                tmp.l |= (1UL << k);
            else
                tmp.l &= ~(1UL << k);
//        }
    }

    outw( tmp.i[0],devaddr + 0);
    outw( tmp.i[1],devaddr + 2);

    rd[0].i[0] = inw(devaddr + 0);
    rd[0].i[1] = inw(devaddr + 2);
    rd[1].i[0] = inw(devaddr + 0);
    rd[1].i[1] = inw(devaddr + 2);
    rd[2].i[0] = inw(devaddr + 0);
    rd[2].i[1] = inw(devaddr + 2);

    devdata->old = tmp.l;

    int i=f2iz3(&rd[0].l);
    if(i<0) {
        drv->error = 0xc0;
        return;
    }

    rd[0].l = rd[i].l ^ tmp.l;
    if(rd[0].l==0) return;
    drv->error |= 0x80;
    
    for (k = 0; k < 32; k++) {// if(rd[0].l == 0)
        //   break;
        if ((rd[0].l & 1UL) != 0) {
            devdata->sost[k].error |= 0x80;
        }
        rd[0].l >>= 1;
    }

    // return; ?
}
