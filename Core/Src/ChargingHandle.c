/*
 * Communication.c
 *
 *  Created on: JULY 20, 2021
 *      Author: LHC
 */

#include "ChargingHandle.h"
#include "dac.h"
#include "adc.h"
#include "rtc.h"
#include "Flash.h"
#include "Dwin.h"
#include "mdrtuslave.h"
#include "stdlib.h"
#include "clibration.h"
#include "cmsis_os.h"
#include "tool.h"
#include "shell.h"
#include "shell_port.h"

TIMER8 g_Timer = {
	.Timer8Count = 0,
	.Timer8Flag = false,
};
/*用户名和密码失效定时器*/
TIMER8 g_Timer1 = {
	.Timer8Count = 0,
	.Timer8Flag = false,
};
ChangeHandle g_Charge = {0}; //__attribute__((at(0x8000000)))
DisChargeHandle g_DisCharge = {0};
PresentBatteryInfomation g_PresentBatteryInfo = {0};

RTC_DateTypeDef SetDate; //设置日期
RTC_TimeTypeDef SetTime; //设置时间

// bool g_FirstFlag = true;	  //首次充电检测
uint32_t t_ChargingTimes = 0; //充电时长
float g_ChargingQuantity = 0; //充电电量
/*错误次数*/
// static uint32_t g_Error_counts = 0;

/*输入的用户名*/
static uint16_t InputUserName = 0;
/*输入的密码*/
static uint16_t InputPassword = 0;

static const uint16_t User_Infomation[USER_NUMBERS][USER_INFOS] = {
	{1001, 6666},
	{2021, 1234},
};

#if defined(KALMAN)
KFP voltage_KfpFilter = {
	LASTP, 0, 0,
	0, COVAR_Q, COVAR_R};

KFP current_KfpFilter = {

	LASTP, 0, 0,
	0, COVAR_Q, COVAR_R};
#else
SideParm SideparmCurrent = {true, {0.0}, &SideparmCurrent.SideBuff[0], 0.0};
SideParm SideparmVoltage = {true, {0.0}, &SideparmVoltage.SideBuff[0], 0.0};
#endif

#if (DEBUGGING == 1)
void User_Debug(void)
{
#if (USING_PART1 == 1U)
	static float temp_voltage = 48.0F;
#else
	static float temp_voltage = 2.0F;
#endif
	uint32_t voltage_ch_sum = 0;
	uint32_t currrent_ch_sum = 0;
	// uint16_t temp_dac = 0;
	// static uint32_t counter = 0;

	//	uint8_t buffer[2] ={ 0 } ;
	//	buffer[0] = GetAdcToDmaValue(1) >> 8;
	//	buffer[1] = GetAdcToDmaValue(1);

	// g_PresentBatteryInfo.PresentCurrent = Get_Current();  //获取电流
	// g_PresentBatteryInfo.PresentVoltage = Get_Voltage();  //获取电压

	g_Timer.Timer8Count = T_2S;
	/*打开底板电源供电开关*/
	HAL_GPIO_WritePin(POWER_ON_GPIO_Port, POWER_ON_Pin, GPIO_PIN_SET);
	/*不能立即打开充电开关*/
	if (!g_Timer.Timer8Flag)
	{
		return;
	}

	/*底板电源开始输出和打开充电开关*/
	HAL_GPIO_WritePin(CHARGING_READY_GPIO_Port, CHARGING_READY_Pin, GPIO_PIN_SET);
	/*打开充电回路*/
	HAL_GPIO_WritePin(CHARGING_GO_GPIO_Port, CHARGING_GO_Pin, GPIO_PIN_SET);
	// HAL_GPIO_TogglePin(CHARGING_GO_GPIO_Port, CHARGING_GO_Pin);
	/*风扇开关打开*/
	HAL_GPIO_WritePin(FANS_POWER_GPIO_Port, FANS_POWER_Pin, GPIO_PIN_SET);

#if (USING_PART1 == 1U)

	if (temp_voltage == 57.0F)
	{
		temp_voltage = 48.0F;
	}
	Set_Voltage(temp_voltage);
	temp_voltage += 1.0F;
#else
	if (temp_voltage >= 60.0F)
	{
		temp_voltage = 2.0F;
	}
	Set_Voltage(temp_voltage);
	osDelay(1000);
	for (uint8_t i = 0; i < 10; i++)
	{
		voltage_ch_sum += GetAdcToDmaValue(1U);
		currrent_ch_sum += GetAdcToDmaValue(0U);
		osDelay(100);
	}
	// Usart1_Printf("voltage_ch = %d, currrent_ch = %d,temp_voltage = %d.\r\n", (voltage_ch_sum / 10), (currrent_ch_sum / 10), temp_voltage);
	shellPrint(&shell, "voltage_ch = %d, currrent_ch = %d,temp_voltage = %.1f.\r\n", (voltage_ch_sum / 10), (currrent_ch_sum / 10), temp_voltage);
	temp_voltage += 5.0F;
#endif

	// switch (counter)
	// {
	// 	case 0 : temp_dac = 0U; break;
	// 	case 1 : temp_dac = 720U; break;
	// 	case 2 : temp_dac = 1440U; break;
	// 	case 3 : temp_dac = 2160U; break;
	// 	case 4 : temp_dac = 2880U; break;
	// 	case 5 : temp_dac = 3600U; break;
	// }

	// if (++counter >= 6U)
	// {
	// 	counter = 0U;
	// }

	// HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, temp_dac);

	//	HAL_UART_Transmit_DMA(&huart2, buffer, 2);
	//	while (__HAL_UART_GET_FLAG(&huart2,UART_FLAG_TC) == RESET) { }
	//
	//	return ;
}
#endif

/*分段存储ADC采集电压比列系数*/
static const float ADC_Param_Interval[][2] = {

	{0.014676054F, 0.941178841F}, /*(0V-12V]*/
	{0.014740869F, 0.909599973F}, /*(12V-24v]*/
	{0.014687892F, 0.997949929F}, /*(24V-36v]*/
	{0.014706447F, 0.94757113F},  /*(36V-48v]*/
	{0.014721239F, 0.88015927F}	  /*(48V-60v]*/
};

/*分段存储ADC采集的数字量*/
static const uint16_t ADC_Value_Interval[][2] = {

	{0U, 780U},		/*(0V-12V]*/
	{780U, 1600U},	/*(12V-24v]*/
	{1600U, 2376U}, /*(24V-36v]*/
	{2376U, 3200U}, /*(36V-48v]*/
	{3200U, 3800U}	/*(48V-60v]*/
};

#if (!USEING_DAC_TABLE)
/*分段存储DAC数字量比列系数*/
static const float DAC_Param_Interval[][2] =
	{
		{59.9018000F, 22.7626840F}, /*(6V-12V]*/
		{59.7551020F, 24.4995920F}, /*(12V-24v]*/
		{59.8039216F, 23.3235300F}, /*(24V-36v]*/
		{60.1479047F, 10.8266230F}, /*(36V-48v]*/
		{60.1941748F, 10.5825243F}	/*(48V-60v]*/
};

