/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   modbus-cpc.c
 * Author: rusin
 * 
 * Created on 27 марта 2018 г., 10:10
 */

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>

#include "variables.h"
#include <modbus/modbus.h>
#include "modbus-cpc.h"

static int work = 1;
static ModbusDevice *mdStep;

/* Возвращает размер переменной в регистрах
 */
int lenRegister(char format) {
    switch (format) {
        case boolean:
        case char1b:
            return 1;
        case uint2b:
        case sint2b:
            return 1;
        case uint4b:
        case sint4b:
        case float4b:
            return 2;
        case sint8b:
        case float8b:
            return 4;
    }
    return 0;
}

/* Возвращает размер переменной в байтах
 */
int lenBytes(char format) {
    switch (format) {
        case boolean:
        case char1b:
            return 1;
        case uint2b:
        case sint2b:
            return 2;
        case uint4b:
        case sint4b:
        case float4b:
            return 4;
        case sint8b:
        case float8b:
            return 8;
    }
    return 0;
}

/* Возвращает размер в регистрах для данногог типа регистров
 */
int getSizeType(ModbusRegister *reg) {
    int n = 0;
    while (reg->format != 0) {
        int t = reg->address + lenRegister(reg->format);
        n = n < t ? t : n;
        reg++;
    };
    return n;
}

static ModbusDevice *mdStart;
static pthread_t thread;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int driversRun = 0;

void runDrivers() {
    driversRun = 1;
}

void stopAll() {
    work = 0;
}

/* Перезапуск всех соеденинений если они порвались
 */
void *Reconnect(void *args) {
    modbus_t *ctx;
    while (1) {
        if (!work)
            pthread_exit(0);
        if (sleep(RECONNECT_TIME) != 0)
            pthread_exit(0);
        updateDataSaver();
        if (driversRun)
            WakeUpDriver();
        ModbusDevice *md = mdStart;
        while (md->port >= 0) {
            if (md->typeDevice == 1) {
                if (md->context->ctx1 == NULL) {
                    syslog(LOG_INFO, "Reconnect %s \n", md->ip1);
                    ctx = modbus_new_tcp(md->ip1, md->port);
                    if (modbus_connect(ctx) == -1) {
                        syslog(LOG_ERR, "Master Connection failed on ip=%s error %s\n", md->ip1, modbus_strerror(errno));
                        modbus_free(ctx);
                        md->context->ctx1 = NULL;
                    } else {
                        pthread_mutex_lock(&mutex);
                        md->context->ctx1 = ctx;
                        pthread_mutex_unlock(&mutex);
                        syslog(LOG_INFO, "Reconnect Sucsess!\n");
                    }
                }
                if (md->context->ctx2 == NULL) {
                    syslog(LOG_INFO, "Reconnect %s \n", md->ip2);
                    ctx = modbus_new_tcp(md->ip2, md->port);
                    if (modbus_connect(ctx) == -1) {
                        syslog(LOG_ERR, "Master Connection failed on ip=%s error %s\n", md->ip2, modbus_strerror(errno));
                        modbus_free(ctx);
                        md->context->ctx2 = NULL;
                    } else {
                        pthread_mutex_lock(&mutex);
                        md->context->ctx2 = ctx;
                        pthread_mutex_unlock(&mutex);
                        syslog(LOG_INFO, "Reconnect Sucsess!\n");
                    }
                }
            }
            md++;
        }
    }
}

void *OnlySaveVariable(void *args) {
    while (1) {
        if (!work)
            pthread_exit(0);
        if (sleep(RECONNECT_TIME) != 0)
            pthread_exit(0);
        updateDataSaver();
        if (driversRun)
            WakeUpDriver();
    }
}

/* производит и инициализацию всех драйверов модбаса
 */

void initModbusDevices(ModbusDevice *modbuses) {
    mdStart = modbuses;
    int isMaster = 0;
    ModbusDevice *md = mdStart;
    mdStep = mdStart;
    pthread_mutex_init(&mutex, NULL);
    while (md->port >= 0) {
        if (md->typeDevice == 0) {
            md->context = slsrv_init(md);
            if (md->context == NULL) {
                syslog(LOG_ERR, "InitModbusDevices Error init slave device on port %d\n", md->port);
            }
        }
        if (md->typeDevice == 1) {
            md->context = master_init(md);
            isMaster = 1;
            if (md->context == NULL) {
                syslog(LOG_ERR, "InitModbusDevices Error init master device on %s %s\n", md->ip1, md->ip2);
            }
        }
        md++;
    };
    if (isMaster) {
        if (pthread_create(&thread, NULL, Reconnect, NULL) != 0) {
            syslog(LOG_ERR, "InitModbusDevices Can't create Reconnect thread\n");
        }
    } else {
        if (pthread_create(&thread, NULL, OnlySaveVariable, NULL) != 0) {
            syslog(LOG_ERR, "InitModbusDevices Can't create OnlySaveVariable thread\n");
        }
    }
}

