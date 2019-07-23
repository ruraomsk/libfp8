/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   UDPTransport.h
 * Author: rusin
 *
 * Created on 4 мая 2018 г., 9:19
 */

#ifndef UDPTransport_H
#define UDPTransport_H

typedef struct {
    char *ip1;
    int port1;
    char *ip2;
    int port2;
    char *varbufer;
    int lenvar;
} SetupUDP;

int initUDP(int master, int nomer, SetupUDP *setup);
void sendVariables(void);
int reciveVariables(void);
int getNomer(void);
void closeUDP(void);
int openUDPport(char *ip, int port);
#endif /* UDPTransport_H */
