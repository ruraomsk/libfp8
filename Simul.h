#ifndef SIMUL_H
#define SIMUL_H

void readAllSimul(void);
void writeAllSimul(void);
int initAllSimul(short CodeSub,Driver *drv,char *SimulIP,int SimulPort);
void S_moveUserToDriver();
void S_moveDriverToUser();
#endif /* SIMUL_H */

