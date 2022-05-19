/* USER CODE BEGIN Header */
/****************************************************************************************
	* 
	* @author  文七电源
	* @淘宝店铺链接：https://shop598739109.taobao.com
	* @file           : Function.c
  * @brief          : Function 功能函数集合
  * @version V1.0
  * @date    01-03-2021
  * @LegalDeclaration ：本文档内容难免存在Bug，仅限于交流学习，禁止用于任何的商业用途
	* @Copyright   著作权文七电源所有
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
	
/* USER CODE END Header */

#include "function.h"//功能函数头文件
#include "CtlLoop.h"//控制环路头文件

struct  _ADI SADC={0,0,2048,0,0,0,0,2048,0,0};//输入输出参数采样值和平均值
struct  _Ctr_value  CtrValue={0,0,0,MIN_BUKC_DUTY,0,0,0};//控制参数
struct  _FLAG    DF={0,0,0,0,0,0,0,0,0,0};//控制标志位
uint16_t ADC1_RESULT[4]={0,0,0,0};//ADC采样外设到内存的DMA数据保存寄存器
uint32_t Vset = 0; 				//UART SET VOLTAGE
uint32_t Iset = 0;				//UART SET CURRENT
//软启动状态标志位
SState_M 	STState = SSInit ;
//OLED刷新计数 5mS计数一次，在5mS中断里累加
uint16_t OLEDShowCnt=0;
//Transmission Buffer
uint8_t aRXBuffer[RXBUFFERSIZE];
uint8_t USART_RX_BUF[USART_REC_LEN];     //Receive buffer, maximum size 200
uint8_t USART_TX_BUF[USART_REC_LEN];
uint16_t USART_RX_STA=0;       					//Receive length
/*
** ===================================================================
**     Funtion Name :   void ADCSample(void)
**     Description :    采样输出电压、输出电流、输入电压、输入电流
**     Parameters  :
**     Returns     :
** ===================================================================
*/
CCMRAM void ADCSample(void)
{
	//输入输出采样参数求和，用以计算平均值
	static uint32_t VinAvgSum=0,IinAvgSum=0,VoutAvgSum=0,IoutAvgSum=0;
	
	//从DMA缓冲器中获取数据 Q15,并对其进行线性矫正-采用线性矫正
	SADC.Vin  = ((uint32_t )ADC1_RESULT[0]*CAL_VIN_K>>12)+CAL_VIN_B;
	SADC.Iin  = (((int32_t )ADC1_RESULT[1]-SADC.IinOffset)*CAL_IIN_K>>12)+CAL_IIN_B;
	SADC.Vout = ((uint32_t )ADC1_RESULT[2]*CAL_VOUT_K>>12)+CAL_VOUT_B;
	SADC.Iout = (((int32_t )ADC1_RESULT[3] - SADC.IoutOffset)*CAL_IOUT_K>>12)+CAL_IOUT_B;

	if(SADC.Vin <100 )//采样有零偏离，采样值很小时，直接为0
		SADC.Vin = 0;	
	if(SADC.Iin <0 )//对电流采样限制，当采样值小于SADC.IinOffset时
		SADC.Iin =0;
	if(SADC.Vout <100 )
		SADC.Vout = 0;
	if(SADC.Iout <0 )
		SADC.Iout = 0;

	
	//计算各个采样值的平均值-滑动平均方式
	VinAvgSum = VinAvgSum + SADC.Vin -(VinAvgSum>>2);//求和，新增入一个新的采样值，同时减去之前的平均值。
	SADC.VinAvg = VinAvgSum>>2;//求平均
	IinAvgSum = IinAvgSum + SADC.Iin -(IinAvgSum>>2);
	SADC.IinAvg = IinAvgSum >>2;
	VoutAvgSum = VoutAvgSum + SADC.Vout -(VoutAvgSum>>2);
	SADC.VoutAvg = VoutAvgSum>>2;
	IoutAvgSum = IoutAvgSum + SADC.Iout -(IoutAvgSum>>2);
	SADC.IoutAvg = IoutAvgSum>>2;	
}