/*分段存储DAC对应的电压值*/
static const float DAC_Voltage_Interval[][2] =
	{
		{0.0F, 12.0F},	/*(6V-12V]*/
		{12.0F, 24.0F}, /*(12V-24v]*/
		{24.0F, 36.0F}, /*(24V-36v]*/
		{36.0F, 48.0F}, /*(36V-48v]*/
		{48.0F, 60.0F}	/*(48V-60v]*/
};
#else
/*分段存储DAC对应的电压值*/
const float DAC_Voltage_Interval[PART_SIZE][2] = {

	{2.0F, 5.0F},	/*(2V-5V]*/
	{5.0F, 10.0F},	/*(5V-10v]*/
	{10.0F, 15.0F}, /*(10V-15v]*/
	{15.0F, 20.0F}, /*(15V-20v]*/
	{20.0F, 25.0F}, /*(20V-25v]*/
	{25.0F, 30.0F}, /*(25V-30v]*/
	{30.0F, 35.0F}, /*(30V-35v]*/
	{35.0F, 40.0F}, /*(35V-40v]*/
	{40.0F, 45.0F}, /*(40V-45v]*/
	{45.0F, 50.0F}, /*(45V-50v]*/
	{50.0F, 55.0F}, /*(50V-55v]*/
	{55.0F, 57.0F}, /*(55V-57v]*/
};

/*分段存储DAC数字量比列系数*/
float DAC_Param_Interval[PART_SIZE][2] = {

	{62.000000F, 16.000000F},  /*(2V-5V]*/
	{63.200001F, 17.000000F},  /*(5V-10v]*/
	{65.199997F, -1.000000F},  /*(10V-15v]*/
	{64.000000F, 19.000000F},  /*(15V-20v]*/
	{65.000000F, 0.0000000F},  /*(20V-25v]*/
	{63.799999F, 32.000000F},  /*(25V-30v]*/
	{63.799999F, 33.000000F},  /*(30V-35v]*/
	{65.199997F, -13.000000F}, /*(35V-40v]*/
	{66.400002F, -60.000000F}, /*(40V-45v]*/
	{64.400002F, 31.0000000F}, /*(45V-50v]*/
	{65.800003F, -36.000244F}, /*(50V-55v]*/
	{65.500000F, -17.500000F}, /*(55V-57v]*/
};
#endif

/*每个12V电池由6个单元组成*/
static const float Voltage_Interval[][2] = {

	{0, 0},
	{6U * SINGLEUNIT_MINVOLTAGE, 6U * SINGLEUNIT_MAXVOLTAGE},
	{12U * SINGLEUNIT_MINVOLTAGE, 12U * SINGLEUNIT_MAXVOLTAGE},
	{18U * SINGLEUNIT_MINVOLTAGE, 18U * SINGLEUNIT_MAXVOLTAGE},
	{24U * SINGLEUNIT_MINVOLTAGE, 24U * SINGLEUNIT_MAXVOLTAGE},
	{30U * SINGLEUNIT_MINVOLTAGE, 30U * SINGLEUNIT_MAXVOLTAGE},
};

/*
 * 获得ADC采集电压的计算区间
 */
uint16_t Get_ADCVoltageInterval(uint16_t adc_value)
{
	uint16_t length = (sizeof(ADC_Value_Interval) / sizeof(uint16_t)) / 2U;

	for (uint16_t i = 0; i < length; i++)
	{
		if ((adc_value > ADC_Value_Interval[i][0]) && (adc_value < ADC_Value_Interval[i][1]))
		{
			return i;
		}
	}
	return 0;
}

/*
 * 获得DAC输出电压的计算区间
 */
uint16_t Get_DACVoltageInterval(float adc_value)
{
	uint16_t length = (sizeof(DAC_Voltage_Interval) / sizeof(float)) / 2U;

	for (uint16_t i = 0; i < length; i++)
	{
		if ((adc_value > DAC_Voltage_Interval[i][0]) && (adc_value < DAC_Voltage_Interval[i][1]))
		{
			return i;
		}
	}
	return 0;
}

/*
 *   获取ADC实际电流值
 */
float Get_Current(void)
{
	uint32_t value = GetAdcToDmaValue(0U);
	/*对应ADC_CHANNEL_6*/
	// if (value == 0)
	// {
	// 	return 0;
	// }
	// return (((float)value * AD_CURRENT_P1) + AD_CURRENT_P2);

	return (value ? ((float)value * AD_CURRENT_P1) + AD_CURRENT_P2 : 0);
}

/*
 *   获取ADC实际电压值
 */
float Get_Voltage(void)
{
	float adc_voltage_p1 = 0;
	float adc_voltage_p2 = 0;
	uint32_t adc_value = 0;
	/*对应ADC_CHANNEL_7*/
	adc_value = GetAdcToDmaValue(1U);

	if (adc_value == 0)
	{
		return 0;
	}
	/*找到当前ADC值对应的电压区间*/
	adc_voltage_p1 = ADC_Param_Interval[Get_ADCVoltageInterval(adc_value)][0];
	adc_voltage_p2 = ADC_Param_Interval[Get_ADCVoltageInterval(adc_value)][1];

	return (((float)adc_value * adc_voltage_p1) + adc_voltage_p2);
}

/*
 * 设置DA输出电压值
 */
void Set_Voltage(float voltage)
{ /*记录上一次需要输出的电压*/
	static float last_voltage = 0;
	float dac_voltage_p1 = 0;
	float dac_voltage_p2 = 0;
	uint32_t dac_value = 0;

	/*限制DA输入最大值,使电压值不超过60V*/
	if (voltage <= HARDWARE_DAVOLTAGE)
	{ /*次数避免数值相同情况下频繁向DA传送数值*/
		if (last_voltage != voltage)
		{
			last_voltage = voltage;
			if (voltage != 0)
			{
				dac_voltage_p1 = DAC_Param_Interval[Get_DACVoltageInterval(voltage)][0];
				dac_voltage_p2 = DAC_Param_Interval[Get_DACVoltageInterval(voltage)][1];
				dac_value = (uint32_t)(dac_voltage_p1 * voltage + dac_voltage_p2);
				/*限制DAC输入值*/
				// if (dac_value > DA_OUTPUTMAX)
				// {
				// 	dac_value = DA_OUTPUTMAX;
				// }
			}
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, (dac_value & 0x0FFF));
		}
	}
}

/*
 * 获得电池信息
 */
uint16_t Get_BaterrryInfo(float persent_voltage)
{
	uint16_t length = (sizeof(Voltage_Interval) / sizeof(float)) / 2U;

	for (uint16_t i = 0; i < length; i++)
	{
		if ((persent_voltage > Voltage_Interval[i][0]) && (persent_voltage < Voltage_Interval[i][1]))
		{
			return i;
		}
	}
	return 0;
}

void Clear_UserInfo(void)
{
	if (g_Timer1.Timer8Flag)
	{
		g_Timer1.Timer8Flag = false;
		InputUserName = InputPassword = 0;
	}
}
/*
 * 设置电池电流参数
 */
static void Set_BaterrryCurrentInfo(void)
{
	/*设置各段电流*/
	g_PresentBatteryInfo.User_TrickleCurrent = g_Charge.BatteryCapacity * SINGLEUNIT_STDCURRENT;
	g_Charge.TrickleCurrent = g_PresentBatteryInfo.User_TrickleCurrent * 10U;

	g_PresentBatteryInfo.User_ConstantCurrent_Current = g_Charge.BatteryCapacity * SINGLEUNIT_MAXCURRENT;
	g_Charge.ConstantCurrent_Current = g_PresentBatteryInfo.User_ConstantCurrent_Current * 10U;

	g_PresentBatteryInfo.User_ConstantVoltageTargetCurrent = g_Charge.BatteryCapacity * SINGLEUNIT_MINCURRENT;
	g_Charge.ConstantVoltageTargetCurrent = g_PresentBatteryInfo.User_ConstantVoltageTargetCurrent * 10U;
}

/*
 * 设置电池电压参数
 */