/* делает один шаг всех драйверов модбаса
 */
void makeStepModbusDevices() {
    if (mdStep->port < 0) mdStep = mdStart;
    if (mdStep->typeDevice == 0) {
        pthread_mutex_lock(&mutex);
        if (slsrv_step(mdStep) != 0) {
            syslog(LOG_ERR, "makeStepModbusDevices Error in/out on port %d\n", mdStep->port);
        }
        pthread_mutex_unlock(&mutex);
    }
    if (mdStep->typeDevice == 1) {
        pthread_mutex_lock(&mutex);
        if (master_step(mdStep) != 0) {
            syslog(LOG_ERR, "makeStepModbusDevices Error in/out on %s %s \n", mdStep->ip1, mdStep->ip2);
        }
        pthread_mutex_unlock(&mutex);
    }
    mdStep++;
}

/* Переписывает из области программы в область дискретных регистров устройства 
 */
void writeDiscRegs(ModbusDevice *md) {
    ModbusRegister *di = md->di;
    while (di->format != 0) {
        modbus_mapping_t *map = md->context->mb_mapping;
        map->tab_input_bits[di->address] = (*((char *) di->value)) ? 0x1 : 0x00;
        di++;
    }
}

/* Переписывает из области дискретных регистров устройства в область программы   
 */
void readDiscRegs(ModbusDevice *md) {
    ModbusRegister *di = md->di;
    while (di->format != 0) {
        modbus_mapping_t *map = md->context->mb_mapping;
        *((char *) di->value) = map->tab_input_bits[di->address];
        di++;
    }
}

/* Переписывает из области программы в область регистров чтения устройства 
 */
void writeInpRegs(ModbusDevice *md) {
    ModbusRegister *ir = md->ir;
    while (ir->format != 0) {
        modbus_mapping_t *map = md->context->mb_mapping;
        switch (ir->format) {
            case char1b:
                *((char *) map->nb_input_registers + ir->address) = *((char *) ir->value);
                break;
            case uint2b:
            case sint2b:
                //                swapBytes(ir->value, map->tab_input_registers + ir->address, lenBytes(ir->format));
                memcpy(map->tab_input_registers + ir->address, ir->value, lenBytes(ir->format));
                break;
            case uint4b:
            case sint4b:
            case float4b:
                MODBUS_SET_INT32_TO_INT16(map->tab_input_registers, ir->address, *((int *) ir->value));
                break;
            case sint8b:
            case float8b:
                MODBUS_SET_INT64_TO_INT16(map->tab_input_registers, ir->address, *((long long int *) ir->value));
                break;
        }
        ir++;
    }
}

/* Переписывает из области регистров чтения устройства в область программы  
 */
void readInpRegs(ModbusDevice *md) {
    ModbusRegister *ir = md->ir;
    while (ir->format != 0) {
        modbus_mapping_t *map = md->context->mb_mapping;
        switch (ir->format) {
            case char1b:
                *((char *) ir->value) = *((char *) map->nb_input_registers + ir->address);
                break;
            case uint2b:
            case sint2b:
                //                swapBytes(map->tab_input_registers + ir->address, ir->value, 2);
                memcpy(ir->value, map->tab_input_registers + ir->address, 2);
                break;
            case uint4b:
            case sint4b:
            case float4b:
                *((int *) ir->value) = MODBUS_GET_INT32_FROM_INT16(map->tab_input_registers, ir->address);
                break;
            case sint8b:
            case float8b:
                *((long long int *) ir->value) = MODBUS_GET_INT64_FROM_INT16(map->tab_input_registers, ir->address);
                break;
        }
        ir++;
    }
}

/* Переписывает из области двоичных регистров чтения устройства в область программы   
 */
