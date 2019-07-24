#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include "ai12_vas.h"
#include <syslog.h>

#define devVasadr (drv->address)
#define devVasini ((AI12vas_inimod *)drv->inimod)
#define devVasdata ((AI12vas_data *)drv->data)

// проставить ошибки в весь массив

void PutError(table_drv* drv, unsigned char Merr) {
    int i;
    for (i = 0; i < 12; i++)
        devVasdata->VasData[i].error = Merr;
}

// проставить ошибки в определнный канал массив

void PutChanError(table_drv* drv, int chan, unsigned char Merr) {
    devVasdata->VasData[chan].error = Merr;
}

void AI12vas_ini(table_drv* drv) {
    int i;
    devVasdata->NumK = 1;
    PutError(drv, 0); // занулим ошибки каналов данных

    // **проверка модуля**
    if (inb(devVasini->addrVas + 0xe) != 'g' || inb(devVasini->addrVas + 0xf) != 11) {
        PutError(drv, VASCRITERROR);
        return;
    }

    // **инициализация**
    outb(0x01, devVasini->addrVas + 0);
    outb(0x55, devVasini->addrVas + 1);
    outb(0x55, devVasini->addrVas + 2);
    outb(0x55, devVasini->addrVas + 3);
    outb(0, devVasini->addrVas + 0);
    outb(0, devVasini->addrVas + 4);
    outb(0, devVasini->addrVas + 5);
    outb('V', devVasini->addrVas + 6);
}


// Считываются за раз 2 канала
// на текущем цикле читаем значения каналов которые были заданы в прошлом цикле

void AI12vas_dr(table_drv* drv) {
    int tempChan, sost0, tempK1, tempK2;
    PutError(drv, 0); // занулим ошибки каналов данных

    // **проверка модуля**
    if (inb(devVasini->addrVas + 0xe) != 'g' || inb(devVasini->addrVas + 0xf) != 11) {
        PutError(drv, VASCRITERROR);
        return;
    }

    // определим какой канал был задан на прошлом цикле
    if (devVasdata->NumK == 1)
        tempChan = 10;
    else
        tempChan = devVasdata->NumK - 3;

    // считываем два канала
    tempK1 = inw(devVasini->addrVas + 4);
    tempK2 = inw(devVasini->addrVas + 8);
    sost0 = inb(devVasini->addrVas); // для проверка на timeout и wrongdata

    // проверка первого канала из пары
    if (!(sost0 & 0x08))
        PutChanError(drv, devVasdata->NumK - 2, VASTIMEOUT);
    else {
        if (tempK1 & 0x8000)
            PutChanError(drv, devVasdata->NumK - 2, VASWRONGDATA);
        else
            devVasdata->VasData[tempChan].i = tempK1;
    }

    // проверка второго канала из пары    
    if (!(sost0 & 0x80))
        PutChanError(drv, devVasdata->NumK - 1, VASTIMEOUT);
    else {
        if (tempK2 & 0x8000)
            PutChanError(drv, devVasdata->NumK - 1, VASWRONGDATA);
        else
            devVasdata->VasData[tempChan + 1].i = tempK2;
    }

    // запись двух следующих каналов
    outb(devVasdata->NumK, devVasini->addrVas + 4);
    outb(devVasdata->NumK + 1, devVasini->addrVas + 5);
    if (0x10 & inb(devVasini->addrVas))
        outb('V', devVasini->addrVas + 6);
    else {
        PutError(drv, VASIOERROR);
        return;
    }

    devVasdata->NumK += 2;
    if (devVasdata->NumK > 12)
        devVasdata->NumK = 1;

}
