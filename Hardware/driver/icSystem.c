
#include "hw.h"
extern uint16_t QUEUE_Num;
extern uint8_t g_nState;

extern uint8_t g_bLowPower,g_bServerLowPwr;
extern uint8_t g_bHaveKeyAction;
//extern uint32_t g_cntStartTime;
uint8_t g_bFirst = 1;
uint8_t g_bStartFlag = 0;

/*
 * GPIO����
 * PA0/SPI_MOSI0/MCLK
 * PA1/SPI_SCLK/I2C_SCL
 * PA2/SPI_SSB0
 * PA3/SPI_MISO0/I2C_SDA
 * PA4/I2S_FS
 * PA5/I2S_BCLK
 * PA6/I2S_SDI
 * PA7/I2S_SDO
 * PA8/UART_TX/I2S_FS
 * PA9/UART_RX/I2S_BCLK
 * PA10/UART_RTSn/I2S_SDI/I2C_SDA
 * PA11/UART_CTSn/I2S_SDO/I2C_SCL
 * PA12/I2S_FS/SPKP/PWM0
 * PA13/I2S_BCLK/SPKM/PWM1
 * PA14/TM0/SDCLK/SDCLKn
 * PA15/TM1/SDIN
 *
 * PB0/SPI_SSB0/CMP0/SPI_SSB1
 * PB1/SPI_SSB1/CMP1/MCLK
 * PB2/SPI_SCLK/CMP2/I2C_SCL
 * PB3/SPI_MISO0/CMP3/I2C_SDA
 * PB4/SPI_MOSI0/CMP4/PWM0B
 * PB5/SPI_MISO1/CMP5/PWM1B
 * PB6/SPI_MOSI1/CMP6/I2S_SDI
 * PB7/CMP7/I2S_SDO
 * WAKEUP ������
 *
 */

