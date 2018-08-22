/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   cycleBuff.c
 * Author: rusin
 * 
 * Created on 13 апреля 2018 г., 10:04
 */
#include <sys/time.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <pthread.h>

#include "variables.h"
#include "cycleBuff.h"

#define LEN_BUFFER 280000
#define MAX_COUNT  3000
static CtrlVarBuf *ctrlVarBufs;
static int count_var;
static void *OUT_BUFFER;
static pthread_t cyclethread;
static pthread_mutex_t cyclemutex = PTHREAD_MUTEX_INITIALIZER;

void time_now(struct timeval *tv1) {
    gettimeofday(tv1, NULL);
}

int time_msec(struct timeval *tv1) {
    struct timeval tv2, dtv;
    gettimeofday(&tv2, NULL);
    dtv.tv_sec = tv2.tv_sec - tv1->tv_sec;
    dtv.tv_usec = tv2.tv_usec - tv1->tv_usec;
    if (dtv.tv_usec < 0) {
        dtv.tv_sec--;
        dtv.tv_usec += 1000000;
    }
    return dtv.tv_sec * 1000 + dtv.tv_usec * 1000;
}

//void recover_time(struct timeval *tv1, int adding) {
//    struct timeval tv2;
//    tv2.tv_sec = adding / 1000;
//    tv2.tv_usec = (adding % 1000)*1000;
//    tv1->tv_sec = tv1->tv_sec + tv2.tv_sec;
//    tv1->tv_usec = tv2.tv_usec + tv1->tv_usec;
//    if (tv1->tv_usec > 1000000) {
//        tv1->tv_sec++;
//        tv1->tv_usec -= 1000000;
//    }
//    return;
//}

void moveOnlyValue(void *dest, void *src, char format) {
    switch (format) {
        case boolean:
        case char1b:
            memcpy(dest, src, 1);
            return;
        case uint2b:
        case sint2b:
            memcpy(dest, src, 2);
            return;
        case uint4b:
        case sint4b:
        case float4b:
            memcpy(dest, src, 4);
            return;
        case sint8b:
        case float8b:
            memcpy(dest, src, 8);
            return;
    }
    return;
}

void moveFullValue(void *dest, void *src, char format) {
    switch (format) {
        case boolean:
        case char1b:
            memcpy(dest, src, 2);
            return;
        case uint2b:
        case sint2b:
            memcpy(dest, src, 3);
            return;
        case uint4b:
        case sint4b:
        case float4b:
            memcpy(dest, src, 5);
            return;
        case sint8b:
        case float8b:
            memcpy(dest, src, 9);
            return;
    }
    return;
}

typedef struct __attribute__ ((packed)) {
    int msec;
    char b[sizeof (double) + 1];
}
valInBuf;

void initCycleBuff(CtrlVarBuf *pCtrl) {
    time_now(&pCtrl->tstart);
    pCtrl->head = pCtrl->start;
    pCtrl->tail = pCtrl->head + pCtrl->el_size;
    *(int *)pCtrl->head=0;
    moveFullValue(pCtrl->head+sizeof(int), pCtrl->value, pCtrl->format);
    moveOnlyValue(pCtrl->tvalue, pCtrl->value, pCtrl->format);
}

int lenght_buf(CtrlVarBuf *pCtrl) {
    if (pCtrl->tail > pCtrl->head) {
        return (pCtrl->tail - pCtrl->head) / pCtrl->el_size;
    }
    return ((pCtrl->end - pCtrl->start) / pCtrl->el_size);
}

void readAllValues(CtrlVarBuf *pCtrl, void *dest) {

    if (pCtrl->tail > pCtrl->head) {
        memcpy(dest, pCtrl->head, pCtrl->tail - pCtrl->head);
        return;
    }
    memcpy(dest, pCtrl->head, pCtrl->end - pCtrl->head);
    dest += pCtrl->end - pCtrl->head;
    memcpy(dest, pCtrl->start, pCtrl->head - pCtrl->start);
}
void addValue(CtrlVarBuf *pCtrl) {
    valInBuf *vb = (valInBuf *) pCtrl->tail;
    vb->msec = time_msec(&pCtrl->tstart);
    moveFullValue(pCtrl->tail+sizeof(int), pCtrl->value, pCtrl->format);
    moveOnlyValue(pCtrl->tvalue, pCtrl->value, pCtrl->format);
    if(pCtrl->tail>pCtrl->head){
        pCtrl->tail += pCtrl->el_size;
        if (pCtrl->tail >= pCtrl->end) pCtrl->tail = pCtrl->start;
        
    }else {
        pCtrl->head += pCtrl->el_size;
        pCtrl->tail = pCtrl->head;
        if(pCtrl->tail>=pCtrl->end){
            pCtrl->tail = pCtrl->start;
            pCtrl->head = pCtrl->start;
        }
    }
}

