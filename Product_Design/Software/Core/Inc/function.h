#ifndef __FUNCTION_H
#define __FUNCTION_H	 

#include "stm32f3xx_hal.h"
#include "hrtim.h"
#include "oled.h"
#include "adc.h"

extern uint16_t ADC1_RESULT[4];
extern struct  _ADI SADC;
extern struct  _Ctr_value  CtrValue;
extern struct  _FLAG    DF;
extern uint16_t OLEDShowCnt;

//Function declarations
void ADCSample(void);
void StateM(void);
void StateMInit(void);
void StateMWait(void);
void StateMRise(void);
void StateMRun(void);
void StateMErr(void);
void ValInit(void);
void VrefGet(void);
void ShortOff(void);
void SwOCP(void);
void VoutSwOVP(void);
void VinSwUVP(void);
void VinSwOVP(void);
void LEDShow(void);
void KEYFlag(void);
void BBMode(void);
void OLEDShow(void);
void MX_OLED_Init(void);

/*****************************Error class*****************/
#define     F_NOERR      			0x0000// No error
#define     F_SW_VIN_UVP  		0x0001// Input under voltage
#define     F_SW_VIN_OVP    	0x0002// Input over voltage 
#define     F_SW_VOUT_UVP  		0x0004// Output under voltage
#define     F_SW_VOUT_OVP    	0x0008// Output over voltage
#define     F_SW_IOUT_OCP    	0x0010// Output over current
#define     F_SW_SHORT  			0x0020// Output short

#define MIN_BUKC_DUTY	80						//BUCK minima Duty cycle
#define MAX_BUCK_DUTY 3809					//BUCK maxima Duty cycle 93%*Q12
#define	MAX_BUCK_DUTY1 3277					//MIX mode BUCK fixed Duty cycle 80%
#define MIN_BOOST_DUTY 80						//BOOST minima Duty cycle
#define MIN_BOOST_DUTY1 283					//MIX mode BOOST fixed Duty cycle 20%
#define MAX_BOOST_DUTY	2662				//BOOST maxima Duty cycle
#define MAX_BOOST_DUTY1	3809				//BOOST maxima Duty cycle 93%*Q12

//Sampling variables struct 
struct _ADI
{
	int32_t   Iout;										//OUTPUT CURRENT
	int32_t   IoutAvg;								//AVERAGE OUTPUT CURRENT 
	int32_t		IoutOffset;							//OUTPUT CURRENT SAMPLING OFFSET
	int32_t   Vout;//������ѹ
	int32_t   VoutAvg;//������ѹƽ��ֵ
	int32_t   Iin;//�������
	int32_t   IinAvg;//�������ƽ��ֵ
	int32_t		IinOffset;//�����������ƫ��
	int32_t   Vin;//������ѹ
	int32_t   VinAvg;//������ѹƽ��ֵ
	int32_t   Vadj;//������������ѹֵ
	int32_t   VadjAvg;//������������ѹƽ��ֵ
};

#define CAL_VOUT_K	4101//Q12�����ѹ����Kֵ
#define CAL_VOUT_B	49//Q12�����ѹ����Bֵ
#define CAL_IOUT_K	4096//Q12�����������Kֵ
#define CAL_IOUT_B	0//Q12�����������Bֵ
#define CAL_VIN_K	4068//Q12�����ѹ����Kֵ
#define CAL_VIN_B	59//Q12�����ѹ����Bֵ
#define CAL_IIN_K	4096//Q12�����������Kֵ
#define CAL_IIN_B	0//Q12�����������Bֵ

struct  _Ctr_value
{
	int32_t		Voref;//�ο���ѹ
	int32_t		Ioref;//�ο�����
	int32_t		ILimit;//�����ο�����
	int16_t		BUCKMaxDuty;//Buck���ռ�ձ�
	int16_t		BoostMaxDuty;//Boost���ռ�ձ�
	int16_t		BuckDuty;//Buck����ռ�ձ�
	int16_t		BoostDuty;//Boost����ռ�ձ�
	int32_t		Ilimitout;//���������
};

//��־λ����
struct  _FLAG
{
	uint16_t	SMFlag;//״̬����־λ
	uint16_t	CtrFlag;//���Ʊ�־λ
	uint16_t  ErrFlag;//���ϱ�־λ
	uint16_t  RunFlag;// C.V/C.C mode setting
	uint8_t	BBFlag;//����ģʽ��־λ��BUCKģʽ��BOOSTģʽ��MIX���ģʽ	
	uint8_t PWMENFlag;//������־λ	
	uint8_t ConFlag;//PC Connection Flag
	uint8_t KeyFlag1;//������־λ
	uint8_t KeyFlag2;//������־λ	
	uint8_t BBModeChange;//����ģʽ�л���־λ
};

//״̬��ö����
typedef enum
{
    Init,//��ʼ��
    Wait,//���еȴ�
    Rise,//����
    Run,//��������
    Err//����
}STATE_M;

//״̬��ö����
typedef enum
{
    NA,//δ����
		Buck,//BUCKģʽ
    Boost,//BOOSTģʽ
    Mix//MIX���ģʽ
}BB_M;

//������ö�ٱ���
typedef enum
{
	SSInit,//������ʼ��
	SSWait,//�����ȴ�
	SSRun//��ʼ����
 } SState_M;

 typedef enum
 {
	 CV, 		//CV mode [Range 0~30V]
	 CC			// CC mode	[Range 0~5A]
 }	Run_M;

#define setRegBits(reg, mask)   (reg |= (unsigned int)(mask))
#define clrRegBits(reg, mask)  	(reg &= (unsigned int)(~(unsigned int)(mask)))
#define getRegBits(reg, mask)   (reg & (unsigned int)(mask))
#define getReg(reg)           	(reg)

#define CCMRAM  __attribute__((section("ccmram")))

#endif