/** ===================================================================
**     Funtion Name :void StateM(void)
**     Description :   状态机函数，在5ms中断中运行，5ms运行一次
**     初始化状态
**     等外启动状态
**     启动状态
**     运行状态
**     故障状态
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateM(void)
{
	//判断状态类型
	switch(DF.SMFlag)
	{
		//初始化状态
		case  Init :StateMInit();
		break;
		//等待状态
		case  Wait :StateMWait();
		break;
		//软启动状态
		case  Rise :StateMRise();
		break;
		//运行状态
		case  Run :StateMRun();
		break;
		//故障状态
		case  Err :StateMErr();
		break;
	}
}
/** ===================================================================
**     Funtion Name :void StateMInit(void)
**     Description :   初始化状态函数，参数初始化
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateMInit(void)
{
	//相关参数初始化
	ValInit();
	//状态机跳转至等待软启状态
	DF.SMFlag  = Wait;
}

/** ===================================================================
**     Funtion Name :void StateMWait(void)
**     Description :   等待状态机，等待1S后无故障则软启
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateMWait(void)
{
	//计数器定义
	static uint16_t CntS = 0;
	static uint32_t	IinSum=0,IoutSum=0;
	
	//关PWM
	DF.PWMENFlag=0;
	//计数器累加
	CntS ++;
	//等待*S，采样输入和输出电流偏置好后， 且无故障情况,切按键按下，启动，则进入启动状态
	if(CntS>256)
	{
		CntS=256;
		HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //开启PWM输出和PWM计时器
	//	if((DF.ErrFlag==F_NOERR)&&((DF.ConFlag==1)||(DF.KeyFlag1==1)))
		if((DF.ErrFlag==F_NOERR)&&(DF.ConFlag==1))
		{
			//计数器清0
			CntS=0;
			IinSum=0;
			IoutSum=0;
			//状态标志位跳转至等待状态
			DF.SMFlag  = Rise;
			//软启动子状态跳转至初始化状态
			STState = SSInit;
		}
	}
	else//进行输入和输出电流1.65V偏置求平均
	{
	  //输入输出电流偏置求和
    IinSum += ADC1_RESULT[1];
		IoutSum += ADC1_RESULT[3];
    //256次数
    if(CntS==256)
    {
        //求平均
				SADC.IinOffset = IinSum >>8;
        SADC.IoutOffset = IoutSum >>8;
    }	
	}
}
/*
** ===================================================================
**     Funtion Name : void StateMRise(void)
**     Description :软启阶段
**     软启初始化
**     软启等待
**     开始软启
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_SSCNT       20//等待100ms
void StateMRise(void)
{
	//计时器
	static  uint16_t  Cnt = 0;
	//最大占空比限制计数器
	static  uint16_t	BUCKMaxDutyCnt=0,BoostMaxDutyCnt=0;

	//判断软启状态
	switch(STState)
	{
		//初始化状态
		case    SSInit:
		{
			//关闭PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭				
			//软启中将运行限制占空比启动，从最小占空比开始启动
			CtrValue.BUCKMaxDuty  = MIN_BUKC_DUTY;
			CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;
			//环路计算变量初始化
			VErr0=0;
			VErr1=0;
			VErr2=0;
			IErr0=0;
			IErr1=0;
			IErr2=0;
			u0 = 0;
			u1 = 0;
			//跳转至软启等待状态
			STState = SSWait;

			break;
		}
		//等待软启动状态
		case    SSWait:
		{
			//计数器累加
			Cnt++;
			//等待100ms
			if(Cnt> MAX_SSCNT)
			{
				//计数器清0
				Cnt = 0;
				//限制启动占空比
				CtrValue.BuckDuty = MIN_BUKC_DUTY;
				CtrValue.BUCKMaxDuty= MIN_BUKC_DUTY;
				CtrValue.BoostDuty = MIN_BOOST_DUTY;
				CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;
				//环路计算变量初始化
				VErr0=0;
				VErr1=0;
				VErr2=0;
				IErr0=0;
				IErr1=0;
				IErr2=0;
				u0 = 0;
				u1 = 0;
				//CtrValue.Voref输出参考电压从一半开始启动，避免过冲，然后缓慢上升
				CtrValue.Voref  = CtrValue.Voref >>1;
				CtrValue.Ioref  = CtrValue.Ioref >>1;
				STState = SSRun;	//跳转至软启状态			
			}
			break;
		}
		//软启动状态
		case    SSRun:
		{
			if(DF.PWMENFlag==0)//正式发波前环路变量清0
			{
				//环路计算变量初始化
				VErr0=0;
				VErr1=0;
				VErr2=0;
				IErr0=0;
				IErr1=0;
				IErr2=0;
				u0 = 0;
				u1 = 0;
				HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //开启PWM输出和PWM计时器
				HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //开启PWM输出和PWM计时器		
			}
			//发波标志位置位
			DF.PWMENFlag=1;
			//最大占空比限制逐渐增加
			BUCKMaxDutyCnt++;
			BoostMaxDutyCnt++;
			//最大占空比限制累加
			CtrValue.BUCKMaxDuty = CtrValue.BUCKMaxDuty + BUCKMaxDutyCnt*10;
			CtrValue.BoostMaxDuty = CtrValue.BoostMaxDuty + BoostMaxDutyCnt*10;
			//累加到最大值
			if(CtrValue.BUCKMaxDuty > MAX_BUCK_DUTY)
				CtrValue.BUCKMaxDuty  = MAX_BUCK_DUTY ;
			if(CtrValue.BoostMaxDuty > MAX_BOOST_DUTY)
				CtrValue.BoostMaxDuty  = MAX_BOOST_DUTY ;
			
			if((CtrValue.BUCKMaxDuty==MAX_BUCK_DUTY)&&(CtrValue.BoostMaxDuty==MAX_BOOST_DUTY))			
			{
				//状态机跳转至运行状态
				DF.SMFlag  = Run;
				//软启动子状态跳转至初始化状态
				STState = SSInit;	
			}
			break;
		}
		default:
		break;
	}
}
/*
** ===================================================================
**     Funtion Name :void StateMRun(void)
**     Description :正常运行，主处理函数在中断中运行
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
void StateMRun(void)
{

}

/*
** ===================================================================
**     Funtion Name :void StateMErr(void)
**     Description :故障状态
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
void StateMErr(void)
{
	//初始化电压参考量
	CtrValue.Voref=0;
	CtrValue.Ioref=0;
	//限制占空比
	CtrValue.BuckDuty = MIN_BUKC_DUTY;
	CtrValue.BUCKMaxDuty= MIN_BUKC_DUTY;
	CtrValue.BoostDuty = MIN_BOOST_DUTY;
	CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;	
	//环路计算变量初始化
	VErr0=0;
	VErr1=0;
	VErr2=0;
	IErr0=0;
	IErr1=0;
	IErr2=0;
	u0 = 0;
	u1 = 0;
	//关闭PWM
	DF.PWMENFlag=0;
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭		
	//若故障消除跳转至等待重新软启
	if(DF.ErrFlag==F_NOERR)
			DF.SMFlag  = Wait;
}

/** ===================================================================
**     Funtion Name :void ValInit(void)
**     Description :   相关参数初始化函数
**     Parameters  :
**     Returns     :
** ===================================================================*/
void ValInit(void)
{
	//关闭PWM
	DF.PWMENFlag=0;
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭		
	//清除故障标志位
	DF.ErrFlag=0;
	//初始化电压参考量
	CtrValue.Voref=0;
	CtrValue.Ioref=0;
	//限制占空比
	CtrValue.BuckDuty = MIN_BUKC_DUTY;
	CtrValue.BUCKMaxDuty= MIN_BUKC_DUTY;
	CtrValue.BoostDuty = MIN_BOOST_DUTY;
	CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;	
	//环路计算变量初始化
	VErr0=0;
	VErr1=0;
	VErr2=0;
	IErr0=0;
	IErr1=0;
	IErr2=0;
	u0 = 0;
	u1 = 0;
}