void readCoils(ModbusDevice *md) {
    ModbusRegister *coil = md->coil;
    while (coil->format != 0) {
        modbus_mapping_t *map = md->context->mb_mapping;
        //        map->tab_input_bits[di->address]=*((char *)di->value);
        *((char *) coil->value) = map->tab_bits[coil->address];
        coil++;
    }
}

/* Переписывает из области программы в область двоичных регистров чтения устройства     
 */
void writeCoils(ModbusDevice *md) {
    ModbusRegister *coil = md->coil;
    while (coil->format != 0) {
        modbus_mapping_t *map = md->context->mb_mapping;
        //        map->tab_input_bits[di->address]=*((char *)di->value);
        map->tab_bits[coil->address] = (*((char *) coil->value)) ? 0x1 : 0x00;
        coil++;
    }
}

/* Переписывает из области  регистров управления устройства в область программы   
 */
void readHoldRegs(ModbusDevice *md) {
    ModbusRegister *hr = md->hr;
    while (hr->format != 0) {
        modbus_mapping_t *map = md->context->mb_mapping;
        switch (hr->format) {
            case char1b:
                *((char *) hr->value) = *((char *) map->nb_registers + hr->address);
                break;
            case uint2b:
            case sint2b:
                //                swapBytes(map->tab_registers + hr->address, hr->value, lenBytes(hr->format));
                memcpy(hr->value, map->tab_registers + hr->address, lenBytes(hr->format));
                break;
            case uint4b:
            case sint4b:
            case float4b:
                *((int *) hr->value) = MODBUS_GET_INT32_FROM_INT16(map->tab_registers, hr->address);
                break;
            case sint8b:
            case float8b:
                *((long long int *) hr->value) = MODBUS_GET_INT64_FROM_INT16(map->tab_registers, hr->address);
                break;
        }
        hr++;
    }
}

/* Переписывает из области программы  в область регистров управления устройства    
 */
void writeHoldRegs(ModbusDevice *md) {
    ModbusRegister *hr = md->hr;
    while (hr->format != 0) {
        modbus_mapping_t *map = md->context->mb_mapping;
        switch (hr->format) {
            case char1b:
                *((char *) map->nb_registers + hr->address) = *((char *) hr->value);
                break;
            case uint2b:
            case sint2b:
                //                swapBytes(hr->value, map->tab_registers + hr->address, 2);
                memcpy(map->tab_registers + hr->address, hr->value, 2);
                break;
            case uint4b:
            case sint4b:
            case float4b:
                MODBUS_SET_INT32_TO_INT16(map->tab_registers, hr->address, *((int *) hr->value));
                break;
            case sint8b:
            case float8b:
                MODBUS_SET_INT64_TO_INT16(map->tab_registers, hr->address, *((long long int *) hr->value));
                break;
        }
        hr++;
    }
}

/* все регистры всех устройсв записываются данными программы 
 */
void initAllregistersModubus() {
    ModbusDevice *md = mdStart;
    while (md->port >= 0) {
        writeDiscRegs(md);
        writeInpRegs(md);
        writeCoils(md);
        writeHoldRegs(md);
        md++;
    };
}

/* Чтение регистров устройств в память программы в зависимости от типа устройства 
 */
void readAllModbus() {
    ModbusDevice *md = mdStart;
    while (md->port >= 0) {
        if (md->typeDevice == 0) {
            readCoils(md);
            readHoldRegs(md);
        } else {
            readDiscRegs(md);
            readInpRegs(md);
        }
        md++;
    };
}

/* Запись регистров устройств из памяти программы в зависимости от типа устройства 
 */

void writeAllModbus() {
    ModbusDevice *md = mdStart;
    while (md->port >= 0) {
        if (md->typeDevice == 0) {
            writeDiscRegs(md);
            writeInpRegs(md);
        } else {
            writeCoils(md);
            writeHoldRegs(md);
        }
        md++;
    };
}

/*  Инициализация Модбас мастера на два канала
 */
