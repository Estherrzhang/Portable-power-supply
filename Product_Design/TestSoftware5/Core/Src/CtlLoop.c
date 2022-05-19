/* USER CODE BEGIN Header */
/****************************************************************************************
	* 
	* @author  ���ߵ�Դ
	* @�Ա��������ӣ�https://shop598739109.taobao.com
	* @file           : CtlLoop.c
  * @brief          : ��·���ܺ���
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
#include "CtlLoop.h"


/****************��·��������**********************/
int32_t   VErr0=0,VErr1=0,VErr2=0;//��ѹ���Q12
int32_t   IErr0=0,IErr1=0,IErr2=0;//��ѹ���Q12
int32_t		u0=0,u1=0;//��ѹ�������

/*
** ===================================================================
**     Funtion Name : void BuckBoostVLoopCtlPID(void)
**     Description :  BUCK-BOOSTģʽ�»�·����
** 										�������ο���ѹ�������ѹ�Ĺ�ϵ���жϻ�·������BUCKģʽ��BOOSTģʽ����BUCK-BOOSTģʽ
** 										BUCKģʽ�£�BOOST�Ŀ��عܹ����ڹ̶�ռ�ձȣ�����BUCK��ռ�ձȿ��������ѹ
**										BOOSTģʽ�£�BUCK�Ŀ��عܹ����ڹ̶�ռ�ձȣ�����BOOST��ռ�ձȿ��������ѹ
**										BUCK-BOOSTģʽ�£�BUCK�Ŀ��عܹ����ڹ̶�ռ�ձȣ�����BOOST��ռ�ձȿ��������ѹ
**     Parameters  :��
**     Returns     :��
** ===================================================================
*/
//��·�Ĳ������������mathcad�����ļ���buck���-��ѹ-PID�Ͳ�������
#define BUCKPIDb0	5203		//Q8
#define BUCKPIDb1	-10246	//Q8
#define BUCKPIDb2	5044		//Q8
//��·�Ĳ������������mathcad�����ļ���BOOST���-��ѹ-PID�Ͳ�������
#define BOOSTPIDb0	8860		//Q8
#define BOOSTPIDb1	-17445	//Q8
#define BOOSTPIDb2	8588		//Q8
CCMRAM void BuckBoostVLoopCtlPID(void)
{		
	int32_t VoutTemp=0;//�����ѹ������
	
	//�����ѹ����
	VoutTemp = ((uint32_t )ADC1_RESULT[2]*CAL_VOUT_K>>12)+CAL_VOUT_B;
	//�����ѹ����������ο���ѹ���������ѹ��ռ�ձ����ӣ����������
	VErr0= CtrValue.Voref  - VoutTemp; 
	//������ƣ���ֹ�����������
	if(VErr0>30)
		VErr0=30;
	//��ģʽ�л�ʱ����Ϊ����ռ�ձȣ�ȷ��ģʽ�л�������
	//BBModeChangeΪģʽ�л�Ϊ����ͬģʽ�л�ʱ����λ�ᱻ��1
	if(DF.BBModeChange)
	{
		u1 = 0;
		DF.BBModeChange =0;
	}
	//�жϹ���ģʽ��BUCK��BOOST��BUCK-BOOST
	switch(DF.BBFlag)
	{
		case  NA ://��ʼ�׶�
		{
			VErr0=0;
			VErr1=0;
			VErr2=0;
			u0 = 0;
			u1 = 0;
			break;
		}
		case  Buck:	//BUCKģʽ	
		{
			//����PID��·���㹫ʽ������PID��·�����ĵ���
			u0 = u1 + VErr0*BUCKPIDb0 + VErr1*BUCKPIDb1 + VErr2*BUCKPIDb2;	
			//��ʷ���ݷ�ֵ
			VErr2 = VErr1;
			VErr1 = VErr0;
			u1 = u0;			
			//��·�����ֵ��u0����8λΪBUCKPIDb0-2Ϊ��Ϊ�Ŵ�Q8����������
			CtrValue.BuckDuty= u0>>8;
			CtrValue.BoostDuty=MIN_BOOST_DUTY1;//BOOST�Ϲ̶ܹ�ռ�ձ�93%���¹�7%			
			//��·��������Сռ�ձ�����
			if(CtrValue.BuckDuty > CtrValue.BUCKMaxDuty)
				CtrValue.BuckDuty = CtrValue.BUCKMaxDuty;	
			if(CtrValue.BuckDuty < MIN_BUKC_DUTY)
				CtrValue.BuckDuty = MIN_BUKC_DUTY;
			break;
		}				
		case  Boost://Boostģʽ
		{
			//����PID��·���㹫ʽ������PID��·�����ĵ���
			u0 = u1 + VErr0*BOOSTPIDb0 + VErr1*BOOSTPIDb1 + VErr2*BOOSTPIDb2;			
			//��ʷ���ݷ�ֵ
			VErr2 = VErr1;
			VErr1 = VErr0;
			u1 = u0;			
			//��·�����ֵ��u0����8λΪBUCKPIDb0-2Ϊ��Ϊ�Ŵ�Q8����������
			CtrValue.BuckDuty = MAX_BUCK_DUTY;//����̶�ռ�ձ�93%
			CtrValue.BoostDuty = u0>>8;	
			//��·��������Сռ�ձ�����
			if(CtrValue.BoostDuty > CtrValue.BoostMaxDuty)
				CtrValue.BoostDuty = CtrValue.BoostMaxDuty;	
			if(CtrValue.BoostDuty < MIN_BOOST_DUTY)
				CtrValue.BoostDuty = MIN_BOOST_DUTY;
			break;
		}
		case  Mix://Mixģʽ
		{
			//����PID��·���㹫ʽ������PID��·�����ĵ���
			u0 = u1 + VErr0*BOOSTPIDb0 + VErr1*BOOSTPIDb1 + VErr2*BOOSTPIDb2;			
			//��ʷ���ݷ�ֵ
			VErr2 = VErr1;
			VErr1 = VErr0;
			u1 = u0;			
			//��·�����ֵ��u0����8λΪBUCKPIDb0-2Ϊ��Ϊ�Ŵ�Q8����������
			CtrValue.BuckDuty = MAX_BUCK_DUTY1;//����̶�ռ�ձ�80%
			CtrValue.BoostDuty = u0>>8;	
			//��·��������Сռ�ձ�����
			if(CtrValue.BoostDuty > CtrValue.BoostMaxDuty)
				CtrValue.BoostDuty = CtrValue.BoostMaxDuty;	
			if(CtrValue.BoostDuty < MIN_BOOST_DUTY)
				CtrValue.BoostDuty = MIN_BOOST_DUTY;
			break;
		}	
	}
	
	//PWMENFlag��PWM������־λ������λΪ0ʱ,buck��ռ�ձ�Ϊ0�������;
	if(DF.PWMENFlag==0)
		CtrValue.BuckDuty = MIN_BUKC_DUTY;
	//���¶�Ӧ�Ĵ���
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR = CtrValue.BuckDuty * PERIOD>>12; //buckռ�ձ�
  HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP3xR = HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR>>1; //ADC����������
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].CMP1xR = PERIOD - (CtrValue.BoostDuty * PERIOD>>12);//Boostռ�ձ�
}