int cmpValue(CtrlVarBuf *pCtrl) {
    switch (pCtrl->format) {
        case boolean:
        case char1b:
            return memcmp(pCtrl->tvalue, pCtrl->value, 1);
        case uint2b:
        case sint2b:
            return memcmp(pCtrl->tvalue, pCtrl->value, 2);
        case uint4b:
        case sint4b:
        case float4b:
            return memcmp(pCtrl->tvalue, pCtrl->value, 4);
        case sint8b:
        case float8b:
            return memcmp(pCtrl->tvalue, pCtrl->value, 8);
    }
    return 0;

}

int isChange(CtrlVarBuf *pCtrl) {
    if (cmpValue(pCtrl) == 0) return 0;
    if (pCtrl->format == boolean || pCtrl->format == char1b) return 1;
    if (pCtrl->apertura == NULL) return 1;
    switch (pCtrl->format) {
        case uint2b:
        {
            unsigned short ustval = *(unsigned short *) pCtrl->tvalue;
            unsigned short usvalue = *(unsigned short *) pCtrl->value;
            unsigned short usapert = *(unsigned short *) pCtrl->apertura;
            return abs(ustval - usvalue) > usapert ? 1 : 0;
        }
        case sint2b:
        {
            signed short stval = *(signed short *) pCtrl->tvalue;
            signed short svalue = *(signed short *) pCtrl->value;
            signed short sapert = *(signed short *) pCtrl->apertura;
            return abs(stval - svalue) > sapert ? 1 : 0;
        }
        case uint4b:
        {
            unsigned int uitval = *(unsigned int *) pCtrl->tvalue;
            unsigned int uivalue = *(unsigned int *) pCtrl->value;
            unsigned int uiapert = *(unsigned int *) pCtrl->apertura;
            return abs(uitval - uivalue) > uiapert ? 1 : 0;

        }
        case sint4b:
        {
            signed int itval = *(signed int *) pCtrl->tvalue;
            signed int ivalue = *(signed int *) pCtrl->value;
            signed int iapert = *(signed int *) pCtrl->apertura;
            return abs(itval - ivalue) > iapert ? 1 : 0;
        }
        case float4b:
        {
            float ftval = *(float *) pCtrl->tvalue;
            float fvalue = *(float *) pCtrl->value;
            float fapert = *(float *) pCtrl->apertura;
            return abs(ftval - fvalue) > fapert ? 1 : 0;
        }
        case sint8b:
        {
            signed long long int ltval = *(signed long long int *) pCtrl->tvalue;
            signed long long int lvalue = *(signed long long int *) pCtrl->value;
            signed long long int lapert = *(signed long long int *) pCtrl->apertura;
            return abs(ltval - lvalue) > lapert ? 1 : 0;
        }
        case float8b:
        {
            double dtval = *(double *) pCtrl->tvalue;
            double dvalue = *(double *) pCtrl->value;
            double dapert = *(double *) pCtrl->apertura;
            return abs(dtval - dvalue) > dapert ? 1 : 0;
        }
    }
    return 0;
}

void printCycleBuff(CtrlVarBuf *pCtrl) {
    printf("id=%d size=%d \n", pCtrl->idVariable, lenght_buf(pCtrl));
}

void doCycleBuffers(void) {
    CtrlVarBuf *pCtrl = ctrlVarBufs;
    pthread_mutex_lock(&cyclemutex);

    for (int i = 0; i < count_var; i++, pCtrl++) {
        if (isChange(pCtrl)) addValue(pCtrl);
        //                printCycleBuff(pCtrl);
    }
    pthread_mutex_unlock(&cyclemutex);
}

int lenCycleBuffer(char *namevar) {
    CtrlVarBuf *pCtrl = ctrlVarBufs;
    void *buffer;
    for (int i = 0; i < count_var; i++, pCtrl++) {
        if (strcmp(pCtrl->name, namevar) == 0) {
            return pCtrl->el_size * lenght_buf(pCtrl);
        }
    }
    return 0;
}

void getCycleBuffer(char *namevar, void *buffer) {
    CtrlVarBuf *pCtrl = ctrlVarBufs;
    for (int i = 0; i < count_var; i++, pCtrl++) {
        if (strcmp(pCtrl->name, namevar) == 0) {
            int recv = lenght_buf(pCtrl);
            if (recv == 0) break;
            int *pos = buffer;
            *pos = sizeof (struct timeval) +(pCtrl->el_size*recv);
            memcpy(buffer + sizeof(int), &pCtrl->tstart, sizeof (struct timeval));
            readAllValues(pCtrl, buffer+sizeof(int)+sizeof(struct timeval));
            initCycleBuff(pCtrl);
            return;
        }
    }
}

void conn_handler(int);
void *cycle_lister(void *);
static pthread_t thread_cycle;
#define cycle_listen 1081