ContextServer *master_init(ModbusDevice *mdev) {
    if (mdev == NULL)
        return NULL;

    if (mdev->typeDevice != 1)
        return NULL; // не master устройство
    ContextServer *cnt = malloc(sizeof (ContextServer));
    if (cnt == NULL)
        return NULL;
    cnt->md = mdev;
    cnt->port = mdev->port;
    cnt->ctx1 = modbus_new_tcp(mdev->ip1, cnt->port);
    cnt->ctx2 = modbus_new_tcp(mdev->ip2, cnt->port);
    
    modbus_set_response_timeout(cnt->ctx1,0,200000);
    modbus_set_response_timeout(cnt->ctx2,0,200000);

    cnt->ncoil = getSizeType(mdev->coil);
    cnt->ndi = getSizeType(mdev->di);
    cnt->nir = getSizeType(mdev->ir);
    cnt->nhr = getSizeType(mdev->hr);

    cnt->mb_mapping1 = modbus_mapping_new(cnt->ncoil, cnt->ndi, cnt->nhr, cnt->nir);
    if (cnt->mb_mapping1 == NULL) {
        syslog(LOG_ERR, "master_init Master Failed to allocate the mapping %s\n", mdev->ip1);
        modbus_free(cnt->ctx1);
        modbus_free(cnt->ctx2);
        free(cnt);
        return NULL;
    }
    cnt->mb_mapping2 = modbus_mapping_new(cnt->ncoil, cnt->ndi, cnt->nhr, cnt->nir);
    if (cnt->mb_mapping2 == NULL) {
        syslog(LOG_ERR, "master_init Master Failed to allocate the mapping %s\n", mdev->ip2);
        modbus_free(cnt->ctx);
        modbus_free(cnt->ctx2);
        modbus_mapping_free(cnt->mb_mapping);
        free(cnt);
        return NULL;
    }

    if (modbus_connect(cnt->ctx1) == -1) {
        syslog(LOG_ERR, "master_init Master Connection failed on ip=%s error%s\n", mdev->ip1, modbus_strerror(errno));
        modbus_free(cnt->ctx1);
        cnt->ctx1 = NULL;
    }
    if (modbus_connect(cnt->ctx2) == -1) {
        syslog(LOG_ERR, "master_init Master Connection failed on ip=%s error%s\n", mdev->ip2, modbus_strerror(errno));
        modbus_free(cnt->ctx2);
        cnt->ctx2 = NULL;
    }
    cnt->ctx = cnt->ctx1;
    cnt->mb_mapping = cnt->mb_mapping1;
    if (cnt->ctx1 == NULL) {
        cnt->ctx = cnt->ctx2;
        cnt->mb_mapping = cnt->mb_mapping2;
    }
    FD_ZERO(&cnt->refset);
    if (cnt->ctx1 != NULL) {
        FD_SET(modbus_get_socket(cnt->ctx1), &cnt->refset);
    }
    if (cnt->ctx2 != NULL) {
        FD_SET(modbus_get_socket(cnt->ctx2), &cnt->refset);
    }
    cnt->stage = 0;
    gettimeofday(&cnt->tstart, NULL);
    cnt->step = mdev->step;
    return cnt;
}

/* Инициализация мобдбас сервера 
 */
ContextServer *slsrv_init(ModbusDevice *mdev) {
    if (mdev == NULL)
        return NULL;

    if (mdev->typeDevice != 0)
        return NULL; // не slave устройство
    ContextServer *cnt = malloc(sizeof (ContextServer));
    if (cnt == NULL)
        return NULL;
    cnt->port = mdev->port;
    cnt->ctx = modbus_new_tcp("0.0.0.0", cnt->port);
    // modbus_set_indication_timeout(cnt->ctx,1,200000);
    cnt->ncoil = getSizeType(mdev->coil);
    cnt->ndi = getSizeType(mdev->di);
    cnt->nir = getSizeType(mdev->ir);
    cnt->nhr = getSizeType(mdev->hr);

    cnt->mb_mapping = modbus_mapping_new(cnt->ncoil, cnt->ndi, cnt->nhr, cnt->nir);
    if (cnt->mb_mapping == NULL) {
        syslog(LOG_ERR, "slsrv_init Server Failed to allocate the mapping\n");
        modbus_free(cnt->ctx);
        free(cnt);
        return NULL;
    }

    cnt->server_socket = modbus_tcp_listen(cnt->ctx, NB_CONNECTION);
    if (cnt->server_socket == -1) {
        syslog(LOG_ERR, "slsrv_init Unable to listen TCP connection on port %d\n", cnt->port);
        modbus_free(cnt->ctx);
        free(cnt);
        return NULL;
    }

    syslog(LOG_INFO, "Listen port %d ... \n", cnt->port);

    /* Clear the reference set of socket */
    FD_ZERO(&cnt->refset);
    /* Add the server socket */
    FD_SET(cnt->server_socket, &cnt->refset);

    /* Keep track of the max file descriptor */
    cnt->fdmax = cnt->server_socket;
    return cnt;
}

