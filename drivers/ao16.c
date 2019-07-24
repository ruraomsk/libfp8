#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/time.h>
#include "ao16.h"
#include <syslog.h>

#define devadr (drv->address)
#define devini ((ao16_inimod *)drv->inimod)
#define devdata ((ao16_data *)drv->data)

extern char flag_ini;
static int aoint;

static void ao16f_outdata(table_drv* drv);

void ao16_ini(table_drv* drv) {
    unsigned char i;
    aoint = 0;
    if (!flag_ini) {
        for (i = 0; i < 16; i++) {
            devdata->adata[i].i = devdata->aold[i] = devini->ao161stout[i];
            devdata->adata[i].error = 0;
            devdata->asost[i].c = AO16FNOCONTROL;
            devdata->asost[i].error = 0;
        }
    }

    if (inb(devadr + 14) != devini->ao16id[0] || inb(devadr + 15) != devini->ao16id[1]
            || devini->ao16id[1] == 8 && (devini->workmsk & 0xff00)) {
        drv->error = 0x80;
        for (i = 0; i < 16; i++)
            devdata->asost[i].c = 0x80;

        return;
    }
    devini->aoeto = 30;
    outb(0, devadr + 1);
    outb(0, devadr);
    drv->error = 0;
    return;
}

void ao16_dw(table_drv* drv) {
    char i,c;
    int temp;
    if ((drv->error & 0x80)
            || inb(devadr + 14) != devini->ao16id[0] || inb(devadr + 15) != devini->ao16id[1]
            || devini->ao16id[1] == 8 && (devini->workmsk & 0xff00)) {
        drv->error = 0x80;
        for (i = 0; i < 16; i++)
            devdata->asost[i].c = 0x80;

        return;
    }

    for (i = 0; i < 16; i++) {
        if (((devini->workmsk >> i) &1) && !devdata->adata[i].error)
            devdata->aold[i] = devdata->adata[i].i;
    }
    drv->error = 0;

    // ao16f_outdata(drv);
    outb(aoint, devadr);
    do {
        c = inb(devadr);
    } while (c & 0x80 == 0);
    temp = (int)(devdata->adata[aoint].i / 3.91);
    outw(temp, devadr + 2);

    if(aoint < 15)
        aoint++;
    else{
        aoint = 0;
    }
    

    return;
}

/*функция выдает aold[i] для задействованных каналов, формирует asost[i]  и drv->error */
void ao16f_outdata(table_drv* drv) {
    char i, c;
    for (i = 0; i < 16; i++) {
        outb(i, devadr);
        do {
            c = inb(devadr);
        } while (c & 0x80 == 0);
        outw((devdata->adata[i].i >> 2), devadr + 2);
    }
}