static void Set_BaterrryVoltageInfo(uint16_t start_section)
{
	/*如果单元个数量不为零，则按照单元标准*/
	if (g_Charge.UnitElements)
	{
		g_PresentBatteryInfo.User_TrickleTargetVlotage = g_Charge.UnitElements * SINGLEUNIT_MINVOLTAGE;
		g_PresentBatteryInfo.User_ConstantVoltage_Voltage = g_Charge.UnitElements * SINGLEUNIT_STDVOLTAGE;
		g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage = g_Charge.UnitElements * SINGLEUNIT_MAXVOLTAGE;
		/*保留用户设置*/
		if (g_PresentBatteryInfo.User_SecondBootVoltage == 0)
		{
			g_PresentBatteryInfo.User_SecondBootVoltage = g_Charge.UnitElements * 2.10F;
		}
	}
	/*自动设置*/
	else
	{
		g_PresentBatteryInfo.User_TrickleTargetVlotage = Voltage_Interval[start_section][0];
		g_PresentBatteryInfo.User_ConstantVoltage_Voltage = SINGLEUNIT_STDVOLTAGE * (Voltage_Interval[start_section][0] / SINGLEUNIT_MINVOLTAGE);
		g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage = Voltage_Interval[start_section][1];
		/*设置二次启动电压*/
		g_PresentBatteryInfo.User_SecondBootVoltage = 2.10F * (Voltage_Interval[start_section][0] / SINGLEUNIT_MINVOLTAGE);
	}
	/*设置屏幕显示时的各段电压*/
	g_Charge.TrickleTargetVlotage = g_PresentBatteryInfo.User_TrickleTargetVlotage * 10U;
	g_Charge.ConstantVoltage_Voltage = g_PresentBatteryInfo.User_ConstantVoltage_Voltage * 10U;
	g_Charge.ConstantCurrentTargetVoltage = g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage * 10U;
	/*把单精度浮点数转换为整数*/
	g_Charge.SecondBootVoltage = g_PresentBatteryInfo.User_SecondBootVoltage * 10U;
}

/*
 * 设置电池参数
 */
void Set_BaterrryInfo(uint16_t start_section)
{
	/*设置当前充电电瓶电流参数*/
	Set_BaterrryCurrentInfo();
	/*设置当前充电电瓶电压参数*/
	Set_BaterrryVoltageInfo(start_section);
}

/*
 * 复位充电电压
 */
static float Reset_ChargingVoltage(void)
{
	float Current_Voltage = Get_Voltage();
	float Compensation = ((float)g_Charge.Compensation) / 1000.0F;

	/*最大补偿值500mv, 最小补偿值100mv*/
	Compensation > MAX_COMPENSATION ? (Compensation = MAX_COMPENSATION) : (Compensation < MIN_COMPENSATION ? (Compensation = MIN_COMPENSATION) : 0);
	/*把当前检测到电瓶电压+0.5V作为起充电压*/
#if (!USEING_COMPENSATION)
	g_PresentBatteryInfo.PresentChargingVoltage = Current_Voltage + 0.5F;
#else
	/*起充电压由屏幕给定*/
	g_PresentBatteryInfo.PresentChargingVoltage = Current_Voltage + ((float)g_Charge.Compensation) / 1000.0F;
#endif

	return Current_Voltage;
}

/*
 * 上报迪文屏幕数据
 */
void Dwin_ReportHadle(void)
{
	uint8_t buffer[32] = {0};
	uint8_t index = Report_DataHandle(buffer);

	Report_RealTime();
	osDelay(1);
	/*其他数据上报*/
	DWIN_WRITE(CHARGE_BATTERY_VOLTAGE_NOW_ADDR, buffer, index);
	osDelay(1);
	/*上报充电器状态*/
	DWIN_WRITE(MACHINE_STATE_ADDR, g_PresentBatteryInfo.ChargerStatus,
			   sizeof(g_PresentBatteryInfo.ChargerStatus));
	osDelay(1);
	/*上报充电状态*/
	DWIN_WRITE(CHARGE_STATE_ADDR, g_PresentBatteryInfo.ChargingStatus,
			   sizeof(g_PresentBatteryInfo.ChargingStatus));
	osDelay(1);
	Charging_Animation();
}

/*
 * 充电动画
 */
void Charging_Animation(void)
{
	uint16_t temp_data = 0x0000;

	/*在恒流模式*/
	if (g_PresentBatteryInfo.Cstate == constant_current)
	{
		if (g_PresentBatteryInfo.QuickChargingFlag == true)
		{
			/*打开快速充电图标*/
			temp_data = 0x0100;
		}
		DWIN_WRITE(QCHARGE_ANIMATION_ADDR, (uint8_t *)&temp_data, sizeof(temp_data)); /*设置当前充电动画地址*/
		osDelay(1);
	}
	/*清空缓冲区*/
	temp_data = 0x0000;
	/*充电计时器打开，也就是充电开始*/
	if (g_PresentBatteryInfo.ChargingTimingFlag == true)
	{
		temp_data = 0x0100;
	}
	/*设置充电动画*/
	DWIN_WRITE(CHARGE_ANIMATION_ADDR, (uint8_t *)&temp_data, sizeof(temp_data));
}

/*
 * 上报实时时间
 */
void Report_RealTime(void)
{
	// RTC_TimeTypeDef stimestructure = {0};
	// RTC_DateTypeDef sdatestructure = {0};
	myRtc_Get_DateTime(&sdatestructure, &stimestructure); //获取当前时间
	// HAL_RTC_GetTime(&hrtc, &stimestructure, RTC_FORMAT_BIN);
	// HAL_RTC_GetDate(&hrtc, &sdatestructure, RTC_FORMAT_BIN);
	// Stamp2time(&sdatestructure, &stimestructure, (uint32_t)(hrtc.Instance->CNTH << 16U | hrtc.Instance->CNTL), 8);
	/*年月日周 时分秒*/
	uint8_t timebuff[] = {
		0x07,
		sdatestructure.Year + 0xD0,
		0x00,
		sdatestructure.Month,
		0x00,
		sdatestructure.Date,
		0x00,
		sdatestructure.WeekDay,
		0x00,
		stimestructure.Hours,
		0x00,
		stimestructure.Minutes,
		0x00,
		stimestructure.Seconds,
	};

	DWIN_WRITE(SHOW_YEAR_ADDR, timebuff, sizeof(timebuff));
}

/*
 * 获得当前充电电量：单位Ah
 */
#define __Get_ChargingQuantity(__Q) ((float)((__Q) / S_CONVERT_HOUR))
/*
 * 获得当前充电时间: 单位min
 */
#define __Get_ChargingTimes(__T) ((uint32_t)((__T) / MS_CONVERT_MINUTE))

/*
 * 充电器实时数据上报处理
 */
