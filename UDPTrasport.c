/*
 * Подпрограммы работы по обмену информацией по UDP  для ФП8
 */

/* 
 * File:   UDPTrasport.c
 * Author: rusin
 * 
 * Created on 4 мая 2018 г., 9:19
 */
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> //inet_addr
#include <fcntl.h>


#include "UDPTrasport.h"

SetupUDP *setUp;
unsigned char * buffer = NULL;
int ismaster;
int udpport = -1;
struct sockaddr_in address;
//char ctrl=0;

int openUDPport(char *ip, int port) {
//    printf("open udp ip=%s port=%d\n", ip, port);
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (handle <= 0) {
        syslog(LOG_ERR,"openUDPport failed to create socket\n");
        return -1;
    }
    memset(&address, 0, sizeof (address));
    address.sin_family = AF_INET;
    if (!ismaster) {
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons((unsigned short) port);
        if (bind(handle, &address, sizeof (address)) < 0) {
            syslog(LOG_ERR,"openUDPport failed to bind socket\n");
            return -1;
        }
        int nonBlocking = 1;
        if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
            syslog(LOG_ERR,"openUDPport failed to set non-blocking socket\n");
            return -1;
        }
        return handle;
    }
    address.sin_port = htons((unsigned short) port);
    if (inet_aton(ip, &address.sin_addr) == 0) {
        syslog(LOG_ERR,"openUDPport failed to ip adr\n");
        return -1;
    }
//    int nonBlocking = 1;
//    if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
//        printf("failed to set non-blocking socket\n");
//        return -1;
//    }
    return handle;
}

void closeUDP() {
    close(udpport);
    free(buffer);
}
// master ==1 то данный процессор должен слать свои данные и статус
// master ==0 то данный процессор должен принимать чужие данные и посылать свой статус

int initUDP(int master, int nomer, SetupUDP *setup) {
    setUp = setup;
    ismaster = master;
    if (nomer == 1) {
        udpport = openUDPport(setup->ip2, setup->port2);
    } else {
        udpport = openUDPport(setup->ip1, setup->port1);
    }
    if (udpport < 0) {
        return -1;
    }
    buffer = malloc(setup->lenvar);
    if (buffer == NULL) {
        syslog(LOG_ERR,"initUDP can't allocated memory for buffer\n");
        return -1;
    }
    return 0;
}

void sendVariables() {
    if (!ismaster) return;
    memcpy(buffer, setUp->varbufer, setUp->lenvar);
    int sent_bytes = sendto(udpport, (const char*) buffer, setUp->lenvar, 0, &address, sizeof (address));
    if (sent_bytes != setUp->lenvar) {
        syslog(LOG_INFO,"sendVariables failed to send packet: return value = %d\n", sent_bytes);
    }
//    printf("send %d\n", *buffer);
}

int reciveVariables() {
        struct sockaddr_in from;
        socklen_t fromLength = sizeof (from);
        int received_bytes = recvfrom(udpport, (char*) buffer, setUp->lenvar, MSG_WAITALL, &from, &fromLength);
        if ((received_bytes <= 0)||(received_bytes != setUp->lenvar)) 
        {
            syslog(LOG_INFO,"reciveVariables failed to read packet: return value = %d\n", received_bytes);
            return 1;
        };

        memcpy(setUp->varbufer, buffer, setUp->lenvar);
	return 0;
}

int getNomer(){
    char hostname[120];
    gethostname(hostname,120);
    char *pos=strstr(hostname,"_");
    if(pos==NULL) return 1;
    if(*++pos=='1') return 1;
    return 2;
}