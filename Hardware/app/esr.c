#include "esr.h"
#include "stat.h"
/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define OBJ_RAM_SIZE		(0x2300)
__align(4) int8_t objram[OBJ_RAM_SIZE] __attribute__ ((section("esrvars"), zero_init));		//ʶ������Ram

#define OBJ_RAM_SIZE_FOR_VAD		(6*1024)
__align(4) int8_t objramForVAD[OBJ_RAM_SIZE_FOR_VAD] __attribute__ ((section("recvars"), zero_init));		//ʶ������Ram

#define RESIDENT_RAM_SIZE	(40)
__align(4) int8_t residentram[RESIDENT_RAM_SIZE];

#define	MAX_FRAME_SIZE		(64)
__align(4) int16_t g_waAdcSwitchBuffer[2][MAX_FRAME_SIZE] __attribute__ ((section("swhvars"), zero_init));
uint8_t g_bCurBuffer = 0;

#define MAX_SPIBUFFERSIZE	(256)
#define MAX_SPIBUFFERCOUNT	(7)
__align(4) int8_t g_waToFlashBuffer[MAX_SPIBUFFERCOUNT][MAX_SPIBUFFERSIZE] __attribute__ ((section("recvars"), zero_init));
int32_t g_dwaEncodeHeader[MAX_SPIBUFFERCOUNT] __attribute__ ((section("recvars"), zero_init));

RECHEADER g_sHeader __attribute__ ((section("recvars"), zero_init));
uint32_t g_waRecHeader[64] __attribute__ ((section("recvars"), zero_init));
uint8_t  g_nRecHeader = 0;

//����Ҫ������BUFFERʹ��֮ǰ

extern uint8_t g_bHaveKeyAction;
extern uint8_t g_bHaveEsrAction;
extern uint8_t g_bHaveRecAction;
extern uint8_t g_nState;

#define  SAMPLE_RATE_SET		(16000)  /* 8000 */
#define  VAD_CHKBACK_FRMNUM		(70)

//--------------------------------------
uint16_t g_wSpiCurPosition = 0;
uint8_t  g_nSpiIndex = 0;
uint16_t g_cntHaveData = 0;
int16_t *g_pWriteBuffer = 0;

struct tagADPCMEncoder tagAdpcmEncoder;
int16_t g_wRecCodeIndex = -1;

uint32_t g_dwRecWriteAddr = 0;
uint32_t g_dwRecWriteHeaderAddr = 0;
uint32_t g_dwRecCodeAddr = 0;
uint32_t g_dwRecBackAddr = 0;

uint32_t g_dwRecDataStartAddr = 0;
uint32_t g_dwRecIndexAddr = 0;

uint32_t g_dwRecIndexIdleStartAddr = 0;
uint32_t g_dwRecIndexIdleEndAddr = 0;

uint8_t	 g_bNoEsr = 0;
uint8_t	 g_bEsrPure = 1;
uint8_t	 g_bHadRecord = 0;
uint8_t	 g_bFirstAreaChecked = 0;

ivPointer pEsrObj;
void ClrESRMemory(void)
{
	memset(residentram,0,sizeof(residentram));
}

