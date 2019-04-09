#ifndef DRVIO_H
#define DRVIO_H

#include "UDPTrasport.h"
#define FDS16R 0x04
#define VDS32R 0xC2
#define VAS84R 0xC6

#define NAME_DRIVER "/dev/dspa_0"

typedef struct __attribute__ ((packed)) {
    unsigned char code_driver;
    unsigned char address;
    unsigned short len_buffer;
}
def_dev;

typedef struct __attribute__ ((packed)) {
    unsigned short codedrv;
    unsigned short address;
    void *inimod;
    void *data;
    unsigned long int time;
    short error;
}
table_drv;

typedef struct __attribute__ ((packed)) {
    void *value;
    char format;
    short address;
}
DriverRegister;

typedef struct __attribute__ ((packed)) {
    unsigned short code_driver;
    unsigned short address;
    short len_init;
    short len_buffer;
    DriverRegister *def_buffer;
    table_drv *table;
}
Driver;

int initAllDrivers(Driver *drv);
int readAllDrivers(void);
int writeAllDrivers(void);
void readAllSimul(void);
void writeAllSimul(void);
int isSlave(void);
int initAllSimul(short CodeSub, Driver *drv, char *SimulIP, int SimulPort);
static char flag_ini = 0;

typedef struct __attribute__ ((packed)) {
    int type;
    void (*init)(table_drv*);
    void (*step1)(table_drv*);
    void (*step2)(table_drv*);
}
type_drivers;

#endif /* DRVIO_H */