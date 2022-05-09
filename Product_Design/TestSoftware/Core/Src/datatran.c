/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    DataTran.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
#include "datatran.h"

uint8_t aRXBuffer[RXBUFFERSIZE];
uint8_t aTXBuffer[RXBUFFERSIZE];
uint8_t USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
uint16_t USART_RX_STA=0;       //����״̬���	
struct  _FLAG    DF={0,0};
struct  _Ctr_value  CtrValue={0,0};

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
//		if(huart->Instance==USART1)//����Ǵ���1
//	{
//		if((USART_RX_STA&0x8000)==0)//����δ���
//		{
//			if(USART_RX_STA&0x4000)//���յ���space
//			{
//				if(aRXBuffer[0]!=0x20)USART_RX_STA=0;//���մ���,���¿�ʼ
//				else USART_RX_STA|=0x8000;	//��������� 
//			}
//			else //��û�յ�0X0D
//			{	
//				if(aRXBuffer[0]==0x20)USART_RX_STA|=0x4000;
//				else
//				{
//					USART_RX_BUF[USART_RX_STA&0X3FFF]=aRXBuffer[0] ;
//					USART_RX_STA++;
//					if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;//�������ݴ���,���¿�ʼ����	  
//				}		 
//			}
//		}
//	USART_RX_BUF[USART_RX_STA&0X3FFF]=aRXBuffer[0] ;
//	USART_RX_STA++;
//	if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;
	}
	
//	if(huart->Instance==USART1)
//	{
//		if(aRXBuffer[0] == 'v')
//		{
//			HAL_UART_Transmit(&huart1, (uint8_t*)aRXBuffer, 16, 1000);
//			while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
//			DF.ConFlag = 1;
//		}
//		else if(aRXBuffer[0] == 's')
//		{
//			if(aRXBuffer[1] == 'c')
//			{
//				DF.RunFlag = Current;
//				CtrValue.Ioref = aRXBuffer[2];
//				HAL_UART_Transmit(&huart1, (uint8_t*)aRXBuffer, 16, 1000);
//				while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
//			}
//			else if(aRXBuffer[1] == 'v')
//			{
//				DF.RunFlag  = Voltage;
//				CtrValue.Voref = aRXBuffer[2];
//				HAL_UART_Transmit(&huart1, (uint8_t*)aRXBuffer, 16, 1000);
//				while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
//			}
//		}
//		else if(aRXBuffer[0] == 'e')
//		{
//				HAL_UART_Transmit(&huart1, (uint8_t*)aRXBuffer, 16, 1000);
//				while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
//	
//	}
//}

void DataTran(void)
{
	USART_RX_BUF[USART_RX_STA&0X3FFF]=aRXBuffer[0] ;
	USART_RX_STA++;
	if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;
}
	
