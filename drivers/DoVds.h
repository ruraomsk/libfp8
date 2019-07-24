
#ifndef DOVDS_H
#define DOVDS_H

#include "../typedef.h"
#include "../drvio.h"

#define DOVDS  0x22

void dovds_ini(table_drv* drv);
void dovds_dw(table_drv* drv);


typedef struct __attribute__((packed)) 
{
    int numE;
    int tempDoVds;
    int NumErr;
} doVds_data;

typedef struct __attribute__((packed)) 
{
  unsigned char init;
}doVds_inipar;

#endif /* DOVDS_H */
