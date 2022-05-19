/* USER CODE BEGIN Header */
/****************************************************************************************
	* 
	* @author  ���ߵ�Դ
	* @�Ա��������ӣ�https://shop598739109.taobao.com
	* @file           : Function.c
  * @brief          : Function ���ܺ�������
  * @version V1.0
  * @date    01-03-2021
  * @LegalDeclaration �����ĵ������������Bug�������ڽ���ѧϰ����ֹ�����κε���ҵ��;
	* @Copyright   ����Ȩ���ߵ�Դ����
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

#include "function.h"//���ܺ���ͷ�ļ�
#include "CtlLoop.h"//���ƻ�·ͷ�ļ�

struct  _ADI SADC={0,0,2048,0,0,0,0,2048,0,0};//���������������ֵ��ƽ��ֵ
struct  _Ctr_value  CtrValue={0,0,0,MIN_BUKC_DUTY,0,0,0};//���Ʋ���
struct  _FLAG    DF={0,0,0,0,0,0,0,0,0,0};//���Ʊ�־λ
uint16_t ADC1_RESULT[4]={0,0,0,0};//ADC�������赽�ڴ��DMA���ݱ���Ĵ���
uint32_t Vset = 0; 				//UART SET VOLTAGE
uint32_t Iset = 0;				//UART SET CURRENT
//������״̬��־λ
SState_M 	STState = SSInit ;
//OLEDˢ�¼��� 5mS����һ�Σ���5mS�ж����ۼ�
uint16_t OLEDShowCnt=0;
//Transmission Buffer
uint8_t aRXBuffer[RXBUFFERSIZE];
uint8_t USART_RX_BUF[USART_REC_LEN];     //Receive buffer, maximum size 200
uint8_t USART_TX_BUF[USART_REC_LEN];
uint16_t USART_RX_STA=0;       					//Receive length
/*
** ===================================================================
**     Funtion Name :   void ADCSample(void)
**     Description :    ���������ѹ����������������ѹ���������
**     Parameters  :
**     Returns     :
** ===================================================================
*/
CCMRAM void ADCSample(void)
{
	//�����������������ͣ����Լ���ƽ��ֵ
	static uint32_t VinAvgSum=0,IinAvgSum=0,VoutAvgSum=0,IoutAvgSum=0;
	
	//��DMA�������л�ȡ���� Q15,������������Խ���-�������Խ���
	SADC.Vin  = ((uint32_t )ADC1_RESULT[0]*CAL_VIN_K>>12)+CAL_VIN_B;
	SADC.Iin  = (((int32_t )ADC1_RESULT[1]-SADC.IinOffset)*CAL_IIN_K>>12)+CAL_IIN_B;
	SADC.Vout = ((uint32_t )ADC1_RESULT[2]*CAL_VOUT_K>>12)+CAL_VOUT_B;
	SADC.Iout = (((int32_t )ADC1_RESULT[3] - SADC.IoutOffset)*CAL_IOUT_K>>12)+CAL_IOUT_B;

	if(SADC.Vin <100 )//��������ƫ�룬����ֵ��Сʱ��ֱ��Ϊ0
		SADC.Vin = 0;	
	if(SADC.Iin <0 )//�Ե����������ƣ�������ֵС��SADC.IinOffsetʱ
		SADC.Iin =0;
	if(SADC.Vout <100 )
		SADC.Vout = 0;
	if(SADC.Iout <0 )
		SADC.Iout = 0;

	
	//�����������ֵ��ƽ��ֵ-����ƽ����ʽ
	VinAvgSum = VinAvgSum + SADC.Vin -(VinAvgSum>>2);//��ͣ�������һ���µĲ���ֵ��ͬʱ��ȥ֮ǰ��ƽ��ֵ��
	SADC.VinAvg = VinAvgSum>>2;//��ƽ��
	IinAvgSum = IinAvgSum + SADC.Iin -(IinAvgSum>>2);
	SADC.IinAvg = IinAvgSum >>2;
	VoutAvgSum = VoutAvgSum + SADC.Vout -(VoutAvgSum>>2);
	SADC.VoutAvg = VoutAvgSum>>2;
	IoutAvgSum = IoutAvgSum + SADC.Iout -(IoutAvgSum>>2);
	SADC.IoutAvg = IoutAvgSum>>2;	
}

