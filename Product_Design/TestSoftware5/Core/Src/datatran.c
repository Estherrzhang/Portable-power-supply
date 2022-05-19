/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    DataTran.c
  * @brief   This file provides code for the communication between PC & MCU
	* @author  Fengxue Zhang
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
#include "usart.h"

uint8_t aRXBuffer[RXBUFFERSIZE];
uint8_t USART_RX_BUF[USART_REC_LEN];     //Receive buffer, maximum size 200
uint8_t USART_TX_BUF[USART_REC_LEN];
uint16_t USART_RX_STA=0;       					//Receive length



void DataTran(void)
{
	USART_RX_BUF[USART_RX_STA]=aRXBuffer[0];
	USART_RX_STA++;
	if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;
}
	
