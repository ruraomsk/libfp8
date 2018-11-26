/*
 * Библиотека работа с сохраненными константами в формате JSON
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>

#include "variables.h"
#include "jfes.h"

#define buffer_size 32000

static char *filenamesaver;
static char *json_data;
static char *json_data_file;
static int json_data_len;
static VarSaveCtrl *sVarSaveCtrl;
static pthread_mutex_t mutex_json=PTHREAD_MUTEX_INITIALIZER;
static int count_maker_saves;
static int interval_makers;

int set_file_content(const char *filename, const char *content, unsigned long content_size) {
    if (!filename || !content || content_size == 0) {
        return 0;
    }
    FILE *f = fopen(filename, "w");
    if (!f) {
        return 0;
    }
    int result = (int) fwrite(content, sizeof (char), content_size, f);
    fclose(f);
    return result;
}

unsigned long get_file_content(const char *filename, char *content, unsigned long max_content_size) {
    if (!filename || !content || !max_content_size || max_content_size == 0) {
        return 0L;
    }
    int descriptor = open(filename, O_RDONLY);
    if (descriptor != -1) {
        struct stat statistics;
        if (fstat(descriptor, &statistics) != -1) {
            if (statistics.st_size > max_content_size) {
                close(descriptor);
                return 0L;
            }
        } else {
            close(descriptor);
            return 0L;
        }
    }
    close(descriptor);

    FILE *f = fopen(filename, "rb");
    if (!f) {
        return 0L;
    }
    unsigned long content_size = fread(content, sizeof (char), max_content_size, f);
    fclose(f);
    return content_size;
}

int findIdFromName(char *name) {
    VarSaveCtrl *vsc = sVarSaveCtrl;
    if (vsc == NULL) return 0;
    while (vsc->nameValue != NULL) {
        if (strcmp(name, vsc->nameValue) == 0) return vsc->idVariable;
        vsc++;
    }
    return 0;
}

char * getJsonString(void *json_data, jfes_token_t tok) {
    char *str = malloc((tok.end - tok.start + 1) * sizeof (char));
    if (str == NULL) return NULL;
    memcpy(str, json_data + tok.start, (tok.end - tok.start) * sizeof (char));
    *(str + tok.end - tok.start) = 0;
    return str;
}

int getBooleanValue(char *strValue) {
    if (strcmp(strValue, "false")) return 0;
    if (strcmp(strValue, "FALSE")) return 0;
    if (strcmp(strValue, "true")) return 1;
    if (strcmp(strValue, "TRUE")) return 1;
    if (strcmp(strValue, "1")) return 1;
    return 0;
}

void setValueAsBool(char *name, int value) {
    int id = findIdFromName(name);
    if (id == 0) {
        syslog(LOG_ERR,"Don't write %s as bool =%d not find id\n", name, value);
        return;
    }
    setAsBool(id,value);
//    syslog(LOG_ERR,"Write %s id=%d as bool =%d \n", name, id, value);
}

int getIntegerValue(char *strValue) {
    return atoi(strValue);
}

void setValueAsInteger(char *name, int value) {
    int id = findIdFromName(name);
    if (id == 0) {
        syslog(LOG_ERR,"Don't write %s as integer =%d not find id\n", name, value);
        return;
    }
    VarCtrl *var=findVariable(id);
    if(var==NULL){
        syslog(LOG_ERR,"Don't write %s as integer =%d not find id\n", name, value);
        return;
    }
    switch(var->format){
        case uint2b:
        case sint2b:
            setAsShort(id,(short)(value & 0xFFFF));
//            syslog(LOG_ERR,"Write %s is id=%d as short =%d \n", name, id, value);
            break;
        case uint4b:
        case sint4b:
            setAsInt(id,value);
//            syslog(LOG_ERR,"Write %s is id=%d as integer =%d \n", name, id, value);
            break;
        case sint8b:
            setAsLong(id,(long long int)value);
//            syslog(LOG_ERR,"Write %s is id=%d as long =%d \n", name, id, value);
            break;
    }
}

double getFloatValue(char *strValue) {
    return atof(strValue);
}

void setValueAsFloat(char *name, double value) {
    int id = findIdFromName(name);
    if (id == 0) {
        syslog(LOG_ERR,"Don't write %s as float =%f not find id\n", name, value);
        return;
    }
    VarCtrl *var=findVariable(id);
    if(var==NULL){
        syslog(LOG_ERR,"Don't write %s as float =%d not find id\n", name, value);
        return;
    }
    switch(var->format){
        case float4b:
            setAsFloat(id,(float)value);
//            syslog(LOG_ERR,"Write %s is id=%d as float =%d \n", name, id, value);
            break;
        case float4b:
            setAsDouble(id,value);
//            syslog(LOG_ERR,"Write %s is id=%d as double =%d \n", name, id, value);
            break;
    }
}

int loadVariablesFromJSON(char *fileName) {
    json_data = malloc(buffer_size * sizeof (char));
    if (json_data == NULL) {
        syslog(LOG_ERR,"Не хватает памяти\n");
        return (EXIT_FAILURE);

    }
    if (!get_file_content(fileName, json_data, buffer_size)) {
        syslog(LOG_ERR,"Нет такого файла %s или ошибка чтения\n", fileName);
        return (EXIT_FAILURE);
    }

    jfes_parser_t parser;
    jfes_token_t tokens[2048];
    jfes_size_t tokens_count = 2048;

    jfes_config_t config;
    config.jfes_malloc = (jfes_malloc_t) malloc;
    config.jfes_free = free;

    jfes_init_parser(&parser, &config);
    jfes_status_t status = jfes_parse_tokens(&parser, json_data, buffer_size, tokens, &tokens_count);
    //    jfes_value_t value;
    //    char *buff =json_data+tokens[i].start;
    //    jfes_parse_to_value(&config, json_data, buffer_size, &value);
    int i = 1;
    for (int j = 0; j < tokens[0].size; j++) {
        char *name;
        char *strValue;
        name = getJsonString(json_data, tokens[i++]);
        jfes_token_type_t type = tokens[i].type;
        strValue = getJsonString(json_data, tokens[i++]);
        switch (type) {
            case jfes_type_boolean:
                setValueAsBool(name, getBooleanValue(strValue));
                break;
            case jfes_type_integer:
                setValueAsInteger(name, getIntegerValue(strValue));
                break;
            case jfes_type_double:
                setValueAsFloat(name, getFloatValue(strValue));
                break;
        }
        free(name);
        free(strValue);
    }
    free(json_data);
}
void initSaver(char *filename, VarSaveCtrl *varSaveCtrls,int interval) {
    filenamesaver = filename;
    sVarSaveCtrl = varSaveCtrls;
    interval_makers=interval;
    if (loadVariablesFromJSON(filename) != 0) {
//        sVarSaveCtrl = NULL;
    }
    json_data=malloc(buffer_size*sizeof(char));
    if(json_data==NULL){
        sVarSaveCtrl = NULL;
//        syslog(LOG_ERR,"Мало памяти для json\n");
        return;
    }
    json_data_file=malloc(buffer_size*sizeof(char));
    if(json_data_file==NULL){
        free(json_data);
        sVarSaveCtrl = NULL;
//        syslog(LOG_ERR,"Мало памяти для json\n");
        return;
    }
    json_data_len=0;
    count_maker_saves=interval_makers;
}
void makeSaveData(){
    if(count_maker_saves-->0) return;
    count_maker_saves=interval_makers;
    pthread_mutex_lock (&mutex_json);
    VarSaveCtrl *vsc = sVarSaveCtrl;
    if (vsc == NULL) return;
    json_data[0]=0;
    strcat(json_data,"{\n");
    while (vsc->nameValue != NULL) {
        strcat(json_data,"    \"");
        strcat(json_data,vsc->nameValue);
        strcat(json_data,"\": ");
        strcat(json_data,variableToString(vsc->idVariable));
        strcat(json_data,",\n");
        vsc++;
    }
    strcat(json_data,"}\n");
    json_data_len=strlen(json_data);
    pthread_mutex_unlock (&mutex_json);
    printf(".");    
}
void updateDataSaver(){
    pthread_mutex_lock (&mutex_json);
    int tmp_len=json_data_len;
    memcpy(json_data_file,json_data,tmp_len);
    pthread_mutex_unlock (&mutex_json);
    set_file_content(filenamesaver,json_data_file,tmp_len);
}
void closeSaver(){
    free(json_data);
    free(json_data_file);
}