/** ===================================================================
**     Funtion Name :void StateM(void)
**     Description :   ״̬����������5ms�ж������У�5ms����һ��
**     ��ʼ��״̬
**     ��������״̬
**     ����״̬
**     ����״̬
**     ����״̬
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateM(void)
{
	//�ж�״̬����
	switch(DF.SMFlag)
	{
		//��ʼ��״̬
		case  Init :StateMInit();
		break;
		//�ȴ�״̬
		case  Wait :StateMWait();
		break;
		//������״̬
		case  Rise :StateMRise();
		break;
		//����״̬
		case  Run :StateMRun();
		break;
		//����״̬
		case  Err :StateMErr();
		break;
	}
}
/** ===================================================================
**     Funtion Name :void StateMInit(void)
**     Description :   ��ʼ��״̬������������ʼ��
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateMInit(void)
{
	//��ز�����ʼ��
	ValInit();
	//״̬����ת���ȴ�����״̬
	DF.SMFlag  = Wait;
}

/** ===================================================================
**     Funtion Name :void StateMWait(void)
**     Description :   �ȴ�״̬�����ȴ�1S���޹���������
**     Parameters  :
**     Returns     :
** ===================================================================*/
void StateMWait(void)
{
	//����������
	static uint16_t CntS = 0;
	static uint32_t	IinSum=0,IoutSum=0;
	
	//��PWM
	DF.PWMENFlag=0;
	//�������ۼ�
	CntS ++;
	//�ȴ�*S������������������ƫ�úú� ���޹������,�а������£����������������״̬
	if(CntS>256)
	{
		CntS=256;
		HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //����PWM�����PWM��ʱ��
	//	if((DF.ErrFlag==F_NOERR)&&((DF.ConFlag==1)||(DF.KeyFlag1==1)))
		if((DF.ErrFlag==F_NOERR)&&(DF.ConFlag==1))
		{
			//��������0
			CntS=0;
			IinSum=0;
			IoutSum=0;
			//״̬��־λ��ת���ȴ�״̬
			DF.SMFlag  = Rise;
			//��������״̬��ת����ʼ��״̬
			STState = SSInit;
		}
	}
	else//����������������1.65Vƫ����ƽ��
	{
	  //�����������ƫ�����
    IinSum += ADC1_RESULT[1];
		IoutSum += ADC1_RESULT[3];
    //256����
    if(CntS==256)
    {
        //��ƽ��
				SADC.IinOffset = IinSum >>8;
        SADC.IoutOffset = IoutSum >>8;
    }	
	}
}
/*
** ===================================================================
**     Funtion Name : void StateMRise(void)
**     Description :�����׶�
**     ������ʼ��
**     �����ȴ�
**     ��ʼ����
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_SSCNT       20//�ȴ�100ms
void StateMRise(void)
{
	//��ʱ��
	static  uint16_t  Cnt = 0;
	//���ռ�ձ����Ƽ�����
	static  uint16_t	BUCKMaxDutyCnt=0,BoostMaxDutyCnt=0;

	//�ж�����״̬
	switch(STState)
	{
		//��ʼ��״̬
		case    SSInit:
		{
			//�ر�PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�				
			//�����н���������ռ�ձ�����������Сռ�ձȿ�ʼ����
			CtrValue.BUCKMaxDuty  = MIN_BUKC_DUTY;
			CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;
			//��·���������ʼ��
			VErr0=0;
			VErr1=0;
			VErr2=0;
			IErr0=0;
			IErr1=0;
			IErr2=0;
			u0 = 0;
			u1 = 0;
			//��ת�������ȴ�״̬
			STState = SSWait;

			break;
		}
		//�ȴ�������״̬
		case    SSWait:
		{
			//�������ۼ�
			Cnt++;
			//�ȴ�100ms
			if(Cnt> MAX_SSCNT)
			{
				//��������0
				Cnt = 0;
				//��������ռ�ձ�
				CtrValue.BuckDuty = MIN_BUKC_DUTY;
				CtrValue.BUCKMaxDuty= MIN_BUKC_DUTY;
				CtrValue.BoostDuty = MIN_BOOST_DUTY;
				CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;
				//��·���������ʼ��
				VErr0=0;
				VErr1=0;
				VErr2=0;
				IErr0=0;
				IErr1=0;
				IErr2=0;
				u0 = 0;
				u1 = 0;
				//CtrValue.Voref����ο���ѹ��һ�뿪ʼ������������壬Ȼ��������
				CtrValue.Voref  = CtrValue.Voref >>1;
				CtrValue.Ioref  = CtrValue.Ioref >>1;
				STState = SSRun;	//��ת������״̬			
			}
			break;
		}
		//������״̬
		case    SSRun:
		{
			if(DF.PWMENFlag==0)//��ʽ����ǰ��·������0
			{
				//��·���������ʼ��
				VErr0=0;
				VErr1=0;
				VErr2=0;
				IErr0=0;
				IErr1=0;
				IErr2=0;
				u0 = 0;
				u1 = 0;
				HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //����PWM�����PWM��ʱ��
				HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //����PWM�����PWM��ʱ��		
			}
			//������־λ��λ
			DF.PWMENFlag=1;
			//���ռ�ձ�����������
			BUCKMaxDutyCnt++;
			BoostMaxDutyCnt++;
			//���ռ�ձ������ۼ�
			CtrValue.BUCKMaxDuty = CtrValue.BUCKMaxDuty + BUCKMaxDutyCnt*10;
			CtrValue.BoostMaxDuty = CtrValue.BoostMaxDuty + BoostMaxDutyCnt*10;
			//�ۼӵ����ֵ
			if(CtrValue.BUCKMaxDuty > MAX_BUCK_DUTY)
				CtrValue.BUCKMaxDuty  = MAX_BUCK_DUTY ;
			if(CtrValue.BoostMaxDuty > MAX_BOOST_DUTY)
				CtrValue.BoostMaxDuty  = MAX_BOOST_DUTY ;
			
			if((CtrValue.BUCKMaxDuty==MAX_BUCK_DUTY)&&(CtrValue.BoostMaxDuty==MAX_BOOST_DUTY))			
			{
				//״̬����ת������״̬
				DF.SMFlag  = Run;
				//��������״̬��ת����ʼ��״̬
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
**     Description :�������У������������ж�������
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
**     Description :����״̬
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
void StateMErr(void)
{
	//��ʼ����ѹ�ο���
	CtrValue.Voref=0;
	CtrValue.Ioref=0;
	//����ռ�ձ�
	CtrValue.BuckDuty = MIN_BUKC_DUTY;
	CtrValue.BUCKMaxDuty= MIN_BUKC_DUTY;
	CtrValue.BoostDuty = MIN_BOOST_DUTY;
	CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;	
	//��·���������ʼ��
	VErr0=0;
	VErr1=0;
	VErr2=0;
	IErr0=0;
	IErr1=0;
	IErr2=0;
	u0 = 0;
	u1 = 0;
	//�ر�PWM
	DF.PWMENFlag=0;
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�		
	//������������ת���ȴ���������
	if(DF.ErrFlag==F_NOERR)
			DF.SMFlag  = Wait;
}

/** ===================================================================
**     Funtion Name :void ValInit(void)
**     Description :   ��ز�����ʼ������
**     Parameters  :
**     Returns     :
** ===================================================================*/
void ValInit(void)
{
	//�ر�PWM
	DF.PWMENFlag=0;
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�		
	//������ϱ�־λ
	DF.ErrFlag=0;
	//��ʼ����ѹ�ο���
	CtrValue.Voref=0;
	CtrValue.Ioref=0;
	//����ռ�ձ�
	CtrValue.BuckDuty = MIN_BUKC_DUTY;
	CtrValue.BUCKMaxDuty= MIN_BUKC_DUTY;
	CtrValue.BoostDuty = MIN_BOOST_DUTY;
	CtrValue.BoostMaxDuty = MIN_BOOST_DUTY;	
	//��·���������ʼ��
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
						HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
						HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�	
					}
					break;
			}
			case Visual:
			{

					//Transfer the ADC sampling value to real value, and x1000(FLOAT -> INT)
					VoutT = (float)SADC.VoutAvg*68000/4096;
					IoutT = (float)SADC.IoutAvg*22000/4096;
					
					//�����ѹ
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
**     Description :   Get and refresh the reference voltage from the dedicated buffer��assign a proper value to Ctr.Voref,
**											In case voltage changes rapidly.
**     Parameters  :
**     Returns     :
** ===================================================================*/
#define MAX_VREF    2921//������ο���ѹ48V  0.5V������   48.5V/68V*Q12
#define MAX_IREF 		931//MAX reference current 5A
#define VREF_K      10//������ݼ�����
#define IREF_K      5//������ݼ�����
void IVrefGet(void)
{
	//��ѹ�ο�ֵ�м����
	int32_t VTemp = 0;	
	int32_t ITemp = 0;
	
	VTemp = (float)Vset/68000*4096;
	ITemp = (float)Iset/22000*4096;
	
	//�������������ݼ���ѹ�ο�ֵ
	if( VTemp> ( CtrValue.Voref + VREF_K))
			CtrValue.Voref = CtrValue.Voref + VREF_K;
	else if( VTemp < ( CtrValue.Voref - VREF_K ))
			CtrValue.Voref =CtrValue.Voref - VREF_K;
	else
			CtrValue.Voref = VTemp ;

	//MIXģʽ�µ�ѹ����-�����ѹ���ﵽ�����ѹ��2��
	if(CtrValue.Voref >(VTemp<<1))//��������������2*vin 
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

	//MIXģʽ�µ�ѹ����-�����ѹ���ﵽ�����ѹ��2��
	if(CtrValue.Ioref >(ITemp<<1))//��������������2*vin 
		CtrValue.Ioref =(ITemp<<1);
	if( CtrValue.Ioref > MAX_IREF )
		CtrValue.Ioref = MAX_IREF;
}

/*
** ===================================================================
**     Funtion Name :void ShortOff(void)
**     Description :��·��������������10��
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_SHORT_I     1396//��·�����о�
#define MIN_SHORT_V     289//��·��ѹ�о�
void ShortOff(void)
{
	static int32_t RSCnt = 0;
	static uint8_t RSNum =0 ;

	//������������� *A���ҵ�ѹС��*Vʱ�����ж�Ϊ������·����
	if((SADC.Iout> MAX_SHORT_I)&&(SADC.Vout <MIN_SHORT_V))
	{
		//�ر�PWM
		DF.PWMENFlag=0;
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�	
		//���ϱ�־λ
		setRegBits(DF.ErrFlag,F_SW_SHORT);
		//��ת������״̬
		DF.SMFlag  =Err;
	}
	//�����·�����ָ�
	//�����������·�������ػ���ȴ�4S�����������Ϣ������ȴ�״̬�ȴ�����
	if(getRegBits(DF.ErrFlag,F_SW_SHORT))
	{
		//�ȴ���������������ۼ�
		RSCnt++;
		//�ȴ�2S
		if(RSCnt >400)
		{
			//����������
			RSCnt=0;
			//��·����ֻ����10�Σ�10�κ�����
			if(RSNum > 10)
			{
				//ȷ����������ϣ�������
				RSNum =11;
				//�ر�PWM
				DF.PWMENFlag=0;
				HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
				HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�	
			}
			else
			{
				//��·�����������ۼ�
				RSNum++;
				//��������������ϱ�־λ
				clrRegBits(DF.ErrFlag,F_SW_SHORT);
			}
		}
	}
}
/*
** ===================================================================
**     Funtion Name :void SwOCP(void)
**     Description :�������������������
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_OCP_VAL     1117//*A���������� 
void SwOCP(void)
{
	//���������оݱ��ּ���������
	static  uint16_t  OCPCnt=0;
	//����������ּ���������
	static  uint16_t  RSCnt=0;
	//������������������
	static  uint16_t  RSNum=0;

	//�������������*A���ұ���500ms
	if((SADC.Iout > MAX_OCP_VAL)&&(DF.SMFlag  ==Run))
	{
		//�������ּ�ʱ
		OCPCnt++;
		//��������50ms������Ϊ��������
		if(OCPCnt > 10)
		{
			//��������0
			OCPCnt  = 0;
			//�ر�PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�	
			//���ϱ�־λ
			setRegBits(DF.ErrFlag,F_SW_IOUT_OCP);
			//��ת������״̬
			DF.SMFlag  =Err;
		}
	}
	else
		//��������0
		OCPCnt  = 0;

	//���������ָ�
	//�����������������������ػ���ȴ�4S�����������Ϣ������ȴ�״̬�ȴ�����
	if(getRegBits(DF.ErrFlag,F_SW_IOUT_OCP))
	{
		//�ȴ���������������ۼ�
		RSCnt++;
		//�ȴ�2S
		if(RSCnt > 400)
		{
			//����������
			RSCnt=0;
			//���������������ۼ�
			RSNum++;
			//��������ֻ����10�Σ�10�κ����������ع��ϣ�
			if(RSNum > 10 )
			{
				//ȷ����������ϣ�������
				RSNum =11;
				//�ر�PWM
				DF.PWMENFlag=0;
				HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
				HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�		
			}
			else
			{
			 //��������������ϱ�־λ
				clrRegBits(DF.ErrFlag,F_SW_IOUT_OCP);
			}
		}
	}
}

/*
** ===================================================================
**     Funtion Name :void SwOVP(void)
**     Description :��������ѹ������������
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_VOUT_OVP_VAL    3012//50V��ѹ����	��50/68��*Q12
void VoutSwOVP(void)
{
	//��ѹ�����оݱ��ּ���������
	static  uint16_t  OVPCnt=0;

	//�������ѹ����50V���ұ���100ms
	if (SADC.Vout > MAX_VOUT_OVP_VAL)
	{
		//�������ּ�ʱ
		OVPCnt++;
		//��������10ms
		if(OVPCnt > 2)
		{
			//��ʱ������
			OVPCnt=0;
			//�ر�PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�		
			//���ϱ�־λ
			setRegBits(DF.ErrFlag,F_SW_VOUT_OVP);
			//��ת������״̬
			DF.SMFlag  =Err;
		}
	}
	else
		OVPCnt = 0;
}

/*
** ===================================================================
**     Funtion Name :void VinSwUVP(void)
**     Description :�������Ƿѹ��������ѹ���뱣��,�ɻָ�
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MIN_UVP_VAL    686//11.4VǷѹ���� ��11.4/68 ��*Q12
#define MIN_UVP_VAL_RE  795//13.2VǷѹ�����ָ� ��13.2/68��*Q12
void VinSwUVP(void)
{
	//��ѹ�����оݱ��ּ���������
	static  uint16_t  UVPCnt=0;
	static  uint16_t	RSCnt=0;

	//���������С����11.4V���ұ���200ms
	if ((SADC.Vin < MIN_UVP_VAL) && (DF.SMFlag != Init ))
	{
		//�������ּ�ʱ
		UVPCnt++;
		//��������10ms
		if(UVPCnt > 2)
		{
			//��ʱ������
			UVPCnt=0;
			RSCnt=0;
			//�ر�PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�		
			//���ϱ�־λ
			setRegBits(DF.ErrFlag,F_SW_VIN_UVP);
			//��ת������״̬
			DF.SMFlag  =Err;
		}
	}
	else
		UVPCnt = 0;
	
	//����Ƿѹ�����ָ�
	//����������Ƿѹ�������ȴ������ѹ�ָ�������ˮƽ��������ϱ�־λ������
	if(getRegBits(DF.ErrFlag,F_SW_VIN_UVP))
	{
		if(SADC.Vin > MIN_UVP_VAL_RE) 
		{
			//�ȴ���������������ۼ�
			RSCnt++;
			//�ȴ�1S
			if(RSCnt > 200)
			{
				RSCnt=0;
				UVPCnt=0;
				//������ϱ�־λ
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
**     Description :��������ѹ������������
**     Parameters  : none
**     Returns     : none
** ===================================================================
*/
#define MAX_VIN_OVP_VAL    3012//50V��ѹ����	��50/68��*Q12
void VinSwOVP(void)
{
	//��ѹ�����оݱ��ּ���������
	static  uint16_t  OVPCnt=0;

	//�������ѹ����50V���ұ���100ms
	if (SADC.Vin > MAX_VIN_OVP_VAL )
	{
		//�������ּ�ʱ
		OVPCnt++;
		//��������10ms
		if(OVPCnt > 2)
		{
			//��ʱ������
			OVPCnt=0;
			//�ر�PWM
			DF.PWMENFlag=0;
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�		
			//���ϱ�־λ
			setRegBits(DF.ErrFlag,F_SW_VIN_OVP);
			//��ת������״̬
			DF.SMFlag  =Err;
		}
	}
	else
		OVPCnt = 0;
}

/** ===================================================================
**     Funtion Name :void LEDShow(void)
**     Description :  LED��ʾ����
**     ��ʼ����ȴ�����״̬�������ȫ��
**     ����״̬��������
**     ����״̬���̵���
**     ����״̬�������
**     Parameters  :
**     Returns     :
** ===================================================================*/
//���״̬�ƺ궨��
 #define SET_LED_G()	HAL_GPIO_WritePin(GPIOB, LED_G_Pin,GPIO_PIN_SET)//�̵���
 #define SET_LED_Y()	HAL_GPIO_WritePin(GPIOB, LED_Y_Pin,GPIO_PIN_SET)//�̵���
 #define SET_LED_R()	HAL_GPIO_WritePin(GPIOB, LED_R_Pin,GPIO_PIN_SET)//�̵���
 #define CLR_LED_G()	HAL_GPIO_WritePin(GPIOB, LED_G_Pin,GPIO_PIN_RESET)//�̵���
 #define CLR_LED_Y()	HAL_GPIO_WritePin(GPIOB, LED_Y_Pin,GPIO_PIN_RESET)//�Ƶ���
 #define CLR_LED_R()	HAL_GPIO_WritePin(GPIOB, LED_R_Pin,GPIO_PIN_RESET)//�����
void LEDShow(void)
{
	switch(DF.SMFlag)
	{
		//��ʼ��״̬�������ȫ��
		case  Init :
		{
			SET_LED_G();
			SET_LED_Y();
			SET_LED_R();
			break;
		}
		//�ȴ�״̬�������ȫ��
		case  Wait :
		{
			SET_LED_G();
			SET_LED_Y();
			SET_LED_R();
			break;
		}
		//������״̬��������
		case  Rise :
		{
			SET_LED_G();
			SET_LED_Y();
			CLR_LED_R();
			break;
		}
		//����״̬���̵���
		case  Run :
		{
			SET_LED_G();
			CLR_LED_Y();
			CLR_LED_R();
			break;
		}
		//����״̬�������
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
**     Description :����������״̬
**		 Ĭ��״̬KEYFlagΪ0.����ʱFlag��1���ٴΰ���Flag��0������ѭ��
**		 �������������У��������������У����°����󣬹ر�������������״̬
**     Parameters  :
**     Returns     :
** ===================================================================*/
#define READ_KEY1() HAL_GPIO_ReadPin(GPIOB, KEY_1_Pin)
#define READ_KEY2() HAL_GPIO_ReadPin(GPIOB, KEY_2_Pin)
void KEYFlag(void)
{
	//��ʱ��������������
	static uint16_t	KeyDownCnt1=0,KeyDownCnt2=0;
	
	//��������
	if(READ_KEY1()==0)
	{
		//��ʱ����������150mS��Ч
		KeyDownCnt1++;
		if(KeyDownCnt1 > 30)
		{
			KeyDownCnt1 = 0;//��ʱ������
			//����״̬�б仯
			if(DF.KeyFlag1==0)
				DF.KeyFlag1 = 1;
			else
				DF.KeyFlag1 = 0;
		}
	}
	else
		KeyDownCnt1 = 0;//��ʱ������
	
	//��������
	if(READ_KEY2()==0)
	{
		//��ʱ���������1�50mS��Ч
		KeyDownCnt2++;
		if(KeyDownCnt2 > 30)
		{
			KeyDownCnt2 = 0;//��ʱ������
			//����״̬�б仯
			if(DF.KeyFlag2==0)
				DF.KeyFlag2 = 1;
			else
				DF.KeyFlag2 = 0;
		}
	}
	else
		KeyDownCnt2 = 0;//��ʱ������

	//�������������У��������������У����°����󣬹ر�������������״̬
	if((DF.KeyFlag1==0)&&((DF.SMFlag==Rise)||(DF.SMFlag==Run)))
	{
		DF.SMFlag = Wait;
		//�ر�PWM
		DF.PWMENFlag=0;
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1|HRTIM_OUTPUT_TA2); //�ر�
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TB1|HRTIM_OUTPUT_TB2); //�ر�	
	}
}


/** ===================================================================
**     Funtion Name :void BBMode(void)
**     Description :����ģʽ�ж�
** 		 BUCKģʽ������ο���ѹ<0.8�������ѹ
** 		 BOOSTģʽ������ο���ѹ>1.2�������ѹ
**		 MIXģʽ��1.15�������ѹ>����ο���ѹ>0.85�������ѹ
**		 ������MIX��buck-boost��ģʽ���˳���BUCK����BOOSTʱ��Ҫ�ͻ�����ֹ���ٽ��������
**     Parameters  :
**     Returns     :
** ===================================================================*/
void BBMode(void)
{
	//��һ��ģʽ״̬��
	uint8_t PreBBFlag=0;
	
	//�ݴ浱ǰ��ģʽ״̬��
	PreBBFlag = DF.BBFlag;
	
	//�жϵ�ǰģ��Ĺ���ģʽ
	switch(DF.BBFlag)
	{
		//NA-��ʼ��ģʽ
		case  NA :		
		{
			if(DF.ModeFlag == CVmode)
			{
		  if(CtrValue.Voref < ((SADC.VinAvg*3277)>>12))//vout<0.8*vin
				DF.BBFlag = Buck;//buck mode
			else if(CtrValue.Voref > ((SADC.VinAvg*4915)>>12))//vout>1.2*vin
				DF.BBFlag = Boost;//boost mode
			else
				DF.BBFlag = Mix;//buck-boost��MIX�� mode
			}
			else
			{
				if(CtrValue.Ioref < ((SADC.IinAvg*3277)>>12))//vout<0.8*vin
				DF.BBFlag = Buck;//buck mode
			else if(CtrValue.Ioref > ((SADC.IinAvg*4915)>>12))//vout>1.2*vin
				DF.BBFlag = Boost;//boost mode
			else
				DF.BBFlag = Mix;//buck-boost��MIX�� mode
			}
			break;
		}
		//BUCKģʽ
		case  Buck :		
		{
			if(DF.ModeFlag == CVmode)
			{
				if(CtrValue.Voref > ((SADC.VinAvg*4915)>>12))//vout>1.2*vin
					DF.BBFlag = Boost;//boost mode
				else if(CtrValue.Voref >((SADC.VinAvg*3482)>>12))//1.2*vin>vout>0.85*vin
					DF.BBFlag = Mix;//buck-boost��MIX�� mode
			}
			else
			{
				if(CtrValue.Ioref > ((SADC.IinAvg*4915)>>12))//vout>1.2*vin
					DF.BBFlag = Boost;//boost mode
				else if(CtrValue.Ioref >((SADC.IinAvg*3482)>>12))//1.2*vin>vout>0.85*vin
					DF.BBFlag = Mix;//buck-boost��MIX�� mode
			}
			break;
		}
		//Boostģʽ
		case  Boost :		
		{
			if(DF.ModeFlag == CVmode)
			{
				if(CtrValue.Voref < ((SADC.VinAvg*3277)>>12))//vout<0.8*vin
					DF.BBFlag = Buck;//buck mode
				else if(CtrValue.Voref < ((SADC.VinAvg*4710)>>12))//0.8*vin<vout<1.15*vin
					DF.BBFlag = Mix;//buck-boost��MIX�� mode
			}
			else
			{
				if(CtrValue.Ioref < ((SADC.IinAvg*3277)>>12))//vout<0.8*vin
					DF.BBFlag = Buck;//buck mode
				else if(CtrValue.Ioref < ((SADC.IinAvg*4710)>>12))//0.8*vin<vout<1.15*vin
					DF.BBFlag = Mix;//buck-boost��MIX�� mode
			}
			break;
		}
		//Mixģʽ
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
	
	
	//��ģʽ�����任ʱ����һ�κ���һ�β�һ����,���־λ��λ����־λ���Ի�·���㸴λ����֤ģʽ�л����̲����д�Ĺ���
	if(PreBBFlag==DF.BBFlag)
		DF.BBModeChange =0;
	else
		DF.BBModeChange =1;
}
/** ===================================================================
**     Funtion Name :void MX_OLED_Init(void)
**     Description :OLED��ʼ������
**     ͨ��OLED��������
**     Parameters  :
**     Returns     :
** ===================================================================*/
void MX_OLED_Init(void)
{
	//��ʼ��OLED
	OLED_Init();
	OLED_CLS();
	//������ʾ�̶����ַ�
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
**     Description : OLED��ʾ����		 
**     ��ʾ����ģʽ-BUCK MODE,BOOST MODE,MIX MODE
**     ��ʾ״̬��IDEL,RISISING,RUNNING,ERROR
**     ��ʾ�����ѹ����λС��
**     ��ʾ�����������λС��
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
	
	//������ֵת����ʵ��ֵ��������100��(��ʾ��Ļ��С����)�����ʾ�����ѹ�������������ע�͵��ļ���
	VoutT = SADC.VoutAvg*6800>>12;
	IoutT = SADC.IoutAvg*2200>>12;
	//VinT = SADC.VinAvg*6800>>12;
	//IinT = SADC.IinAvg*2200>>12;
	//VadjT = CtrValue.Voref*6800>>12;
	
	//�ֱ𱣴�ʵ�ʵ�ѹ�͵�����ÿһλ��С�������λ�����������ʾ�����ѹ�������������ע�͵��ļ���
	//�����ѹ
	Vtemp[0] = (u8)(VoutT/1000);
	Vtemp[1] = (u8)((VoutT-(uint8_t)Vtemp[0]*1000)/100);
	Vtemp[2] = (u8)((VoutT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100)/10);
	Vtemp[3] = (u8)(VoutT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100-(uint16_t)Vtemp[2]*10);	
	//�����ѹ
//	Vtemp[0] = (u8)(VinT/1000);
//	Vtemp[1] = (u8)((VinT-(uint8_t)Vtemp[0]*1000)/100);
	//Vtemp[2] = (u8)((VinT -(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100)/10);
//	Vtemp[3] = (u8)(VinT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100-(uint16_t)Vtemp[2]*10);	
	//�������
	Itemp[0] = (u8)(IoutT/1000);
	Itemp[1] = (u8)((IoutT-(uint8_t)Itemp[0]*1000)/100);
	Itemp[2] = (u8)((IoutT-(uint16_t)Itemp[0]*1000-(uint16_t)Itemp[1]*100)/10);
	Itemp[3] = (u8)(IoutT-(uint16_t)Itemp[0]*1000-(uint16_t)Itemp[1]*100-(uint16_t)Itemp[2]*10);
//	//�������
//	Itemp[0] = (u8)(IinT/1000);
//	Itemp[1] = (u8)((IinT-(uint8_t)Itemp[0]*1000)/100);
//	Itemp[2] = (u8)((IinT-(uint16_t)Itemp[0]*1000-(uint16_t)Itemp[1]*100)/10);
//	Itemp[3] = (u8)(IinT-(uint16_t)Itemp[0]*1000-(uint16_t)Itemp[1]*100-(uint16_t)Itemp[2]*10); 
//	//�ο���ѹ
//	Vtemp[0] = (u8)(VadjT/1000);
//	Vtemp[1] = (u8)((VadjT-(uint8_t)Vtemp[0]*1000)/100);
//	Vtemp[2] = (u8)((VadjT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100)/10);
//	Vtemp[3] = (u8)(VadjT-(uint16_t)Vtemp[0]*1000-(uint16_t)Vtemp[1]*100-(uint16_t)Vtemp[2]*10);	
	
	//�������ģʽ�б仯���������Ļ
	if(BBFlagTemp!= DF.BBFlag)
	{
		//�ݴ��־λ
		BBFlagTemp = DF.BBFlag;
		//��ʾ����ģʽ
		switch(DF.BBFlag)
		{
			//NA
			case  NA :		
			{
				OLED_ShowStr(25,0,"MODE:*NA* ",2);
				break;
			}
			//BUCKģʽ
			case  Buck :		
			{
				OLED_ShowStr(25,0,"MODE:BUCK ",2);
				break;
			}
			//Boostģʽ
			case  Boost :		
			{
				OLED_ShowStr(25,0,"MODE:BOOST",2);
				break;
			}
			//Mixģʽ
			case  Mix :		
			{
				OLED_ShowStr(25,0,"MODE:MIX  ",2);
				break;
			}
		}
	}
	
	//��Դ����״̬�б任��������Ļ
	if(SMFlagTemp!= DF.SMFlag)
	{	
		SMFlagTemp = DF.SMFlag;
		//��ʾ��Դ����״̬
		switch(DF.SMFlag)
		{
			//��ʼ��״̬
			case  Init :
			{
				OLED_ShowStr(55,2,"Init  ",2);
				break;
			}
			//�ȴ�״̬
			case  Wait :
			{
				OLED_ShowStr(55,2,"Waiting",2);
				break;
			}
			//������״̬
			case  Rise :
			{
				OLED_ShowStr(55,2,"Rising",2);
				break;
			}
			//����״̬
			case  Run :
			{
				OLED_ShowStr(55,2,"Running",2);
				break;
			}
			//����״̬
			case  Err :
			{
				OLED_ShowStr(55,2,"Error  ",2);
				break;
			}
		}	
	}
	
	//��ʾ��ѹ����ÿһλ
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
	