/** ===================================================================
**     Funtion Name :void DataAna(void)
**     Description :   Get the Vref/Iref from the USB port via USART transmission
**     Parameters  :
**     Returns     :
** ===================================================================*/
void DataAna(void)
{
	uint32_t VoutT=0;
	uint32_t IoutT=0;
	if(USART_RX_BUF[0] == Start && USART_RX_BUF[7] == End)	//Command verification bytes, first a, last b.
	{
		switch(USART_RX_BUF[1])
		{
			case	Connect:						//Check connection-->s
			{
					//if(DF.SMFlag == Wait && DF.KeyFlag1 == 0)
				if(DF.SMFlag == Wait)
					{
						USART_TX_BUF[0] = 's';
						HAL_UART_Transmit(&huart1,(uint8_t*)USART_TX_BUF,1,1000);	
						while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
					}
					break;
			}
			case 	Control:						//Control mode & Status Wait->Run
			{
				DF.ConFlag = 1;		// 	State transfer 
				switch(USART_RX_BUF[2])
				{
					case CV:							//Voltage adjustment-->v
					{
						DF.ModeFlag = CVmode; //CV mode
						Vset  = ((uint32_t)USART_RX_BUF[3]<<24)|((uint32_t)USART_RX_BUF[4]<<16)|((uint32_t)USART_RX_BUF[5]<<8)|((uint32_t)USART_RX_BUF[6]); //4*8 bits
						USART_TX_BUF[0] = 'c';
						HAL_UART_Transmit(&huart1,(uint8_t*)USART_TX_BUF,1,1000);	
						while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
						break;
					}
					case CC:							//Current adjustment-->c
					{
						DF.ModeFlag = CCmode; //CC mode
						Iset = ((uint32_t)USART_RX_BUF[3]<<24)|((uint32_t)USART_RX_BUF[4]<<16)|((uint32_t)USART_RX_BUF[5]<<8)|((uint32_t)USART_RX_BUF[6]); //4*8 bits
						//CtrValue.Ioref = Iset/100/68<<12;
						//USART_TX_BUF[0] = CtrValue.Ioref;
						USART_TX_BUF[0] = 'c';
						HAL_UART_Transmit(&huart1,(uint8_t*)USART_TX_BUF,1,1000);	
						while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
						break;
					}
				}
			}break;
			case	Stop:							//Stop command
			{
				if((DF.ConFlag==1)&&((DF.SMFlag==Rise)||(DF.SMFlag==Run)))
					{
						DF.SMFlag = Wait;
						DF.ConFlag = 0;
						Vset = 0;
						USART_TX_BUF[0] = 'e';
						HAL_UART_Transmit(&huart1,(uint8_t*)USART_TX_BUF,1,1000);	
						while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
						//Close PWM
						DF.PWMENFlag=0;
						HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
						HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭	
					}
					break;
			}
			case Visual:
			{

					//Transfer the ADC sampling value to real value, and x1000(FLOAT -> INT)
					VoutT = (float)SADC.VoutAvg*68000/4096;
					IoutT = (float)SADC.IoutAvg*22000/4096;
					
					//输出电压
					USART_TX_BUF[0] = VoutT;
					USART_TX_BUF[1] = VoutT>>8;
					USART_TX_BUF[2] = VoutT>>16;
					USART_TX_BUF[3] = VoutT>>24;	
				  USART_TX_BUF[4] = IoutT;
					USART_TX_BUF[5] = IoutT>>8;
					USART_TX_BUF[6] = IoutT>>16;
					USART_TX_BUF[7] = IoutT>>24;	
					HAL_UART_Transmit(&huart1,(uint8_t*)USART_TX_BUF,8,1000);	
					while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);

				break;
			}
		}
	}
	USART_RX_STA = 0;
	USART_RX_BUF[0] = 'b';
}	
/** ===================================================================
**     Funtion Name :void VrefGet(void)
**     Description :   Get and refresh the reference voltage from the dedicated buffer，assign a proper value to Ctr.Voref,
**											In case voltage changes rapidly.
**     Parameters  :
**     Returns     :
** ===================================================================*/
#define MAX_VREF    2921//输出最大参考电压48V  0.5V的余量   48.5V/68V*Q12
#define MAX_IREF 		931//MAX reference current 5A
#define VREF_K      10//递增或递减步长
#define IREF_K      5//递增或递减步长
void IVrefGet(void)
{
	//电压参考值中间变量
	int32_t VTemp = 0;	
	int32_t ITemp = 0;
	
	VTemp = (float)Vset/68000*4096;
	ITemp = (float)Iset/22000*4096;
	
	//缓慢递增或缓慢递减电压参考值
	if( VTemp> ( CtrValue.Voref + VREF_K))
			CtrValue.Voref = CtrValue.Voref + VREF_K;
	else if( VTemp < ( CtrValue.Voref - VREF_K ))
			CtrValue.Voref =CtrValue.Voref - VREF_K;
	else
			CtrValue.Voref = VTemp ;

	//MIX模式下调压限制-输出电压最大达到输入电压的2倍
	if(CtrValue.Voref >(VTemp<<1))//输出限制在输入的2*vin 
		CtrValue.Voref =(VTemp<<1);
	if( CtrValue.Voref > MAX_VREF )
		CtrValue.Voref = MAX_VREF;
	
/*Current adjustment*/	
	if( ITemp> ( CtrValue.Ioref + IREF_K))
			CtrValue.Ioref = CtrValue.Ioref + IREF_K;
	else if( ITemp < ( CtrValue.Ioref - IREF_K ))
			CtrValue.Ioref =CtrValue.Ioref - IREF_K;
	else
			CtrValue.Ioref = ITemp ;

	//MIX模式下调压限制-输出电压最大达到输入电压的2倍
	if(CtrValue.Ioref >(ITemp<<1))//输出限制在输入的2*vin 
		CtrValue.Ioref =(ITemp<<1);
	if( CtrValue.Ioref > MAX_IREF )
		CtrValue.Ioref = MAX_IREF;
}

