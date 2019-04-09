#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "drvio.h"

static Driver *Simul_drv_ptr;
static int handlerUDPSimulSend;
static int handlerUDPSimulRecive;
extern struct sockaddr_in addressRecive;
extern struct sockaddr_in addressSend;
extern void *IObuf;
int sockfd = 0; //socket для обмена с Simul
int lenBufferSimul;

int initAllSimul(short CodeSub, Driver *drv, char *SimulIP, int SimulPort) {
    Simul_drv_ptr = drv;
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
    Driver *drv = Simul_drv_ptr;
    int pos = 0, len = 0;

    struct sockaddr_in from;
    socklen_t fromLength = sizeof (from);
    int received_bytes = recv(handlerUDPSimulRecive, IObuf, lenBufferSimul, 0);
    if (received_bytes <= 0)
        return;
    if (received_bytes != lenBufferSimul)
        return;
    drv = Simul_drv_ptr;
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
    Driver *drv = Simul_drv_ptr;
    int pos = 0;

    while (drv->code_driver != 0) {
        table_drv *table = drv->table;
        memcpy(IObuf + pos, table->data, drv->len_buffer);
        pos += drv->len_buffer;
        drv++;
    }
    int sended_bytes = send(handlerUDPSimulSend, IObuf, lenBufferSimul, 0);
}