#include <sys/io.h>
#include "DoVds.h"
#include <stdio.h>
#include <syslog.h>

#define devaddr (drv->address)
#define devdata ((doVds_data *)(drv->data))
#define devinimod ((doVds_inimod *)(drv->inimod))

#define addrVDS 0x310
#define addrFDS 0x100
extern char flag_ini;

unsigned char vds32pti1_ini(int addr) {
    unsigned char kan;
    outb( 0,addr + 0);
    outb( 0,addr + 5 );
    outb( 0x00,addr + 6);
    outb( 0x00,addr + 13); /* bez preryvanii */
    return 0;
}

int sw(int num){
    switch (num) { 
        case 0: return 0x55555555;  //0x0101010101010101;
        case 1: return 0xAAAAAAAA;  //0x1010101010101010;
        case 2: return 0xf0f0f0f0;  //0x1111000011110000;
        case 3: return 0x0f0f0f0f;  //0x0000111100001111;
        case 4: return 0xffff0000;  //0x1111111100000000;
        case 5: return 0x0000ffff;  //0x0000000011111111;
        case 6: return 0xffffffff;  //0x1111111111111111;
        case 7: return 0x0;         //0;
    }
}

 
void dovds_ini(table_drv *drv){
    vds32pti1_ini(addrVDS);
    devdata->numE = 0;
    devdata->NumErr = 0;
}

void dovds_dw(table_drv *drv){
    unsigned char k;
    int i_fds;

    union {
        unsigned int i;
        unsigned short s[2];
    } vds_date, tmp;    

   // vds
    
    vds_date.s[0] = inw(addrVDS + 1);
    vds_date.s[1] = inw(addrVDS + 3);

    if(devdata->tempDoVds != vds_date.i){
        devdata->NumErr++;
    }

    // fds

    i_fds = sw(devdata->numE);
    devdata->tempDoVds = i_fds;
    outw( ((unsigned short *)&i_fds)[0],addrFDS + 0);
    outw( ((unsigned short *)&i_fds)[1],addrFDS + 2);



    // работаем по циклу от 0 до 7
    if (devdata->numE < 7)
        devdata->numE++;
    else
        devdata->numE = 0;
}