/*----------------------------------------------------------------------------------------
������: InitEsr
����:
		None
����ֵ:
		ivFalse/ivTrue: ��ʼ��ʶ���Ƿ�ɹ�
����:
		����ʶ��ģ�飬����ADC��PDMA
----------------------------------------------------------------------------------------*/
static uint8_t InitEsr(int8_t* _objram, ivSize obj_ram_size, ivCPointer _g_pu8ResData)
{
	EsErrID err;
	ivSize	nESRObjSize;
	ivPointer pResidentRAM;
	ivUInt16 nResidentRAMSize;

	LOG(("INIT ESR...\r\n"));

	pEsrObj = _objram;
	nESRObjSize = obj_ram_size;
	pResidentRAM = residentram;
	nResidentRAMSize = RESIDENT_RAM_SIZE;

	memset(_objram,0,obj_ram_size);
	memset(g_waAdcSwitchBuffer,0,sizeof(g_waAdcSwitchBuffer));
//	err = ESRCreate(pEsrObj, &nESRObjSize, pResidentRAM, &nResidentRAMSize, (ivCPointer)g_pu8ResData, 0);
	err = ESRCreate(pEsrObj, &nESRObjSize, pResidentRAM, &nResidentRAMSize, (ivCPointer)_g_pu8ResData, 0,SAMPLE_RATE_SET, VAD_CHKBACK_FRMNUM);
    if(ivESR_OK != err){
        LOG(("ESRCreate failed! err=%d\r\n", (int16_t)err));
        return ivFalse;
    }
	err = ESRSetParam(pEsrObj, ES_PARAM_SPEECH_FRM_MAX, 400);
	if(err != ivESR_OK)
	{
		LOG(("ESR set ES_PARAM_SPEECH_FRM_MAX Err\n"));
		return ivFalse;
	}
    err = ESRSetParam(pEsrObj, ES_PARAM_SPEECH_FRM_MIN, 20);
//	err = ESRSetParam(pEsrObj, ES_PARAM_AMBIENTNOISE, 5000);
	if(err != ivESR_OK)
	{
		LOG(("ESR set ES_PARAM_SPEECH_FRM_MIN Err\n"));
		return ivFalse;
	}
	ADC_Init();
	PdmaForAdc();
	LOG(("INIT ESR OK!\r\n"));

	return ivTrue; 
}
/*----------------------------------------------------------------------------------------
������: IsContinueEsr
����:
		None
����ֵ:
		ivFalse/ivTrue: ����ִ��/�˳�ʶ��
����:
		�ж���û���˳�ʶ�������
----------------------------------------------------------------------------------------*/
static uint8_t IsContinueEsr(void)
{
	KEYMSG msg;

	if(KEY_MsgGet(&msg)){
		g_bHaveKeyAction = 1;
		if(msg.Key_MsgValue == KEY_ON_OFF){
			if(msg.Key_MsgType == KEY_TYPE_SP){
				g_nState = PLAY_STATE;
				return ivFalse;
			}else if(msg.Key_MsgType == KEY_TYPE_LP){
				g_nState = PWRDOWN_STATE;
				return ivFalse;
			}
		}else if(msg.Key_MsgValue == KEY_RECORD){
			if(msg.Key_MsgType == KEY_TYPE_DOWN){
				g_nState = RECORD_STATE;
				return ivFalse;
			}
		}
	}
	if(GetSysTicks() >= ESR_DENY_TIME)
	{
		g_bHaveEsrAction = 1;
		g_nState = PLAY_STATE;
		return ivFalse;
	}
	return ivTrue;
}
/*----------------------------------------------------------------------------------------
������: GetEsrCode
����:
		None
����ֵ:
		(nResultID+ESR_CODE_START)/ivTrue: ʶ��Ľ��/ʶ�𵽻����������߾�ʶ
����:
		���ʶ��Ľ��
----------------------------------------------------------------------------------------*/
int32_t GetEsrCode(ESR_NET EsrNet)
{
	ivESRStatus nStatus = 0;
	PCEsrResult pResult = ivNull;
	ivUInt32 dwMsg = 0;
	EsErrID err;

	LOG(("ESR STARTING...\r\n"));
	if(EsrNet == ESR_MAJOR_NET){
		if(!InitEsr(objram, OBJ_RAM_SIZE, g_pu8ResData)){
			LOG(("Esr Init Failed!\r\n"));
			return ivFalse;
		}
	}else if(EsrNet == ESR_MINOR_NET){
		if(!InitEsr(objramForVAD, OBJ_RAM_SIZE_FOR_VAD, g_pu8ResDataForVAD)){
			LOG(("Esr Init Failed!\r\n"));
			return ivFalse;
		}
	}
    while(1) {
		if(!IsContinueEsr()){
			LOG(("err\n"));
			ADC_Term();
			return ivFalse ;	
		}
		err = ESRRunStep(pEsrObj, dwMsg, &nStatus, &pResult);
		dwMsg = 0;
		if((ivESR_OK != err) && (EsErr_BufferEmpty != err)){
			LOG(("ESRRunStep failed! err=%d\r\n", (int16_t)err));
			break;
		}
        if (ES_STATUS_FINDSTART == nStatus) {
            LOG(("vad\r\n"));
			dwMsg = ES_MSG_RESERTSEARCHR;
        }
        if (ES_STATUS_RESULT == nStatus) {
			LOG(("RESULT OK\r\n"));
            if(ES_TYPE_RESULT == pResult->nResultType) {
				g_bHaveEsrAction = 1;
                LOG(("----------------nResultID=%d, fConfidence=%d\r\n",pResult->nResultID, pResult->nConfidenceScore));
				g_nState = PLAY_STATE;
                break;
            }
            else if ( ES_TYPE_FORCERESULT == pResult->nResultType) {
				g_bHaveEsrAction = 1;
                LOG(("----------------nResultID=%d, fConfidence=%d.������ʱ\r\n",pResult->nResultID, pResult->nConfidenceScore));
                break;
            }
            else if (ES_TYPE_AMBIENTNOISE == pResult->nResultType) {
                LOG(("----------------��������\r\n"));
                break;
            }
            else if (ES_TYPE_RESULTNONE == pResult->nResultType) {
                LOG(("----------------��ʶ\r\n"));
                break;
            }
        }
    }
//	DrvPDMA_Close();
	LOG(("EXIT!!\r\n"));
	ADC_Term();
	if (ES_STATUS_RESULT == nStatus) {
	    if(ES_TYPE_RESULT == pResult->nResultType){
	        return (pResult->nResultID + ESR_CODE_START);		//+ESR_CODE_START�����Ƿ��������˳�
	    }
	}
	return ivTrue;
}
///*----------------------------------------------------------------------------------------
//������: ErarseRecord
//����:
//		dwRecCode: ¼������Ӧ����ֵ
//����ֵ:
//		1/0�� ׼���ɹ�/׼��ʧ��
//����:
//		��ȡ¼��������ȷ��¼����ַ���ҵ�¼����ֵ��������ʼ����������������ʾ������ʼ��VAD
//----------------------------------------------------------------------------------------*/
static void ErarseRecord(int16_t wCodeIndex)
{
	uint32_t dwRecFirstClusterIndex = 1;
	uint32_t dwRecClusterIndex = 0;
	uint32_t dwRecClusterJumpAddr = 0;
	uint32_t dwRecClusterJumpEndAddr = 0;
	uint32_t dwRecCodeIndexAddr = g_dwRecCodeAddr + wCodeIndex*8;
	
	LOG(("ERASING...\r\n"));
	
	LOG(("dwReadAudioIndexAddr:%x, dwRecCode:%d\r\n",dwRecCodeIndexAddr,wCodeIndex));
	FMC_ReadBuffer(dwRecCodeIndexAddr+4, &dwRecFirstClusterIndex, sizeof(dwRecFirstClusterIndex));
	LOG(("dwRecFirstClusterIndex:%d\r\n",dwRecFirstClusterIndex));
//	FMD_Write2Byte(&wNoRecFlag, (g_dwRecStartAddr + (dwRecCode-REC_CODE_START)*2));		//ɾCODE�ṹ---------------���ﲻ��ȷ�����޸�
	dwRecClusterJumpAddr = g_dwRecDataStartAddr + dwRecFirstClusterIndex*FLASH_MIN_ERASE_SIZE + sizeof(RECHEADER);
	dwRecClusterJumpEndAddr = g_dwRecDataStartAddr + dwRecFirstClusterIndex*FLASH_MIN_ERASE_SIZE + FLASH_PAGE_SIZE;
	
	dwRecClusterIndex = dwRecFirstClusterIndex;
	
	do{
		LOG(("g_dwRecIndexIdleEndAddr:%x,wRecClusterIndex:%x\r\n",g_dwRecIndexIdleEndAddr,dwRecClusterIndex));
		
		FMC_WriteBuffer(g_dwRecIndexIdleEndAddr, &dwRecClusterIndex, sizeof(dwRecClusterIndex));
		g_dwRecIndexIdleEndAddr += 4;
		
		if(((g_dwRecIndexIdleEndAddr - g_dwRecIndexAddr) % FMC_SECTOR_SIZE) == 0){
			if(g_dwRecIndexIdleEndAddr >= (g_dwRecIndexAddr + REC_INDEX_SIZE))
				g_dwRecIndexIdleEndAddr = g_dwRecIndexAddr;
			FMC_Erase(g_dwRecIndexIdleEndAddr);
		}
		FMD_ReadData(&dwRecClusterIndex, dwRecClusterJumpAddr, sizeof(dwRecClusterIndex));
		dwRecClusterJumpAddr += 4;
		if(dwRecClusterJumpAddr == dwRecClusterJumpEndAddr){			//һҳ���꣬��ʱ����
//			dwRecClusterJumpAddr = g_dwRecDataStartAddr + wRecClusterIndex[(~bSwitchFlag) & 0xFF]*FLASH_MIN_ERASE_SIZE;
//			dwRecClusterJumpEndAddr = dwRecClusterJumpAddr + FLASH_PAGE_SIZE;
		}
	}while(dwRecClusterIndex != 0xFFFFFFFF);	
	LOG(("ERASE OVER!\r\n"));
}
//*----------------------------------------------------------------------------------------
//������: FindIdleCluster
//����:
//		wcntUsedArea�����ʹ�ù���Ƭ����
//����ֵ:
//		iFindIdle: �ҵ�����Ƭ����λ�ã���������û���ҵ�����λ�ã������ִ��󷵻�-1
//����:
//		�������������Ƭ��ʹ��������������ʹ�ù���Ƭ����Ѱ�ҿհ�Ƭ��
//----------------------------------------------------------------------------------------*
static int32_t FindIdleCluster(){
	uint32_t dwRecIndexIdleStart = 0;
	uint32_t dwRecClusterAddr = 0;
//	uint16_t dwRecIndexIdleEnd = 0;
//	while(dwRecIndexIdleStart == 0)
	LOG(("ALLOCATE A CLUSTER...\r\n"));
	
	FMC_ReadBuffer(g_dwRecIndexIdleStartAddr,&dwRecIndexIdleStart,sizeof(dwRecIndexIdleStart));
	g_dwRecIndexIdleStartAddr += 4;
	if(g_dwRecIndexIdleStartAddr >= g_dwRecIndexAddr + REC_INDEX_SIZE){
		g_dwRecIndexIdleStartAddr = g_dwRecIndexAddr;
	}
//	FMD_ReadData(&dwRecIndexIdleEnd,g_dwRecIndexIdleEndAddr,2);
	
//	
//	if(dwRecIndexIdleStart <= dwRecIndexIdleEnd)
	LOG(("ALLOCATE A CLUSTER OK! \r\ndwRecIndexIdleStart:%d\t%x\r\n",dwRecIndexIdleStart,g_dwRecIndexIdleStartAddr));
	dwRecClusterAddr = g_dwRecDataStartAddr + dwRecIndexIdleStart * FLASH_MIN_ERASE_SIZE;
	if(dwRecClusterAddr >= SPI_FLASH_SIZE)
	{	
		LOG(("Error:memory is not enough, please change bigger memory"));
		return -1;
	}else{
		return dwRecIndexIdleStart;
	}
}
///*----------------------------------------------------------------------------------------
//������: InitIndex
//����:
//		dwRecCode: ¼������Ӧ����ֵ
//����ֵ:
//		1/0�� ׼���ɹ�/׼��ʧ��
//����:
//		��ȡ¼��������ȷ��¼����ַ���ҵ�¼����ֵ��������ʼ����������������ʾ������ʼ��VAD
//----------------------------------------------------------------------------------------*/
void InitIndex()
{	
	uint32_t dwcntRemainCluster = (SPI_FLASH_SIZE - g_dwRecDataStartAddr)/FLASH_MIN_ERASE_SIZE;
	uint16_t iInitIndex;
	uint32_t dwUSEDFlag = 0x44455355;
	uint32_t waInitIndex[64];
	memset(waInitIndex,0xFF,sizeof(waInitIndex));
	
	LOG(("FIRST USED THIS FLASH, INITAL...\r\n"));
	
	FMC_Erase(DATAF_BASE_ADDR);
	FMC_WriteBuffer(DATAF_BASE_ADDR, &dwUSEDFlag, sizeof(dwUSEDFlag));
	
	for(iInitIndex=0; iInitIndex < REC_INDEX_SIZE; iInitIndex+=FMC_SECTOR_SIZE)		//�ٿ�1024��BLOCK
		FMC_Erase(g_dwRecIndexAddr + iInitIndex*FMC_SECTOR_SIZE);

	for(iInitIndex = 0; iInitIndex < dwcntRemainCluster; iInitIndex++){
		if((iInitIndex%64 == 0) && (iInitIndex != 0)){
			FMC_WriteBuffer(g_dwRecIndexAddr + (iInitIndex/64 - 1)*64*2,(uint32_t*)waInitIndex,sizeof(waInitIndex));
			memset(waInitIndex,0xFF,sizeof(waInitIndex));
		}
		waInitIndex[iInitIndex%64] = iInitIndex;
	}
	FMC_WriteBuffer(g_dwRecIndexAddr + (iInitIndex/64)*64*2,(uint32_t*)waInitIndex,sizeof(waInitIndex));
	LOG(("INITAL THIS FLASH OK!\r\n"));//dwcntRemainCluster:%d, iInitIndex:%d\r\n",dwcntRemainCluster,iInitIndex);
	g_dwRecIndexIdleStartAddr = g_dwRecIndexAddr;
	g_dwRecIndexIdleEndAddr = g_dwRecIndexAddr + dwcntRemainCluster*4;
}
/////*----------------------------------------------------------------------------------------
////������: GetRecCodeAddr
////����:
////		dwRecCode: ¼������Ӧ����ֵ
////����ֵ:
////		1/0�� ׼���ɹ�/׼��ʧ��
////����:
////		��ȡ¼��������ȷ��¼����ַ���ҵ�¼����ֵ��������ʼ����������������ʾ������ʼ��VAD
////----------------------------------------------------------------------------------------*/
//uint32_t GetRecCodeAddr(int32_t dwRecCode)
//{
//	uint32_t dwcntRecCode = 0;
//	uint32_t dwTempAddr = 0;
//	uint32_t dwaTranferBuffer[2] = {0};
//	
//	FMC_ReadBuffer(g_dwRecCodeValidAddr,&dwcntRecCode,sizeof(dwcntRecCode));
//	if(dwcntRecCode == 0xFFFFFFFF){		//����ʹ��
//		FMC_Erase(g_dwRecCodeValidAddr);
//		dwcntRecCode = 1;
//		FMC_WriteBuffer(g_dwRecCodeValidAddr,&dwcntRecCode,sizeof(dwcntRecCode));
//		memset(dwaTranferBuffer,0xFF,sizeof(dwaTranferBuffer));
//		dwaTranferBuffer[0] = dwRecCode;
//		dwaTranferBuffer[1] = (uint32_t)FindIdleCluster;
//		FMC_WriteBuffer(g_dwRecCodeValidAddr,dwaTranferBuffer,sizeof(dwaTranferBuffer));
//		return (g_dwRecCodeValidAddr+8);
//	}else{
//		if(g_dwRecCodeValidAddr == g_dwRecCodeAddr)
//			dwRecCodeUnvalidAddr = g_dwRecBackAddr;
//		else
//			dwRecCodeUnvalidAddr = g_dwRecCodeAddr;
//		
//		FMC_Erase(dwRecCodeUnvalidAddr);
//		dwcntRecCode++;
//		FMC_WriteBuffer(dwRecCodeUnvalidAddr,&dwcntRecCode,sizeof(dwcntRecCode));
//		for(dwTempAddr=(g_dwRecCodeValidAddr+8); dwTempAddr < g_dwRecCodeValidAddr+(dwcntRecCode+1)*8; dwTempAddr+=8){
//			FMC_ReadBuffer(dwTempAddr,dwaTranferBuffer,sizeof(dwaTranferBuffer));
//			if(dwaTranferBuffer[0] == dwRecCode)
//				continue;
//			FMC_WriteBuffer(dwRecCodeUnvalidAddr,dwaTranferBuffer,sizeof(dwaTranferBuffer));	
//		}
//		dwaTranferBuffer[0] = dwRecCode;
//		dwaTranferBuffer[1] = (uint32_t)FindIdleCluster;
//		FMC_WriteBuffer(g_dwRecCodeValidAddr,dwaTranferBuffer,sizeof(dwaTranferBuffer));
//		return (g_dwRecCodeValidAddr + (dwcntRecCode+1)*8);
//	}
//	
//}
uint8_t CodeIsExist(int32_t dwRecCode, uint16_t* wCodeIndex)		//�������ԣ��к��޸�CODE����������ֵ
{
	uint32_t dwTempCode;
	uint16_t iFindCode = 1;
	LOG(("JUDGE %d IS EXIST?\r\n",dwRecCode));
	//��һ��8�ֽ�������
	FMC_ReadBuffer(DATAF_BASE_ADDR+iFindCode*8,&dwTempCode,sizeof(dwTempCode));
	while(dwTempCode != 0xFFFFFFFF){
		if(dwRecCode == dwTempCode){
			*wCodeIndex = iFindCode;
			LOG(("EXIST!\r\n"));
			return iFindCode;
		}
		iFindCode++;
		FMC_ReadBuffer(DATAF_BASE_ADDR+iFindCode*8,&dwTempCode,sizeof(dwTempCode));
	}
	*wCodeIndex = iFindCode;
	LOG(("Not EXIST!\r\n"));
	return 0;
}
/*----------------------------------------------------------------------------------------
������: PrepareForRecord
����:
		dwRecCode: ¼������Ӧ����ֵ
����ֵ:
		1/0�� ׼���ɹ�/׼��ʧ��
����:
		��ȡ¼��������ȷ��¼����ַ���ҵ�¼����ֵ��������ʼ����������������ʾ������ʼ��VAD
----------------------------------------------------------------------------------------*/
static int8_t PrepareForRecord(int32_t dwRecCode, uint32_t* dwRecIndexIdleStartAddrBack)
{
	uint32_t dwSourceSize = 0;
	uint32_t dwUSEDFlag = 0;
	int32_t  dwIdleCluster = 0;
	uint16_t wCodeIndex = 0;
//	uint32_t waInitIndex[64];
//	uint16_t iInitIndex = 0;
	
	//׼������ȫ�ֱ���
	FMD_ReadData(&dwSourceSize, RECORD_START, sizeof(dwSourceSize));				//0x10000��Ϊ����Դ��С
	g_dwRecDataStartAddr = (dwSourceSize/FLASH_MIN_ERASE_SIZE + 1) * FLASH_MIN_ERASE_SIZE;	//���������ڲ�FLASH�����Դ˴�Ϊ���ݿ�Ŀ�ʼ
	
	//FMC����
	g_dwRecCodeAddr = DATAF_BASE_ADDR;
	g_dwRecBackAddr = g_dwRecCodeAddr + REC_CODE_SIZE;
	g_dwRecIndexAddr = g_dwRecBackAddr + REC_CODE_SIZE;
	
	LOG(("g_dwAddr:%x\r\n",g_dwRecDataStartAddr));
	
//	FMC_ReadBuffer(g_dwRecIndexAddr,waInitIndex,sizeof(waInitIndex));
//	for(iInitIndex = 0; iInitIndex < 64; iInitIndex++)
//		LOG("waInitIndex:%x\r\n",waInitIndex[iInitIndex]);
		
	FMC_ReadBuffer((g_dwRecBackAddr-8),&g_dwRecIndexIdleStartAddr,4);
	FMC_ReadBuffer((g_dwRecBackAddr-4),&g_dwRecIndexIdleEndAddr,4);
	
	LOG(("ALLOCATE CLUSTER ADDR:\r\ng_dwRecIndexIdleStartAddr:%d\tg_dwRecIndexIdleEndAddr:%d\r\n",g_dwRecIndexIdleStartAddr,g_dwRecIndexIdleEndAddr));
	
	FMC_ReadBuffer(g_dwRecCodeAddr,&dwUSEDFlag,4);
	if(dwUSEDFlag != 0x44455355){		//USED��ASCII��
		InitIndex();
	}
//	FMC_ReadBuffer(g_dwRecIndexAddr,waInitIndex,sizeof(waInitIndex));
//	for(iInitIndex = 0; iInitIndex < 64; iInitIndex++)
//		LOG("waInitIndex:%x\r\n",waInitIndex[iInitIndex]);
	
//	if((g_dwRecIndexIdleStartAddr == 0xFFFFFFFF) || (g_dwRecIndexIdleEndAddr == 0xFFFFFFFF)){
//		FMC_ReadBuffer((g_dwRecIndexAddr-8),&g_dwRecIndexIdleStartAddr,4);
//		FMC_ReadBuffer((g_dwRecIndexAddr-4),&g_dwRecIndexIdleEndAddr,4);
//		if((g_dwRecIndexIdleStartAddr == 0xFFFFFFFF) || (g_dwRecIndexIdleEndAddr == 0xFFFFFFFF)){
//			InitIndex();						//��ַ��ϵͳ��ʼֵ����ʼ��������
//			g_dwRecCodeValidAddr = g_dwRecCodeAddr;
//			g_dwRecCodeUnvalidAddr = g_dwRecBackAddr;
//		}else{
//			g_dwRecCodeValidAddr = g_dwRecBackAddr;
//			g_dwRecCodeUnvalidAddr = g_dwRecCodeAddr;
//		}
//	}else{
//		g_dwRecCodeValidAddr = g_dwRecCodeAddr;
//		g_dwRecCodeValidAddr = g_dwRecBackAddr;
//	}
//	FMC_Erase(g_dwRecCodeUnvalidAddr);
	if(CodeIsExist(dwRecCode, &wCodeIndex)){
		ErarseRecord(wCodeIndex);			//�������ⲿFLASH�����䶯�ڲ�FLASH
	}
	*dwRecIndexIdleStartAddrBack = g_dwRecIndexIdleStartAddr; 
	dwIdleCluster = FindIdleCluster();				//�����һ��
	if(dwIdleCluster == -1)
		return 0;
	g_dwRecWriteAddr = g_dwRecDataStartAddr + dwIdleCluster*FLASH_MIN_ERASE_SIZE;			//�������������޸�
	g_dwRecWriteHeaderAddr = g_dwRecWriteAddr;	//ͷҲд������
	
	PlayBegin(PEN_RECTIP_AUDIO);				//ע�������ٻ���esrvars���еı������ر���g_sHeader
	while(!PlayWork(0,0,0));
	
	g_sHeader.rec_audio_size = 0xFFFFFFFF;		//�����һ�εĴ�С
	g_sHeader.rec_max_value = 0xFFFF;		   	//�����һ�ε�¼�����ֵ
	memset(g_waRecHeader,0xFFFF,sizeof(g_waRecHeader));
	g_nRecHeader = (uint8_t)sizeof(g_sHeader)/4;
//	g_waRecHeader[g_nRecHeader++] = wIdleCluster;
	
	g_cntHaveData = 0;
	g_wSpiCurPosition = 0;
	g_nSpiIndex = 0;
	g_bFirstAreaChecked = 0;
	ivADPCM_InitCoder(&tagAdpcmEncoder);	

	if(!InitEsr(objramForVAD, OBJ_RAM_SIZE_FOR_VAD, g_pu8ResDataForVAD)){
		LOG(("Esr Init Failed!\r\n"));
		return 0;
	}
	FMD_EraseBlock(g_dwRecWriteAddr);
	g_dwRecWriteAddr += FLASH_PAGE_SIZE;
	return 1;	
}
//*----------------------------------------------------------------------------------------
//������: WriteRecHeader
//����:
//		dwRecCode: ¼������Ӧ����ֵ
//����ֵ:
//		i: ��Ϊ��ǰ��ֵ��¼���������е�λ�ã�û���ҵ���ֵ����û�ж���ռ䴴������-1
//����:
//		������ֵ�ҵ����������е�λ�ã�û��ʱ���´���һ��λ��
//----------------------------------------------------------------------------------------*
void WriteRecHeader(uint8_t bFirstHeader)
{
	int32_t *pHeader = (int32_t *)&g_sHeader;
	uint8_t iTranfer; 

	if(bFirstHeader){
		for(iTranfer=2; iTranfer<sizeof(RECHEADER)/4; iTranfer++){
			g_waRecHeader[iTranfer] = *(pHeader + iTranfer);
		}
	}
	FMD_WriteData(g_waRecHeader,g_dwRecWriteHeaderAddr);
}
//*----------------------------------------------------------------------------------------
//������: ModifyCodeFirstIndex
//����:
//		dwRecCode: ¼������Ӧ����ֵ
//����ֵ:
//		i: ��Ϊ��ǰ��ֵ��¼���������е�λ�ã�û���ҵ���ֵ����û�ж���ռ䴴������-1
//����:
//		������ֵ�ҵ����������е�λ�ã�û��ʱ���´���һ��λ��
//----------------------------------------------------------------------------------------*
void ModifyCodeFirstIndex(int32_t dwRecCode, int32_t dwRecFirstClusterIndex)
{
	int32_t aTranferBuffer[2];
	uint8_t iRead,iWrite = 0;
	uint8_t bHaveCode = 0;
	uint16_t wTranferBufferSize = sizeof(aTranferBuffer);
	uint16_t wCodeIndex = 0;
	
	bHaveCode = CodeIsExist(dwRecCode,&wCodeIndex);
	
	FMC_Erase(g_dwRecBackAddr);
	for(iRead=0; iRead<127; iRead++){		//16?256?4K
		FMC_ReadBuffer((g_dwRecCodeAddr + iRead*wTranferBufferSize), (uint32_t*)aTranferBuffer, wTranferBufferSize);
		if(bHaveCode && iRead == wCodeIndex){
			continue;
		}
		FMC_WriteBuffer(g_dwRecBackAddr+(iWrite++)*wTranferBufferSize, (uint32_t*)aTranferBuffer, wTranferBufferSize);
	}
	aTranferBuffer[0] = g_dwRecIndexIdleStartAddr;		//�޸���������ʼ�ͽ�����ַ
	aTranferBuffer[1] = g_dwRecIndexIdleEndAddr;
	FMC_WriteBuffer(g_dwRecBackAddr+127*wTranferBufferSize, (uint32_t*)aTranferBuffer, wTranferBufferSize);
	FMC_Erase(g_dwRecCodeAddr);
	for(iRead=0; iRead<128; iRead++){
		FMC_ReadBuffer((g_dwRecBackAddr+iRead*wTranferBufferSize), (uint32_t*)aTranferBuffer, wTranferBufferSize);
		FMC_WriteBuffer(g_dwRecCodeAddr+iRead*wTranferBufferSize, (uint32_t*)aTranferBuffer, wTranferBufferSize);
	}
	
	if(dwRecFirstClusterIndex != 0xFFFFFFFF){
		if(!CodeIsExist(dwRecCode,&wCodeIndex)){
			LOG(("wCodeIndex:%d\r\n",wCodeIndex));
			aTranferBuffer[0] = dwRecCode;
			aTranferBuffer[1] = dwRecFirstClusterIndex;
			FMC_WriteBuffer(g_dwRecCodeAddr+wCodeIndex*8, (uint32_t*)aTranferBuffer, wTranferBufferSize);
			FMC_ReadBuffer(g_dwRecCodeAddr+wCodeIndex*8, (uint32_t*)aTranferBuffer,4);
			LOG(("aTranferBuffer:%x\r\n",aTranferBuffer[0]));
		}else{
			LOG(("modify first index error!!"));
		}
	}
}
/*----------------------------------------------------------------------------------------
������: RecordBegin
����:
		dwRecCode: ¼������Ӧ����ֵ
����ֵ:
		None
����:
		�����������ֵ����¼��
----------------------------------------------------------------------------------------*/
void RecordBegin(int32_t dwRecCode, REC_MODE RecMode)
{
	ivESRStatus nStatus = 0;
	PCEsrResult pResult = ivNull;
	ivUInt32 dwMsg = 0;
	KEYMSG 	 msg;
	uint16_t  ncntPage = 0;
	uint16_t cntWritedPage = 0;
	int32_t dwIdleCluster = 0, dwRecFirstClusterIndex = 0;
	uint8_t bEraseFlag = 1,bFirstHeader = 1;
	uint32_t dwRecIndexIdleStartAddrBack = 0;
	uint8_t iTranfer, cntWritedBlock = 1;
	int32_t *pHeader = 0;
	g_bEsrPure = 0;

	if((dwRecCode < REC_CODE_START) || (dwRecCode > REC_CODE_END)){		//��ֵ����¼����Χ��
		LOG(("Failed code for record!"));
		g_nState = PLAY_STATE;
		return ;
	}
	if(!PrepareForRecord(dwRecCode, &dwRecIndexIdleStartAddrBack)){
		LOG(("Prepare for record failed!"));
		g_nState = PLAY_STATE;
		return ;
	}	
	
	if(RecMode == REC_MAVO)
		g_nState = RECORD_STATE;

    while(g_nState == RECORD_STATE && cntWritedPage < PAGE_NUM){
		g_bHaveRecAction = 1;
		if(dwMsg == 0){
			ESRRunStep(pEsrObj, dwMsg, &nStatus, &pResult);
		}

        if(ES_STATUS_FINDSTART == nStatus) {
			nStatus = 5;														//��㸳ֵһ����Ӱ������ֵ
			LedFlashForEsr(LED_ON);
			if(g_cntHaveData > MAX_SPIBUFFERCOUNT){
				g_wSpiCurPosition = ((g_cntHaveData + 2) % MAX_SPIBUFFERCOUNT);	//����д��spi_wirte+1��BUFFER���ܻ������g_cntHaveData+2��BUFFER
				g_cntHaveData = MAX_SPIBUFFERCOUNT - 2;							//һ������ʹ�ã�һ��������ʹ��
			}
			g_sHeader.rec_encoder.m_nPrevVal = (g_dwaEncodeHeader[g_wSpiCurPosition]>>16) & 0xFFFF;
			g_sHeader.rec_encoder.m_nIndex 	 =	g_dwaEncodeHeader[g_wSpiCurPosition] & 0xFFFF; 
            dwMsg = ES_MSG_RESERTSEARCHR;
			g_bNoEsr = 1;
			ncntPage = 1;			//��һ��4K�ĵ�һ��ҳ��4K����ռ��
			bEraseFlag = 0;
			LedFlashForEsr(LED_ON);
        }
		if(dwMsg == ES_MSG_RESERTSEARCHR){		//VAD��⵽��ʼ		
			if (g_cntHaveData > 0)
			{
				FMD_WriteData(g_waToFlashBuffer[g_wSpiCurPosition],g_dwRecWriteAddr);
				g_dwRecWriteAddr += MAX_SPIBUFFERSIZE;
				g_wSpiCurPosition = (g_wSpiCurPosition + 1)	% MAX_SPIBUFFERCOUNT;
				g_cntHaveData--;
				cntWritedPage++;		//�ܹ�д��page��
	
				ncntPage++;		//��ǰblock�е�page��
				if(ncntPage >= 256){
					g_bHaveRecAction = 1;
					if(g_nRecHeader == 63){
						cntWritedBlock++;		//�ܹ�ռ�ö��ٸ�block
						dwIdleCluster = FindIdleCluster();
						if(dwIdleCluster == -1)
							break;
						g_waRecHeader[g_nRecHeader] = dwIdleCluster;
						WriteRecHeader(bFirstHeader);				//д���һ��ͷ
						bFirstHeader = 0;
						g_dwRecWriteAddr = g_dwRecDataStartAddr + dwIdleCluster*FLASH_MIN_ERASE_SIZE;
						FMD_EraseBlock(g_dwRecWriteAddr);
						g_dwRecWriteHeaderAddr = g_dwRecWriteAddr;
						g_dwRecWriteAddr += FLASH_PAGE_SIZE;
						memset(g_waRecHeader,0xFF,sizeof(g_waRecHeader));
						g_nRecHeader = 0;
						ncntPage = 1;
						
						LOG(("RecHeader full, allocate new memory!!"));			//�����������Ĵ���
					}else{
						ncntPage = 0;
						dwIdleCluster = FindIdleCluster();
						if(dwIdleCluster == -1)
							break;
						g_dwRecWriteAddr = g_dwRecDataStartAddr + dwIdleCluster*FLASH_MIN_ERASE_SIZE;
						FMD_EraseBlock(g_dwRecWriteAddr);
						g_waRecHeader[g_nRecHeader++] = dwIdleCluster;
					}
				}
			}		
		}
		if(KEY_MsgGet(&msg)){
			g_bHaveKeyAction = 1;
			if(RecMode == REC_PURE){
				if(msg.Key_MsgValue == KEY_RECORD){
					if(msg.Key_MsgType == KEY_TYPE_UP){
						LOG(("RECORD END!!"));
						g_nState = PLAY_STATE;
					}
				}
			}else if(RecMode == REC_MAVO){
				if(msg.Key_MsgValue == KEY_MACESR){
					if(msg.Key_MsgType == KEY_TYPE_UP){
						LOG(("MACREC END!!"));
						g_nState = PLAY_STATE;
					}
				}
			}
			if(msg.Key_MsgValue == KEY_ON_OFF){
//			 	if(msg.Key_MsgType == KEY_TYPE_SP){	//����Ƭ��
//					ErarseRecord(dwRecCode);
//					bEraseFlag = 1;
//					g_nState	= PLAY_STATE;
//				}else 
				if(msg.Key_MsgType == KEY_TYPE_LP){
					g_nState = PWRDOWN_STATE;	
				}
			}else if(msg.Key_MsgValue == KEY_TOUCH){
				if(RecMode == REC_MAVO){
					if(msg.Key_MsgType == KEY_TYPE_UP){
						g_nState = PLAY_STATE;
					}
				}
			}
		}
    }
//	DrvPDMA_Close();
	ADC_Term();
	g_bNoEsr = 0;
	g_sHeader.rec_audio_size = cntWritedPage * FLASH_PAGE_SIZE - cntWritedBlock*FLASH_PAGE_SIZE;

	if(bEraseFlag){
		LedFlashForEsr(LED_ON);
		g_dwRecIndexIdleStartAddr = dwRecIndexIdleStartAddrBack;	//����ǲ������Ͱѷ����ַ��ԭ������֮ǰ��״̬��
		ModifyCodeFirstIndex(dwRecCode, (int32_t)0xFFFFFFFF);
	}else{
		WriteRecHeader(bFirstHeader);
		FMC_ReadBuffer(dwRecIndexIdleStartAddrBack, (uint32_t*)&dwRecFirstClusterIndex,4);
		ModifyCodeFirstIndex(dwRecCode,dwRecFirstClusterIndex);
		
		pHeader = (int32_t*)&g_sHeader;
		memset(g_waRecHeader,0xFF,sizeof(g_waRecHeader));
		for(iTranfer=0; iTranfer<2; iTranfer++){
			g_waRecHeader[iTranfer] = *(pHeader + iTranfer);
		}
		FMD_WriteData(g_waRecHeader,g_dwRecDataStartAddr + dwRecFirstClusterIndex*FLASH_MIN_ERASE_SIZE);
	}
//	ModifyIndexPointer();		//����ʱ�޸�����������ַ
	LedFlashForEsr(LED_OFF);
}
/*----------------------------------------------------------------------------------------
������: CBPdmdForAdc
����:
		None
����ֵ:
		None
����:
		ADC�Ļص���������ʼʱ��ʶ�����ݣ�����ѹ�����ݲ�д��g_pWriteBuffer
----------------------------------------------------------------------------------------*/
void CBPdmdForAdc(void)
{
	EsErrID err;
	int32_t dwTemp = 0;
	uint8_t iFindMax = 0;
	uint8_t bCurBufferBak = g_bCurBuffer;
	static uint8_t	cntDiscardStart = 0;
//	static int16_t Test = 0,TestFlag = 0;
//	int16_t i = 0;
	
	g_bCurBuffer ^= 1;
	PDMA->channel[eDRVPDMA_CHANNEL_0].DAR = (uint32_t)&g_waAdcSwitchBuffer[g_bCurBuffer][0];
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);
	
	if(cntDiscardStart > 0)	cntDiscardStart--; 	//����ǰ15��ҳ120ms����ȥħ����ͷ�����Ӳ���Ӱ�죻
	else
	{
		if(!g_bNoEsr){
			err = ESRAppendData(pEsrObj, g_waAdcSwitchBuffer[bCurBufferBak], MAX_FRAME_SIZE);
			if (err != ivESR_OK)
			{
				LOG(("!"));
			}
		}
		else
		for(iFindMax = 0; iFindMax < MAX_FRAME_SIZE; iFindMax++)
		{
//			if(!TestFlag)
//				g_waAdcSwitchBuffer[bCurBufferBak][iFindMax] = Test++;
//			else
//				g_waAdcSwitchBuffer[bCurBufferBak][iFindMax] = Test--;	
//			if(Test == 30000)	TestFlag = 1;
//			if(Test == -30000)	TestFlag = 0;
			dwTemp = g_waAdcSwitchBuffer[bCurBufferBak][iFindMax];
			dwTemp = dwTemp>0 ? dwTemp : -dwTemp;
			if(dwTemp > g_sHeader.rec_max_value)	
				g_sHeader.rec_max_value = dwTemp;
		}
	
		if(!g_bEsrPure){
			ivADPCM_Encode(&tagAdpcmEncoder, g_waAdcSwitchBuffer[bCurBufferBak], MAX_FRAME_SIZE, (uint8_t *)g_pWriteBuffer);
			g_pWriteBuffer += MAX_FRAME_SIZE / 4;
			g_nSpiIndex++;
			if(g_nSpiIndex == (MAX_SPIBUFFERSIZE/2) / (MAX_FRAME_SIZE/4)){
				g_nSpiIndex = 0;
				g_cntHaveData++;
				g_dwaEncodeHeader[(g_wSpiCurPosition + g_cntHaveData) % MAX_SPIBUFFERCOUNT] = (int32_t)tagAdpcmEncoder.m_nPrevVal << 16;
				g_dwaEncodeHeader[(g_wSpiCurPosition + g_cntHaveData) % MAX_SPIBUFFERCOUNT] += tagAdpcmEncoder.m_nIndex;
				g_pWriteBuffer = (int16_t *)g_waToFlashBuffer[(g_wSpiCurPosition + g_cntHaveData) % MAX_SPIBUFFERCOUNT];
	//			LOG("buffer:%d,%d,%d\r\n",(g_wSpiCurPosition + g_cntHaveData) % MAX_SPIBUFFERCOUNT,g_wSpiCurPosition,g_cntHaveData);
			}
		}
	}
}
/*----------------------------------------------------------------------------------------
������: CBPdmdForAdc
����:
		None
����ֵ:
		None
����:
		ADC�Ļص���������ʼʱ��ʶ�����ݣ�����ѹ�����ݲ�д��g_pWriteBuffer
----------------------------------------------------------------------------------------*/
void PdmaForAdc(void)
{
	STR_PDMA_T sPDMA;

	DrvPDMA_Init();
	g_bCurBuffer = 0;
	g_pWriteBuffer = (int16_t *)g_waToFlashBuffer[(g_wSpiCurPosition + g_cntHaveData) % MAX_SPIBUFFERCOUNT];
	
	// CH2 ADC RX Setting 
	sPDMA.sSrcAddr.u32Addr          = (uint32_t)&SDADC->ADCOUT; 
	sPDMA.sDestAddr.u32Addr         = (uint32_t)&g_waAdcSwitchBuffer[g_bCurBuffer][0];
	sPDMA.u8Mode                    = eDRVPDMA_MODE_APB2MEM;
	sPDMA.u8TransWidth              = eDRVPDMA_WIDTH_16BITS;
	sPDMA.sSrcAddr.eAddrDirection   = eDRVPDMA_DIRECTION_FIXED; 
	sPDMA.sDestAddr.eAddrDirection  = eDRVPDMA_DIRECTION_INCREMENTED;   
	sPDMA.i32ByteCnt				= MAX_FRAME_SIZE * sizeof(int16_t);
	DrvPDMA_Open(eDRVPDMA_CHANNEL_0, &sPDMA);
	
	// PDMA Setting 
	DrvPDMA_SetCHForAPBDevice(
	    eDRVPDMA_CHANNEL_0, 
	    eDRVPDMA_ADC,
	    eDRVPDMA_READ_APB    
	);
	
	// Enable INT 
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_0, eDRVPDMA_BLKD );
	    
	// Install Callback function    
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_0, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK)CBPdmdForAdc ); 
	
	// Enable ADC PDMA and Trigger PDMA specified Channel 
	DrvADC_PdmaEnable();
	
	// Start A/D conversion 
	DrvADC_StartConvert();
	
	// start ADC PDMA transfer
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_0);
}