uint8_t Report_DataHandle(uint8_t *buffer)
{
	uint8_t index = 0;
	uint16_t temp_times = 0;

	g_PresentBatteryInfo.ChargingQuantity = __Get_ChargingQuantity(g_ChargingQuantity); //获取充电电量
	g_PresentBatteryInfo.ChargingTimes = __Get_ChargingTimes(t_ChargingTimes);			//获取充电时长
	float data[] = {
		g_PresentBatteryInfo.PresentVoltage,		 //获取电瓶实际电压值
		g_PresentBatteryInfo.PresentChargingVoltage, //获取当前充电电压值
		g_PresentBatteryInfo.PresentCurrent,		 //获取当前充电电流值
		g_PresentBatteryInfo.ChargingQuantity,		 //获取当前充电电量值
		g_PresentBatteryInfo.PresentWireLossVoltage, //获取当前输电线电压损耗
	};
	/*把数据记录到本地寄存器*/
	mdRTU_WriteHoldRegs(Slave1_Object, MDREGISTER_STARA_ADDR, sizeof(data) / sizeof(mdU16), (mdU16 *)data);
	mdRTU_WriteHoldReg(Slave1_Object, MDREGISTER_CHARGINGTIMES_ADDR, g_PresentBatteryInfo.ChargingTimes);
	if (Slave1_Object->updateFlag && Slave1_Object)
	{
		Slave1_Object->updateFlag = false;
		/*先把数据读取到临时变量*/
		mdRTU_ReadHoldRegister(Slave1_Object, MDREGISTER_TAGATTIMES_ADDR, &temp_times);
		/*首次上电导致数据错误*/
		if ((temp_times != 0U) && (temp_times != g_Charge.TargetTime))
		{
			g_Charge.TargetTime = temp_times;
			/*充电时间存到存到flash，并且更新屏幕值*/
			g_Charge.IsSavedFlag = true;
			/*注意不要使用g_Charge.TargetTime*/
			Endian_Swap((uint8_t *)&temp_times, 0, sizeof(temp_times));
			DWIN_WRITE(CHARGE_TARGET_TIME_ADDR, (uint8_t *)&temp_times, sizeof(temp_times));
		}
	}
	else
	{
		/*目标充电时长*/
		mdRTU_WriteHoldReg(Slave1_Object, MDREGISTER_TAGATTIMES_ADDR, g_Charge.TargetTime);
	}

	/*把ARM架构的小端存储模式转换为大端模式*/
	for (uint8_t i = 0; i < sizeof(data) / sizeof(float); i++)
	{
		Endian_Swap((uint8_t *)&data[i], 0, sizeof(float));
	}
	memcpy(buffer, data, sizeof(data));
	index = sizeof(data);
	/*当前充电时长*/
	buffer[index++] = g_PresentBatteryInfo.ChargingTimes >> 8;
	buffer[index++] = g_PresentBatteryInfo.ChargingTimes;

	return index;
}

/*
 * 充电器上报后台充电参数到迪文屏幕
 */
void Report_BackChargingData(void)
{
	uint8_t reportbuff[64];
	memcpy(reportbuff, &g_Charge, sizeof(g_Charge));

	for (uint8_t i = 0; i < sizeof(g_Charge); i++)
	{
		Endian_Swap(&reportbuff[i * 2], 0, sizeof(uint16_t));
	}
	/*不上报第一个标志数据*/
	DWIN_WRITE(TRICKLE_CHARGE_CURRENT_ADDR, (uint8_t *)&(reportbuff[2]), sizeof(g_Charge) - sizeof(uint16_t));
}

/*
 * FLASH操作
 */
void Flash_Operation(void)
{
	if (g_Charge.IsSavedFlag == true)
	{ /*此处flash只能从1写为0,不能把0写成1*/

		// g_Charge.TargetTime = 0;
		// FLASH_Read(CHARGE_SAVE_ADDRESS, (uint16_t *)&g_Charge, sizeof(g_Charge));
#if (DEBUGGING)
		/*先写入Flash,在清除单次触发标志*/
		uint32_t error = FLASH_Write(CHARGE_SAVE_ADDRESS, (uint16_t *)&g_Charge, sizeof(g_Charge));
		// Usart1_Printf("error is 0x%x,minutes is %d\r\n", error, g_Charge.TargetTime);
		shellPrint(&shell, "error is 0x%x,minutes is %d\r\n", error, g_Charge.TargetTime);
#else
		FLASH_Write(CHARGE_SAVE_ADDRESS, (uint16_t *)&g_Charge, sizeof(g_Charge));
#endif
		g_Charge.IsSavedFlag = false;
	}
}

/*
 *  采样电流、电压值
 */
void Sampling_handle(void)
{
#if defined(KALMAN)
	g_PresentBatteryInfo.PresentVoltage = kalmanFilter(&voltage_KfpFilter, Get_Voltage());
	/*只有在充电状态下，才利用滤波检测电瓶真实电流*/
	if (g_PresentBatteryInfo.Mstate == charging)
	{
		g_PresentBatteryInfo.PresentCurrent = kalmanFilter(&current_KfpFilter, Get_Current());
		/*动态的校准由于外部线路或器件引起的压降*/
		g_PresentBatteryInfo.PresentWireLossVoltage = g_PresentBatteryInfo.PresentChargingVoltage - g_PresentBatteryInfo.PresentVoltage;
		// g_PresentBatteryInfo.PresentVoltage -= g_PresentBatteryInfo.PresentWireLossVoltage;
	}
	else
	{ /*清除采样电流*/
		g_PresentBatteryInfo.PresentCurrent = 0;
	}
#else
	g_PresentBatteryInfo.PresentVoltage = sidefilter(&SideparmVoltage, Get_Voltage());
	/*只有在充电状态下，才利用滤波检测电瓶真实电流*/
	if (g_PresentBatteryInfo.Mstate == charging)
	{
		/*动态的校准由于外部线路或器件引起的压降*/
		g_PresentBatteryInfo.PresentWireLossVoltage = g_PresentBatteryInfo.PresentChargingVoltage - g_PresentBatteryInfo.PresentVoltage;
		g_PresentBatteryInfo.PresentCurrent = sidefilter(&SideparmCurrent, Get_Current());
	}
#endif
}

/*
 *  充电断路检测
 */
void Check_ChargingDisconnection()
{
	/*开路次数*/
	static uint8_t currentopen_counts = 0;
	static uint8_t error_counts = 0;
	static uint8_t fault_counts = 0;

	/*检测电流值低于充电截止电流*/
	if (g_PresentBatteryInfo.PresentCurrent < CHARGING_ENDCURRENT)
	{
		currentopen_counts++;
		if (++currentopen_counts > BREAKCOUNTS)
		{
			/*识别为电池开路或者电压低导致的无电流*/
			g_PresentBatteryInfo.ZeroCurrentFlag = true;
			currentopen_counts = 0;
		}
	}
	else
	{
		currentopen_counts = 0;
	}
	/*检测底板是否报错*/
	if (g_PresentBatteryInfo.PresentChargingVoltage)
	{
		if (HAL_GPIO_ReadPin(POWER_OK_GPIO_Port, POWER_OK_Pin) == GPIO_PIN_SET)
		{
			if (++fault_counts > BREAKCOUNTS)
			{
				g_PresentBatteryInfo.ChargingFaultFlag = true;
			}
		}
		else
		{
			fault_counts = 0;
		}
	}
	/*充电故障：充电电压<当前电瓶两端电压或者充电过程中硬件异常导致的充电电压过高,报故障*/
	if ((g_PresentBatteryInfo.PresentChargingVoltage < (g_PresentBatteryInfo.PresentVoltage - MIN_OFFSET_VOLTAGE)) ||
		(g_PresentBatteryInfo.PresentChargingVoltage > (g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage +
														MAX_OFFSET_VOLTAGE)) ||
		(g_PresentBatteryInfo.PresentVoltage >
		 (g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage + MAX_OFFSET_VOLTAGE)))
	{
		/*复位充电电压*/
		Reset_ChargingVoltage();
		if (++error_counts >= MAX_DEFAULT_COUNTS)
		{
			error_counts = 0U;
			/*故障提醒*/
			g_PresentBatteryInfo.ChargingFaultFlag = true;
			// g_PresentBatteryInfo.Cstate = error;
		}
	}
	else
	{
		error_counts = 0U;
	}
}

/*
 * 	获取机器状态
 *  注：1.恒压和恒流区分不明显
 *
 */
