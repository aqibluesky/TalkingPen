#include "timer.h"

#define USE_TIMER	0
uint32_t g_cntSysTicks = 0; //û�ж���ʱ���м�ʱ��
uint32_t g_cntWaitForRecTicks = 0;		//��ס¼������10s֮�ڻ�ȡ������ȷ����ֵ����¼��
uint32_t g_cntPwrCheckTicks = 10;
uint8_t g_bRecordStart = 0;

uint8_t g_bHaveKeyAction = 0;
uint8_t g_bHavePlayAction = 0;
uint8_t g_bHaveEsrAction = 0;
uint8_t g_bHaveRecAction = 0;
uint8_t g_bHaveCamAction = 0;

uint8_t g_bContinueTick = 0;

uint8_t g_bLowPower = 0;
uint8_t g_bServerLowPwr = 0;
uint32_t g_cntLowPwrTicks = 0;

uint8_t g_bReminderAudio = 0;
uint8_t g_bShutDown = 0;
uint8_t g_bCheckPower = 0;
uint8_t g_bLowPwrAudio = 0;

extern uint8_t g_nState;

//int16_t abc = 0,abc1 = 0;
/*----------------------------------------------------------------------------------------
������: icTimerInit
����:
		None
����ֵ:
		None
����:
		����ѡ��Ķ�ʱ������Ӳ����ʼ��
----------------------------------------------------------------------------------------*/
void icTimerInit(void)
{
#if USE_TIMER == 0
	// ����ʱ��0
	SYSCLK->CLKSEL1.TMR0_S = 4; // ѡ��48M��ʱƵ��
	SYSCLK->APBCLK.TMR0_EN = 1; // ��ʱ��
	outpw((uint32_t)&TIMER0->TCSR ,0 ); // �رն�ʱ��
	TIMER0->TISR.TIF = 1; // ����жϱ�־
	TIMER0->TCMPR = 49152; // ����10 Ƶ��49.152MHz ��1KHZ
	TIMER0->TCSR.PRESCALE = 0;
	outpw((uint32_t)&TIMER0->TCSR,  (uint32_t)((1 << 30) | (1 << 29) | (1 << 27))); // ʹ�ܶ�ʱ����ʹ���жϣ�ģʽ01�����ظ�����
	NVIC_EnableIRQ(TMR0_IRQn);
#else
	// ����ʱ��1
	SYSCLK->CLKSEL1.TMR1_S = 0; // ѡ��10K��ʱƵ��
	SYSCLK->APBCLK.TMR1_EN =1; // ��ʱ��
	outpw((uint32_t)&TIMER1->TCSR ,0 ); // �رն�ʱ��
	TIMER1->TISR.TIF = 1; // ����жϱ�־
	TIMER1->TCMPR = 10; // ����10 Ƶ��10KHZ ��1KHZ
	TIMER1->TCSR.PRESCALE = 0;
	outpw((uint32_t)&TIMER1->TCSR,  (uint32_t)((1 << 30) | (1 << 29) | (1 << 27))); // ʹ�ܶ�ʱ����ʹ���жϣ�ģʽ01�����ظ�����
	NVIC_EnableIRQ(TMR1_IRQn);
#endif

}
/*----------------------------------------------------------------------------------------
������: icTimerTerm
����:
		None
����ֵ:
		None
����:
		����ѡ��Ķ�ʱ������Ӳ�����ʼ��
----------------------------------------------------------------------------------------*/
void icTimerTerm(void)
{
#if USE_TIMER == 0
	// �رն�ʱ��0
	TIMER0->TCSR.IE = 0; // ���ж�
	TIMER0->TCSR.CRST = 1; //���ö�ʱ���� ͣ�ö�ʱ��
	SYSCLK->APBCLK.TMR0_EN = 0; // ��ʱ��
	NVIC_DisableIRQ(TMR0_IRQn);
#else
	// �رն�ʱ��1
	TIMER1->TCSR.IE = 0; // ���ж�
	TIMER1->TCSR.CRST = 1; //���ö�ʱ���� ͣ�ö�ʱ��
	SYSCLK->APBCLK.TMR1_EN = 0; // ��ʱ��
	NVIC_DisableIRQ(TMR1_IRQn);
#endif
}

#if USE_TIMER == 0
void TMR0_IRQHandler(void)
#else
void TMR1_IRQHandler(void)
#endif
{
#if USE_TIMER == 0
	TIMER0->TISR.TIF =1; // ����жϱ�־
#else
	TIMER1->TISR.TIF =1; // ����жϱ�־
#endif		
}

