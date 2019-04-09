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