/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   netphoto.c
 * Author: rusin
 * 
 * Created on 28 марта 2018 г., 15:44
 */
#include <pthread.h>
#include <syslog.h>
#include "netphoto.h"
#include "variables.h"
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>
#include <stdlib.h> //for threading , link with lpthread

//the thread function
//void connection_handler(int *);
static pthread_t thread_net;
static int socket_desc;
#define port_listen 1080
#define SLEEP_TIME 10

void *lister(void *port) {
    int client_sock, c, *new_sock;
    struct sockaddr_in server, client;

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        syslog(LOG_ERR, "Could not create socket for Netphoto \n");
        pthread_exit(0);
    }
    //    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port_listen);

    //Bind
    while (bind(socket_desc, (struct sockaddr *) &server, sizeof (server)) < 0) {
        //print the error message
        syslog(LOG_ERR, "Netphoto bind failed. Error waiting %d seconds...\n",SLEEP_TIME);
        if (sleep(SLEEP_TIME) != 0)
            pthread_exit(0);
    }
    //Listen
    listen(socket_desc, 10);

    //Accept and incomi7ng connection
    //    puts("Waiting for incoming connections...");
    //    c = sizeof(struct sockaddr_in);


    //Accept and incoming connection
    syslog(LOG_INFO, "Netphoto waiting for incoming connections:%d...\n", port_listen);
    c = sizeof (struct sockaddr_in);
    while ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*) & c))) {
        new_sock = malloc(sizeof(client_sock));
        *new_sock = client_sock;
        connection_handler(new_sock);
    }

    if (client_sock < 0) {
        syslog(LOG_ERR, "Netphoto accept failed\n");
        close(client_sock);
        pthread_exit(0);
    }

}

void connection_handler(int *socket_desc) {
    //Get the socket descriptor
    int sock = *socket_desc;
    int read_size;
    char message[120], client_message[200], *out_message;
    #define len_mess 128000
    //    while (1) {
    memset(client_message, 0, 200);
    read_size = recv(sock, client_message, 199, 0);
    //Send the message back to client
    if (read_size == 0) {
        syslog(LOG_ERR, "Netphoto recv failed\n");
        close(sock);
        free(socket_desc);
        return;
        //            continue;
    } else if (read_size < 0) {
        syslog(LOG_ERR, "Netphoto recv failed\n");
        close(sock);
        free(socket_desc);
        return;
        //            pthread_exit(0);
    }
    if (client_message[0] == 'R') {
        client_message[0] = ' ';
        int stId = 0, endId = 0;
        sscanf(client_message, " %d %d", &stId, &endId);
        out_message = malloc(len_mess);
        if (out_message == NULL) {
            syslog(LOG_ERR, "Netphoto no allocated memory \n");
            close(sock);
            free(socket_desc);
            return;
            //            pthread_exit(0);
        }
        strcpy(out_message, "<vals>\n");
        char *pos=out_message+strlen(out_message);
        if ((endId >= stId) && (stId > 0) && (endId > 0)) {
            for (int i = stId; i <= endId; i++) {
                VarCtrl *vc = findVariable(i);
                if (vc == NULL) continue;
                if (vc->size > 1) continue;
                sprintf(message, "<val id=\"%d\" value=\"%s\"/>\n", i, variableToString(i));
//                strcat(out_message, message);
                int i;
                char *tpos=message;
                while(*tpos!=0){
                    *pos++ =*tpos++;
                }
                *pos++=0;
                if ((pos-out_message)>(len_mess - 1000)) break;
            }
            strcat(out_message, "</vals>\n");
        }
        //Send some messages to the client
        //    sprintf(message,"%6d",strlen(out_message));
        write(sock, out_message, strlen(out_message));
        free(out_message);

    };
    if (client_message[0] == 'W') {
        client_message[0] = ' ';
        int stId = 0;
        char value[120];
        sscanf(client_message, " %d %s", &stId, value);
        syslog(LOG_ERR, "Netphoto id=%d , value=%s\n", stId, value);
        //        printf("%d %s \n", stId, value);
        if (stId > 0) {
            stringToVariable(stId, value);
        }
    }
    if (client_message[0] == 'A') {
        client_message[0] = ' ';
        int stId = 0, lenId = 0;
        sscanf(client_message, " %d %d", &stId, &lenId);
        out_message = malloc(len_mess);
        if (out_message == NULL) {
            syslog(LOG_ERR, "Netphoto no allocated memory \n");
            close(sock);
            free(socket_desc);
            return;
            //            pthread_exit(0);
        }
        strcpy(out_message, " ");
        VarCtrl *vc = findVariable(stId);
        if (vc == NULL) {
            syslog(LOG_ERR, "Netphoto not found variable %d\n", stId);
            close(sock);
            free(socket_desc);
            return;
        }
        for (int i = 0; i < lenId; i++) {
            sprintf(message, "%s ", variableArrayToString(stId, i));
            strcat(out_message, message);
            if (strlen(out_message)>(len_mess - 1000)) break;
        }
        write(sock, out_message, strlen(out_message));
        free(out_message);
    }

    close(sock);
    free(socket_desc);
    return;
    //    }
}

void initNetPhoto() {
    if (pthread_create(&thread_net, NULL, lister, NULL) != 0) {
        syslog(LOG_ERR, "Netphoto Can't create listen thread\n");
    }
}

void stopNetPhoto() {
    close(socket_desc);
    syslog(LOG_ERR, "netPhoto stoped...\n");
}
