/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
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
#include "drvio.h"
#include "variables.h"
#include "modbus-cpc.h"

#include <sys/reboot.h>

#pragma pack(push, 1)

typedef struct
{
    unsigned char code_driver;
    unsigned char address;
    unsigned short len_buffer;
} def_dev;
#pragma pop

static union {
    __off_t ppos;
    def_dev tdef;
} tfd;

void *IObuf;
int lenBufferSimul;

static int dfd; // Для работы с драйверами ввода/вывода
static char imp_drv[] = {
    FDS16R,
    VDS32R,
    VAS84R,
    0,
};
#define MANE_DRIVER "/dev/dspa_0"
static Driver *drv_ptr;
static char NeedReInit;
#define READ_ALL 1
#define WRITE_ALL 2
#define REINIT_X80 3
static int handlerUDPSimul;
void iniBufDrivers()
{
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0)
    {
        table_drv *table = drv->table;
        memset(table->data, 0xff, drv->len_buffer);
        drv++;
    }
}

void moveShort(void *buf, short value)
{
    short *sh = (short *)buf;
    *sh = value;
}

int initAllSimul(short CodeSub, Driver *drv, char *SimulIP, int SimulPort)
{
    struct sockaddr_in serv_addr;
    drv_ptr = drv;
    char buffer[2048];
    // moveShort(buffer, CodeSub);
    // int pos = 0;
    int len = 0;
    while (drv->code_driver != 0)
    {
        len += drv->len_buffer;
        drv++;
    }
    lenBufferSimul = len;
    // if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    //     syslog(LOG_ERR,"\n Error : Could not create socket \n");
    //     return 1;
    // }
    // serv_addr.sin_family = AF_INET;
    // serv_addr.sin_port = htons(SimulPort);
    // if (inet_pton(AF_INET, SimulIP, &serv_addr.sin_addr) <= 0) {
    //     syslog(LOG_ERR,"\n inet_pton error occured\n");
    //     return 1;
    // }
    // if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
    //     syslog(LOG_ERR,"\n Error : Connect Failed \n");
    //     return 1;
    // }
    // if (send(sockfd, buffer, pos, 0) < 0) {
    //     syslog(LOG_ERR,"\n Error : Failed Send Init\n");
    //     return 1;
    // }
    handlerUDPSimul = openUDPport(SimulIP, SimulPort + CodeSub);
    if (handlerUDPSimul < 0)
    {
        syslog(LOG_ERR, "\n Error : Not binding Simul port\n");
        return 1;
    }
    IObuf = malloc(lenBufferSimul);
    if (IObuf == NULL)
    {
        syslog(LOG_ERR, "\n Error : Failed Allocation Memory\n");
        return 1;
    }
    return 0;
}

void readAllSimul(void)
{
    //    moveUserToDriver();
    Driver *drv = drv_ptr;
    int pos = 0, len = 0;

    // while (drv->code_driver != 0) {
    //         //    table_drv *table = drv->table;
    //         //    memcpy(IObuf+pos,table->data, drv->len_buffer);
    //         //    pos+=drv->len_buffer;
    //     len += drv->len_buffer;
    //     drv++;
    // }
    //    if(send(sockfd,IObuf,len,0)!=len) return;
    struct sockaddr_in from;
    socklen_t fromLength = sizeof(from);
    int received_bytes = recvfrom(handlerUDPSimul, IObuf, lenBufferSimul, 0, &from, &fromLength);
    if (received_bytes <= 0)
        return;
    if (received_bytes != lenBufferSimul)
        return;
    drv = drv_ptr;
    pos = 0;
    while (drv->code_driver != 0)
    {
        table_drv *table = drv->table;
        memcpy(table->data, IObuf + pos, drv->len_buffer);
        pos += drv->len_buffer;
        drv++;
    }
    moveDriverToUser();
    return;
}

