#include <sys/io.h>
#include "vds32_pti.h"
#include <stdio.h>
#include <syslog.h>


#define devaddr (drv->address)
#define devdata ((vds32pti_data *)(drv->data))
#define devinimod ((vds32pti_inimod *)(drv->inimod))

extern char flag_ini;

void vds32pti_ini(table_drv *drv) {
    unsigned char kan;

    if (!flag_ini) {
        for (kan = 0; kan < 32; kan++)
            devdata->data[kan].b = 0;
    }

    if ((inb(devaddr + 14) != 'D') || (inb(devaddr + 15) != 0)) {
        drv->error = 0x80;
        devdata->workok = 0;
        for (kan = 0; kan < 32; kan++)
            devdata->data[kan].error = 0x80;

        return;
    }

    for (kan = 0; kan < 32; kan++)
        devdata->data[kan].error = 0;

    outb( 0,devaddr + 0);
    outb(devinimod->tadr,devaddr + 5 );
    outb( 0x00,devaddr + 6);
    outb( 0x00,devaddr + 13); /* bez preryvanii */
    drv->error = 0;
    devdata->workok = 1;
}

void vds32pti_dr(table_drv *drv) {
    unsigned char kan;
   
    union {
        unsigned int l;
        unsigned short i[2];
    } rd[3];

    if (!devdata->workok || inb(devaddr + 14) != 'D' || inb(devaddr + 15) != 0) {
        drv->error = 0x80;
        devdata->workok = 0;
        for (kan = 0; kan < 32; kan++)
            devdata->data[kan].error = 0x80;
        return;
    }

    rd[0].i[0] = inw(devaddr + 1);
    rd[0].i[1] = inw(devaddr + 3);
//    rd[1].i[0] = inw(devaddr + 1);
//    rd[1].i[1] = inw(devaddr + 3);
//    rd[2].i[0] = inw(devaddr + 1);
//    rd[2].i[1] = inw(devaddr + 3);
//
//    rd[0].l ^= rd[1].l;
    long maska=1UL;
    for (kan = 0; kan < 32; kan++) {
        
        if ((rd[0].l & maska) != 0)
            devdata->data[kan].b = 1;
        else
            devdata->data[kan].b = 0;
        devdata->data[kan].error = 0;
        maska=maska<<1;
    }

    drv->error = 0;
}