void getMachineState(void)
{
	float Current_Voltage = 0;
	mdBit AcInput_Bit = false;
	static bool mutex_flag = false;

	/*读出交流输入线圈状态值*/
	mdRTU_ReadCoil(Slave1_Object, MDREGISTER_CLOSE_ACINPUT_ADDR, AcInput_Bit);
	g_PresentBatteryInfo.CloseAcInputFlag = AcInput_Bit ? true : false;
	/*完全断路或者充电结束、充电故障、远程主动关闭*/
	if ((g_PresentBatteryInfo.PresentVoltage < CHECK_VOLTAGE) || (g_PresentBatteryInfo.ChargingEndFlag == true) ||
		(g_PresentBatteryInfo.ZeroCurrentFlag == true) || (g_PresentBatteryInfo.ChargingFaultFlag == true) ||
		(g_PresentBatteryInfo.CloseAcInputFlag == true))
	{
		mutex_flag = false;
		g_PresentBatteryInfo.ChargingTimingFlag = false;
		g_PresentBatteryInfo.Mstate = standby;
	}
	else
	{
		// if (g_FirstFlag == true)
		if (!mutex_flag)
		{
			// g_FirstFlag = false;
			mutex_flag = true;
			/*把当前检测到电瓶电压+0.5V作为起充电压*/
			Current_Voltage = Reset_ChargingVoltage();
			/*自动设置当前电池的充电信息*/
			Set_BaterrryInfo(Get_BaterrryInfo(Current_Voltage));
			/*更新充电器后台充电参数*/
			Report_BackChargingData();
			/*触发开始打开底板供电即计时*/
			g_Timer.Timer8Count = T_5S;
		}
		/*充电计时器开启*/
		g_PresentBatteryInfo.ChargingTimingFlag = true;
		g_PresentBatteryInfo.Mstate = charging;
	}
}

/*获取充电结束时上一次与当前允许的电流波动误差:恒流电流的MIN_OFFSET_CURRENT倍*/
#define GET_OFFSET_CURRENT(Target_Current) ((float)(Target_Current * MIN_OFFSET_CURRENT))

/*
 * 获取当前充电状态
 * 通过ADC采集电压和DAC输出电压作为联合判断条件，决定当前处于那个阶段，解决图标跳动问题。
 */
void getChargingState(void)
{
	static uint32_t StartTimes = 0;
	static float LastCurrent = 0;

	/*涓流模式*/
	if ((g_PresentBatteryInfo.PresentVoltage < g_PresentBatteryInfo.User_TrickleTargetVlotage) ||
		(g_PresentBatteryInfo.PresentChargingVoltage < g_PresentBatteryInfo.User_TrickleTargetVlotage))
	{
		g_PresentBatteryInfo.Cstate = trickle;
	}
	/*恒流模式g_PresentBatteryInfo.PresentChargingVoltage*/
	else if ((g_PresentBatteryInfo.PresentVoltage < g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage) ||
			 (g_PresentBatteryInfo.PresentChargingVoltage < g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage))
	{ /*对输出电压进行限幅*/
		/*可以在恒流阶段加入<恒流电流时，进入恒压充电*/
		g_PresentBatteryInfo.Cstate = constant_current;
		/*捕捉到最后一次从恒流转入恒压前电流值*/
		// LastCurrent = g_PresentBatteryInfo.PresentCurrent;
		/*如果是由于抖动造成的状态切换，清空计时*/
		StartTimes = 0;
	}
	/*恒压模式*/
	else
	{ /*条件g_PresentBatteryInfo.PresentVoltage > g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage满足*/
		g_PresentBatteryInfo.Cstate = constant_voltage;
		/*500MS一次，总共计数30min，若电流变化<0.05，则认为电池充满*/
		if (++StartTimes > __CHARGING_END_TIMES())
		{
			StartTimes = 0;
			if (fabs(LastCurrent - g_PresentBatteryInfo.PresentCurrent) <
					GET_OFFSET_CURRENT(g_Charge.ConstantCurrent_Current) ||
				(g_PresentBatteryInfo.PresentCurrent < g_PresentBatteryInfo.User_TrickleCurrent))
			{ /*正常充电结束的标志是：当前充电电流小于涓流电流，或者充电时长超过设定时间*/
				/*清除前30分钟电流值*/
				LastCurrent = 0;
				g_PresentBatteryInfo.Cstate = chargingend;
			}
			/*更新当前电流值*/
			LastCurrent = g_PresentBatteryInfo.PresentCurrent;
			// g_PresentBatteryInfo.Cstate = chargingend;
		}
	}
	/*把充电时间独立出来，避免充电后在检测*/
	if (g_PresentBatteryInfo.ChargingTimes > g_Charge.TargetTime)
	{
		g_PresentBatteryInfo.Cstate = chargingend;
	}
}

/*
 *  充电器处理事件处理
 */
void Charger_Handle(void)
{
	/*获取一下充电器当前状态*/
	getMachineState();
	/*根据当前充电器状态，做出调整*/
	switch (g_PresentBatteryInfo.Mstate)
	{
	case charging:
	{
		ChargerEvent();
		g_PresentBatteryInfo.ChargerStatus[1] = 1 << 0;
	}
	break;
	case standby:
	{
		StandbyEvent();
		/*充电器状态待机*/
		g_PresentBatteryInfo.ChargerStatus[1] = 1 << 1;
	}
	break;
	default:
		break;
	}
	if (g_PresentBatteryInfo.ChargingFaultFlag == true)
	{
		/*充电状态故障*/
		g_PresentBatteryInfo.ChargingStatus[1] = 1 << 0;
	}
}

/**
 * Get x sign bit only for little-endian
 * if x >= 0 then  1
 * if x <  0 then -1
 */
#define MathUtils_SignBit(x) \
	(((signed char *)&x)[sizeof(x) - 1] >> 7 | 1)

/*
 * 获取电压调整补偿
 */
void Get_VoltageMicroCompensate(const float UserCurrent)
{
	volatile float Difference = UserCurrent - g_PresentBatteryInfo.PresentCurrent;
	volatile float Absolute = fabs(Difference);
	float CurrentGap = 0;
	float Coefficient = UserCurrent / CURRENT_RATIO;

	/*目标电流不可能是负数*/
	if (MathUtils_SignBit(UserCurrent) == -1)
	{
		return;
	}
	if (Absolute && (Absolute > Coefficient))
	{
		/*获取符号位*/
		CurrentGap = MathUtils_SignBit(Difference) * MIN_OFFSET_CURRENT;
	}
	else
	/*允许误差或者已经调节在点上*/
	{
		CurrentGap = 0;
	}
	g_PresentBatteryInfo.PresentChargingVoltage += CurrentGap;

#if (USE_VAGUE_PID)
	/*Obtain the PID controller vector according to the parameters*/
	struct PID **pid_vector = fuzzy_pid_vector_init(fuzzy_pid_params, 2.0f, 4, 1, 0, mf_params, rule_base, DOF);
	bool direct[DOF] = {true, false, false, false, true, true};
	int out = fuzzy_pid_motor_pwd_output(g_PresentBatteryInfo.PresentVoltage, g_PresentBatteryInfo.PresentChargingVoltage, direct[5], pid_vector[5]);
	g_PresentBatteryInfo.PresentVoltage += (float)(out - middle_pwm_output) / (float)middle_pwm_output * (float)max_error * 0.1f;
	g_PresentBatteryInfo.PresentChargingVoltage = g_PresentBatteryInfo.PresentVoltage;
	delete_pid_vector(pid_vector, DOF);
#endif
}

