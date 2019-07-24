#include <sys/io.h>
// #include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include "emul8enc.h"

#define devaddr (drv->address)
// #define g_pDevData ((em8encpti_data *)(drv->data))
// #define g_pDevIni ((em8encpti_inimod *)(drv->inimod))

em8encpti_data *g_pDevData;
em8encpti_inimod *g_pDevIni;
table_drv* g_pDriver;

void em8encpti_ini(table_drv* drv) {
    drv->error = 0;
}

// unsigned char LatchOn(){
//     int tempLatch = inw(0x340 + 1);
//     return (tempLatch & 0x4) != 0 ?1 : 0; 
// }

int SearchBus() {
    int tempBus, filtBus[3], i, j = 0;
    while (1) {
        for (i = 0; i < 3; i++)
            filtBus[i] = inw(0x340 + 1);
        if ((filtBus[0] == filtBus[1]) && (filtBus[0] == filtBus[2])&& (filtBus[1] == filtBus[2])) {
            tempBus = (filtBus[0] & filtBus[1]) | (filtBus[1] & filtBus[2]) | (filtBus[0] & filtBus[2]);
            break;
        }
        if (j < 10)
            j++;
        else {
            tempBus = 0xee;
            break;
        }
    }
    switch (tempBus & 0xff) {
        case 0x01: return 1;
        case 0x02: return 2;
        case 0x04: return 3;
        case 0x08: return 4;
        case 0x10: return 5;
        case 0x20: return 6;
        case 0x40: return 7;
        case 0x80: return 8;
        default: return (tempBus & 0xff); // у нас нету выбранного баса!!
    }
}

void outEnc(int nBus) {
    switch (nBus) {
        case 1: g_pDevData->data[nBus] = 0x10101010;
            break;
        case 2: g_pDevData->data[nBus] = 0x20202020;
            break;
        case 3: g_pDevData->data[nBus] = 0x30303030;
            break;
        case 4: g_pDevData->data[nBus] = 0x40404040;
            break;
        case 5: g_pDevData->data[nBus] = 0x50505050;
            break;
        case 6: g_pDevData->data[nBus] = 0x60606060;
            break;
        case 7: g_pDevData->data[nBus] = 0x70707070;
            break;
        case 8: g_pDevData->data[nBus] = 0x80808080;
            break;
        default:
            syslog(LOG_ERR, "Encoder vrong BUS num - %d\n", nBus);
            g_pDevData->data[nBus] = 0xaaaaaaaa;
    }
    outw(g_pDevData->data[nBus], g_pDriver->address);
    outw(g_pDevData->data[nBus] >> 16, g_pDriver->address + 2);
}

void em8enc_dw(table_drv* drv) {
    int waitBus;
    unsigned char oldBus = 0, tempBus;
    g_pDevData = (em8encpti_data *) drv->data;
    g_pDevIni = (em8encpti_inimod *) drv->inimod;
    g_pDriver = drv;

    // g_pDevData->data[0] = 0xaaddbbcc;
    int i, j;

    // while(LatchOn()){
    for (waitBus = 0; waitBus < 255; waitBus++) {
        tempBus = SearchBus();
        if ((oldBus != tempBus) && (tempBus < 9)) {
            outEnc(tempBus);
            oldBus = tempBus;
            break;
        }
    }
    // }
}