/*
** ===================================================================
**     Funtion Name :void ShortOff(void)
**     Description :短路保护，可以重启10次
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_SHORT_I     1396//短路电流判据
#define MIN_SHORT_V     289//短路电压判据
void ShortOff(void)
{
	static int32_t RSCnt = 0;
	static uint8_t RSNum =0 ;

	//当输出电流大于 *A，且电压小于*V时，可判定为发生短路保护
	if((SADC.Iout> MAX_SHORT_I)&&(SADC.Vout <MIN_SHORT_V))
	{
		//关闭PWM
		DF.PWMENFlag=0;
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭	
		//故障标志位
		setRegBits(DF.ErrFlag,F_SW_SHORT);
		//跳转至故障状态
		DF.SMFlag  =Err;
	}
	//输出短路保护恢复
	//当发生输出短路保护，关机后等待4S后清楚故障信息，进入等待状态等待重启
	if(getRegBits(DF.ErrFlag,F_SW_SHORT))
	{
		//等待故障清楚计数器累加
		RSCnt++;
		//等待2S
		if(RSCnt >400)
		{
			//计数器清零
			RSCnt=0;
			//短路重启只重启10次，10次后不重启
			if(RSNum > 10)
			{
				//确保不清除故障，不重启
				RSNum =11;
				//关闭PWM
				DF.PWMENFlag=0;
				HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
				HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭	
			}
			else
			{
				//短路重启计数器累加
				RSNum++;
				//清除过流保护故障标志位
				clrRegBits(DF.ErrFlag,F_SW_SHORT);
			}
		}
	}
}
/*
** ===================================================================
**     Funtion Name :void SwOCP(void)
**     Description :软件过流保护，可重启
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_OCP_VAL     1117//*A过流保护点 
void SwOCP(void)
{
	//过流保护判据保持计数器定义
	static  uint16_t  OCPCnt=0;
	//故障清楚保持计数器定义
	static  uint16_t  RSCnt=0;
	//保留保护重启计数器
	static  uint16_t  RSNum=0;

	//当输出电流大于*A，且保持500ms
	if((SADC.Iout > MAX_OCP_VAL)&&(DF.SMFlag  ==Run))
	{
		//条件保持计时
		OCPCnt++;
		//条件保持50ms，则认为过流发生
		if(OCPCnt > 10)
		{
			//计数器清0
			OCPCnt  = 0;
			//关闭PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭	
			//故障标志位
			setRegBits(DF.ErrFlag,F_SW_IOUT_OCP);
			//跳转至故障状态
			DF.SMFlag  =Err;
		}
	}
	else
		//计数器清0
		OCPCnt  = 0;

	//输出过流后恢复
	//当发生输出软件过流保护，关机后等待4S后清楚故障信息，进入等待状态等待重启
	if(getRegBits(DF.ErrFlag,F_SW_IOUT_OCP))
	{
		//等待故障清楚计数器累加
		RSCnt++;
		//等待2S
		if(RSCnt > 400)
		{
			//计数器清零
			RSCnt=0;
			//过流重启计数器累加
			RSNum++;
			//过流重启只重启10次，10次后不重启（严重故障）
			if(RSNum > 10 )
			{
				//确保不清除故障，不重启
				RSNum =11;
				//关闭PWM
				DF.PWMENFlag=0;
				HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
				HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭		
			}
			else
			{
			 //清除过流保护故障标志位
				clrRegBits(DF.ErrFlag,F_SW_IOUT_OCP);
			}
		}
	}
}

/*
** ===================================================================
**     Funtion Name :void SwOVP(void)
**     Description :软件输出过压保护，不重启
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_VOUT_OVP_VAL    3012//50V过压保护	（50/68）*Q12
void VoutSwOVP(void)
{
	//过压保护判据保持计数器定义
	static  uint16_t  OVPCnt=0;

	//当输出电压大于50V，且保持100ms
	if (SADC.Vout > MAX_VOUT_OVP_VAL)
	{
		//条件保持计时
		OVPCnt++;
		//条件保持10ms
		if(OVPCnt > 2)
		{
			//计时器清零
			OVPCnt=0;
			//关闭PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭		
			//故障标志位
			setRegBits(DF.ErrFlag,F_SW_VOUT_OVP);
			//跳转至故障状态
			DF.SMFlag  =Err;
		}
	}
	else
		OVPCnt = 0;
}

/*
** ===================================================================
**     Funtion Name :void VinSwUVP(void)
**     Description :输入软件欠压保护，低压输入保护,可恢复
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MIN_UVP_VAL    686//11.4V欠压保护 （11.4/68 ）*Q12
#define MIN_UVP_VAL_RE  795//13.2V欠压保护恢复 （13.2/68）*Q12
void VinSwUVP(void)
{
	//过压保护判据保持计数器定义
	static  uint16_t  UVPCnt=0;
	static  uint16_t	RSCnt=0;

	//当输出电流小于于11.4V，且保持200ms
	if ((SADC.Vin < MIN_UVP_VAL) && (DF.SMFlag != Init ))
	{
		//条件保持计时
		UVPCnt++;
		//条件保持10ms
		if(UVPCnt > 2)
		{
			//计时器清零
			UVPCnt=0;
			RSCnt=0;
			//关闭PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭		
			//故障标志位
			setRegBits(DF.ErrFlag,F_SW_VIN_UVP);
			//跳转至故障状态
			DF.SMFlag  =Err;
		}
	}
	else
		UVPCnt = 0;
	
	//输入欠压保护恢复
	//当发生输入欠压保护，等待输入电压恢复至正常水平后清楚故障标志位，重启
	if(getRegBits(DF.ErrFlag,F_SW_VIN_UVP))
	{
		if(SADC.Vin > MIN_UVP_VAL_RE) 
		{
			//等待故障清楚计数器累加
			RSCnt++;
			//等待1S
			if(RSCnt > 200)
			{
				RSCnt=0;
				UVPCnt=0;
				//清楚故障标志位
				clrRegBits(DF.ErrFlag,F_SW_VIN_UVP);
			}	
		}
		else	
			RSCnt=0;	
	}
	else
		RSCnt=0;
}

/*
** ===================================================================
**     Funtion Name :void VinSwOVP(void)
**     Description :软件输入过压保护，不重启
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_VIN_OVP_VAL    3012//50V过压保护	（50/68）*Q12
void VinSwOVP(void)
{
	//过压保护判据保持计数器定义
	static  uint16_t  OVPCnt=0;

	//当输出电压大于50V，且保持100ms
	if (SADC.Vin > MAX_VIN_OVP_VAL )
	{
		//条件保持计时
		OVPCnt++;
		//条件保持10ms
		if(OVPCnt > 2)
		{
			//计时器清零
			OVPCnt=0;
			//关闭PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭		
			//故障标志位
			setRegBits(DF.ErrFlag,F_SW_VIN_OVP);
			//跳转至故障状态
			DF.SMFlag  =Err;
		}
	}
	else
		OVPCnt = 0;
}

/** ===================================================================
**     Funtion Name :void LEDShow(void)
**     Description :  LED显示函数
**     初始化与等待启动状态，红黄绿全亮
**     启动状态，黄绿亮
**     运行状态，绿灯亮
**     故障状态，红灯亮
**     Parameters  :
**     Returns     :
** ===================================================================*/
//输出状态灯宏定义
 #define SET_LED_G()	HAL_GPIO_WritePin(GPIOB, LED_G_Pin,GPIO_PIN_SET)//绿灯亮
 #define SET_LED_Y()	HAL_GPIO_WritePin(GPIOB, LED_Y_Pin,GPIO_PIN_SET)//绿灯亮
 #define SET_LED_R()	HAL_GPIO_WritePin(GPIOB, LED_R_Pin,GPIO_PIN_SET)//绿灯亮
 #define CLR_LED_G()	HAL_GPIO_WritePin(GPIOB, LED_G_Pin,GPIO_PIN_RESET)//绿灯灭
 #define CLR_LED_Y()	HAL_GPIO_WritePin(GPIOB, LED_Y_Pin,GPIO_PIN_RESET)//黄灯灭
 #define CLR_LED_R()	HAL_GPIO_WritePin(GPIOB, LED_R_Pin,GPIO_PIN_RESET)//红灯灭
