#ifndef __CTLLOOP_H
#define __CTLLOOP_H	 

#include "stm32f3xx_hal.h"
#include "function.h"

void BuckBoostVLoopCtlPID(void);
void BuckBoostILoopCtlPID(void);
extern int32_t  VErr0,VErr1,VErr2;
extern int32_t  IErr0,IErr1,IErr2;
extern int32_t	u0,u1;

//һ���������������� 
#define PERIOD 10240	 
#endif
