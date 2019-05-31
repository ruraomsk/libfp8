#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/reboot.h>

#include "variables.h"
#include "modbus-cpc.h"
#include "drvio.h"

#include "drivers/ao16.h"
#include "drivers/do32_pti.h"
#include "drivers/vds32_pti.h"
#include "drivers/emul8enc.h"
#include "drivers/DoVds.h"
#include "drivers/ai12_vas.h"

static type_drivers tab_tp[] = {
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
    off_t ppos;
    def_dev tdef;
} tfd;

void *IObuf;
static int dfd; // Для работы с драйверами ввода/вывода
static Driver *drv_ptr;
static char NeedReInit;
static int handlerUDPSimulSend;
static int handlerUDPSimulRecive;
static struct sockaddr_in addressRecive;
static struct sockaddr_in addressSend;
int sockfd = 0; //socket для обмена с Simul
int lenBufferSimul;

int initAllSimul(short CodeSub, Driver *drv, char *SimulIP, int SimulPort) {
    drv_ptr = drv;
    int len = 0;
    while (drv->code_driver != 0) {
        len += drv->len_buffer;
        drv++;
    }
    lenBufferSimul = len;
    handlerUDPSimulRecive = openUDPRecive(SimulPort);
    handlerUDPSimulSend = openUDPSend(SimulIP, SimulPort + CodeSub);
    if ((handlerUDPSimulRecive < 0) || (handlerUDPSimulSend < 0)) {
        syslog(LOG_ERR, "\n Error : Not binding Simul ports\n");
        return 1;
    }
    IObuf = malloc(lenBufferSimul);
    if (IObuf == NULL) {
        syslog(LOG_ERR, "\n Error : Failed Allocation Memory\n");
        return 1;
    }
    return 0;
}

void readAllSimul(void) {
    //    moveUserToDriver();
    Driver *drv = drv_ptr;
    int pos = 0, len = 0;
    struct sockaddr_in from;
    socklen_t fromLength = sizeof (from);
    int received_bytes = recv(handlerUDPSimulRecive, IObuf, lenBufferSimul, 0);
    if (received_bytes <= 0)
        return;
    if (received_bytes != lenBufferSimul)
        return;
    drv = drv_ptr;
    pos = 0;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        memcpy(table->data, IObuf + pos, drv->len_buffer);
        pos += drv->len_buffer;
        drv++;
    }
    moveDriverToUser();
    return;
}

void writeAllSimul(void) {
    socklen_t toLength = sizeof (addressSend);
    moveUserToDriver();
    Driver *drv = drv_ptr;
    int pos = 0;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        memcpy(IObuf + pos, table->data, drv->len_buffer);
        pos += drv->len_buffer;
        drv++;
    }
    int sended_bytes = send(handlerUDPSimulSend, IObuf, lenBufferSimul, 0);
}

void iniBufDrivers() {
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        // memset(table->data, 0xff, drv->len_buffer);
        memset(table->data, 0x0, drv->len_buffer);
        drv++;
    }
}

void call_ini(table_drv *table) {
    int i = 0;
    while (tab_tp[i].type > 0) {
        if (tab_tp[i].type == table->codedrv) {
            void (*ptr)(table_drv *) = NULL;
            ptr = tab_tp[i].init;
            ptr(table);
            return;
        }
        i++;
    }
}

void call_read(table_drv *table) {
    int i = 0;
    while (tab_tp[i].type > 0) {
        if (tab_tp[i].type == table->codedrv) {
            void (*ptr)(table_drv *) = NULL;
            ptr = tab_tp[i].step1;
            if (ptr != NULL) ptr(table);
            return;
        }
        i++;
    }
}

void call_write(table_drv *table) {
    int i = 0;
    while (tab_tp[i].type > 0) {
        if (tab_tp[i].type == table->codedrv) {
            void (*ptr)(table_drv *) = NULL;
            ptr = tab_tp[i].step2;
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

int initAllDrivers(Driver *drv) {
    //    printf("len table_drv %d %d %d\n", sizeof (table_drv), sizeof (short), sizeof (long long int));
    dfd = open(NAME_DRIVER, O_RDWR);
    if (dfd < 0) {
        //        fprintf(stderr, "open device %s error %d", NAME_DRIVER, dfd);
        return EXIT_FAILURE;
    }
    NeedReInit = 0;
    drv_ptr = drv;
    iniBufDrivers();
    while (drv->code_driver != 0) {
        tfd.tdef.len_buffer = (unsigned char) (drv->len_buffer & 0xff);
        tfd.tdef.code_driver = (unsigned char) (drv->code_driver & 0xff);
        tfd.tdef.address = drv->address;
        table_drv *table = drv->table;
        table->address = drv->address;
        table->codedrv = drv->code_driver;
        ssize_t res = pwrite(dfd, drv->table, drv->len_init, tfd.ppos);
        if (res != drv->len_init) {
            syslog(LOG_ERR, "init device driver %hhx code %hhx adr error %d", drv->code_driver, drv->address, res);
            return EXIT_FAILURE;
        }
        drv++;
    }
    runDrivers();
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

void setErrorDriver(short *ret_error) {

    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        table->error = *ret_error++;
        drv++;
    }
}

void WakeUpDriver() {
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        if (table->error == 0x80 || table->error == 0x90) {
            NeedReInit = 1;
        }
        drv++;
    }
}

int readAllDriversPTI(void) {
    moveUserToDriver();
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        call_read(table);
        drv++;
    }
    moveDriverToUser();
    return 0;
}

int readAllDrivers(void) {
    unsigned long int t;
    short ret_error[256];
    moveUserToDriver();
    if (pread(dfd, &t, 1, READ_ALL) != 0) {
        stopAll();
        return -1;
    };
    pread(dfd, ret_error, 2, 0);
    if (NeedReInit) {
        if (pread(dfd, &t, 1, REINIT_X80)) {
            stopAll();
            return -1;
        };
        NeedReInit = 0;
    }
    setErrorDriver(ret_error);
    moveDriverToUser();
    return 0;
}

int isSlave() {
    unsigned long int t;
    return pread(dfd, &t, 1, 0);
}

int writeAllDriversPTI(void) {
    moveUserToDriver();
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        call_write(table);
        drv++;
    }
    moveDriverToUser();
    return 0;
}

int writeAllDrivers(void) {
    unsigned long int t;
    short ret_error[256];
    moveUserToDriver();
    if (pread(dfd, &t, 1, WRITE_ALL) != 0) {
        stopAll();
        return -1;
    }
    pread(dfd, ret_error, 2, 0);
    setErrorDriver(ret_error);
    moveDriverToUser();
    return 0;
}