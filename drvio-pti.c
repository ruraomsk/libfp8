#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "variables.h"

#include "drvio-pti.h"
#include <fp8/drivers/ao16.h>
#include <fp8/drivers/do32_pti.h>
#include <fp8/drivers/vds32_pti.h>
#include <fp8/drivers/emul8enc.h>
#include <fp8/drivers/DoVds.h>
#include <fp8/drivers/ai12_vas.h>





static type_drivers tab_t[] = {
    { AO16_PTI,
        &ao16_ini,
        NULL,
        &ao16_dw,},

    { VDS32_PTI,
        &vds32pti_ini,
        &vds32pti_dr,
        NULL,},

    { AI12_VAS_PTI,
        &AI12vas_ini,
        &AI12vas_dr,
        NULL,},

    { DO32_PTI,
        &do32_ini,
        NULL,
        &do32_dw,},
    { EM8ENC_PTI,
        &em8encpti_ini,
        NULL,
        &em8enc_dw,},
    { DOVDS,
        &dovds_ini,
        NULL,
        &dovds_dw,},
    {-1, NULL, NULL, NULL},
};

static union {
    __off_t ppos;
    def_dev tdef;
} tfd;

void *IObuf;

static Driver *drv_ptr;
#define READ_ALL 1
#define WRITE_ALL 2
#define REINIT_X80 3

static struct sockaddr_in addressRecive;
static struct sockaddr_in addressSend;

void iniBufDrivers() {
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        memset(table->data, 0xff, drv->len_buffer);
        drv++;
    }
}

void call_ini(table_drv *table) {
    int i = 0;
    while (tab_t[i].type > 0) {
        if (tab_t[i].type == table->codedrv) {
            void (*ptr)(table_drv *) = NULL;
            ptr = tab_t[i].init;
            ptr(table);
            return;
        }
        i++;
    }
}

void call_read(table_drv *table) {
    int i = 0;
    while (tab_t[i].type > 0) {
        if (tab_t[i].type == table->codedrv) {
            void (*ptr)(table_drv *) = NULL;
            ptr = tab_t[i].step1;
            if (ptr != NULL) ptr(table);
            return;
        }
        i++;
    }
}

void call_write(table_drv *table) {
    int i = 0;
    while (tab_t[i].type > 0) {
        if (tab_t[i].type == table->codedrv) {
            void (*ptr)(table_drv *) = NULL;
            ptr = tab_t[i].step2;
            if (ptr != NULL) ptr(table);
            return;
        }
        i++;
    }
}

void moveShort(void *buf, short value) {
    short *sh = (short *) buf;
    *sh = value;
}

int openUDPRecive(int port) {
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (handle <= 0) {
        syslog(LOG_ERR, "openUDPRecive failed to create socket\n");
        return -1;
    }
    memset(&addressRecive, 0, sizeof (addressRecive));
    addressRecive.sin_family = AF_INET;
    addressRecive.sin_addr.s_addr = htonl(INADDR_ANY);
    addressRecive.sin_port = htons((unsigned short) port);
    if (bind(handle, &addressRecive, sizeof (addressRecive)) < 0) {
        syslog(LOG_ERR, "openUDPRecive failed to bind socket\n");
        return -1;
    }
    int nonBlocking = 1;
    if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
        syslog(LOG_ERR, "openUDPRecive failed to set non-blocking socket\n");
        return -1;
    }
    return handle;
}

int openUDPSend(char *ip, int port) {
    //    printf("open udp ip=%s port=%d\n", ip, port);
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (handle <= 0) {
        syslog(LOG_ERR, "openUDPSend failed to create socket\n");
        return -1;
    }
    memset(&addressSend, 0, sizeof (addressSend));
    addressSend.sin_family = AF_INET;
    addressSend.sin_port = htons((unsigned short) port);
    if (inet_aton(ip, &addressSend.sin_addr) == 0) {
        syslog(LOG_ERR, "openUDPSend failed to ip adr\n");
        return -1;
    }
    if (connect(handle, &addressSend, sizeof (addressSend)) < 0) {
        syslog(LOG_ERR, "openUDPSend failed to bind socket\n");
        return -1;
    }
    return handle;
}


int initAllDriversPTI(Driver *drv) {
    drv_ptr = drv;
    iniBufDrivers();
    ioperm(0x100, 0x80, 1);
    ioperm(0x200, 0x80, 1);
    ioperm(0x300, 0x80, 1);
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        table->address = drv->address;
        table->codedrv = drv->code_driver;
        call_ini(table);
        drv++;
    }
    return 0;
}

void moveUserToDriver() {
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        DriverRegister *def_buffer = drv->def_buffer;
        table_drv *table = drv->table;
        char *base = (char *) table->data;
        while (def_buffer->format != 0) {
            switch (def_buffer->format) {
                case boolean:
                case char1b:
                    memcpy(base + def_buffer->address, def_buffer->value, 2);
                    break;
                case uint2b:
                case sint2b:
                    memcpy(base + def_buffer->address, def_buffer->value, 3);
                    break;
                case uint4b:
                case sint4b:
                case float4b:
                    memcpy(base + def_buffer->address, def_buffer->value, 5);
                    break;
                case sint8b:
                case float8b:
                    memcpy(base + def_buffer->address, def_buffer->value, 9);
                    break;
            }
            def_buffer++;
        }
        drv++;
    }
}

void moveDriverToUser() {
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        DriverRegister *def_buffer = drv->def_buffer;
        table_drv *table = drv->table;
        char *base = (char *) table->data;
        while (def_buffer->format != 0) {
            switch (def_buffer->format) {
                case boolean:
                case char1b:
                    memcpy(def_buffer->value, base + def_buffer->address, 2);
                    break;
                case uint2b:
                case sint2b:
                    memcpy(def_buffer->value, base + def_buffer->address, 3);
                    break;
                case uint4b:
                case sint4b:
                case float4b:
                    memcpy(def_buffer->value, base + def_buffer->address, 5);
                    break;
                case sint8b:
                case float8b:
                    memcpy(def_buffer->value, base + def_buffer->address, 9);
                    break;
            }
            def_buffer++;
        }
        drv++;
    }
}

void printDriver() {
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        printf("Driver adr=%x ret=%x buf=[", table->address, table->error);
        for (int i = 0; i < drv->len_buffer; i++) {
            printf("%hhx,", *((char *) (table->data + i)));
        }
        printf("]\n");
        drv++;
    }
}

void readAllDriversPTI(void) {
    moveUserToDriver();
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        call_read(table);
        drv++;
    }
    moveDriverToUser();
}

void writeAllDriversPTI(void) {
    moveUserToDriver();
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        call_write(table);
        drv++;
    }
    moveDriverToUser();
}