void LEDShow(void)
{
	switch(DF.SMFlag)
	{
		//初始化状态，红黄绿全亮
		case  Init :
		{
			SET_LED_G();
			SET_LED_Y();
			SET_LED_R();
			break;
		}
		//等待状态，红黄绿全亮
		case  Wait :
		{
			SET_LED_G();
			SET_LED_Y();
			SET_LED_R();
			break;
		}
		//软启动状态，黄绿亮
		case  Rise :
		{
			SET_LED_G();
			SET_LED_Y();
			CLR_LED_R();
			break;
		}
		//运行状态，绿灯亮
		case  Run :
		{
			SET_LED_G();
			CLR_LED_Y();
			CLR_LED_R();
			break;
		}
		//故障状态，红灯亮
		case  Err :
		{
			CLR_LED_G();
			CLR_LED_Y();
			SET_LED_R();
			break;
		}
	}
}

/** ===================================================================
**     Funtion Name :void KEYFlag(void)
**     Description :两个按键的状态
**		 默认状态KEYFlag为0.按下时Flag变1，再次按下Flag变0，依次循环
**		 当机器正常运行，或者启动过程中，按下按键后，关闭输出，进入待机状态
**     Parameters  :
**     Returns     :
** ===================================================================*/
#define READ_KEY1() HAL_GPIO_ReadPin(GPIOB, KEY_1_Pin)
#define READ_KEY2() HAL_GPIO_ReadPin(GPIOB, KEY_2_Pin)
void KEYFlag(void)
{
	//计时器，按键消抖用
	static uint16_t	KeyDownCnt1=0,KeyDownCnt2=0;
	
	//按键按下
	if(READ_KEY1()==0)
	{
		//计时，按键按下150mS有效
		KeyDownCnt1++;
		if(KeyDownCnt1 > 30)
		{
			KeyDownCnt1 = 0;//计时器清零
			//按键状态有变化
			if(DF.KeyFlag1==0)
				DF.KeyFlag1 = 1;
			else
				DF.KeyFlag1 = 0;
		}
	}
	else
		KeyDownCnt1 = 0;//计时器清零
	
	//按键按下
	if(READ_KEY2()==0)
	{
		//计时，按键按󒑃0mS有效
		KeyDownCnt2++;
		if(KeyDownCnt2 > 30)
		{
			KeyDownCnt2 = 0;//计时器清零
			//按键状态有变化
			if(DF.KeyFlag2==0)
				DF.KeyFlag2 = 1;
			else
				DF.KeyFlag2 = 0;
		}
	}
	else
		KeyDownCnt2 = 0;//计时器清零

	//当机器正常运行，或者启动过程中，按下按键后，关闭输出，进入待机状态
	if((DF.KeyFlag1==0)&&((DF.SMFlag==Rise)||(DF.SMFlag==Run)))
	{
		DF.SMFlag = Wait;
		//关闭PWM
		DF.PWMENFlag=0;
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //关闭
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //关闭	
	}
}


