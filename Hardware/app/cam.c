//-------------------------��ͼ������йصĳ���
#include "cam.h"

//PARAM_ADDR �ǲ�����spiflash�е���ʼ��ַ

/*
����ʹ�ã�
SPI:
PA0--MOSI0
PA1--SCLK
PA2--SSB0
PA3--MISO0

IIC:
PB2--SCK
PB3--SDA

PWM:
PA12--0ͨ��
PA13--1ͨ��
*/
extern uint8_t g_nState;
extern uint8_t g_bRecordStart;
extern uint8_t g_bHaveKeyAction;
extern uint8_t g_bHaveRecAction;

int8_t get_param_from_file(int32_t *pParam1, int32_t *pParam2, int32_t *pParam3, int32_t *pParam4,int32_t *pParam5)
{
	int32_t buf[5];

	FMD_ReadData(buf, PARAM_ADDR, 5 * 4);//5��int������
	*pParam1 = buf[0];
	*pParam2 = buf[1];
	*pParam3 = buf[2];
	*pParam4 = buf[3];
	*pParam5 = buf[4];
	return 1;	
}

void save_param_to_file(int32_t param1, int32_t param2, int32_t param3, int32_t param4, int32_t param5)
{
	uint8_t buf[256];
	uint32_t i;
	for(i = 0; i < 256; i++)
	{
		buf[i] = 0;
	}
	*(int32_t*)&buf[0] = param1;
	*(int32_t*)&buf[4] = param2;
	*(int32_t*)&buf[8] = param3;
	*(int32_t*)&buf[12] = param4;
	*(int32_t*)&buf[16] = param5;
	
	FMD_EraseBlock(PARAM_ADDR); //����PARAM_ADDR���ڵ�Sector(4K)
	FMD_WriteData(buf, PARAM_ADDR); //һ��дһҳ 		
}


  
T_VOID cam_adjust_check(void)
{
    int32_t param1, param2, param3, param4, param5;

    if(get_param_from_file(&param1, &param2, &param3, &param4, &param5)){
		if(param5==0xAA){
	        cam_set_adj_param(param1, param2, param3, param4, param5);            
			LOG(("Get param OK\n"));
			return;
		}          
    }else
//	{
//		LOG("Get param NG\n");  
//    }
//    
//    LOG("Begin to adjust...\n");
    //play_beep(200);
    
    if(cam_get_adj_param((T_U32 *)0x20000000, 1024*3, &param1, &param2, &param3, &param4,&param5)){
        //save the data in file
        LOG(("Camera adjusting OK\n"));
        save_param_to_file(param1, param2, param3, param4, param5);	
        LOG(("Sava param OK\n"));
        cam_set_adj_param(param1, param2, param3, param4, param5);
        //play_beep(500);
        return ;       
    }else{
        LOG(("Camera adjust NG\n"));           
    }

}

//__align(4) uint8_t cam_buf[3 * 1024];
T_VOID us_delay(T_U32 us)
{
    
		us = us * 10 ;//>> 2;//5;       // time = time/32
        
        while(us--);
        {
            *(volatile T_U32 *)0;
        }
}
static void audio_codec(void)
{
	us_delay(1000*9); 
}
/*----------------------------------------------------------------------------------------
������: CameraStart
����:
		None
����ֵ:
		None
����:
		��������ʵ�����ͷ������У����ͷ
----------------------------------------------------------------------------------------*/
unsigned char buf[1600] __attribute__ ((section("camvars"), zero_init));
void InitCam(void)
{
	uint8_t	bRet;
//	unsigned char buf[1600];
//	memset(buf,0,sizeof(buf));
    bRet = cam_init(eDRVPDMA_CHANNEL_3,buf,1600);	//��ͷ��ʼ������
    if(!bRet)
    {
		LOG(("camera init fail\n"));
		while(1);
    }
	else
	{
		LOG(("camera init ok \n"));
	}
}
void CameraStart(void)
{
	LOG(("CAMERA INIT...\r\n"));
	InitCam();
	cam_adjust_check();		   //��ͷ��У������
	cam_reg_audio_cb(audio_codec);
	LOG(("CAMERA INIT OK!\r\n"));
}
/*----------------------------------------------------------------------------------------
������: GetCamCode
����:
		None
����ֵ:
		dwCode
����:
		��ȡ����ͷ����ֵ��������
----------------------------------------------------------------------------------------*/
//T_U8  *show_log_buf = (T_U8 *)(0x20000000 + 1024*5);
int32_t GetCamCode(void)
{
	int32_t dwCode;
//	extern int16_t abc;

	DrvPDMA_DisableInt(eDRVPDMA_CHANNEL_2, eDRVPDMA_WAR);
//	__disable_irq();
	DrvTIMER_Ioctl(TMR0,TIMER_IOC_STOP_COUNT,0);
//	while(1){						 
	cam_get_frame((T_U32 *)0x20000000, 304, (T_U8*)(0x20000000+308), 50,
	(T_U8 *)(0x20000000+308 + 50), 50);
	dwCode = cam_get_code();
//	abc++;
//	LOG(("dwCode=%d\r\n",dwCode));
//	}
//	__enable_irq();
	DrvTIMER_Ioctl(TMR0,TIMER_IOC_START_COUNT,0);
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_2,eDRVPDMA_WAR);
	
	GPIOToSpi();	
	return dwCode;
}
/*----------------------------------------------------------------------------------------
������: GetRecCode
����:
		None
����ֵ:
		dwCode��¼����ʹ�õ���ֵ
����:
		��ȡ����ͷ����ֵ������������¼�������ڻ�ȡ����ֵ֮ǰ�а��������߳�ʱ���
----------------------------------------------------------------------------------------*/
int32_t GetRecCode(void)
{
	KEYMSG msg;
	int32_t dwRecCode = 0;
	uint8_t bReturnFlag = 0;

//	CameraStart();
	g_bRecordStart = 1;
	LedFlashBeforeRec(LED_ON);

//	LOG("start by selecting a code, end by bouncing key\r\n");
	while( (dwRecCode = GetCamCode()) == -1 )		//��������ֵ
	{
	 	if(GetWaitForRecTicks() >= REC_EXIT_TIME){
			bReturnFlag = 1;	
		}
		if(KEY_MsgGet(&msg)){
			g_bHaveKeyAction = 1;
			if(msg.Key_MsgValue == KEY_RECORD){
				if(msg.Key_MsgType == KEY_TYPE_UP){
//					LOG("record end!!");
					bReturnFlag = 1;
				}
			}
		}

		if(bReturnFlag)
		{
			bReturnFlag = 0;
			g_nState = PLAY_STATE;
			break;
		}
	}
	LOG(("dwRecCode:%d",dwRecCode));
	g_bRecordStart = 0;
	g_bHaveRecAction = 1;
	LedFlashBeforeRec(LED_OFF);
	return dwRecCode;
}