/*----------------------------------------------------------------------------------------
������: TMRCBForSysTick
����:
		None
����ֵ:
		None
����:
		��ʱ���ص����������ڿ��м�ʱ��¼���ȴ���ʱ��ʶ���ʱ����������ʱ�ȵȼ�ʱ����
----------------------------------------------------------------------------------------*/
void TMRCBForSysTick(void)
{
	g_cntSysTicks++;
	if(g_bHavePlayAction && !g_bContinueTick){
		g_cntSysTicks = 0;
		g_bHavePlayAction = 0;
	}
	if(g_bHaveKeyAction || g_bHaveEsrAction || g_bHaveRecAction || g_bHaveCamAction){
		g_cntSysTicks = 0;
		g_bHaveKeyAction = 0;
		g_bHaveEsrAction = 0;
		g_bHaveRecAction = 0;
		g_bHaveCamAction = 0;
		g_bContinueTick = 0;
	}

	//------------------------------
	if(g_bRecordStart)
		g_cntWaitForRecTicks++;
	else
		g_cntWaitForRecTicks = 0;

	//-----------------------------10s�Ӽ��һ�ε���
	g_cntPwrCheckTicks++;
	if(g_cntPwrCheckTicks >= 10){
		g_bCheckPower = 1;
		g_cntPwrCheckTicks = 0;
	}
	//-----------------------------
	if(!(DrvGPIO_GetBit(GPA,15))){
		if(g_bLowPower){
			RED_LED_ON;
			if(g_cntLowPwrTicks % LOWPWR_REMINDER_TIME == 0)	g_bLowPwrAudio = 1;
			g_cntLowPwrTicks++;
		}else if(g_bServerLowPwr){
			g_cntLowPwrTicks = LOW_POWER_TIME;
		}else
			g_cntLowPwrTicks = 0;
	}

//	LOG("--%d\t%d\r\n",abc,abc1);
//	abc = 0;
//	abc1 = 0;
}
/*----------------------------------------------------------------------------------------
������: GetSysTicks
����:
		None
����ֵ:
		g_cntSysTicks��ϵͳ��ʱ����ʱ��
����:
		���ڻ�ȡϵͳ��ʱ���Ŀ���ʱ�䣨Ҳ�����ǵȴ�ʱ�䣩
----------------------------------------------------------------------------------------*/
int16_t GetSysTicks(void)
{
	return g_cntSysTicks;
}
/*----------------------------------------------------------------------------------------
������: GetWaitForRecTicks
����:
		None
����ֵ:
		g_cntWaitForRecTicks��ϵͳ��ʱ����ʱ��
����:
		��ȡ�ӽ���¼��ģʽ����ʼ¼����ʱ��
----------------------------------------------------------------------------------------*/
int16_t GetWaitForRecTicks(void)
{
	return g_cntWaitForRecTicks;
}
/*----------------------------------------------------------------------------------------
������: TimerInit
����:
		None
����ֵ:
		None
����:
		��ʼ����ʱ������������һ�����ⶨʱ����1sΪ����
----------------------------------------------------------------------------------------*/
void TimerInit(void)
{
	DrvTIMER_Init();
	DrvTIMER_Open(TMR1, 1000, PERIODIC_MODE);
	DrvTIMER_SetTimerEvent(TMR1,1000, (TIMER_CALLBACK)TMRCBForSysTick,0);
	DrvTIMER_EnableInt(TMR1);
	DrvTIMER_Ioctl(TMR1, TIMER_IOC_START_COUNT, 0);
}
/*----------------------------------------------------------------------------------------
������: CommonCheck
����:
		None
����ֵ:
		None
����:
		����һЩ����ļ�⣺��ʾ�����ػ���������⡢�͵紦���
----------------------------------------------------------------------------------------*/
void CommonCheck(void)
{
//	if(g_cntSysTicks == REMINDER_TIME){
//		g_cntSysTicks = REMINDER_TIME + 1;
//		g_bReminderAudio = 0;
//		g_bContinueTick = 1;
//		PlayBegin(PEN_TIMEOUT_AUDIO);
//	}
	if(g_cntSysTicks >= SHUTDOWN_TIME){
		g_bShutDown = 0;
		g_bContinueTick = 0;
		g_nState = SLEEP_STATE;
		g_cntSysTicks = 0;
	}

	if(g_bCheckPower){
		g_bCheckPower = 0;
		PowerCheck();
	}
	if(g_bLowPwrAudio)
	{
		g_bLowPwrAudio = 0;
		PlayBegin(PEN_LOWPWR_AUDIO);
		g_nState = PWRDOWN_STATE;		
	}
//	if(g_cntLowPwrTicks >= LOW_POWER_TIME)		//�����ʱ����Ҫ����ѹ���ͣ�
//	{
//		g_nState = SLEEP_STATE;
//	}
}