/*
 *充电模式调整
 */
void ChargingProcess()
{
	/*临时变量存储快充电流*/
	float PresentChargingCurrent = 0;
	/*底板输出错误计数*/
	// static uint8_t counts = 0;
#if (USE_ORDINARY_PID)
	/*临时存储PID补偿值*/
	float TempPidValue = 0;
#endif
	/*读出快充线圈状态值*/
	mdBit QuickCharging_Bit = false;

	switch (g_PresentBatteryInfo.Cstate)
	{
	/*涓流充电模式*/
	case trickle:
	{
		/*获取当前电流与期望电流之间的补偿值，调整当前电压*/
		Get_VoltageMicroCompensate(g_PresentBatteryInfo.User_TrickleCurrent);
		g_PresentBatteryInfo.ChargingStatus[1] = 1 << 1;
	}
	break;
	/*恒流充电模式*/
	case constant_current:
	{ /*只有在恒流模式下才能开启快充功能*/
		mdRTU_ReadCoil(Slave1_Object, MDREGISTER_FASTCHARGING_ADDR, QuickCharging_Bit);
		if (QuickCharging_Bit == true) /*一键快充*/
		{
			g_PresentBatteryInfo.QuickChargingFlag = true;
			/*增加一个变量保证原本的电流值不发生恶意篡改*/
			PresentChargingCurrent = g_PresentBatteryInfo.User_ConstantCurrent_Current;
			PresentChargingCurrent *= FAST_CHARGE_CURRENT_COEFFICENT;
			Get_VoltageMicroCompensate(PresentChargingCurrent);
		}
		else
		{
			g_PresentBatteryInfo.QuickChargingFlag = false;
			Get_VoltageMicroCompensate(g_PresentBatteryInfo.User_ConstantCurrent_Current);
		}
		g_PresentBatteryInfo.ChargingStatus[1] = 1 << 2;
	}
	break;
	/*恒压充电模式*/
	case constant_voltage:
	{
		/*恒压模式电压即为恒流模式调整得到的充电电压*/
		g_PresentBatteryInfo.ChargingStatus[1] = 1 << 3;
	}
	break;
	/*充电结束*/
	case chargingend:
	{
		g_PresentBatteryInfo.ChargingEndFlag = true;	 //充电结束标志
		g_PresentBatteryInfo.PresentChargingVoltage = 0; //充电电压输出为0V
		g_PresentBatteryInfo.ChargingStatus[1] = 1 << 4;
	}
	break;
		/*充电错误*/
		// case error:
		// {
		// 	g_PresentBatteryInfo.ChargingStatus[1] = 1 << 0;
		// }
		// break;

	default:
		break;
	}
	/*加入PID调整算法，控制输出电压*/
	/*存在不稳定性，断开几次后会出现负值*/
#if (USE_ORDINARY_PID)
	TempPidValue = PID_realize(g_PresentBatteryInfo.PresentChargingVoltage, g_PresentBatteryInfo.PresentVoltage);
	/*PID失控，排除错误情况*/
	if ((TempPidValue < 0.5F) && (TempPidValue > -0.5F))
	{ /*原因g_PresentBatteryInfo.PresentVoltage << g_PresentBatteryInfo.PresentChargingVoltage*/
		g_PresentBatteryInfo.PresentChargingVoltage += TempPidValue;
	}
#endif
	/*输出当前电压值*/
	Set_Voltage(g_PresentBatteryInfo.PresentChargingVoltage);
}

/*
 * 充电状态事件处理
 */
void ChargerEvent(void)
{
	// if (!g_PresentBatteryInfo.CloseAcInputFlag)
	{
		/*打开底板电源供电开关*/
		HAL_GPIO_WritePin(POWER_ON_GPIO_Port, POWER_ON_Pin, GPIO_PIN_SET);
		// /*不能立即打开充电开关*/
		if (!g_Timer.Timer8Flag)
		{
			return;
		}
		/*底板电源开始输出和打开充电开关*/
		HAL_GPIO_WritePin(CHARGING_READY_GPIO_Port, CHARGING_READY_Pin, GPIO_PIN_SET);
		/*打开充电回路*/
		HAL_GPIO_WritePin(CHARGING_GO_GPIO_Port, CHARGING_GO_Pin, GPIO_PIN_SET);
		/*风扇开关打开*/
		HAL_GPIO_WritePin(FANS_POWER_GPIO_Port, FANS_POWER_Pin, GPIO_PIN_SET);
		/*获取充电状态*/
		getChargingState();
	}
	/*断路检测(不是充电结束)*/
	// if (g_PresentBatteryInfo.Cstate != chargingend)
	// {
	// 	Check_ChargingDisconnection();
	// }
	/*充电过程调整*/
	ChargingProcess();
	/*断路检测(不是充电结束)*/
	// if (g_PresentBatteryInfo.Cstate != chargingend)
	// {
	/*不能立即打开充电开关*/
	Check_ChargingDisconnection();
	// }
}

/*
 * 其他事件处理
 */
void OtherEvent(void)
{
	static uint32_t count1, count2;
	/*如果充电结束之后，还没有拔下，只有等到电压值降到一定程度才可以继续充电*/
	if (g_PresentBatteryInfo.ChargingEndFlag == true)
	{ /*复位充电输出电压*/
		g_PresentBatteryInfo.PresentChargingVoltage = 0U;
		if (g_PresentBatteryInfo.ChargingTimes < g_Charge.TargetTime)
		{ /*电池电压低于User_SecondBootVoltage后并且小于充电时间才开始充电*/
			if (g_PresentBatteryInfo.PresentVoltage < g_PresentBatteryInfo.User_SecondBootVoltage)
			{
				/*清除充电结束标志*/
				g_PresentBatteryInfo.ChargingEndFlag = false;
				/*重新获取充电电压*/
				// Reset_ChargingVoltage();
			}
		}
	}
	else /*充电结束正常显示充电结束*/
	{	 /*待机下清除充电状态*/
		g_PresentBatteryInfo.ChargingStatus[1] = 0 << 0;
	}
	/*达到断路次数后，进入待机模式*/
	/*如果当前断路是由于零电流造成，则可能是电池内部电压不稳定，此时不能清空计时和电量*/
	/*此时可能是电流为0，电压为0或者电流不为零，电压为0（正常不会出现），判别为电池已经被断开连接充电器物理线路*/
	if (g_PresentBatteryInfo.ZeroCurrentFlag == true)
	{
		/*考虑电路中电容特性：8S后再检测*/
		if (count1++ == __RECOVERYCOUNTS())
		{
			count1 = 0;
			g_PresentBatteryInfo.ZeroCurrentFlag = false;
		}
		count2++;
	}
	/*三次零电流开路直接上升为硬件故障*/
	if (count2 == MAX_DEFAULT_COUNTS)
	{
		count2 = 0;
		g_PresentBatteryInfo.ChargingFaultFlag = true;
	}
	/*充电故障导致的待机，设置输出电压为0v*/
	// if (g_PresentBatteryInfo.ChargingFaultFlag == true)
	// {
	// 	g_PresentBatteryInfo.PresentChargingVoltage = 0U;
	// }
}

/*
 * 待机状态事件处理
 */