/** ===================================================================
**     Funtion Name :void BBMode(void)
**     Description :运行模式判断
** 		 BUCK模式：输出参考电压<0.8倍输入电压
** 		 BOOST模式：输出参考电压>1.2倍输入电压
**		 MIX模式：1.15倍输入电压>输出参考电压>0.85倍输入电压
**		 当进入MIX（buck-boost）模式后，退出到BUCK或者BOOST时需要滞缓，防止在临界点来回振荡
**     Parameters  :
**     Returns     :
** ===================================================================*/
void BBMode(void)
{
	//上一次模式状态量
	uint8_t PreBBFlag=0;
	
	//暂存当前的模式状态量
	PreBBFlag = DF.BBFlag;
	
	//判断当前模块的工作模式
	switch(DF.BBFlag)
	{
		//NA-初始化模式
		case  NA :		
		{
			if(DF.ModeFlag == CVmode)
			{
		  if(CtrValue.Voref < ((SADC.VinAvg*3277)>>12))//vout<0.8*vin
				DF.BBFlag = Buck;//buck mode
			else if(CtrValue.Voref > ((SADC.VinAvg*4915)>>12))//vout>1.2*vin
				DF.BBFlag = Boost;//boost mode
			else
				DF.BBFlag = Mix;//buck-boost（MIX） mode
			}
			else
			{
				if(CtrValue.Ioref < ((SADC.IinAvg*3277)>>12))//vout<0.8*vin
				DF.BBFlag = Buck;//buck mode
			else if(CtrValue.Ioref > ((SADC.IinAvg*4915)>>12))//vout>1.2*vin
				DF.BBFlag = Boost;//boost mode
			else
				DF.BBFlag = Mix;//buck-boost（MIX） mode
			}
			break;
		}
		//BUCK模式
		case  Buck :		
		{
			if(DF.ModeFlag == CVmode)
			{
				if(CtrValue.Voref > ((SADC.VinAvg*4915)>>12))//vout>1.2*vin
					DF.BBFlag = Boost;//boost mode
				else if(CtrValue.Voref >((SADC.VinAvg*3482)>>12))//1.2*vin>vout>0.85*vin
					DF.BBFlag = Mix;//buck-boost（MIX） mode
			}
			else
			{
				if(CtrValue.Ioref > ((SADC.IinAvg*4915)>>12))//vout>1.2*vin
					DF.BBFlag = Boost;//boost mode
				else if(CtrValue.Ioref >((SADC.IinAvg*3482)>>12))//1.2*vin>vout>0.85*vin
					DF.BBFlag = Mix;//buck-boost（MIX） mode
			}
			break;
		}
		//Boost模式
		case  Boost :		
		{
			if(DF.ModeFlag == CVmode)
			{
				if(CtrValue.Voref < ((SADC.VinAvg*3277)>>12))//vout<0.8*vin
					DF.BBFlag = Buck;//buck mode
				else if(CtrValue.Voref < ((SADC.VinAvg*4710)>>12))//0.8*vin<vout<1.15*vin
					DF.BBFlag = Mix;//buck-boost（MIX） mode
			}
			else
			{
				if(CtrValue.Ioref < ((SADC.IinAvg*3277)>>12))//vout<0.8*vin
					DF.BBFlag = Buck;//buck mode
				else if(CtrValue.Ioref < ((SADC.IinAvg*4710)>>12))//0.8*vin<vout<1.15*vin
					DF.BBFlag = Mix;//buck-boost（MIX） mode
			}
			break;
		}
		//Mix模式
		case  Mix :		
		{
			if(DF.ModeFlag == CVmode)
			{
		  if(CtrValue.Voref < ((SADC.VinAvg*3277)>>12))//vout<0.8*vin
				DF.BBFlag = Buck;//buck mode
			else if(CtrValue.Voref > ((SADC.VinAvg*4915)>>12))//vout>1.2*vin
				DF.BBFlag = Boost;//boost mode
			}
			else
			{
			if(CtrValue.Ioref < ((SADC.IinAvg*3277)>>12))//vout<0.8*vin
				DF.BBFlag = Buck;//buck mode
			else if(CtrValue.Ioref > ((SADC.IinAvg*4915)>>12))//vout>1.2*vin
				DF.BBFlag = Boost;//boost mode
			}
			break;
		}	
	}
	
	
	//当模式发生变换时（上一次和这一次不一样）,则标志位置位，标志位用以环路计算复位，保证模式切换过程不会有大的过冲
	if(PreBBFlag==DF.BBFlag)
		DF.BBModeChange =0;
	else
		DF.BBModeChange =1;
}
/** ===================================================================
**     Funtion Name :void MX_OLED_Init(void)
**     Description :OLED初始化函数
**     通用OLED驱动程序
**     Parameters  :
**     Returns     :
** ===================================================================*/
void MX_OLED_Init(void)
{
	//初始化OLED
	OLED_Init();
	OLED_CLS();
	//以下显示固定的字符
	OLED_ShowStr(25,0,"BUCK-BOOST",2);
	OLED_ShowStr(0,2,"State:",2);
	OLED_ShowStr(0,4,"Vout:",2);
	OLED_ShowStr(68,4,".",2);
	OLED_ShowStr(95,4,"V",2);
	OLED_ShowStr(0,6,"Iout:",2);
	OLED_ShowStr(68,6,".",2);
	OLED_ShowStr(95,6,"A",2);
	OLED_ON(); 	
}