void move_mapping(const modbus_mapping_t *src, modbus_mapping_t *des) {
    for (int i = 0; i < src->nb_bits; i++) {
        des->tab_bits[i] = src->tab_bits[i];
    }
    for (int i = 0; i < src->nb_input_bits; i++) {
        des->tab_input_bits[i] = src->tab_input_bits[i];
    }
    for (int i = 0; i < src->nb_input_registers; i++) {
        des->tab_input_registers[i] = src->tab_input_registers[i];
    }
    for (int i = 0; i < src->nb_registers; i++) {
        des->tab_registers[i] = src->tab_registers[i];
    }
}

/* Закрываем соединение потому что ошибка
 */
void close_connect(modbus_t *ctx) {
    if (ctx == NULL)
        return;
    modbus_close(ctx);
    modbus_free(ctx);
}

long int time_after(struct timeval *tv1) {
    struct timeval tv2, dtv;
    gettimeofday(&tv2, NULL);
    dtv.tv_sec = tv2.tv_sec - tv1->tv_sec;
    dtv.tv_usec = tv2.tv_usec - tv1->tv_usec;
    if (dtv.tv_usec < 0) {
        dtv.tv_sec--;
        dtv.tv_usec += 1000000;
    }
    return dtv.tv_sec * 1000 + dtv.tv_usec / 1000;
}

/*  Шаг Модбас Мастера
 */
int master_step(ModbusDevice *md) {
    if (md->typeDevice != 1)
        return -1;
    ContextServer *cnt = md->context;
    if (cnt == NULL)
        return -1;
    int now = clock();
    cnt->ctx = cnt->ctx1;
    cnt->mb_mapping = cnt->mb_mapping1;
    if (cnt->ctx1 == NULL) {
        cnt->ctx = cnt->ctx2;
        cnt->mb_mapping = cnt->mb_mapping2;
    }
    // если есть хотя бы одно соединение работаем
    if (cnt->ctx == NULL)
        return 0;
    // уравниваем регистровые поля на обеих устройствах
    if (cnt->ctx1 != NULL && cnt->ctx1 != cnt->ctx) {
        move_mapping(cnt->mb_mapping, cnt->mb_mapping1);
    }
    if (cnt->ctx2 != NULL && cnt->ctx2 != cnt->ctx) {
        move_mapping(cnt->mb_mapping, cnt->mb_mapping2);
    }
    if (cnt->stage == 0) {
        if (time_after(&cnt->tstart) < cnt->step)
            return 0;
    }
    if (cnt->stage == 0) {
        // отправляем все Coils
        if (cnt->ncoil != 0) {
            if (cnt->ctx1 != NULL) {
                if (modbus_write_bits(cnt->ctx1, cnt->mb_mapping1->start_bits, cnt->mb_mapping1->nb_bits, cnt->mb_mapping1->tab_bits) < 0) {
                    close_connect(cnt->ctx1);
                    cnt->ctx1 = NULL;
                };
            }
            if (cnt->ctx2 != NULL) {
                if (modbus_write_bits(cnt->ctx2, cnt->mb_mapping2->start_bits, cnt->mb_mapping2->nb_bits, cnt->mb_mapping2->tab_bits) < 0) {
                    close_connect(cnt->ctx2);
                    cnt->ctx2 = NULL;
                };
            }
        }
    }
    if (cnt->stage == 1) {
        // Читаем  все Di
        if (cnt->ndi != 0) {
            if (cnt->ctx1 != NULL) {
                if (modbus_read_input_bits(cnt->ctx1, cnt->mb_mapping1->start_input_bits, cnt->mb_mapping1->nb_input_bits, cnt->mb_mapping1->tab_input_bits) < 0) {
                    close_connect(cnt->ctx1);
                    cnt->ctx1 = NULL;
                };
            }
            if (cnt->ctx2 != NULL) {
                if (modbus_read_input_bits(cnt->ctx2, cnt->mb_mapping2->start_input_bits, cnt->mb_mapping2->nb_input_bits, cnt->mb_mapping2->tab_input_bits) < 0) {
                    close_connect(cnt->ctx2);
                    cnt->ctx2 = NULL;
                };
            }
        }
    }
    if (cnt->stage == 2) {
        // Читаем  все IR
        if (cnt->nir != 0) {
            if (cnt->ctx1 != NULL) {
                if (modbus_read_input_registers(cnt->ctx1, cnt->mb_mapping1->start_input_registers, cnt->mb_mapping1->nb_input_registers, cnt->mb_mapping1->tab_input_registers) < 0) {
                    close_connect(cnt->ctx1);
                    cnt->ctx1 = NULL;
                };
            }
            if (cnt->ctx2 != NULL) {
                if (modbus_read_input_registers(cnt->ctx2, cnt->mb_mapping2->start_input_registers, cnt->mb_mapping2->nb_input_registers, cnt->mb_mapping2->tab_input_registers) < 0) {
                    close_connect(cnt->ctx2);
                    cnt->ctx2 = NULL;
                };
            }
        }
    }
    if (cnt->stage == 3) {
        // Читаем  все Holds
        if (cnt->nhr != 0) {
            if (cnt->ctx2 != NULL) {
                if (modbus_write_registers(cnt->ctx1, cnt->mb_mapping1->start_registers, cnt->mb_mapping1->nb_registers, cnt->mb_mapping1->tab_registers) < 0) {
                    close_connect(cnt->ctx1);
                    cnt->ctx1 = NULL;
                };
            }
            if (cnt->ctx2 != NULL) {
                if (modbus_write_registers(cnt->ctx2, cnt->mb_mapping2->start_registers, cnt->mb_mapping2->nb_registers, cnt->mb_mapping2->tab_registers) < 0) {
                    close_connect(cnt->ctx2);
                    cnt->ctx2 = NULL;
                };
            }
        }
    }
    cnt->stage++;
    if (cnt->stage > 3) {
        cnt->stage = 0;
        gettimeofday(&cnt->tstart, NULL);
    }
    return 0;
}