void writeAllSimul(void)
{
    struct sockaddr_in to;
    socklen_t toLength = sizeof(to);

    moveUserToDriver();
    Driver *drv = drv_ptr;
    int pos = 0;

    while (drv->code_driver != 0)
    {
        table_drv *table = drv->table;
        memcpy(IObuf + pos, table->data, drv->len_buffer);
        pos += drv->len_buffer;
        drv++;
    }
    int sended_bytes = sendto(handlerUDPSimul, IObuf, lenBufferSimul, 0, &to, &toLength);
    // if (sended_bytes != lenBufferSimul)
    // {
    //     syslog(LOG_INFO, "writeAllSimul failed to send packet: return value = %d\n", sended_bytes);
    // }
    // return;
}

int initAllDrivers(Driver *drv)
{
    //    printf("len table_drv %d %d %d\n", sizeof (table_drv), sizeof (short), sizeof (long long int));
    dfd = open(NAME_DRIVER, O_RDWR);
    if (dfd < 0)
    {
        //        fprintf(stderr, "open device %s error %d", NAME_DRIVER, dfd);
        return EXIT_FAILURE;
    }
    NeedReInit = 0;
    drv_ptr = drv;
    iniBufDrivers();
    while (drv->code_driver != 0)
    {
        tfd.tdef.len_buffer = (unsigned char)(drv->len_buffer & 0xff);
        tfd.tdef.code_driver = (unsigned char)(drv->code_driver & 0xff);
        tfd.tdef.address = drv->address;
        table_drv *table = drv->table;
        table->address = drv->address;
        table->codedrv = drv->code_driver;
        ssize_t res = pwrite(dfd, drv->table, drv->len_init, tfd.ppos);
        if (res != drv->len_init)
        {
            syslog(LOG_ERR, "init device driver %hhx code %hhx adr error %d", drv->code_driver, drv->address, res);
            return EXIT_FAILURE;
        }
        drv++;
    }
    runDrivers();
    return 0;
}

void moveUserToDriver()
{
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0)
    {
        DriverRegister *def_buffer = drv->def_buffer;
        table_drv *table = drv->table;
        char *base = (char *)table->data;
        while (def_buffer->format != 0)
        {
            switch (def_buffer->format)
            {
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

void moveDriverToUser()
{
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0)
    {
        DriverRegister *def_buffer = drv->def_buffer;
        table_drv *table = drv->table;
        char *base = (char *)table->data;
        while (def_buffer->format != 0)
        {
            switch (def_buffer->format)
            {
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

void printDriver()
{
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0)
    {
        table_drv *table = drv->table;
        printf("Driver adr=%x ret=%x buf=[", table->address, table->error);
        for (int i = 0; i < drv->len_buffer; i++)
        {
            printf("%hhx,", *((char *)(table->data + i)));
        }
        printf("]\n");
        drv++;
    }
}

void setErrorDriver(short *ret_error)
{

    Driver *drv = drv_ptr;
    while (drv->code_driver != 0)
    {
        table_drv *table = drv->table;
        table->error = *ret_error++;
        drv++;
    }
}

void WakeUpDriver()
{
    Driver *drv = drv_ptr;
    while (drv->code_driver != 0)
    {
        table_drv *table = drv->table;
        if (table->error == 0x80 || table->error == 0x90)
        {
            NeedReInit = 1;
        }
        drv++;
    }
}

int readAllDrivers(void)
{
    unsigned long int t;
    short ret_error[256];
    moveUserToDriver();
    if (pread(dfd, &t, 1, READ_ALL) != 0)
    {
        stopAll();
        return -1;
    };
    pread(dfd, ret_error, 2, 0);
    if (NeedReInit)
    {
        if (pread(dfd, &t, 1, REINIT_X80))
        {
            stopAll();
            return -1;
        };
        NeedReInit = 0;
    }
    setErrorDriver(ret_error);
    moveDriverToUser();
    return 0;
}

int isSlave()
{
    unsigned long int t;
    return pread(dfd, &t, 1, 0);
}

int writeAllDrivers(void)
{
    unsigned long int t;
    short ret_error[256];
    moveUserToDriver();
    if (pread(dfd, &t, 1, WRITE_ALL) != 0)
    {
        stopAll();
        return -1;
    }
    pread(dfd, ret_error, 2, 0);
    setErrorDriver(ret_error);
    //    printDriver();
    moveDriverToUser();
    return 0;
}
