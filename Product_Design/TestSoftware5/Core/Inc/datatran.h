#ifndef __DATATRAN_H
#define __DATATRAN_H	 

#include "stm32f3xx_hal.h"
#include "stm32f3xx_hal_uart.h"
#include "usart.h"

#define RXBUFFERSIZE 1 														//command,voltage/current,voltage value/current value
#define USART_REC_LEN 200  												//Maxima receive data 
#define Voltage 0
#define Current 1
#define Start 		0x61		//a
#define End				0x62		//b
#define Connect 	0x73 		//s
#define CV				0x76		//v
#define CC				0x63		//c
#define Stop			0x65		//e
#define Control 	0x74		//t
extern uint8_t aRXBuffer[RXBUFFERSIZE];
extern uint8_t USART_RX_BUF[USART_REC_LEN];     	//Receiver buffer
extern uint16_t USART_RX_STA;      							 //Buffer actual length
extern struct  _FLAG    DF;
extern struct  _Ctr_value  CtrValue;

void DataTran(void);
void DataAna(void);
//extern void Uart1_Send(uint8_t *aTxBuffer);

struct  _FLAG
{
	uint16_t  RunFlag;	// C.V/C.C mode setting
	uint16_t  ErrFlag;
	uint8_t ConFlag;		//PC Connection Flag
};

struct  _Ctr_value
{
	int32_t		Voref;		//Reference voltage
	int32_t		Ioref;		//Reference current
};
#endif
