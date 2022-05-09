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
	int32_t   Vout;//输出电电压
	int32_t   VoutAvg;//输出电电压平均值
	int32_t   Iin;//输出电流
	int32_t   IinAvg;//输出电流平均值
	int32_t		IinOffset;//输入电流采样偏置
	int32_t   Vin;//输出电电压
	int32_t   VinAvg;//输出电电压平均值
	int32_t   Vadj;//滑动变阻器电压值
	int32_t   VadjAvg;//滑动变阻器电压平均值
};

#define CAL_VOUT_K	4101//Q12输入电压矫正K值
#define CAL_VOUT_B	49//Q12输入电压矫正B值
#define CAL_IOUT_K	4096//Q12输出电流矫正K值
#define CAL_IOUT_B	0//Q12输入电流矫正B值
#define CAL_VIN_K	4068//Q12输出电压矫正K值
#define CAL_VIN_B	59//Q12输出电压矫正B值
#define CAL_IIN_K	4096//Q12输入电流矫正K值
#define CAL_IIN_B	0//Q12输出电流矫正B值

struct  _Ctr_value
{
	int32_t		Voref;//参考电压
	int32_t		Ioref;//参考电流
	int32_t		ILimit;//限流参考电流
	int16_t		BUCKMaxDuty;//Buck最大占空比
	int16_t		BoostMaxDuty;//Boost最大占空比
	int16_t		BuckDuty;//Buck控制占空比
	int16_t		BoostDuty;//Boost控制占空比
	int32_t		Ilimitout;//电流环输出
};

//标志位定义
struct  _FLAG
{
	uint16_t	SMFlag;//状态机标志位
	uint16_t	CtrFlag;//控制标志位
	uint16_t  ErrFlag;//故障标志位
	uint16_t  RunFlag;// C.V/C.C mode setting
	uint8_t	BBFlag;//运行模式标志位，BUCK模式，BOOST模式，MIX混合模式	
	uint8_t PWMENFlag;//启动标志位	
	uint8_t ConFlag;//PC Connection Flag
	uint8_t KeyFlag1;//按键标志位
	uint8_t KeyFlag2;//按键标志位	
	uint8_t BBModeChange;//工作模式切换标志位
};

//状态机枚举量
typedef enum
{
    Init,//初始化
    Wait,//空闲等待
    Rise,//软启
    Run,//正常运行
    Err//故障
}STATE_M;

//状态机枚举量
typedef enum
{
    NA,//未定义
		Buck,//BUCK模式
    Boost,//BOOST模式
    Mix//MIX混合模式
}BB_M;

//软启动枚举变量
typedef enum
{
	SSInit,//软启初始化
	SSWait,//软启等待
	SSRun//开始软启
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
