#include "led.h"
#include "play.h"
#include "timer.h"

uint8_t g_bLedFlashForCam = 0;
uint8_t g_bLedFlashForCharging = 0;
uint8_t g_bAdapterConnected = 0;
uint8_t g_bLedFlashForNoEsring = 0;
uint8_t g_bLedFlashBeforeRec = 0;
uint8_t g_bLowPwrFlag = 0;

int16_t g_wMixDuty = 0,g_wMixCycle = 0;
/*----------------------------------------------------------------------------------------
������: TMRCBForLedCommon
����:
		None
����ֵ:
		None
����:
		LED��˸ͨ�õĳ���ȥ�����ʱ�ĺ�����
----------------------------------------------------------------------------------------*/ 
void TMRCBForLedCommon(void)
{
//	static int8_t bReverse = 0;
	static int16_t wCountForCam = 0,wCountForNoEsr = 0,wCountForBeforeRec = 0;//,wCountForCharge = 0
	static uint8_t cntFlashNum = 0;

	if( g_bLedFlashForCam ){				//cam
		if(!g_bLowPwrFlag)		RED_LED_OFF;
		else					RED_LED_ON;
		if(wCountForCam < 3*2)
			GREEN_LED_ON;
		else if(wCountForCam == 3*2)
			GREEN_LED_OFF;
		else if(wCountForCam >= 53*2){
			GREEN_LED_OFF;
			wCountForCam = -1;
		}
		wCountForCam++;
	}

	if( g_bLedFlashForNoEsring ){			//nocam:led flash 2s
		if(!g_bLowPwrFlag)		RED_LED_OFF;
		else					RED_LED_ON;
		if(wCountForNoEsr < 5){
			GREEN_LED_OFF;
		}else if( wCountForNoEsr == 5){
			GREEN_LED_ON;
		}else if( wCountForNoEsr > 10){
			GREEN_LED_OFF;
			wCountForNoEsr = -1;
			cntFlashNum++;
		}
		if(cntFlashNum == 2){
			GREEN_LED_ON;
			g_bLedFlashForNoEsring = 0;
			cntFlashNum = 0;
		}
		wCountForNoEsr++;
	}

	if( g_bLedFlashBeforeRec ){
		if(!g_bLowPwrFlag)		RED_LED_OFF;
		else					RED_LED_ON;
		if( wCountForBeforeRec < 5*2){
			GREEN_LED_ON;
		}else if( wCountForBeforeRec == 5*2 ){
			GREEN_LED_OFF;
		}else if( wCountForBeforeRec >= 10*2 ){
			GREEN_LED_OFF;
			wCountForBeforeRec = -1;
		}
		wCountForBeforeRec++;		
	}else wCountForBeforeRec = 0;

//	if( g_bAdapterConnected ){
//		if( g_bLedFlashForCharging ){		//charging
//			g_wMixCycle = 2*10;
//			if(!bReverse){
//				if(g_wMixDuty == g_wMixCycle)
//					bReverse = 1;
//				g_wMixDuty++;
//			}else{
//				if(g_wMixDuty == -1){
//					bReverse = 0;
//					g_bSwitchLedFlag = ~g_bSwitchLedFlag;
//				}
//				g_wMixDuty--;
//			}	
//		}else if( g_bLedFlashForCharging == 0 ){ 		//chargeover
//			g_wMixCycle = 2*10;
//			g_bSwitchLedFlag = 0;
//			if(!bReverse){
//				if(g_wMixDuty == g_wMixCycle)
//					bReverse = 1;
//				g_wMixDuty++;
//			}else{
//				if(g_wMixDuty == 0)
//					bReverse = 0;
//				g_wMixDuty--;
//			}	
//		}
//	}else{
//		g_wMixCycle = 0;
//	}
}
/*----------------------------------------------------------------------------------------
������: TMRCBForLedBreath
����:
		None
����ֵ:
		None
����:
		���ͳ�����ʱ�ĺ�����Ч��
----------------------------------------------------------------------------------------*/
void TMRCBForLedBreath(void)
{
	static uint8_t nMixCount = 0;
	static uint8_t bReverse = 0;
	static uint16_t wCountForCharge = 0;
	static uint8_t g_bSwitchLedFlag = 0;

#define BREATH_CYCLE	100
	
	if( g_bAdapterConnected ){
		wCountForCharge++;
		if(wCountForCharge == BREATH_CYCLE){
			wCountForCharge = 0;
			if( g_bLedFlashForCharging ){		//charging
				g_wMixCycle = 10;
				if(!bReverse){
					if(g_wMixDuty >= g_wMixCycle)
						bReverse = 1;
					else if(g_wMixDuty >= 10)
						g_wMixDuty += 2;
					else
						g_wMixDuty++;
				}else{
					if(g_wMixDuty <= 0){
						bReverse = 0;
						g_bSwitchLedFlag = ~g_bSwitchLedFlag;
					}
					else if(g_wMixDuty >= 10)
						g_wMixDuty -= 2;
					else
						g_wMixDuty--;
				}
			}else if( g_bLedFlashForCharging == 0 ){ 		//chargeover
				g_wMixCycle = 10;
				g_bSwitchLedFlag = 0;
				if(!bReverse){
					if(g_wMixDuty >= g_wMixCycle)
						bReverse = 1;
					else if(g_wMixDuty > 10)
						g_wMixDuty += 2;
					else
						g_wMixDuty++;
				}else{
					if(g_wMixDuty <= 0)
						bReverse = 0;
					else if(g_wMixDuty > 10)
						g_wMixDuty -= 2;
					else if(g_wMixDuty > 0)
						g_wMixDuty--;
				}	
			}
		}
	}else{
		g_wMixCycle = 0;
	}

	if(g_wMixCycle){
		nMixCount++;
		if(nMixCount < g_wMixDuty){
			if(!g_bSwitchLedFlag){
				GREEN_LED_ON;
				RED_LED_OFF;
			}else{
				GREEN_LED_OFF;
				RED_LED_ON;
			}
		}else if(nMixCount == g_wMixDuty){
			GREEN_LED_OFF;
			RED_LED_OFF;
		}else if(nMixCount >= g_wMixCycle){		//�涨Ϊһ��LED���������ڣ�
			GREEN_LED_OFF;
			RED_LED_OFF;
			nMixCount = 0;
		}
	}
}
/*----------------------------------------------------------------------------------------
������: LedInit
����:
		None
����ֵ:
		None
����:
		��ʼ��GPIO����ʼ��������ʱ��
----------------------------------------------------------------------------------------*/
void LedInit(void)
{
	DrvGPIO_Open(GPB,GREEN_LED,IO_OUTPUT); 			//��ɫ��
	GREEN_LED_OFF;

	DrvGPIO_Open(GPB,RED_LED,IO_OUTPUT);		   	//��ɫ��
	RED_LED_OFF;

	DrvGPIO_Open(GPA,9,IO_OUTPUT);		   	//����ʹ��
	GPIOA->DOUT &= ~(1 << 9);

	g_bLedFlashForCam = 0;
	g_bLedFlashForCharging = 0;
	g_bAdapterConnected = 0;
	g_bLedFlashForNoEsring = 0;
	g_bLedFlashBeforeRec = 0;

	g_wMixDuty = 0;
	g_wMixCycle = 0;

	DrvTIMER_SetTimerEvent(TMR1,50, (TIMER_CALLBACK)TMRCBForLedCommon,0);
	DrvTIMER_SetTimerEvent(TMR1,1, (TIMER_CALLBACK)TMRCBForLedBreath,0);
}
/*----------------------------------------------------------------------------------------
������: LedFlashForCam
����:
		bFlag������ͷLED���� LED_ON/LED_OFF ��/��
����ֵ:
		None
����:
		����ͷ�ɲ�����green0.3->green5s��˸�����ɲ����ر�LED
----------------------------------------------------------------------------------------*/
void LedFlashForCam(uint8_t bFlag) 		//green:0.3s--5s flash_on
{
	if(!bFlag){
		g_bLedFlashForCam = 0;
		RED_LED_OFF;
		GREEN_LED_OFF;
	}else if(bFlag){
		g_bLedFlashForCam = 1;
//		wCountForCam = 0;
	}
}
/*----------------------------------------------------------------------------------------
������: LedFlashForEsr
����:
		bFlag��Esr״̬LED���أ�ͬʱҲ������¼��״̬��ʾ�� LED_ON/LED_OFF ��/��
����ֵ:
		None
����:
		Esr and record״̬��green�����������ر�LED
----------------------------------------------------------------------------------------*/
void LedFlashForEsr(uint8_t bFlag)		//green:long
{
	if(!bFlag)
		GREEN_LED_OFF;
	else if(bFlag)
		GREEN_LED_ON;
}
/*----------------------------------------------------------------------------------------
������: LedFlashForNoEsr
����:
		None
����ֵ:
		None
����:
		��ʶ֮��Green0.25s->0.25s��˸����
----------------------------------------------------------------------------------------*/
void LedFlashForNoEsr()		//��ʶ	green:0.25s-0.25s -> 0.25s--0.25s
{
	g_bLedFlashForNoEsring = 1;
}
/*----------------------------------------------------------------------------------------
������: LedFlashForLowPwr
����:
		bFlag���͵�״̬LED���� LED_ON/LED_OFF ��/��
����ֵ:
		None
����:
		�͵�״̬��red�����������ر�LED
----------------------------------------------------------------------------------------*/
void LedFlashForLowPwr(uint8_t bFlag) 	//red:long
{
	if(!bFlag)
		RED_LED_OFF;
	else if(bFlag){
		RED_LED_ON;
		g_bLowPwrFlag = 1;
	}
}
/*----------------------------------------------------------------------------------------
������: LedFlashForCharge
����:
		None
����ֵ:
		None
����:
		������״̬��green1s->red1s������˸�������ر�LED
----------------------------------------------------------------------------------------*/
void LedFlashForCharge()	  	//green:1s -> red:1s -> green:1s .... 
{
	g_bAdapterConnected = 1;
	g_bLedFlashForCharging = 1;	
}
/*----------------------------------------------------------------------------------------
������: LedFlashForChargeover
����:
		None
����ֵ:
		None
����:
		������״̬��green1s->1s������˸�������ر�LED
----------------------------------------------------------------------------------------*/
void LedFlashForChargeover()	//green:1s -> green:1s -> green:1s ....
{
	g_bAdapterConnected = 1;
	g_bLedFlashForCharging = 0;
}
/*----------------------------------------------------------------------------------------
������: AdapterDisconnect
����:
		None
����ֵ:
		None
����:
		������Դ�������Ƿ�Ͽ���ִ���������˵���������Ѿ��Ͽ�
----------------------------------------------------------------------------------------*/
void AdapterDisconnect(void)
{
	g_bAdapterConnected = 0;
	GREEN_LED_OFF;
	RED_LED_OFF;
}
/*----------------------------------------------------------------------------------------
������: LedFlashBeforeRec
����:
		bFlag��״̬LED���� LED_ON/LED_OFF ��/��
����ֵ:
		None
����:
		¼��֮ǰ��һ����˸״̬��green0.5s->0.5s��˸�������ر�LED
----------------------------------------------------------------------------------------*/
void LedFlashBeforeRec(uint8_t bFlag)
{
	if(bFlag){
		g_bLedFlashBeforeRec = 1;
//		wCountForBeforeRec = 0;
	}else{ 
		g_bLedFlashBeforeRec = 0;
		RED_LED_OFF;
		GREEN_LED_OFF;
	}

}
/*----------------------------------------------------------------------------------------
������: LedUninit
����:
		None
����ֵ:
		None
����:
		LEDGPIO�ڵ����ʼ�����ر�LED
----------------------------------------------------------------------------------------*/
void LedUninit()
{
	DrvGPIO_SetBit(GPB,GREEN_LED);
	DrvGPIO_SetBit(GPB,RED_LED);
}


