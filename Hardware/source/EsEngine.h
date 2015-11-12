/*********************************************************************#
//	�ļ���		��ESREngine.h
//	�ļ�����	��
//	����		��Truman
//	����ʱ��	��2007��8��1��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/


#if !defined(ES_TEAM__2007_07_10__ESKERNEL_ESRENGINE__H)
#define ES_TEAM__2007_07_10__ESKERNEL_ESRENGINE__H

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#include "EsSearch.h"
#include "ESFront.h"
#include "ivESR.h"

#define DW_ENGINE_CHECK			0x20070814

#define ESENGINE_REC			(309) /* ����ʶ������ */
#define ESENGINE_TAG			(302) /* ����������ǩ���� */

typedef struct tagResidentRAMHdr
{
    ivUInt32 dwCheck;
    ivUInt32 nCMNCRC;
    ivInt16	 ps16CMNMean[TRANSFORM_CEPSNUM_DEF+1];
}TResidentRAMHdr, ivPtr PResidentRAMHdr;

struct tagEsrEngine					
{
	TESRFront	tFront;				
	TEsSearch	tSearch;			

	ivUInt32	dwCheck;			

    PResidentRAMHdr pResidentRAMHdr;

	/* Buffer for Allocation */
	ivPUInt16	pBufferBase;		
	ivPUInt16	pBufferEnd;			
	ivPUInt16	pBuffer;			

    ivUInt16	iEngineType;
    ivUInt16    iEngineStatus;

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT	
	ivUInt16	nTagID;				
	ivPointer	pTagCmdBuf;			

    ivPointer   pSPIBufBase;    /* �û������SPIBuf��Ϣ */
    ivPointer   pSPIBufEnd;     /* �û������SPIBuf��Ϣ */
#endif

	ivUInt16    nMinSpeechFrm;		/* �������֡��.������margin(SPEECH_END_MARGIN) */
    ivUInt16	bRun;			
};

EsErrID EsInit(PEsrEngine pEngine,ivCPointer pModel);

EsErrID EsValidate(PEsrEngine pEngine);
EsErrID EsReset(PEsrEngine pEngine);
EsErrID EsRunStep(PEsrEngine pEngine, ivUInt32 dwMessage, ivESRStatus ivPtr pStatus, PCEsrResult ivPtr ppResult);

void EsOutputResult(PEsrEngine pEngine, PEsrResult ppResult);

ivPUInt16 EsAlloc(PEsrEngine pEngine,ivUInt16 nSize);

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT
EsErrID EsTagOutputResult(PEsrEngine pEngine);
#endif

#endif /* !defined(ES_TEAM__2007_07_10__ESKERNEL_ESRENGINE__H) */