void StandbyEvent(void)
{
	/*充电结束后以及由断路造成的其他信号处理*/
	OtherEvent();
	/*复位充电输出电压*/
	g_PresentBatteryInfo.PresentChargingVoltage = 0U;
	/*零电压开路：电压低于2V,则认为是对电池插拔*/
	if (g_PresentBatteryInfo.PresentVoltage < g_PresentBatteryInfo.User_TrickleTargetVlotage)
	{
		/*首次接入电瓶后跟新当前电池充电参数，电瓶拔掉后视为当前电瓶本次充电周期结束*/
		// g_FirstFlag = true;
		/*复位充电故障标志*/
		g_PresentBatteryInfo.ChargingFaultFlag = false;
		/*复位充电电流*/
		g_PresentBatteryInfo.PresentCurrent = 0U;
		/*复位充电输出电压*/
		// g_PresentBatteryInfo.PresentChargingVoltage = 0U;
		/*同时清除充电时间表和电量计*/
		t_ChargingTimes = 0U;
		g_ChargingQuantity = 0U;
		/*清除充电结束标志*/
		g_PresentBatteryInfo.ChargingEndFlag = false;
		/*风扇开关关闭*/
		HAL_GPIO_WritePin(FANS_POWER_GPIO_Port, FANS_POWER_Pin, GPIO_PIN_RESET);
		// /*关闭底板电源供电开关*/
		// HAL_GPIO_WritePin(CHIP_POWER_GPIO_Port, CHIP_POWER_Pin, GPIO_PIN_RESET);
	}
	/*待机但是没有拔掉设备，让PresentChargingVoltage不复位*/
	/*电压输出为当前PresentChargingVoltage*/
	Set_Voltage(g_PresentBatteryInfo.PresentChargingVoltage);
	/*充电结束进入待机模式时，关闭总电源开关和充电输出开关*/
	/*断开充电回路*/
	HAL_GPIO_WritePin(CHARGING_GO_GPIO_Port, CHARGING_GO_Pin, GPIO_PIN_RESET);
	/*关闭充电开关*/
	HAL_GPIO_WritePin(CHARGING_READY_GPIO_Port, CHARGING_READY_Pin, GPIO_PIN_RESET);
	/*关闭底板电源*/
	HAL_GPIO_WritePin(POWER_ON_GPIO_Port, POWER_ON_Pin, GPIO_PIN_RESET);
	/*待机下清除充电状态*/
	// g_PresentBatteryInfo.ChargingStatus[1] = 0 << 0;
	/*清除定时标志*/
	g_Timer.Timer8Flag = false;
}

/*
 * 充电时间电量计时器
 */
void ChargeTimer(void)
{
	/*充电计时开始并且充电没有结束*/
	if ((g_PresentBatteryInfo.ChargingTimingFlag == true) && (g_PresentBatteryInfo.ChargingEndFlag == false))
	{ /*统计充电时长1s/次*/
		t_ChargingTimes += 1U;
		/*统计充电电量*/
		g_ChargingQuantity += g_PresentBatteryInfo.PresentCurrent;
	}
}

/*
 * 上电读取FLASH参数
 */
void FlashReadInit(void)
{ /*错误次数计数器*/
	uint32_t Error_Counter = 0;
	float Temp_Voltage = 2.0F;

	Error_Counter = FLASH_Read(CHARGE_SAVE_ADDRESS, &g_Charge, sizeof(g_Charge));
	// Usart1_Printf("flag is  0x%X, minutes is %d \r\n", g_Charge.IsSavedFlag, g_Charge.TargetTime);
	shellPrint(&shell, "flag is  0x%X, minutes is %d \r\n", g_Charge.IsSavedFlag, g_Charge.TargetTime);
	/*以下参数以24单元，5Ah容量电池计算：！= 1，没有写过flash，=1写过flash*/
	if (g_Charge.IsSavedFlag != true)
	{
		g_Charge.TrickleTargetVlotage = SINGLEUNIT_MINVOLTAGE * 24U * 10U;
		g_Charge.TrickleCurrent = SINGLEUNIT_STDCURRENT * 5U * 10U;
		g_Charge.ConstantCurrentTargetVoltage = SINGLEUNIT_MAXVOLTAGE * 24U * 10U;
		g_Charge.ConstantCurrent_Current = SINGLEUNIT_MAXCURRENT * 5U * 10U;
		g_Charge.ConstantVoltage_Voltage = SINGLEUNIT_STDVOLTAGE * 24U * 10U;
		g_Charge.ConstantVoltageTargetCurrent = SINGLEUNIT_MINCURRENT * 5U * 10U;
		g_Charge.BatteryCapacity = 15U;
		g_Charge.TargetTime = 480U;
		g_Charge.Compensation = 500U;
		g_Charge.UnitElements = 0U;
		g_Charge.SecondBootVoltage = 510U;
	}
	/*清除写Flash操作*/
	g_Charge.IsSavedFlag = false;

	/*读取充电校准系数*/
	Error_Counter = FLASH_Read(CLIBRATION_SAVE_ADDR, &Dac, sizeof(Dac));
	// Usart1_Printf("error is 0x%X \r\n", Error_Counter);
	shellPrint(&shell, "error is 0x%X \r\n", Error_Counter);

	Error_Counter = 0U;
	/*充电系数没有校准*/
	if (Dac.Finish_Flag)
	{
		/*打开底板电源供电开关*/
		HAL_GPIO_WritePin(POWER_ON_GPIO_Port, POWER_ON_Pin, GPIO_PIN_SET);
		/*底板电源开始输出和打开充电开关*/
		HAL_GPIO_WritePin(CHARGING_READY_GPIO_Port, CHARGING_READY_Pin, GPIO_PIN_SET);
		/*打开充电回路*/
		HAL_GPIO_WritePin(CHARGING_GO_GPIO_Port, CHARGING_GO_Pin, GPIO_PIN_SET);
		// HAL_GPIO_TogglePin(CHARGING_GO_GPIO_Port, CHARGING_GO_Pin);
		/*风扇开关打开*/
		HAL_GPIO_WritePin(FANS_POWER_GPIO_Port, FANS_POWER_Pin, GPIO_PIN_SET);
		/*启动校准,并等待校准成功*/
		while (!Dac_Clibration())
		{
			/*三次校准不成功，报故障*/
			if (++Error_Counter > 3U)
			{
				Error_Counter = 0;
				/*故障灯闪烁*/
				while (1)
				{
					/*充电状态故障*/
					g_PresentBatteryInfo.ChargingStatus[1] ^= 1;
					DWIN_WRITE(CHARGE_STATE_ADDR, g_PresentBatteryInfo.ChargingStatus,
							   sizeof(g_PresentBatteryInfo.ChargingStatus));
					HAL_Delay(1000);
					// osDelay(1000);
				}
			}
		}
		/*检测输出点电压是否对应:此处Dac.Finish_Flag的初始值为0xFFFF*/
		while (Temp_Voltage < HARDWARE_DAVOLTAGE)
		{
			Set_Voltage(Temp_Voltage);
			Temp_Voltage += 5.0F;
			HAL_Delay(5000);
		}
		/*清除校准完成标志*/
		// Dac.Finish_Flag = true;
	}
	/*拷贝充电系数到指定区域*/
	memcpy((uint8_t *)&DAC_Param_Interval, (uint8_t *)&Dac.Para_Arry, sizeof(DAC_Param_Interval));

	for (uint16_t i = 0; i < DAC_NUMS; i++)
	{
		for (uint16_t j = 0; j < 2U; j++)
		{
			shellPrint(&shell, "DAC_Param_Interval[%d][%d] = %f\t", i, j, DAC_Param_Interval[i][j]);
		}
		shellPrint(&shell, "\r\n");
	}
}

/*
 *   修改时间
 */
