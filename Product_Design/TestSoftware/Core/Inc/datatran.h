#ifndef __DATATRAN_H
#define __DATATRAN_H	 

#include "stm32f3xx_hal.h"
#include "stm32f3xx_hal_uart.h"
#include "usart.h"

#define RXBUFFERSIZE 1 //command,voltage/current,voltage value/current value
#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define Voltage 0
#define Current 1
extern uint8_t aRXBuffer[RXBUFFERSIZE];
extern uint8_t USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
extern uint16_t USART_RX_STA;       //接收状态标记	
extern struct  _FLAG    DF;
extern struct  _Ctr_value  CtrValue;

void DataTran(void);
//extern void Uart1_Send(uint8_t *aTxBuffer);

struct  _FLAG
{
	uint16_t  RunFlag;	// C.V/C.C mode setting
	uint8_t ConFlag;		//PC Connection Flag
};

struct  _Ctr_value
{
	int32_t		Voref;		//Reference voltage
	int32_t		Ioref;		//Reference current
};
#endif
