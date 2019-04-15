#ifndef DRVIO_H
#define DRVIO_H

#include "UDPTrasport.h"
#define FDS16R 0x04
#define VDS32R 0xC2
#define VAS84R 0xC6
#define READ_ALL 1
#define WRITE_ALL 2
#define REINIT_X80 3
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

void iniBufDrivers();
void call_ini(table_drv *table);
void call_read(table_drv *table);
void call_write(table_drv *table);
void moveShort(void *buf, short value);
int openUDPRecive(int port);
int openUDPSend(char *ip, int port);
int initAllDriversPTI(Driver *drv);
int initAllDrivers(Driver *drv);
void moveUserToDriver();
void moveDriverToUser();
void printDriver();
void setErrorDriver(short *ret_error);
void WakeUpDriver();
int readAllDriversPTI(void);
int readAllDrivers(void);
int isSlave();
int writeAllDriversPTI(void);
int writeAllDrivers(void);
void readAllSimul(void);
void writeAllSimul(void);
int initAllSimul(short CodeSub,Driver *drv,char *SimulIP,int SimulPort);


static char flag_ini = 0;

typedef struct __attribute__ ((packed)) {
    int type;
    void (*init)(table_drv*);
    void (*step1)(table_drv*);
    void (*step2)(table_drv*);
}
type_drivers;

#endif /* DRVIO_H */