CCMRAM void BuckBoostILoopCtlPID(void)
{		
	int32_t IoutTemp=0;//�����ѹ������
	
	//�����ѹ����
	IoutTemp = ((uint32_t )ADC1_RESULT[3]*CAL_IOUT_K>>12)+CAL_IOUT_B;
	//�����ѹ����������ο���ѹ���������ѹ��ռ�ձ����ӣ����������
	IErr0= CtrValue.Ioref  - IoutTemp; 
	//������ƣ���ֹ�����������
	if(IErr0>30)
		IErr0=30;
	//��ģʽ�л�ʱ����Ϊ����ռ�ձȣ�ȷ��ģʽ�л�������
	//BBModeChangeΪģʽ�л�Ϊ����ͬģʽ�л�ʱ����λ�ᱻ��1
	if(DF.BBModeChange)
	{
		u1 = 0;
		DF.BBModeChange =0;
	}
	//�жϹ���ģʽ��BUCK��BOOST��BUCK-BOOST
	switch(DF.BBFlag)
	{
		case  NA ://��ʼ�׶�
		{
			VErr0=0;
			VErr1=0;
			VErr2=0;
			u0 = 0;
			u1 = 0;
			break;
		}
		case  Buck:	//BUCKģʽ	
		{
			//����PID��·���㹫ʽ������PID��·�����ĵ���
			u0 = u1 + IErr0*BUCKPIDb0 + IErr1*BUCKPIDb1 + IErr2*BUCKPIDb2;	
			//��ʷ���ݷ�ֵ
			IErr2 = IErr1;
			IErr1 = IErr0;
			u1 = u0;			
			//��·�����ֵ��u0����8λΪBUCKPIDb0-2Ϊ��Ϊ�Ŵ�Q8����������
			CtrValue.BuckDuty= u0>>8;
			CtrValue.BoostDuty=MIN_BOOST_DUTY1;//BOOST�Ϲ̶ܹ�ռ�ձ�93%���¹�7%			
			//��·��������Сռ�ձ�����
			if(CtrValue.BuckDuty > CtrValue.BUCKMaxDuty)
				CtrValue.BuckDuty = CtrValue.BUCKMaxDuty;	
			if(CtrValue.BuckDuty < MIN_BUKC_DUTY)
				CtrValue.BuckDuty = MIN_BUKC_DUTY;
			break;
		}				
		case  Boost://Boostģʽ
		{
			//����PID��·���㹫ʽ������PID��·�����ĵ���
			u0 = u1 + IErr0*BOOSTPIDb0 + IErr1*BOOSTPIDb1 + IErr2*BOOSTPIDb2;			
			//��ʷ���ݷ�ֵ
			IErr2 = IErr1;
			IErr1 = IErr0;
			u1 = u0;			
			//��·�����ֵ��u0����8λΪBUCKPIDb0-2Ϊ��Ϊ�Ŵ�Q8����������
			CtrValue.BuckDuty = MAX_BUCK_DUTY;//����̶�ռ�ձ�93%
			CtrValue.BoostDuty = u0>>8;	
			//��·��������Сռ�ձ�����
			if(CtrValue.BoostDuty > CtrValue.BoostMaxDuty)
				CtrValue.BoostDuty = CtrValue.BoostMaxDuty;	
			if(CtrValue.BoostDuty < MIN_BOOST_DUTY)
				CtrValue.BoostDuty = MIN_BOOST_DUTY;
			break;
		}
		case  Mix://Mixģʽ
		{
			//����PID��·���㹫ʽ������PID��·�����ĵ���
			u0 = u1 + IErr0*BOOSTPIDb0 + IErr1*BOOSTPIDb1 + IErr2*BOOSTPIDb2;			
			//��ʷ���ݷ�ֵ
			IErr2 = IErr1;
			IErr1 = IErr0;
			u1 = u0;			
			//��·�����ֵ��u0����8λΪBUCKPIDb0-2Ϊ��Ϊ�Ŵ�Q8����������
			CtrValue.BuckDuty = MAX_BUCK_DUTY1;//����̶�ռ�ձ�80%
			CtrValue.BoostDuty = u0>>8;	
			//��·��������Сռ�ձ�����
			if(CtrValue.BoostDuty > CtrValue.BoostMaxDuty)
				CtrValue.BoostDuty = CtrValue.BoostMaxDuty;	
			if(CtrValue.BoostDuty < MIN_BOOST_DUTY)
				CtrValue.BoostDuty = MIN_BOOST_DUTY;
			break;
		}	
	}
	
	//PWMENFlag��PWM������־λ������λΪ0ʱ,buck��ռ�ձ�Ϊ0�������;
	if(DF.PWMENFlag==0)
		CtrValue.BuckDuty = MIN_BUKC_DUTY;
	//���¶�Ӧ�Ĵ���
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR = CtrValue.BuckDuty * PERIOD>>12; //buckռ�ձ�
  HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP3xR = HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR>>1; //ADC����������
	HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].CMP1xR = PERIOD - (CtrValue.BoostDuty * PERIOD>>12);//Boostռ�ձ�
}