/*----------------------------------------------------------------------------------------
������: SystemClockInit
����:
		None
����ֵ:
		None
����:
		��ʼ��ϵͳʱ�ӣ���LDO
----------------------------------------------------------------------------------------*/
static void SystemClockInit(void)
{
    /* Unlock the protected registers */	
	UNLOCKREG();

	/* HCLK clock source. */
	//DrvSYS_SetHCLKSource(0);
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->PWRCON.OSC10K_EN = 1;
//	SYSCLK->PWRCON.XTL32K_EN = 1;
	SYSCLK->CLKSEL0.STCLK_S = 3; /* Use internal HCLK */

	SYSCLK->APBCLK.ANA_EN = 1;
	ANA->LDOSET = 3;
	ANA->LDOPD.PD = 0;
	 		
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */ 
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */	
	DrvSYS_SetIPClockSource(E_SYS_TMR0_CLKSRC, 4);
	DrvSYS_SetIPClockSource(E_SYS_TMR1_CLKSRC, 4);

	LOCKREG();

	/* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */
	//DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); 
}
/*----------------------------------------------------------------------------------------
������: IsStart
����:
		None
����ֵ:
		None
����:
		�ж��Ƿ����㿪���������������ֱ�ӿ��������������ж���û��USB���о���ʾ���״̬���޹ػ�
----------------------------------------------------------------------------------------*/
void IsStart(void)
{
	KEYMSG msg;
	uint8_t bHaveUsb = 0;

	while(1){
		g_bHaveKeyAction = 1;
		if(KEY_MsgGet(&msg)){			
			if(msg.Key_MsgValue == KEY_ON_OFF){
				if(msg.Key_MsgType == KEY_TYPE_LP){
					g_bStartFlag = 1;
					GPIOA->DOUT |= (1 << 14);
					PlayBegin(PEN_START_AUDIO);
				}else if(msg.Key_MsgType == KEY_TYPE_UP && g_bStartFlag == 1){
					g_nState = PLAY_STATE;
					QUEUE_Num = 0;
					AdapterDisconnect();
					break;
				}
			}
		}
		if(g_bStartFlag){
			AdapterDisconnect();
			PlayWork(0,0,0);			
		}else{			
			g_bStartFlag = 0;
			if(DrvGPIO_GetBit(GPA,15))		//remove vbus
				bHaveUsb = 1;
			else{
				bHaveUsb = 0;
				DrvGPIO_ClrBit(GPA,14);
			}
	
			if(bHaveUsb == 1)
			{
				if(DrvGPIO_GetBit(GPA,7))		//charging
					LedFlashForChargeover();
				else
					LedFlashForCharge();	
			}else{
				AdapterDisconnect();
			}
		}
	}
	CameraStart();
	g_bStartFlag = 0;
}
/*----------------------------------------------------------------------------------------
������: HW_Init
����:
		None
����ֵ:
		None
����:
		����ϵͳ�ĳ�ʼ������
----------------------------------------------------------------------------------------*/
void HW_Init(void)
{
	SystemClockInit();
	DrvGPIO_Open(GPA,14,IO_OUTPUT);
	GPIOA->DOUT |= (1 << 14);
	memset((int32_t *)0x20000000,0,0x2300);
	icTimerInit();
	TimerInit(); 
	KEY_Init();
	SysTimerDelay(100000);
	DrvPDMA_Init();
	Uart_Init();
	SPI_Init();
	LedInit();
	InitialFMC();
	TouchSenseInit();
	ClrESRMemory();
	ADC_Init();
	ADC_Term();
//	{
//		uint8_t i;
//		for(i=0; i<255; i++){
//			LOG(("%d ",(rand()%100)));
//		}
//	}
//	{
//		int test;
//		for(test = 0x2000273C; test < 0x20002e00; test++)
//		{
//			*(int8_t *)test = 0x55;
//		}
//	}
	if(g_bFirst){
		IsStart();
		g_bFirst = 0;
	}
}
/*----------------------------------------------------------------------------------------
������: HW_Term
����:
		None
����ֵ:
		None
����:
		����ϵͳ����ֹ���򣬷�������ʹ��
----------------------------------------------------------------------------------------*/
void HW_Term(void)
{
	/* �ر�GPIO�ж� */
	SPI_Term();
	LedUninit();
	DrvDPWM_Close();
	DrvPDMA_Close();
	DrvTIMER_Close(TMR1);
}
/*----------------------------------------------------------------------------------------
������: gpab_irq
����:
		None
����ֵ:
		None
����:
		���ӵ���������������
----------------------------------------------------------------------------------------*/
GPIO_GPAB_CALLBACK gpab_irq(uint32_t p, uint32_t q)
{
	return 0;
}
/*----------------------------------------------------------------------------------------
������: SetWakeupPin
����:
		None
����ֵ:
		None
����:
		�������߻������ţ����涨GPB2�½��ػ����ж�
----------------------------------------------------------------------------------------*/
void SetWakeupPin(void)
{
	DrvGPIO_SetIntCallback(gpab_irq(0, 4));
//	DrvGPIO_EnableInt(GPA, 10, IO_FALLING, MODE_EDGE);	
//	GPIOA->IEN |= (1 << (10 + 16));//PA10 ������
	GPIOA->IEN |= (1 << 10);//PA10�½����ж�
	GPIOB->IEN |= (1 << 16); //PB0 ���������ж�
	GPIOB->IEN |= (1 << 17); //PB1 �������ж�
	GPIOB->IEN |= (1 << 2);


	NVIC_SetPriority (GPAB_IRQn, (1<<__NVIC_PRIO_BITS) - 2);
	NVIC_EnableIRQ(GPAB_IRQn);
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
}
/*----------------------------------------------------------------------------------------
������: halt
����:
		None
����ֵ:
		None
����:
		�������ߵĳ���
----------------------------------------------------------------------------------------*/
void halt(void) //����stopģʽ ������< 10uA
{			
//	SYS->GPB_ALT.GPB0 = 0;
	SYS->GPA_ALT.GPA4 = 0;
	SYS->GPA_ALT.GPA5 = 0;
	SYS->GPA_ALT.GPA6 = 0;
	SYS->GPA_ALT.GPA10 = 0;
	SYS->GPA_ALT.GPA11 = 0;
	SYS->GPA_ALT.GPA12 = 0;
	SYS->GPA_ALT.GPA13 = 0;
	SYS->GPA_ALT.GPA14 = 0;

	DrvGPIO_Open(GPA, 4, IO_OUTPUT);
	DrvGPIO_Open(GPA, 5, IO_OUTPUT);
	DrvGPIO_Open(GPA, 6, IO_OUTPUT);
	DrvGPIO_Open(GPA, 10, IO_OUTPUT);
	DrvGPIO_Open(GPA, 11, IO_OUTPUT);
	DrvGPIO_Open(GPA, 12, IO_OUTPUT);
	DrvGPIO_Open(GPA, 13, IO_OUTPUT);
	DrvGPIO_Open(GPA, 14, IO_OUTPUT);

	DrvGPIO_ClrBit(GPA,4);
	DrvGPIO_ClrBit(GPA,5);
	DrvGPIO_ClrBit(GPA,6);
	DrvGPIO_ClrBit(GPA,10);
	DrvGPIO_ClrBit(GPA,11);
	DrvGPIO_ClrBit(GPA,12);
	DrvGPIO_ClrBit(GPA,13);
	DrvGPIO_ClrBit(GPA,14);

	SYS->GPA_ALT.GPA8 = 0;
	SYS->GPA_ALT.GPA9 = 0;
	GPIOA->PMD.PMD8 = IO_INPUT;
	GPIOA->PMD.PMD9 = IO_INPUT;

	UNLOCKREG();
	ANA->LDOPD.PD = 1;
	SYSCLK->APBCLK.ANA_EN = 0;
	LOCKREG();

	SYS->WAKECR.WAKE_TRI = 0; //ʹ��wakeup�����ڲ�����
	
	ANA->ISRC.EN = 0; //����������������������Ϊ16uA
	ACMP->CMPCR[0].CMPEN = 0;
	DrvADC_AnaClose(); //LDO

	SetWakeupPin();		
	UNLOCKREG();
	SYSCLK->PWRCON.STOP = 1; //��һλ���1,Stopģʽ�������ܽ���10uA
	SYSCLK->PWRCON.STANDBY_PD = 0;
	SYSCLK->PWRCON.DEEP_PD = 0;
	SCB->SCR = 1 << 2; //����sleepΪdeep sleep
	LOCKREG();	
//RTCʱ�ӿ���ʱ������״̬��Ҫʹ��RTCģ���ʱ��
//ʹ�ô������ѵ�ʱ��ҲҪʹ��RTC
	*(uint32_t*)(&SYSCLK->CLKSLEEP) = 1;	
	*(uint32_t*)(&SYSCLK->APBCLK) = 1;
//	sleep = 1;
	__wfi();
}
/*----------------------------------------------------------------------------------------
������: HW_EnterPowerDown
����:
		None
����ֵ:
		None
����:
		�ж��Ƿ�������ߣ�����������ʾ���״̬
----------------------------------------------------------------------------------------*/
void HW_EnterPowerDown(PEN_STATE PenState)
{
	// Uninitialize all IPs
	KEYMSG msg;
	int16_t count = 0,temp = 0,bHaveUsb = 0;
	if(DrvGPIO_GetBit(GPA,15))
		bHaveUsb = 1;		//��USB
	else bHaveUsb = 0;
	HW_Term();
HALT:
	if(!bHaveUsb){
//		NVIC_SystemReset();
		halt();
		count = 0;
		while(DrvGPIO_GetBit(GPB,0))
		{
			SysTimerDelay(100000);
			count++;
			if(count > 13)
			{
				goto WAKE_UP;
			}
		}
		goto HALT;
	}
WAKE_UP:	
	// Re-initialize
	QUEUE_Num = 0;
	g_nState = PLAY_STATE;
	HW_Init();
	if(!bHaveUsb)	temp = KEY_TYPE_DOWN;
	else		temp = KEY_TYPE_LP;

	while(1){
		g_bHaveKeyAction = 1;
		if(KEY_MsgGet(&msg)){
//		LOG("hello --- %d | %d\r\n",msg.Key_MsgValue,msg.Key_MsgType);
			if(msg.Key_MsgValue == KEY_ON_OFF){
				if(msg.Key_MsgType == temp){
					PlayBegin(PEN_START_AUDIO);
					g_bStartFlag = 1;
				}else if(msg.Key_MsgType == KEY_TYPE_UP && g_bStartFlag){		//long press KEY_ON_OFF	
					g_nState = PLAY_STATE;
					QUEUE_Num = 0;
					AdapterDisconnect();
					break;
				}
			}
		}
		if(g_bStartFlag){
			PlayWork(0,0,0);
			AdapterDisconnect();
		}else{
			if(DrvGPIO_GetBit(GPA,15))		//remove vbus
				bHaveUsb = 1;
			else{
				DrvGPIO_ClrBit(GPA,14);
				bHaveUsb = 0;
			}
	
			if(bHaveUsb)
			{
				if(DrvGPIO_GetBit(GPA,7))
					LedFlashForChargeover();
				else
					LedFlashForCharge();	
			}else{
				AdapterDisconnect();
			}
		}
	}
	g_bStartFlag = 0;
	CameraStart();	
}

