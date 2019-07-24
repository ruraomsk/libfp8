#ifndef AI12_VAS_H
#define AI12_VAS_H
#define AI12_VAS_PTI 0x33
#include "../typedef.h"
#include "../drvio.h"
#define VASCRITERROR 0x80
#define VASWRONGDATA 0x10
#define VASTIMEOUT   0x04
#define VASIOERROR   0x02





typedef struct __attribute__ ((packed))
{ 
    unsigned short addrVas;
} AI12vas_inimod;

typedef struct __attribute__ ((packed))
{ 
    ssint VasData[12];
    short NumK;
    char emptyspace[64];
} AI12vas_data;

void AI12vas_ini(table_drv* drv);
void AI12vas_dr(table_drv* drv);

#endif /* AI12_VAS_H */