/* Шаг мобдбас сервера 
 */
int slsrv_step(ModbusDevice *md) {
    if (md->typeDevice != 0)
        return -1;
    ContextServer *cnt = md->context;
    if (cnt == NULL)
        return -1;

    fd_set rdset = cnt->refset;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    int result = select(cnt->fdmax + 1, &rdset, NULL, NULL, &timeout);
    if (result == 0)
        return 0;
    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
    for (int master_socket = 0; master_socket <= cnt->fdmax; master_socket++) {

        if (!FD_ISSET(master_socket, &rdset)) {
            continue;
        }

        if (master_socket == cnt->server_socket) {
            /* A client is asking a new connection */
            //                printf("New connect..\n");
            socklen_t addrlen;
            struct sockaddr_in clientaddr;
            int newfd;

            /* Handle new connections */
            addrlen = sizeof (clientaddr);
            memset(&clientaddr, 0, sizeof (clientaddr));
            newfd = accept(cnt->server_socket, (struct sockaddr *) &clientaddr, &addrlen);
            if (newfd == -1) {
                syslog(LOG_ERR, "slsrv_step Server accept() error");
            } else {
                FD_SET(newfd, &cnt->refset);

                if (newfd > cnt->fdmax) {
                    /* Keep track of the maximum */
                    cnt->fdmax = newfd;
                }
                syslog(LOG_INFO, "New connection from %s:%d on socket %d\n",
                        inet_ntoa(clientaddr.sin_addr), (int) clientaddr.sin_port, newfd);
            }
        } else {
            modbus_set_socket(cnt->ctx, master_socket);
            int rc = modbus_receive(cnt->ctx, query);
            //            printf("receive.");
            if (rc > 0) {

                modbus_reply(cnt->ctx, query, rc, cnt->mb_mapping);
                //                printf("response\n");
            } else if (rc == -1) {
                syslog(LOG_INFO, "Connection closed on socket %d\n", master_socket);
                close(master_socket);
                /* Remove from reference set */
                FD_CLR(master_socket, &cnt->refset);
                if (master_socket == cnt->fdmax) {
                    cnt->fdmax--;
                }
            }
        }
    }
    return 0;
}

/* Останов мобдбас сервера 
 */
void slsrv_stop(ModbusDevice *md) {
    if (md->typeDevice != 0)
        return;
    ContextServer *cnt = md->context;
    if (cnt->server_socket != -1) {
        close(cnt->server_socket);
    }
    modbus_free(cnt->ctx);
    modbus_mapping_free(cnt->mb_mapping);
    free(cnt);
}