int initCycleBuffers(BufVar *bufVar) {
    // Посчитаем колво переменных с циклическим буфером
    BufVar *tpos = bufVar;
    OUT_BUFFER = malloc(LEN_BUFFER);
    if (OUT_BUFFER == NULL) {
        syslog(LOG_ERR,"initCycleBuffers Error not memory for main CycleBuffers!\n");
        return 1;
    }
    count_var = 0;
    while (tpos->idVariable > 0) {
        count_var++;
        tpos++;
    }
    if (count_var == 0) {
        ctrlVarBufs = NULL;
        return 0;
    }
    ctrlVarBufs = malloc(count_var * sizeof (CtrlVarBuf));
    if (ctrlVarBufs == NULL) {
        syslog(LOG_ERR,"initCycleBuffers Error not memory for CycleBuffers!\n");
        return 1;
    }
    tpos = bufVar;
    CtrlVarBuf *pCtrl = ctrlVarBufs;
    while (tpos->idVariable > 0) {
        VarCtrl *var = findVariable(tpos->idVariable);

        pCtrl->format = var->format;
        pCtrl->value = var->value;
        pCtrl->idVariable = var->idVariable;
        pCtrl->name = tpos->name;
        if (tpos->size > MAX_COUNT) tpos->size = MAX_COUNT;
        if (var->format == boolean) {
            pCtrl->apertura = NULL;
        } else {
            if (tpos->idApertura != 0) {
                VarCtrl *apr = findVariable(tpos->idApertura);
                pCtrl->apertura = apr->value;
                if (var->format != apr->format) {
                    syslog(LOG_ERR,"initCycleBuffers Error CycleBuffers varid=%d appid=%d different format !\n", tpos->idVariable, tpos->idApertura);
                    return 1;
                }
            } else pCtrl->apertura = NULL;
        }
        pCtrl->el_size = varLen(var) + 1 + sizeof (int);
        pCtrl->start = malloc(pCtrl->el_size * tpos->size);
        if (pCtrl->start == NULL) {
            syslog(LOG_ERR,"initCycleBuffers Error not memory for %d size %d!\n", tpos->idVariable, pCtrl->el_size * tpos->size);
            return 1;
        }
        pCtrl->end = pCtrl->start + (pCtrl->el_size * tpos->size);
        initCycleBuff(pCtrl);
        tpos++;
        pCtrl++;
    }
    pthread_mutex_init(&cyclemutex, NULL);
    if (pthread_create(&thread_cycle, NULL, cycle_lister, NULL) != 0) {
        syslog(LOG_ERR,"initCycleBuffers Can't create listen thread for cycle buffer\n");
        return 1;
    }
    return 0;

}

void *cycle_lister(void * arg) {
    int socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in server, client;
    int lport;
    lport = cycle_listen;
    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("cycle_lister Could not create socket for cycle buffer\n");
        pthread_exit(0);
    }
    //    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(lport);

    //Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof (server)) < 0) {
        //print the error message
        printf("cycle_lister bind failed for cycle buffer. Error\n");
        pthread_exit(0);
    }
    //    puts("bind done");

    //Listen
    listen(socket_desc, 3);

    //Accept and incomi7ng connection
    //    puts("Waiting for incoming connections...");
    //    c = sizeof(struct sockaddr_in);


    //Accept and incoming connection
    printf("Waiting for incoming connections for cycle buffer:%d...\n", lport);
    c = sizeof (struct sockaddr_in);
    while ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*) & c))) {
        conn_handler(client_sock);
        close(client_sock);
    }

    if (client_sock < 0) {
        printf("for cycle buffer accept failed\n");
        pthread_exit(0);
    }

}

char namevar[120];

void conn_handler(int sock) {
    //Get the socket descriptor
    int read_size;
    int res;
    //    while (1) {
    memset(namevar, 0, 200);
    read_size = recv(sock, namevar, 199, 0);
    printf("for cycle buffer socket %d name %s len=%d\n", sock, namevar, read_size);
    //Send the message back to client
    if (read_size == 0) {
        printf("for cycle buffer recv failed len=0\n");
        return;
        //            continue;
    } else if (read_size < 0) {
        printf("for cycle buffer recv failed len <0\n");
        return;
        //            pthread_exit(0);
    }
    pthread_mutex_lock(&cyclemutex);
    void *buffer = NULL;
    int len = lenCycleBuffer(namevar);
    if (len == 0) {
        pthread_mutex_unlock(&cyclemutex);
        printf("No data for %s\n", namevar);
        memset(namevar, 0, 200);
        res = write(sock, namevar, sizeof (int));
        return;
    }
    buffer = OUT_BUFFER;
    if (buffer == NULL) {
        pthread_mutex_unlock(&cyclemutex);
        printf("for cycle buffer not memory\n");
        return;
    }

    getCycleBuffer(namevar, buffer);
    pthread_mutex_unlock(&cyclemutex);
    len = *(int *) buffer;
    res = write(sock, buffer, len + sizeof (int));
    printf("sended %d\n", res);
    return;
}

