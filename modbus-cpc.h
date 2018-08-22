/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   modbus-cpc.h
 * Author: rusin
 *
 * Created on 27 марта 2018 г., 10:10
 */

#ifndef MODBUS_CPC_H
#define MODBUS_CPC_H
#include <modbus/modbus.h>
#include <sys/time.h>
#define NB_CONNECTION    20
#define RECONECT_TIME    5

typedef struct __attribute__((packed)){
    int port;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;
    modbus_t *ctx1;
    modbus_mapping_t *mb_mapping1;
    modbus_t *ctx2;
    modbus_mapping_t *mb_mapping2;
    int server_socket;
    fd_set refset;
    /* Maximum file descriptor number */
    int fdmax;
    void *md;
    int ncoil;
    int ndi;
    int nir;
    int nhr;
    int stage;
    struct timeval tstart;
    int step;
    
} ContextServer;

typedef struct __attribute__((packed)){
    void *value;
    char format;
    short address;
}ModbusRegister;
typedef struct __attribute__((packed)){
    char typeDevice;
    int port;
    ModbusRegister *coil;
    ModbusRegister *di;
    ModbusRegister *ir;
    ModbusRegister *hr;
    ContextServer  *context;
    char *ip1;
    char *ip2;
    int step;
} ModbusDevice;

ContextServer * slsrv_init( ModbusDevice *md);
int slsrv_step(ModbusDevice *md);
ContextServer * master_init( ModbusDevice *md);
int master_step(ModbusDevice *md);
void initModbusDevices(ModbusDevice *md);
void makeStepModbusDevices(void);
void initAllregistersModubus(void);
void readAllModbus(void);
void writeAllModbus(void);
void runDrivers(void);
void stopAll(void);
#define ADDR_START 0
#define OP_OF_STEP 2 // число операцмй вывода за шаг


#endif /* MODBUS_CPC_H */