void Setting_RealTime(uint8_t *dat, uint8_t length)
{
	if (dat[1])
	{
		uint32_t stramp = time2Stamp(SetDate, SetTime);
		SetRtcCount(stramp);
		// HAL_RTC_SetTime(&hrtc, &SetTime, RTC_FORMAT_BIN);
		// HAL_RTC_SetDate(&hrtc, &SetDate, RTC_FORMAT_BIN);
	}
}

/*
 *  输入确认
 */
void InputConfirm(uint8_t *dat, uint8_t length)
{
	uint8_t error[2][2] = {0x00, 0x00, 0x00, 0x00};
	uint8_t current_user = 0;

#define RESET_USER_INFO
	{
		g_Timer1.Timer8Count = T_180S;
	}
	for (current_user = 0; current_user < USER_NUMBERS; current_user++)
	{
		if (InputUserName == User_Infomation[current_user][0])
		{
			/*清除账户错误图标*/
			error[0][1] = 0x00;
			DWIN_WRITE(ACCOUNNT_NUMBER_ERROR_ADDR, &error[0][0], sizeof(uint16_t));
			// HAL_Delay(2);
			osDelay(1);
			if (InputPassword == User_Infomation[current_user][1])
			{
				/*清除密码错误图标*/
				error[1][1] = 0x00;
				DWIN_WRITE(PASSWORD_ERROR_ADDR, &error[1][0], sizeof(uint16_t));
				// HAL_Delay(2);
				osDelay(1);
				/*系统用户*/
				if (InputUserName == 2021U)
				{
					DWIN_PageChange(0x11);
				}
				else /*普通用户*/
				{
					DWIN_PageChange(0x05);
				}
				return;
			}
			else /*密码错误*/
			{
				error[1][1] = 0x01;
				/*显示密码错误图标*/
				DWIN_WRITE(PASSWORD_ERROR_ADDR, &error[1][0], sizeof(uint16_t));
				return;
			}
		}
		else
		{
			continue;
		}
	}
	/*遍历到结尾，仍然没有找到对应用户名*/
	if (current_user == USER_NUMBERS)
	{
		/*用户名错误*/
		error[0][1] = 0x01;
		/*显示用户账号错误图标*/
		DWIN_WRITE(ACCOUNNT_NUMBER_ERROR_ADDR, &error[0][0], sizeof(uint16_t));
	}
}

/*
 *  输入注销
 */
void InputCancel(uint8_t *dat, uint8_t length)
{
	uint8_t none[2] = {0x00, 0x00};
	/*清除账户错误图标*/
	DWIN_WRITE(ACCOUNNT_NUMBER_ERROR_ADDR, none, sizeof(uint16_t));
	/*清除密码错误图标*/
	DWIN_WRITE(PASSWORD_ERROR_ADDR, none, sizeof(uint16_t));
}

/*
 *  用户账号检查
 */
void CheckUserName(uint8_t *dat, uint8_t length)
{
	InputUserName = ((uint16_t)dat[0]) << 8 | dat[1];
}

/*
 *  密码检查
 */
void CheckPassword(uint8_t *dat, uint8_t length)
{
	InputPassword = ((uint16_t)dat[0]) << 8 | dat[1];
}

/*
 *   恢复出厂设置
 */
void RestoreFactory(uint8_t *dat, uint8_t length)
{
}

/*
 *   修改充电参数
 */
void ChargeTargetTime(uint8_t *dat, uint8_t length)
{
	/*不论是进那个映射函数，都必须带上此标志，掉电后参数恢复标志*/
	g_Charge.IsSavedFlag = true;
	g_Charge.TargetTime = ((uint16_t)dat[0]) << 8 | dat[1];
	/*把目标充电时间写入保持寄存器对应地址单元*/
	mdRTU_WriteHoldReg(Slave1_Object, MDREGISTER_TAGATTIMES_ADDR, g_Charge.TargetTime);
	//    HAL_UART_Transmit_DMA(&huart1, dat, length);	//测试
}

void TrickleChargeCurrent(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.TrickleCurrent = ((uint16_t)dat[0]) << 8 | dat[1];

	g_PresentBatteryInfo.User_TrickleCurrent = g_Charge.TrickleCurrent / 10.0F;
}

void TrickleChargeTargetVoltage(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.TrickleTargetVlotage = ((uint16_t)dat[0]) << 8 | dat[1];

	g_PresentBatteryInfo.User_TrickleTargetVlotage = g_Charge.TrickleTargetVlotage / 10.0F;
}

void ConstantCurrent_Current(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.ConstantCurrent_Current = ((uint16_t)dat[0]) << 8 | dat[1];

	g_PresentBatteryInfo.User_ConstantCurrent_Current = g_Charge.ConstantCurrent_Current / 10.0F;
}

void ConstantCurrentTargetVoltage(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.ConstantCurrentTargetVoltage = ((uint16_t)dat[0]) << 8 | dat[1];

	g_PresentBatteryInfo.User_ConstantCurrentTargetVoltage = g_Charge.ConstantCurrentTargetVoltage / 10.0F;
}

void ConstantVoltage_Voltage(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.ConstantVoltage_Voltage = ((uint16_t)dat[0]) << 8 | dat[1];

	g_PresentBatteryInfo.User_ConstantVoltage_Voltage = g_Charge.ConstantVoltage_Voltage / 10.0F;
}

void ConstantVoltageTargetCurrent(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.ConstantVoltageTargetCurrent = ((uint16_t)dat[0]) << 8 | dat[1];

	g_PresentBatteryInfo.User_ConstantVoltageTargetCurrent = g_Charge.ConstantVoltageTargetCurrent / 10.0F;
}

void SetBatteryCapacity(uint8_t *dat, uint8_t length)
{
	g_Charge.BatteryCapacity = ((uint16_t)dat[0]) << 8 | dat[1];

	/*电池容量不超过500Ah*/
	if (g_Charge.BatteryCapacity <= MAX_BATTERY_CAPCITY)
	{
		g_Charge.IsSavedFlag = true;
		/*一旦更新过电池容量，立马更新充电电流信息*/
		Set_BaterrryCurrentInfo();
		/*更新屏幕信息*/
		Report_BackChargingData();
	}
}

/*设置起充电压步进补偿值*/
void ChargeCompensation(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.Compensation = ((uint16_t)dat[0]) << 8 | dat[1];
}

/*设置电瓶单元个数*/
void Set_UnitElements(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.UnitElements = ((uint16_t)dat[0]) << 8 | dat[1];
	/*校准当前电池的单元个数信息*/
	Set_BaterrryVoltageInfo(0U);
	/*更新充电器后台充电参数*/
	Report_BackChargingData();
}

/*设置二次起充电压*/
void Set_SecondBootvoltage(uint8_t *dat, uint8_t length)
{
	g_Charge.IsSavedFlag = true;
	g_Charge.SecondBootVoltage = ((uint16_t)dat[0]) << 8 | dat[1];

	g_PresentBatteryInfo.User_SecondBootVoltage = g_Charge.SecondBootVoltage / 10.0F;
}

void Set_Year(uint8_t *dat, uint8_t length)
{
	SetDate.Year = dat[1];
}

void Set_Month(uint8_t *dat, uint8_t length)
{
	SetDate.Month = dat[1];
}

void Set_Date(uint8_t *dat, uint8_t length)
{
	/*日期默认设置比屏幕上多一天才对*/
	SetDate.Date = dat[1]; //+ 1U
}

void Set_Hour(uint8_t *dat, uint8_t length)
{
	SetTime.Hours = dat[1];
}

void Set_Min(uint8_t *dat, uint8_t length)
{
	SetTime.Minutes = dat[1];
}

void Set_Sec(uint8_t *dat, uint8_t length)
{
	SetTime.Seconds = dat[1];
}