int16_t adc_res;

typedef struct {
    BOOL                   bOpenFlag;
    DRVADC_ADC_CALLBACK    *g_ptADCCallBack;
    DRVADC_ADCMP0_CALLBACK *g_ptADCMP0CallBack;
    DRVADC_ADCMP1_CALLBACK *g_ptADCMP1CallBack;
    uint32_t g_pu32UserData[3];
} S_DRVADC_TABLE;

extern S_DRVADC_TABLE gsAdcTable;
/*----------------------------------------------------------------------------------------
������: ADCCBForPwrCheck
����:
		None
����ֵ:
		None
����:
		ADC���ڵ������Ļص�����
----------------------------------------------------------------------------------------*/
void ADCCBForPwrCheck(uint32_t parm)
{
	uint8_t i;
	for(i = 0; i < 4; i++)
	{
		adc_res = SDADC->ADCOUT;
	}	
}
/*----------------------------------------------------------------------------------------
������: ADCCBForPwrCheck
����:
		wSample��ADC���ɼ�������ֵ
����ֵ:
		dwAdcVal��ת��õ�ADC�ɼ�����ʵֵ
����:
		���ڵ������ֵ��ת��
������
		adc�����0V��Ӧ162  3.3V��ӦֵΪ-293 1.2VΪ0
		�Ѳ������ת����0��455֮���ֵ
		0��Ӧ0V 455��Ӧ3.3V => ת��[0:256]����		
----------------------------------------------------------------------------------------*/
int16_t AdcValueConvert(int16_t wSample)
{
	int32_t dwAdcVal;	
	dwAdcVal = wSample / 100;
	if(wSample > 0)
	{
		dwAdcVal = 163 - dwAdcVal;
		wSample = -wSample;
		if(wSample % 100 > 50)
		{
			dwAdcVal--;
		}			
	}
	else
	{
		dwAdcVal = -dwAdcVal;
		dwAdcVal += 163;
		if(wSample % 100 > 50)
		{
			dwAdcVal++;
		}			
	}
	//dwAdcVal:0��Ӧ0V 455��Ӧ3.3V
	//ת��[0:255]
//	dwAdcVal = 256 * dwAdcVal / 455;
//	if(dwAdcVal > 255)
//	{
//		dwAdcVal = 255; 
//	}		
	return dwAdcVal;	
}
/*----------------------------------------------------------------------------------------
������: AdcPwrCheck
����:
		None
����ֵ:
		wResult���ɼ�������ֵ
����:
		ADC���ڵ������ĳ�ʼ����ɼ���ֵ
----------------------------------------------------------------------------------------*/
uint32_t AdcPwrCheck(void)
{
	uint32_t i;
	uint16_t wResult;
	SYS->IPRSTC2.ADC_RST = 1;
	SYS->IPRSTC2.ADC_RST = 0;
	SYSCLK->APBCLK.ADC_EN = 1;

//ADC set
	SDADC->INT.IE = 0;		
	SDADC->CLK_DIV = 15;//49.152M / (15 + 1) = 3.072MHz	
	SDADC->DEC.OSR = 3;	//3.072MHz / 384 = 8KHz 0:64 1:128 2:192 3:384
	SDADC->DEC.GAIN = 0;
	SDADC->ADCPDMA.RxDmaEn = 0;
	SDADC->ADCMPR[0].CMPEN = 0;
	SDADC->ADCMPR[0].CMPIE = 0;
	gsAdcTable.g_ptADCCallBack = ADCCBForPwrCheck;
	SDADC->INT.FIFO_IE_LEV = 4;	
	SDADC->INT.IE = 1;
	NVIC_EnableIRQ(ADC_IRQn);
	
		
//channel mux
	GPIOB->PMD.PMD6 = 0;     /* configure GPB6 as input mode */
	//DrvADC_SetAMUX(AMUX_GPIO, eDRVADC_MUXP_NONE, eDRVADC_MUXN_GPB6_SEL);		 
	ANA->AMUX.MIC_SEL = 0;
	ANA->AMUX.TEMP_SEL = 0;
	ANA->AMUX.MUXP_SEL = 0;
	ANA->AMUX.MUXN_SEL = 1 << 6; //PB6
	ANA->AMUX.EN = 1;

//PGA set	
	ANA->PGA_GAIN.GAIN = 8;//-6db

	ANA->PGAEN.BOOSTGAIN = 0;
	ANA->PGAEN.PU_IPBOOST = 1;
	ANA->PGAEN.PU_PGA = 1;
	ANA->PGAEN.REF_SEL = 1; //ref voltage:1.2V 

	ANA->SIGCTRL.MUTE_IPBOOST = 0;
	ANA->SIGCTRL.MUTE_PGA = 0;
	ANA->SIGCTRL.PU_MOD = 1;
	ANA->SIGCTRL.PU_IBGEN = 1;
	ANA->SIGCTRL.PU_BUFADC = 1;
	ANA->SIGCTRL.PU_BUFPGA = 1;

//ALC
	ALC->ALC_CTRL.ALCSEL = 0;		
	SDADC->EN = 1;
	i = 20000;//�ȴ�һ�ᣬ��ADC��������
	while(i-- >0) ;
    SDADC->EN = 0;
	SYSCLK->APBCLK.ADC_EN = 0;
	gsAdcTable.g_ptADCCallBack = NULL;
	NVIC_DisableIRQ(ADC_IRQn);
	wResult = AdcValueConvert(adc_res);
//	LOG("sys volt: %d %d\n",adc_res, wResult);
	return wResult; 	
}