/** ===================================================================
**     Funtion Name :void OLEDShow(void)
**     Description : OLED显示函数		 
**     显示运行模式-BUCK MODE,BOOST MODE,MIX MODE
**     显示状态：IDEL,RISISING,RUNNING,ERROR
**     显示输出电压：两位小数
**     显示输出电流：两位小数
**     Parameters  :
**     Returns     :
** ===================================================================*/
void OLEDShow(void)
{
	u8 Vtemp[4]={0,0,0,0};
	u8 Itemp[4]={0,0,0,0};
	uint32_t VoutT=0,IoutT=0;
	//uint32_t VinT=0,IinT=0,VadjT=0;
	static uint16_t BBFlagTemp=10,SMFlagTemp=10;
//		printf("The voltage value:\n");
//		printf("%d",SADC.VoutAvg*6800>>12);
//	//case	CCmode:
//		printf("The current value:\n");
//		printf("%d",SADC.IoutAvg*2200>>12);		
	
	//将采样值转换成实际值，并扩大100倍(显示屏幕带小数点)如果显示输入电压电流，则打开另外注释掉的计算
	VoutT = SADC.VoutAvg*6800>>12;
	IoutT = SADC.IoutAvg*2200>>12;
	//VinT = SADC.VinAvg*6800>>12;
	//IinT = SADC.IinAvg*2200>>12;
	//VadjT = CtrValue.Voref*6800>>12;
	
	//分别保存实际电压和电流的每一位，小数点后两位保留，如果显示输入电压电流，则打开另外注释掉的计算
	//输出电压
	Vtemp[0] = (u8)(VoutT/1000);
	Vtemp[1] = (u8)((VoutT-(uint8_t)Vtemp[0]*1000)/100);
	Vtemp[2] = (u8)((VoutT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100)/10);
	Vtemp[3] = (u8)(VoutT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100-(uint16_t)Vtemp[2]*10);	
	//输入电压
//	Vtemp[0] = (u8)(VinT/1000);
//	Vtemp[1] = (u8)((VinT-(uint8_t)Vtemp[0]*1000)/100);
	//Vtemp[2] = (u8)((VinT -(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100)/10);
//	Vtemp[3] = (u8)(VinT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100-(uint16_t)Vtemp[2]*10);	
	//输出电流
	Itemp[0] = (u8)(IoutT/1000);
	Itemp[1] = (u8)((IoutT-(uint8_t)Itemp[0]*1000)/100);
	Itemp[2] = (u8)((IoutT-(uint16_t)Itemp[0]*1000-(uint16_t)Itemp[1]*100)/10);
	Itemp[3] = (u8)(IoutT-(uint16_t)Itemp[0]*1000-(uint16_t)Itemp[1]*100-(uint16_t)Itemp[2]*10);
//	//输入电流
//	Itemp[0] = (u8)(IinT/1000);
//	Itemp[1] = (u8)((IinT-(uint8_t)Itemp[0]*1000)/100);
//	Itemp[2] = (u8)((IinT-(uint16_t)Itemp[0]*1000-(uint16_t)Itemp[1]*100)/10);
//	Itemp[3] = (u8)(IinT-(uint16_t)Itemp[0]*1000-(uint16_t)Itemp[1]*100-(uint16_t)Itemp[2]*10); 
//	//参考电压
//	Vtemp[0] = (u8)(VadjT/1000);
//	Vtemp[1] = (u8)((VadjT-(uint8_t)Vtemp[0]*1000)/100);
//	Vtemp[2] = (u8)((VadjT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100)/10);
//	Vtemp[3] = (u8)(VadjT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100-(uint16_t)Vtemp[2]*10);	
	
	//如果运行模式有变化，则更改屏幕
	if(BBFlagTemp!= DF.BBFlag)
	{
		//暂存标志位
		BBFlagTemp = DF.BBFlag;
		//显示运行模式
		switch(DF.BBFlag)
		{
			//NA
			case  NA :		
			{
				OLED_ShowStr(25,0,"MODE:*NA* ",2);
				break;
			}
			//BUCK模式
			case  Buck :		
			{
				OLED_ShowStr(25,0,"MODE:BUCK ",2);
				break;
			}
			//Boost模式
			case  Boost :		
			{
				OLED_ShowStr(25,0,"MODE:BOOST",2);
				break;
			}
			//Mix模式
			case  Mix :		
			{
				OLED_ShowStr(25,0,"MODE:MIX  ",2);
				break;
			}
		}
	}
	
	//电源工作状态有变换，更新屏幕
	if(SMFlagTemp!= DF.SMFlag)
	{	
		SMFlagTemp = DF.SMFlag;
		//显示电源工作状态
		switch(DF.SMFlag)
		{
			//初始化状态
			case  Init :
			{
				OLED_ShowStr(55,2,"Init  ",2);
				break;
			}
			//等待状态
			case  Wait :
			{
				OLED_ShowStr(55,2,"Waiting",2);
				break;
			}
			//软启动状态
			case  Rise :
			{
				OLED_ShowStr(55,2,"Rising",2);
				break;
			}
			//运行状态
			case  Run :
			{
				OLED_ShowStr(55,2,"Running",2);
				break;
			}
			//故障状态
			case  Err :
			{
				OLED_ShowStr(55,2,"Error  ",2);
				break;
			}
		}	
	}
	
	//显示电压电流每一位
	OLEDShowData(50,4,Vtemp[0]);
	OLEDShowData(60,4,Vtemp[1]);
	OLEDShowData(75,4,Vtemp[2]);
	OLEDShowData(85,4,Vtemp[3]);
	OLEDShowData(50,6,Itemp[0]);
	OLEDShowData(60,6,Itemp[1]);
	OLEDShowData(75,6,Itemp[2]);
	OLEDShowData(85,6,Itemp[3]);
}

void DataTran(void)
{
	USART_RX_BUF[USART_RX_STA]=aRXBuffer[0];
	USART_RX_STA++;
	if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA = 0;
}
	



