#ifndef EMUL8ENC_H
#define EMUL8ENC_H
#define EM8ENC_PTI 0x05
#include "../typedef.h"
#include "../drvio-pti.h"
#include <stdint.h>


void em8encpti_ini(table_drv* drv);
void em8enc_dw(table_drv* drv);

typedef struct __attribute__((packed))
{ 
  unsigned char addrModBus; // адресс модуля принимающего номер BUS
  unsigned char maskLatch;  // маска байта латч
} em8encpti_inimod;

typedef struct __attribute__((packed))
{
  unsigned long data[8];    // показания энкодера по каналам
} em8encpti_data;

#endif /* EMUL8ENC_H */