#define LOW_POWER_CHECK_ENABLE	(1)
#define POWER_CHECK_USE_ADC		(1)

//��[0:455]֮��ĵ�ѹֵת����mV
#define convert_2_mv(v) ((v - 37) * 100 / 13 + 108)

#define UP_RES		(4700)
#define DOWN_RES	(4700)
#define IN_RES		(107000)

#define DOWN_ALL_RES (DOWN_RES*IN_RES/(DOWN_RES + IN_RES))

#define CONVERT_IN_INRES(x)	(x*(UP_RES+DOWN_ALL_RES)/DOWN_ALL_RES + 100)		//100Ϊ��ʵ�ʲ���֮��Ĳ��
/*----------------------------------------------------------------------------------------
������: HW_PowerCheck
����:
		None
����ֵ:
		wResult���ɼ�������ֵ
����:
		ADC���ڵ������ĳ�ʼ����ɼ���ֵ
----------------------------------------------------------------------------------------*/
uint16_t HW_PowerCheck()
{
#if LOW_POWER_CHECK_ENABLE == 1	
#if POWER_CHECK_USE_ADC == 1 //ʹ��ADC�����ϵͳ��ѹ������������PB6
	uint16_t wSysVol;
	wSysVol = AdcPwrCheck();

//	LOG("wSysVol:%d",CONVERT_IN_INRES(convert_2_mv(wSysVol)));

	return CONVERT_IN_INRES(convert_2_mv(wSysVol));
	
		
#else //CHECK_USE_ADC ʹ���ڲ��ıȽ�������������⣬�Ƚϵ�ֵ��1.2V
    SYS->GPB_ALT.GPB6 = 2; //����PB6Ϊ�Ƚ��������
	SYSCLK->APBCLK.ANA_EN   = 1;/* Turn on analog peripheral clock */
	SYSCLK->APBCLK.ACMP_EN  = 1;
	//DrvACMP_Ctrl(CMP1, CMPCR_CN1_VBG, CMPCR_CMPIE_DIS, CMPCR_CMPEN_EN);
	ACMP->CMPCR[1].CMPCN = CMPCR_CN1_VBG;	//ʹ��VBG 1.2V��Ϊ�ο���ѹ
	ACMP->CMPCR[1].CMPIE = CMPCR_CMPIE_DIS;	//���ж�
	ACMP->CMPCR[1].CMPEN = CMPCR_CMPEN_EN;	//��ʼ�Ƚ�
	if(ACMP->CMPSR.CO1 == 0)
	{
		g_power_state = POWER_SHUTDOWN;	
		return 0;
	}
	else
	{
		g_power_state = POWER_SAFE;
		return 255;
	}
#endif //CHECK_USE_ADC
#else
	g_power_state = POWER_SAFE;
	return 255;
#endif
}
/*----------------------------------------------------------------------------------------
������: PowerCheck
����:
		None
����ֵ:
		2/1/0���͵�/���ص͵�/����
����:
		�ж�ϵͳ���ں���״̬
----------------------------------------------------------------------------------------*/
uint8_t PowerCheck(void)
{
	int16_t sys_vol = 0;
	sys_vol = HW_PowerCheck();
	if(sys_vol < SERVER_VOL){
		g_bLowPower = 0;
		g_bServerLowPwr = 1;
		LedFlashForLowPwr(LED_ON);
		return POWER_SHUTDOWN;
	}else if(sys_vol < SAFE_VOL){
		g_bLowPower = 1;
		g_bServerLowPwr = 0;
		return POWER_LOW;
	}else{
		g_bLowPower = 0;
		g_bServerLowPwr = 0;
		LedFlashForLowPwr(LED_OFF);
		return POWER_SAFE;
	}
}
