/*********************************************************************#
//	�ļ���		��ivESR.c
//	�ļ�����	��
//	����		��Truman
//	����ʱ��	��2007��8��9��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/

#include "EsKernel.h"
#include "ivESR.h"
#include "EsEngine.h"
#include "EsKernel.c"

/* Parameter ID for ESR Disable VAD */
#define ES_PARAM_DISABLEVAD				(7)
#define ES_DISABLEVAD_ON			((ivCPointer)1)
#define ES_DISABLEVAD_OFF			((ivCPointer)0)
#define ES_DEFAULT_DISABLEVAD		ES_DISABLEVAD_OFF/* yfhu.added at [2009-4-8 9:37:08] */

#define DW_OBJ_CHECK			(0x20091009)

#define DW_RESIDENT_RAM_CHECK	(0x20100914) 

#if MINI5_ENHANCE_VAD
extern ivUInt16 g_bEnhanceVAD;
#endif /* MINI5_ENHANCE_VAD */

extern ivUInt16 g_bChkAmbientNoise;

#if RATETSET_USE_VAD
extern ivUInt16 g_bDisableVad;
#endif

EsErrID ivCall ESRCreate(ivPointer pEsrObj, ivSize ivPtr pnESRObjSize, ivPointer pResidentRAM, ivPUInt16 pnResidentRAMSize, ivCPointer pResource, ivUInt32 nGrmID)
{
	PEsrEngine pEngine;
	EsErrID err;
	ivPUInt32 pTmp;

	ivCPointer  pModel;
	PGrmMdlHdr  pGrmMdlHdr;
	ivCPointer  pLexicon = ivNull;
    PGrmDesc    pGrmDesc = ivNull;
	PLexiconHeader pLexHdr;

	ivSize nSize;
	ivPUInt16 pBuffer;
	ivUInt32 nCounter =0, nCRC = 0;
	ivPInt16 ps16CMNMean;

	ivUInt32 i;

    PResidentRAMHdr pResidentHdr;

#if MINI5_SPI_MODE
    ivPointer   pSPIBuf;
#endif

#if MINI5_API_PARAM_CHECK
    if(ivNull == pEsrObj || ivNull == pnESRObjSize || ivNull == pResidentRAM 
        || ivNull == pnResidentRAMSize || ivNull == pResource) {
            ivAssert(ivFalse);
            return EsErr_InvArg;
    }
#endif
    /* Memory align */
    pEsrObj = (ivPointer)(((ivAddress)pEsrObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));	 

#if MINI5_SPI_MODE
    /* pResource����SPI�� */
    err = EsSPIReadBuf(pEsrObj, pResource, sizeof(TGrmMdlHdr));
    if (EsErrID_OK != err) {
        ivAssert(ivFalse);
        return EsErr_Failed;
    }
    pGrmMdlHdr = (PGrmMdlHdr)pEsrObj;
#else
	pGrmMdlHdr = (PGrmMdlHdr)pResource;
#endif
	if(DW_MINI_RES_CHECK != pGrmMdlHdr->dwCheck) {
        ivAssert(ivFalse);
		return EsErr_InvRes;
	}

#if MINI5_SPI_MODE
    pSPIBuf = (ivPointer)((ivAddress)pResource+sizeof(TGrmMdlHdr)+sizeof(TGrmDesc)*(pGrmMdlHdr->nTotalGrm-1));
    ivAssert(sizeof(TEsrEngine) >= ivGridSize(sizeof(TGrmMdlHdr)) + MINI5_SPI_PAGE_SIZE);
    pBuffer = (ivPUInt16)((ivAddress)pEsrObj + ivGridSize(sizeof(TGrmMdlHdr)));

    err = EsSPICalcCRC((ivPointer)pBuffer, pSPIBuf, pGrmMdlHdr->nCRCCnt * sizeof(ivInt16), &nCRC, &nCounter);
    ivAssert(EsErrID_OK == err);
    if (EsErrID_OK != err) {
        return EsErr_Failed;
    }
#else
	ivMakeCRC((ivPointer)((ivAddress)pResource+sizeof(TGrmMdlHdr)+sizeof(TGrmDesc)*(pGrmMdlHdr->nTotalGrm-1)), pGrmMdlHdr->nCRCCnt * sizeof(ivInt16), &nCRC, &nCounter);
#endif
	if(nCRC != pGrmMdlHdr->nCRC) {
        ivAssert(ivFalse);
        return EsErr_InvRes;
	}

#if MINI5_API_PARAM_CHECK
    nSize = ivGridSize(sizeof(TEsrEngine) + IV_PTR_GRID);   /* Obj size */
    nSize += ivGridSize(sizeof(ivInt16)*(TRANSFORM_FFTNUM_DEF+2)); /* FFT */    
    nSize += ivGridSize(ESR_MFCC_BACK_FRAMES*(TRANSFORM_CEPSNUM_DEF+1)*sizeof(ivInt16)); /* MFCC Buffer */
	nSize += ivGridSize(ESR_HIPASSENERGY_NUM * sizeof(ivInt32));    /* For Filter BandPass */

    #if ESR_CPECTRAL_SUB
    nSize += ivGridSize((TRANSFORM_HALFFFTNUM_DEF+1) * sizeof(ivUInt16) * 3);
    #endif /* #if ESR_CPECTRAL_SUB */
#endif

	/* Set lexicon */
	ivAssert(pGrmMdlHdr->nTotalGrm >= 1);
	if(0 == pGrmMdlHdr->nTotalGrm) {
        ivAssert(ivFalse);
        return EsErr_InvRes;
	}

#if MINI5_SPI_MODE
    pGrmDesc = (PGrmDesc)((ivAddress)pEsrObj + ivGridSize(sizeof(TGrmMdlHdr)));
    pSPIBuf = (ivPointer)((ivAddress)pResource+sizeof(TGrmMdlHdr) -sizeof(TGrmDesc));
    err = EsSPIReadBuf((ivPointer)pGrmDesc, pSPIBuf, sizeof(TGrmDesc)*(pGrmMdlHdr->nTotalGrm));
#endif

	for(i=0; i<pGrmMdlHdr->nTotalGrm; i++) {
#if !MINI5_SPI_MODE
        pGrmDesc = &(pGrmMdlHdr->tGrmDesc[i]);
#endif
        if(nGrmID == pGrmDesc->nGrmID) {
            pLexicon = (ivCPointer)((ivAddress)pResource + pGrmDesc->nGrmOffset * sizeof(ivInt16));
            ivAssert(pGrmDesc->nSpeechTimeOutFrm >= 0 && pGrmDesc->nSpeechTimeOutFrm <= 2000);			
            break;
        }
#if MINI5_SPI_MODE
        pGrmDesc = (PGrmDesc)((ivAddress)pGrmDesc + sizeof(TGrmDesc));
#endif
	}
	if(i >= pGrmMdlHdr->nTotalGrm) {
        ivAssert(ivFalse);
        return EsErr_Failed;
	}

#if MINI5_SPI_MODE
    ivMemCopy(pGrmMdlHdr->tGrmDesc, pGrmDesc, sizeof(TGrmDesc));
    pGrmDesc = pGrmMdlHdr->tGrmDesc;

    pBuffer = (ivPUInt16)((ivAddress)pEsrObj + ivGridSize(sizeof(TGrmMdlHdr)));
    ivAssert(sizeof(TEsrEngine) >= ivGridSize(sizeof(TGrmMdlHdr)) + sizeof(TLexiconHeader));

    err = EsSPIReadBuf((ivPointer)pBuffer, pLexicon, sizeof(TLexiconHeader));
    ivAssert(EsErrID_OK == err);
    if (EsErrID_OK != err) {
        return EsErr_Failed;
    }
    pLexHdr = (PLexiconHeader)pBuffer;
#else
	pLexHdr = (PLexiconHeader)pLexicon;
#endif
	ivAssert(DW_LEXHEADER_CHECK == pLexHdr->dwCheck);
	if(DW_LEXHEADER_CHECK != pLexHdr->dwCheck) {
        ivAssert(ivFalse);
        return EsErr_InvRes;
	}

#if MINI5_API_PARAM_CHECK
	nSize += sizeof(TSearchNode) * (pLexHdr->nTotalNodes + 1);
	if(*pnESRObjSize < nSize) {
		*pnESRObjSize = nSize;
        ivAssert(ivFalse);
        return EsErr_OutOfMemory;
	} 

    if(*pnResidentRAMSize < sizeof(TResidentRAMHdr)) {
        /* Check buf size if enough */
        *pnResidentRAMSize = sizeof(TResidentRAMHdr); 
        ivAssert(ivFalse);
        return EsErr_OutOfMemory;
    }
#endif

	pTmp = (ivPUInt32)pEsrObj;
	*pTmp = DW_OBJ_CHECK;
	pEsrObj = (ivPointer)(pTmp+1);

	pBuffer = (ivPUInt16)pEsrObj;

	pEngine = (PEsrEngine)pBuffer;
	pEngine->tFront.m_nSpeechTimeOut = (ivInt32)(pGrmDesc->nSpeechTimeOutFrm); /* Speech time out frame */
	pEngine->nMinSpeechFrm = (ivUInt16)(pGrmDesc->nMinSpeechFrm);
	pBuffer = (ivPUInt16)((ivAddress)pBuffer + sizeof(TEsrEngine));

	/* ResidentRAM */
    pResidentHdr = (PResidentRAMHdr)pResidentRAM;		
	pEngine->pResidentRAMHdr = pResidentHdr;

	ps16CMNMean = pResidentHdr->ps16CMNMean;
	nCRC = 0;
	nCounter = 0;
	ivMakeCRC(ps16CMNMean, sizeof(pResidentHdr->ps16CMNMean), &nCRC, &nCounter);
	if(DW_RESIDENT_RAM_CHECK != pResidentHdr->dwCheck || nCRC != pResidentHdr->nCMNCRC) {
		/* ����ĳ�פ�ڴ��Ѿ����û���д,��ʹ�ó�פ�ڴ��е�CMN������Ϣ */
		
		/* Init CMN Mean 9.6, C0:11.4 */
		for(i=0; i<TRANSFORM_CEPSNUM_DEF+1; i++) {
			ps16CMNMean[i] = g_s16CMNCoef[i];
		}

		nCRC = 0;
		nCounter = 0;
		ivMakeCRC(ps16CMNMean, sizeof(pResidentHdr->ps16CMNMean), &nCRC, &nCounter);
		pResidentHdr->nCMNCRC = nCRC;
		pResidentHdr->dwCheck = DW_RESIDENT_RAM_CHECK;
	}

	pEngine->pBufferBase = pBuffer;
	pEngine->pBuffer = pBuffer;
	pEngine->pBufferEnd = (ivPUInt16)((ivAddress)pEsrObj + *pnESRObjSize);
    pEngine->iEngineType = ESENGINE_REC;

	pModel = (ivCPointer)((ivAddress)pResource + pGrmMdlHdr->nModelOffset * sizeof(ivInt16));
	err = EsInit(pEngine, pModel);
	ivAssert(EsErrID_OK == err);
#if MINI5_API_PARAM_CHECK
	if(EsErrID_OK != err) {
		return err;
	}
#endif

	/* Set lexicon */
	err = EsSetLexicon(&pEngine->tSearch, pLexicon, pEngine);
	ivAssert(EsErrID_OK == err);
#if MINI5_API_PARAM_CHECK
	if(EsErrID_OK != err) {
		return err;
	}
#endif

	return err;
}

EsErrID ivCall ESRSetParam(ivPointer pEsrObj, ivUInt32 nParamID,ivUInt16 nParamValue) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: �����ӿ� */
{
	PEsrEngine pEngine;
    EsErrID err;

	/* Memory align */
	pEsrObj = (ivPointer)(((ivUInt32)pEsrObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));

#if MINI5_API_PARAM_CHECK
	ivAssert(ivNull != pEsrObj);
	if(ivNull == pEsrObj ){
		return EsErr_InvArg;
	}
	if(DW_OBJ_CHECK != *(ivPUInt32)pEsrObj){
		return EsErr_InvCal;
	}
#endif

	pEsrObj = (ivPointer)((ivPUInt32)pEsrObj + 1);

	pEngine = (PEsrEngine)pEsrObj;
#if MINI5_API_PARAM_CHECK
	err = EsValidate(pEngine);
	if(EsErrID_OK != err){
		return err;
	}
#endif

	switch(nParamID){
#if MINI5_ENHANCE_VAD
	case ES_PARAM_ENHANCEVAD:
		g_bEnhanceVAD = nParamValue;
		break;
#endif
	case ES_PARAM_AMBIENTNOISE:
		g_bChkAmbientNoise = nParamValue;
		break;
#if RATETSET_USE_VAD
	case ES_PARAM_DISABLEVAD:
		g_bDisableVad = nParamValue;
		break;
#endif /* RATETSET_USE_VAD */
	default:
		return EsErr_InvArg;
	}

	return EsErrID_OK;
}

EsErrID ivCall ESRReset(ivPointer pEsrObj) /* ��˶��ƽ̨AitalkMini1.0������ͬ */
{
	PEsrEngine pEngine;
    EsErrID err;

	/* Memory align */
	pEsrObj = (ivPointer)(((ivUInt32)pEsrObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));

#if MINI5_API_PARAM_CHECK
	ivAssert(ivNull != pEsrObj);
	if(ivNull == pEsrObj ){
		return EsErr_InvArg;
	}
	if(DW_OBJ_CHECK != *(ivPUInt32)pEsrObj){
		return EsErr_InvCal;
	}
#endif

	pEsrObj = (ivPointer)((ivPUInt32)pEsrObj + 1);

	pEngine = (PEsrEngine)pEsrObj;
#if MINI5_API_PARAM_CHECK
	err = EsValidate(pEngine);
	if(EsErrID_OK != err){
		return err;
	}
#endif

	return EsReset(pEngine);
}

EsErrID ivCall ESRRunStep(ivPointer pEsrObj, ivUInt32 dwMessage, ivESRStatus ivPtr pStatus, PCEsrResult ivPtr ppResult)
{
    PEsrEngine pEngine;	
    EsErrID err;

#if MINI5_API_PARAM_CHECK
    ivAssert(ivNull != pEsrObj && ivNull != pStatus && ivNull != ppResult);
    if(ivNull == pEsrObj || ivNull == pStatus || ivNull == ppResult){
        return EsErr_InvArg;
    }
#endif

    /* Memory align */
    pEsrObj = (ivPointer)(((ivUInt32)pEsrObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));	 
#if MINI5_API_PARAM_CHECK
    if(ivNull == pEsrObj ){
        return EsErr_InvArg;
    }
    if(DW_OBJ_CHECK != *(ivPUInt32)pEsrObj){
        return EsErr_InvCal;
    }
#endif

    pEsrObj = (ivPointer)((ivPUInt32)pEsrObj + 1);
    pEngine = (PEsrEngine)pEsrObj;
#if MINI5_API_PARAM_CHECK
    err = EsValidate(pEngine);
    if(EsErrID_OK != err){
        return err;
    }
#endif

    *pStatus = 0;
    *ppResult = ivNull;

    return EsRunStep(pEngine, dwMessage, pStatus, ppResult);
}

EsErrID ivCall ESRAppendData(ivPointer pEsrObj,
								  ivCPointer pData,
								  ivUInt16 nSamples		/* In samples */ 	 						 
								  )
{
	PEsrEngine pEngine;
    EsErrID err;

	/* Memory align */
	pEsrObj = (ivPointer)(((ivUInt32)pEsrObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));

#if MINI5_API_PARAM_CHECK
	ivAssert(ivNull != pEsrObj && ivNull != pData);
	if(ivNull == pEsrObj || ivNull == pData){
		return EsErr_InvArg;
	}
	if(DW_OBJ_CHECK != *(ivPUInt32)pEsrObj){
		return EsErr_InvCal;
	}

    if(nSamples > APPENDDATA_SAMPLES_MAX) {
        return EsErr_InvCal;
    }
#endif

	pEsrObj = (ivPointer)((ivPUInt32)pEsrObj + 1);

	pEngine = (PEsrEngine)pEsrObj;

	err = EsFrontAppendData(&pEngine->tFront,pData,nSamples);
	//ivAssert(EsErrID_OK == iStatus);

	return err;
}

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT

/* �������:48, 2012.5.24��� */
EsErrID ivCall ESRCreateTagObj(
								   ivPointer		pTagObj,	/* Tag Object */
								   ivSize ivPtr 	pnTagObjSize,	/* [In/Out] Size of Tag object */
								   ivCPointer  		pModel,		/* [In] Model */	
								   ivUInt16			nTagID,
								   ivPointer		pSPIBuf,		/* PC��ʱģ��.���ڴ洢ÿ֡path��feature��Ϣ��SPI��ַ*/
								   ivSize			nSPIBufSize
								   )
{
	PEsrEngine pEngine;
	EsErrID err;
	ivPUInt32 pTmp;

	ivSize nSize;
	ivPUInt16 pBuffer;
	ivInt16 i;
    PResHeader pResHeader;

	if(ivNull == pTagObj || ivNull == pnTagObjSize || ivNull == pModel || ivNull == pSPIBuf){
		ivAssert(ivFalse);
		return EsErr_InvArg;
	}

    /* Memory align */
    pBuffer = (ivPointer)(((ivUInt32)pTagObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));	

	/* -----------------------Obj Size------------------- */
	nSize = ivGridSize(sizeof(TEsrEngine) + sizeof(ivUInt32) + IV_PTR_GRID);

    /* -----------------------Front--------------------- */
    /* PCMBuffer  */
    /* nSize += ivGridSize(sizeof(ivInt16)*ESR_PCMBUFFER_SIZE); */

    /* Energy Buffer */
#if ESR_ENERGY_LOG
    nSize += ivGridSize(sizeof(ivInt16)*ESR_BACK_FRAMES);
#else
    nSize += ivGridSize(sizeof(ivInt16)*ESR_BACK_FRAMES);
#endif

    /* Frame Cache */	
    nSize += ivGridSize(sizeof(ivInt16)*(TRANSFORM_FFTNUM_DEF+2));
    /* CMN Buffer */
    nSize += ivGridSize(sizeof(ivInt16)*(TRANSFORM_CEPSNUM_DEF+1));
    nSize += ivGridSize(sizeof(ivInt16)*(TRANSFORM_CEPSNUM_DEF+1));
    nSize += ivGridSize(sizeof(ivInt32)*(TRANSFORM_CEPSNUM_DEF+1));

    /* MFCC Buffer */
    nSize += ivGridSize(ESR_MFCC_BACK_FRAMES*(TRANSFORM_CEPSNUM_DEF+1)*sizeof(ivInt16));

    /* #if ESR_ENABLE_ENHANCE_VAD */
    /* For Filter BandPass */
    nSize += ivGridSize(ESR_HIPASSENERGY_NUM * sizeof(ivInt32));

#if MINI5_SPI_MODE
    err = EsSPIReadBuf(pTagObj,pModel, sizeof(TResHeader));
    ivAssert(EsErrID_OK == err);
    if (EsErrID_OK != err) {
        return EsErr_Failed;
    }
    pResHeader = (PResHeader)pTagObj;
#else
    pResHeader = (PResHeader)pModel;
#endif
    if(DW_TAG_MODEL_CHECK != pResHeader->dwCheck) {
        return EsErr_InvRes;
    }

	/* -----------------------Search--------------------- */
	nSize += ivGridSize(sizeof(ivInt16) * ((pResHeader->nMixSModelNum-2)+4));	/* pScoreCache. -2:sil,filler */
	nSize += ivGridSize(sizeof(ivInt16) * ((pResHeader->nMixSModelNum-2)+4));   /* pnRepeatFrmCache */ 

	if(*pnTagObjSize < nSize){
		*pnTagObjSize = nSize;
		return EsErr_OutOfMemory;
	} 

	pTmp = (ivPUInt32)pBuffer;
	*pTmp = DW_OBJ_CHECK;
	pBuffer = (ivPointer)(pTmp+1);

	pEngine = (PEsrEngine)pBuffer;
	pBuffer = (ivPUInt16)((ivAddress)pBuffer + ivGridSize(sizeof(TEsrEngine)));
	pEngine->pResidentRAMHdr = (PResidentRAMHdr)pBuffer;
	
	/* Init CMN Mean 9.6, C0:11.4 */
	for(i=0; i<TRANSFORM_CEPSNUM_DEF+1; i++){
		pEngine->pResidentRAMHdr->ps16CMNMean[i] = g_s16CMNCoef[i];
	}
	
	pBuffer = (ivPUInt16)((ivAddress)pBuffer + sizeof(TResidentRAMHdr));
	pEngine->pBufferBase = pBuffer;
	pEngine->pBuffer = pBuffer;
	pEngine->pBufferEnd = (ivPUInt16)((ivAddress)pTagObj + *pnTagObjSize);
	pEngine->tFront.m_nSpeechTimeOut = (ivInt32)TAG_SPEECH_TIMEOUT; /* Speech time out frame */

	pEngine->iEngineType = ESENGINE_TAG;
	pEngine->nTagID = nTagID;
	pEngine->pSPIBufBase = pSPIBuf;
	pEngine->pSPIBufEnd = (ivPointer)((ivAddress)pSPIBuf + nSPIBufSize);
	pEngine->pTagCmdBuf = ivNull;

	err = EsInit(pEngine, pModel);
	ivAssert(EsErrID_OK == err);
	if(EsErrID_OK != err){
		return err;
	}

	return err;
}

//#define TAG_LOG
#ifdef TAG_LOG
#include <stdio.h>
ivBool ParserLexicon(ivPointer pLex, PMixStateModel pMixSModel, int nMix, char *szOptFile)
{
	PLexiconHeader pLexHdr;
	PLexNode pLexRoot, pLexNode;
	PCmdDesc pCmdDesc;
	int i, j, n, m;
	FILE *fp;
	ivPUInt16 pStateLst;

	if(NULL == pLex || NULL == pMixSModel || NULL == szOptFile){
		return ivFalse;
	}
	pLexHdr = (PLexiconHeader)pLex;
	pLexRoot = (PLexNode)((ivUInt32)pLex + pLexHdr->nLexRootOffset);
	pCmdDesc = (PCmdDesc)((ivUInt32)pLex + pLexHdr->nCmdDescOffset);

	pStateLst = (ivPUInt16)malloc(1000 * sizeof(ivInt16));

	/* ����������ת�� */
	for(i=0; i<pLexHdr->nTotalNodes/2; i++){
		TLexNode tTmpNode;
		n = i;
		m = pLexHdr->nTotalNodes - 1 - i;			
		ivMemCopy(&tTmpNode, pLexRoot+n, sizeof(TLexNode));
		ivMemCopy(pLexRoot+n, pLexRoot+m, sizeof(TLexNode));
		ivMemCopy(pLexRoot+m, &tTmpNode, sizeof(TLexNode));
	}

	fp = fopen(szOptFile, "wb");

	fprintf(fp, "nTotalNode=%d\r\n", pLexHdr->nTotalNodes);
	for(i=0; i<pLexHdr->nTotalNodes; i++){
		fprintf(fp, "i=%d, iParent=%d\r\n", i, pLexRoot[i].iParent);
	}
	fprintf(fp, "\r\n");

	j = 0;
	for(i=0; i<pLexHdr->nCmdNum; i++){
		int nState = 0;
		pLexNode = pLexRoot + pLexHdr->nTotalNodes - pLexHdr->nCmdNum + i; /* Leaf Node */
		while(1){
			pStateLst[nState++] = pLexNode->iSModeIndex;

			if(0 == pLexNode->iParent){
				break;
			}

			pLexNode = pLexRoot+ pLexNode->iParent - 1;
		}

		fprintf(fp, "CmdID=%d, Thresh=%d, bTag=%d\r\n", pCmdDesc[i].nID, pCmdDesc[i].nCMThresh, pCmdDesc[i].bTag);
		//for(j=0; j<nState; j++){
		//	fprintf(fp, "%d, ", pStateLst[nState - 1 - j]);
		//}

		//fprintf(fp,"\r\n");
		for(j=0; j<nState; j++){
			PMixStateModel pState;
			int k, idx, m, n;
			idx = pStateLst[nState - 1 -j]/ sizeof(TStateModel);
			pState = (PMixStateModel)((ivUInt32)pMixSModel + pStateLst[nState - 1 -j] );
			fprintf(fp, " iState=%d:\r\n", j);
			for(k=0; k<nMix; k++){
				fprintf(fp, "   iMix=%d, fGConst=%d, fFea=", k, pState->tGModel[k].fGConst);
				for(m=0; m<FEATURE_DIMNESION; m++){
					fprintf(fp, "%d, ", pState->tGModel[k].fMeanVar[m]);
				}
				fprintf(fp,"\r\n");
			}
		}

		fprintf(fp, "\r\n\r\n");
	}

	fclose(fp);

	/* �����������ٵ���ȥ */
	for(i=0; i<pLexHdr->nTotalNodes/2; i++){
		TLexNode tTmpNode;
		n = i;
		m = pLexHdr->nTotalNodes - 1 - i;			
		ivMemCopy(&tTmpNode, pLexRoot+n, sizeof(TLexNode));
		ivMemCopy(pLexRoot+n, pLexRoot+m, sizeof(TLexNode));
		ivMemCopy(pLexRoot+m, &tTmpNode, sizeof(TLexNode));
	}

	return ivTrue;
}

ivBool ParserGrm(PGrmMdlHdr pGrmMdlHdr, char *szFileName)
{
	int nSize, i;
	ivUInt32 nCRC =0, nCounter = 0;
	ivPointer pLexicon;
	char szTmp[1024];
	PResHeader pModelHdr;
	PMixStateModel pMixSModel;
	FILE *fp;
	int nMix;
	
	nSize = sizeof(TGrmMdlHdr) + sizeof(TGrmDesc) * (pGrmMdlHdr->nTotalGrm - 1);
	ivMakeCRC((ivPCInt16)((ivUInt32)pGrmMdlHdr+nSize), pGrmMdlHdr->nCRCCnt * sizeof(ivInt16), &nCRC, &nCounter);
	if(nCRC != pGrmMdlHdr->nCRC){
		return ivFalse;
	}

	pModelHdr = (PResHeader)((ivUInt32)pGrmMdlHdr + pGrmMdlHdr->nModelOffset * sizeof(ivInt16));
	pMixSModel = (PMixStateModel)((ivUInt32)pModelHdr+(pModelHdr->nMixSModelOffset * sizeof(ivInt16)));
	nMix = pModelHdr->nMixture;

	for(i=0; i<pGrmMdlHdr->nTotalGrm; i++){
		pLexicon = (ivPointer)((ivUInt32)pGrmMdlHdr + pGrmMdlHdr->tGrmDesc[i].nGrmOffset * sizeof(ivInt16));
		sprintf(szTmp, "%s_%d.log", szFileName, i+1);
		ParserLexicon(pLexicon, pMixSModel, nMix, szTmp);		
	}

	nCRC = 0;nCounter = 0;
	nSize = sizeof(TGrmMdlHdr) + sizeof(TGrmDesc) * (pGrmMdlHdr->nTotalGrm - 1);
	ivMakeCRC((ivPCInt16)((ivUInt32)pGrmMdlHdr+nSize), pGrmMdlHdr->nCRCCnt * sizeof(ivInt16), &nCRC, &nCounter);
	if(nCRC != pGrmMdlHdr->nCRC){
		return ivFalse;
	}

	return ivTrue;
}
#endif

void EsReverserOrder(PLexNode pLexRoot, ivUInt16 nTotalNode)
{
	ivUInt16 i;
	PLexNode pNode1;
	PLexNode pNode2;
	TLexNode tTmpNode;


	/* ����������ڵ����ɵ���洢ת��,�������Ͳ��� */
	pNode1 = pLexRoot;
	pNode2 = pLexRoot + nTotalNode - 1;
	for(i=0; i<nTotalNode/2; i++, pNode1++, pNode2--){	
		/* ivMemCopy(&tTmpNode, pNode1, sizeof(TLexNode)); */
		tTmpNode.iParent = pNode1->iParent;
        tTmpNode.iSModeIndex = pNode1->iSModeIndex;
        tTmpNode.nStateCount = pNode1->nStateCount;

		/* ivMemCopy(pNode1, pNode2, sizeof(TLexNode)); */
		pNode1->iParent = pNode2->iParent;
        pNode1->iSModeIndex = pNode2->iSModeIndex;
        pNode1->nStateCount = pNode2->nStateCount;
		/* ivMemCopy(pNode2, &tTmpNode, sizeof(TLexNode)); */
		pNode2->iParent = tTmpNode.iParent;
        pNode2->iSModeIndex = tTmpNode.iSModeIndex;
        pNode2->nStateCount = tTmpNode.nStateCount;
	}
}

ivESRStatus EsAddTagToLex(				
					ivPointer	pLexBuf,		/* WorkSpace */		
					ivUInt16	nTagStateNum,	
					PCmdDesc	pTagCmdDesc,								
					ivUInt16	iTagSModelIndex
					)
{
	PLexiconHeader pLexHdr;
	PLexNode pLexRoot;
	PLexNode pSrcNode;
	PLexNode pDstNode;
	PCmdDesc pCmdDesc;
	PCmdDesc pDstCmdDsc, pSrcCmdDsc;
	ivUInt16 i, n, iParent;
	ivUInt16 nTagHead;
	ivUInt16 nTagMid;
	ivUInt16 nTagTail;
	ivUInt16 nOrgHead;
	ivUInt16 nOrgMid;
	ivUInt16 nOrgTail;

	if(ivNull == pLexBuf || ivNull == pTagCmdDesc){
		ivAssert(ivFalse);
		return ivESR_INVARG;
	}

	/*
	�﷨���ṹ˵����
	*/
	pLexHdr = (PLexiconHeader)pLexBuf;
	pLexRoot = (PLexNode)((ivAddress)pLexHdr + pLexHdr->nLexRootOffset);
	pCmdDesc = (PCmdDesc)((ivAddress)pLexHdr + pLexHdr->nCmdDescOffset);
	
	/* �޸�pCmdDescλ��.����ֱ�ӵ���ivMemCopy,�ڴ�������ص�,�Ḳ����Ч��Ϣ */
	pLexHdr->nCmdDescOffset += nTagStateNum * sizeof(TLexNode);
	pSrcCmdDsc = pCmdDesc + pLexHdr->nCmdNum - 1;
   
    pCmdDesc = (PCmdDesc)((ivAddress)pLexHdr + pLexHdr->nCmdDescOffset); 
    pDstCmdDsc = pCmdDesc + pLexHdr->nCmdNum - 1;
    for(n=0; n<pLexHdr->nCmdNum; n++, pSrcCmdDsc--, pDstCmdDsc--)
    {
        pDstCmdDsc->bTag = pSrcCmdDsc->bTag;
        pDstCmdDsc->nCMThresh = pSrcCmdDsc->nCMThresh;
        pDstCmdDsc->nID = pSrcCmdDsc->nID;
    }

	pCmdDesc[pLexHdr->nCmdNum].nID = pTagCmdDesc->nID;
	pCmdDesc[pLexHdr->nCmdNum].nCMThresh = pTagCmdDesc->nCMThresh;
	pCmdDesc[pLexHdr->nCmdNum].bTag = pTagCmdDesc->bTag;

	/* ����������ڵ����ɵ���洢ת��,�������Ͳ��� */
	EsReverserOrder(pLexRoot, pLexHdr->nTotalNodes);

	/* ͷ��β��ʾ:����չ�ڵ������м�ڵ�����Ҷ�ӽڵ��� */
	/* ��������β�ڵ���� */
	nOrgHead = pLexHdr->nExtendNodes;
	nOrgTail = (ivUInt16)pLexHdr->nCmdNum;
	nOrgMid = pLexHdr->nTotalNodes - nOrgHead - nOrgTail;
	nTagHead = ivMin(8, nTagStateNum-1);
	/* nExtendNode < nTotalNode/2. ������Ҫ */
	/* while((nTagHead + nOrgHead)*2 > pLexHdr->nTotalNodes + nTagStateNum){ */
	while(pLexHdr->nTotalNodes + nTagStateNum - (nTagHead + nOrgHead)*2 < 0){
		nTagHead --;
	}
	nTagTail = 1;
	nTagMid = nTagStateNum - nTagHead - nTagTail;	

	/* ���õĴ洢˳���� OrgHead TagHead TagMid OrgMid OrgTail TagTail */

	/* --------------------------Step1. Org Head ����--------------------- */

	/* --------------------------Step2. Org Tail-------------------------- */
	n = nTagHead + nTagMid;
	pSrcNode = pLexRoot + pLexHdr->nTotalNodes - 1;
	pDstNode = pSrcNode + n;
	for(i=0; i<pLexHdr->nCmdNum; i++, pSrcNode--, pDstNode--){
        pDstNode->iSModeIndex = pSrcNode->iSModeIndex;
        pDstNode->nStateCount = pSrcNode->nStateCount;
		if(pSrcNode->iParent > nOrgHead)
		{
			/* ���ڵ���OrgMid */
			pDstNode->iParent = pSrcNode->iParent + n;
		}
		else{
			/* ���ڵ���OrgHead */
			pDstNode->iParent = pSrcNode->iParent;
		}
	}

	/* --------------------------Step3. Org Mid-------------------------- */
	n = nTagHead + nTagMid;
	pSrcNode = pLexRoot + pLexHdr->nTotalNodes - pLexHdr->nCmdNum - 1;
	pDstNode = pSrcNode + n;
	for(i=0; i<nOrgMid; i++, pSrcNode--, pDstNode--){/* �Ӻ���ǰ,��ֹ����δ���Ƶĸ����� */		
        pDstNode->iSModeIndex = pSrcNode->iSModeIndex;
        pDstNode->nStateCount = pSrcNode->nStateCount;
		if(pSrcNode->iParent > nOrgHead){
			/* ���ڵ���OrgMid */
			pDstNode->iParent = pSrcNode->iParent + n;
		}
		else{
			/* ���ڵ���OrgHead */
			pDstNode->iParent = pSrcNode->iParent;
		}
	}

	/* ------------------------Step4. Tag Head,Mid------------------------- */   
	iParent = 0;
	pDstNode = pLexRoot + nOrgHead;
	for(i=0; i<nTagHead+nTagMid; i++, pDstNode++){
		pDstNode->iParent = iParent;
		pDstNode->iSModeIndex = iTagSModelIndex;
        pDstNode->nStateCount = 0;
	
		iParent = nOrgHead + i + 1; /* �Ǹ��ڵ�ʵ��������+1 */
        iTagSModelIndex ++;
	}

	/* --------------------------Step5. Tag Tail-------------------------- */
	pDstNode = pLexRoot + pLexHdr->nTotalNodes + nTagStateNum - 1;
	pDstNode->iParent = iParent;
	pDstNode->iSModeIndex = iTagSModelIndex;
    pDstNode->nStateCount = nTagStateNum;
	
	/* ------------------------------------------------------------------- */

	pLexHdr->nTotalNodes += nTagStateNum;
	pLexHdr->nExtendNodes += nTagHead;
	pLexHdr->nCmdNum ++;

	/* ���������絹��洢 */
	EsReverserOrder(pLexRoot, pLexHdr->nTotalNodes);

	return ivESR_OK;
}

EsErrID EsDelTagFromLex(PLexiconHeader pLexHdr, ivUInt16 nDelCmdID, ivPUInt16 pnTagStateNum)
{
    PCmdDesc pCmdDesc, pNewCmdDesc;
    PLexNode pLexRoot;
    ivUInt16 i, iTag;
    ivUInt16 iDelNode,iDelNodeParent;
    ivUInt16 nTotalNodes;
    ivUInt16 nTagStateNum;

    /* �ú�����ȫ���ڴ���� */
    if(DW_LEXHEADER_CHECK != pLexHdr->dwCheck){
        ivAssert(ivFalse);
        return EsErr_InvRes;
    }

    /* �ض�λ */
    pCmdDesc = (PCmdDesc)((ivAddress)pLexHdr + (ivUInt32)(pLexHdr->nCmdDescOffset));
    for(i=0; i<pLexHdr->nCmdNum; i++) {
        if(1 == pCmdDesc[i].bTag && nDelCmdID == pCmdDesc[i].nID) {
            break;
        }
    }
    if(i >= pLexHdr->nCmdNum) {
        /* û����Ҫɾ����Tag,ֱ��go out */
        *pnTagStateNum = 0; 
        return ivESR_OK;
    }

    iTag = i;

    pLexRoot = (PLexNode)((ivAddress)pLexHdr + (ivUInt32)(pLexHdr->nLexRootOffset));
    nTotalNodes = pLexHdr->nTotalNodes;

    EsReverserOrder(pLexRoot, nTotalNodes);

    /* �ҵ�Ҫɾ��·����Ҷ�ӽڵ�,��β��ͷ·�����ɾ�� */
    nTagStateNum = 0;
    iDelNode = (ivUInt16)(nTotalNodes - pLexHdr->nCmdNum + iTag);
    iDelNodeParent = pLexRoot[iDelNode].iParent;	/* ->iParent��1��ʼ */
    while(1) {
        for(i=iDelNode+1; i<nTotalNodes; i++) {
            pLexRoot[i-1].iSModeIndex = pLexRoot[i].iSModeIndex;
#if MINI5_USE_NEWCM_METHOD
            pLexRoot[i-1].nStateCount = pLexRoot[i].nStateCount;
#endif
            if(pLexRoot[i].iParent > iDelNode) {
                pLexRoot[i-1].iParent = pLexRoot[i].iParent - 1;
            }
            else {
                pLexRoot[i-1].iParent = pLexRoot[i].iParent;
            }
        }

        nTotalNodes --;
        nTagStateNum ++;

        if(0 == iDelNodeParent) {
            break;
        }
        iDelNode = iDelNodeParent - 1;
        iDelNodeParent = pLexRoot[iDelNode].iParent ;
    }

    *pnTagStateNum = nTagStateNum;

    EsReverserOrder(pLexRoot, nTotalNodes);

    /* �޸�PCmdDesc��ƫ�� */
    pNewCmdDesc = (PCmdDesc)((ivAddress)pCmdDesc - nTagStateNum*sizeof(TLexNode));
    for(i=0; i<pLexHdr->nCmdNum; i++) {
        if(i == iTag) {
            continue;
        }
        ivMemCopy(pNewCmdDesc, pCmdDesc+i, sizeof(TCmdDesc));
        pNewCmdDesc ++;
    }

    pLexHdr->nCmdDescOffset -= nTagStateNum*sizeof(TLexNode);
    pLexHdr->nTotalNodes = nTotalNodes;
    pLexHdr->nCmdNum --;

    return ivESR_OK;	
}

/* �������:50, 2012.5.31��� */
#if MINI5_SPI_MODE
EsErrID ivCall ESRAddTagToGrm(
			ivPointer	pTagObj, 
			ivCPointer	pOrgRes, 
			ivPointer	pNewRes, 
			ivPUInt32	pnNewResSize, /* ��λΪƽ̨��С���ʵ�λ */ 
			ivPUInt16	pGrmIDLst, 
			ivUInt16	nGrmIDNum, 
			ivUInt16	nTagThresh
			)
{
	PEsrEngine pEngine;
	PTagCmd pTagCmdSPI;
	ivUInt16 nTagStateNum;
	ivPointer pWorkSpace;
	PGrmMdlHdr pOrgGrmMdlHdr;
	ivUInt16 nGrmMdlHdrSize;
	PResHeader pNewModelHdr;
	PGrmDesc pOrgGrmDesc;
	ivUInt16 i, j;
	ivUInt32 nCRC;
	ivUInt32 nCounter;
	ivUInt32 nWorkSpaceSize;
	ivBool bValidGrmID;
	ivUInt32 nOffset;
	ivUInt32 nGrmSize;
	EsErrID err;
	ivCPointer pOrgModelHdrSPI;
	ivUInt16 nMixSModelWSize;
	ivUInt16 nMixSModelNum;
	TCmdDesc tTagCmdDesc;
	
	ivUInt32 nModelCRC;
	ivUInt32 nModelCnt;

    ivUInt32 nOrgSilSModelOffset;
	ivUInt32 nNewLexSize;

    ivUInt32 nSize;
    ivPCInt16 pData;
	ivUInt32 nTagModelSize;

    ivPointer pRamBuffer;
    ivPointer pSPIBuffer;
    PSPICache pSPICache;

	if(ivNull == pTagObj || ivNull == pOrgRes  || ivNull == pNewRes || ivNull == pGrmIDLst || ivNull == pnNewResSize){
		ivAssert(ivFalse);
		return EsErr_InvArg;
	}

	if(nTagThresh > 100){
		return ivESR_INVCAL;
	}

	/* Memory align */
	pTagObj = (ivPointer)(((ivUInt32)pTagObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));
	if(DW_OBJ_CHECK != *(ivPUInt32)pTagObj){
		return EsErr_InvCal;
	}

	pEngine = (PEsrEngine)((ivPUInt32)pTagObj + 1);

	/* ����pTagObj�����ڴ� */	
    nSize = ivGridSize(sizeof(TEsrEngine) + IV_PTR_GRID);
	pWorkSpace = (ivPointer)((ivAddress)pEngine + nSize);

	/* ��OrgRes����Դͷ��SPI�п������ڴ��� */
	pOrgGrmMdlHdr = (PGrmMdlHdr)pWorkSpace;
	nGrmMdlHdrSize = sizeof(TGrmMdlHdr);

    err = EsSPIReadBuf((ivPointer)pOrgGrmMdlHdr, pOrgRes, nGrmMdlHdrSize);
    ivAssert(EsErrID_OK == err);
    if (EsErrID_OK != err) {
        return EsErr_Failed;
    }

	if(DW_MINI_RES_CHECK != pOrgGrmMdlHdr->dwCheck) {
		return EsErr_InvRes;
	}
	if(pOrgGrmMdlHdr->nTotalGrm > 1) {
		/* ��SPI Copy */
        pSPIBuffer = (ivPointer)((ivAddress)pOrgRes + nGrmMdlHdrSize);
        pRamBuffer = (ivPointer)((ivAddress)pOrgGrmMdlHdr + nGrmMdlHdrSize);
        err = EsSPIReadBuf(pRamBuffer, pSPIBuffer, sizeof(TGrmDesc)*(pOrgGrmMdlHdr->nTotalGrm-1));
        ivAssert(EsErrID_OK == err);
        if (EsErrID_OK != err) {
            return EsErr_Failed;
        }
        nGrmMdlHdrSize += sizeof(TGrmDesc)*(pOrgGrmMdlHdr->nTotalGrm-1);
	}
    /* ����pTagObj������Դ���﷨ */
    ivAssert(MINI5_SPI_PAGE_SIZE >= nGrmMdlHdrSize);
	pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(MINI5_SPI_PAGE_SIZE));

    /* ����pTagObj�����TagCmd��SPI */
    /* SPI Buf */
    pTagCmdSPI = (PTagCmd)pWorkSpace;
    if(ivNull == pTagCmdSPI) {
        ivAssert(ivFalse);
        return EsErr_InvCal;
    }
    err = EsSPIReadBuf(pWorkSpace, (ivPointer)pEngine->pTagCmdBuf, sizeof(TTagCmd));
    ivAssert(EsErrID_OK == err);
    if (EsErrID_OK != err) {
        return EsErr_Failed;
    }

    nTagStateNum = pTagCmdSPI->nState;
	if(DW_TAGCMD_CHECK != pTagCmdSPI->dwCheck) {
		ivAssert(ivFalse);
		return EsErr_InvCal;
	}
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(sizeof(TTagCmd)));

    /* ����pTagObj��ΪSPICache */
    pSPICache = (PSPICache)pWorkSpace;
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(sizeof(TSPICache)));
    pSPICache->iCacheSPIWrite = 0;
    pSPICache->pCacheSPI = pWorkSpace;
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(MINI5_SPI_PAGE_SIZE));

    /* ����pTagObj��Ϊģ����Դͷ */
    pNewModelHdr = (PResHeader)pWorkSpace;
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(sizeof(TResHeader)));

    pOrgGrmDesc = (PGrmDesc)pWorkSpace;
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(sizeof(TGrmDesc)));

	nWorkSpaceSize = (ivAddress)pEngine->pBufferEnd - (ivAddress)pWorkSpace;	

	/* ��SPI�е�OrgRes����CRC */
	nCRC = 0;
	nCounter = 0;
    pSPIBuffer = (ivPointer)((ivAddress)pOrgRes + nGrmMdlHdrSize);
    err = EsSPICalcCRC(pWorkSpace, pSPIBuffer, pOrgGrmMdlHdr->nCRCCnt * sizeof(ivInt16), &nCRC, &nCounter);
    ivAssert(EsErrID_OK == err);
    if(nCRC != pOrgGrmMdlHdr->nCRC) {
		return EsErr_InvRes;
	}

	/* �ڴ����.���Ƿ�����Ҫ���Tag�� */
	bValidGrmID = ivFalse;
    ivMemCopy(pOrgGrmDesc, pOrgGrmMdlHdr->tGrmDesc, sizeof(TGrmDesc));
    pSPIBuffer = (ivPointer)((ivAddress)pOrgRes + (sizeof(TGrmMdlHdr) - sizeof(TGrmDesc)));

	tTagCmdDesc.nID = pEngine->nTagID;
	tTagCmdDesc.nCMThresh = nTagThresh;
	tTagCmdDesc.bTag = 1;

	/* ��SPI�е�ģ����Ϣ���������tagʱ��iSModelOffset���� */
	pOrgModelHdrSPI = (ivCPointer)((ivAddress)pOrgRes + pOrgGrmMdlHdr->nModelOffset * sizeof(ivInt16));
    err = EsSPIReadBuf((ivPointer)pNewModelHdr, pOrgModelHdrSPI, sizeof(TResHeader));
    ivAssert(EsErrID_OK == err);
    if (EsErrID_OK != err) {
        return EsErr_Failed;
    }
    nOrgSilSModelOffset = pNewModelHdr->nSilSModelOffset;

	nMixSModelNum = (ivUInt16)(pNewModelHdr->nMixSModelNum); /* ���ں���tag����ʱpLexNode->iSModeOffset */
	nMixSModelWSize = (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel)); /* word */
	
    pSPICache->pSPIBufBase = (ivPointer)((ivAddress)pNewRes + MINI5_SPI_PAGE_SIZE);
    pSPICache->pSPIBuf = pSPICache->pSPIBufBase;
    pSPICache->pSPIBufEnd = (ivPointer)((ivAddress)pNewRes + *pnNewResSize);

	nCRC =0; 
	nCounter = 0;
	nOffset = nGrmMdlHdrSize;
	/* pNewRes����GrmMdlHdrͷ��Ϣ,������nCRC��Ҫȫ��������ŵõ�. ����ͷ��д */
	for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++) {
		ivPointer pOrgLex;
		bValidGrmID = ivFalse;
		j = 0;
		while(j < nGrmIDNum) {
			if(pOrgGrmDesc->nGrmID == pGrmIDLst[j]) {
				/* Need to add tag cmd */
				bValidGrmID = ivTrue;
				break;
			}
			j++;
		} /* while(j < nGrmIDNum) */

		pOrgLex = (ivPointer)((ivAddress)pOrgRes + pOrgGrmDesc->nGrmOffset * sizeof(ivInt16));
		nGrmSize = pOrgGrmDesc->nGrmSize * sizeof(ivInt16);
		pOrgGrmDesc->nGrmOffset = nOffset / sizeof(ivInt16);

		if(nWorkSpaceSize < nGrmSize) {
			ivAssert(ivFalse);
			return EsErr_OutOfMemory;
		}

		/* ��SPI�е��﷨���翽�����ڴ��н������tag�Ĳ��� */
        err = EsSPIReadBuf(pWorkSpace, pOrgLex, nGrmSize);
        ivAssert(EsErrID_OK == err);
        if (EsErrID_OK != err) {
            return EsErr_Failed;
        }

		if(bValidGrmID) {
			/* �����������tag */
			nNewLexSize = nGrmSize + nTagStateNum * sizeof(TLexNode) + sizeof(TCmdDesc);
			if(nWorkSpaceSize < nNewLexSize) {
				ivAssert(ivFalse);
				return EsErr_OutOfMemory;
			}

			if(nOffset + nNewLexSize > *pnNewResSize) {
				ivAssert(ivFalse);
				return EsErr_OutOfMemory;
			}

			err = EsAddTagToLex(pWorkSpace, nTagStateNum, &tTagCmdDesc, (ivUInt16)(nMixSModelNum - 2 - pNewModelHdr->nOldFillerCnt)); /* -2:sil,filler */
			if(ivESR_OK != err) {
				ivAssert(ivFalse);
				return err;
			}

			pOrgGrmDesc->nGrmSize = nNewLexSize / sizeof(ivInt16);

			nGrmSize = nNewLexSize;			
		} /* if(bValidGrmID) */
		else {
			if(nOffset + nGrmSize > *pnNewResSize) {
				ivAssert(ivFalse);
				return EsErr_OutOfMemory;
			}			
		} /* else */

		ivMakeCRC(pWorkSpace, nGrmSize, &nCRC, &nCounter);		
		nOffset += nGrmSize;

        nSize = 0;
        ivAssert(nGrmMdlHdrSize  < MINI5_SPI_PAGE_SIZE);
        if (nGrmMdlHdrSize < MINI5_SPI_PAGE_SIZE) {
            pRamBuffer = (ivPointer)((ivAddress)pOrgGrmMdlHdr + nGrmMdlHdrSize);
            nSize = MINI5_SPI_PAGE_SIZE - nGrmMdlHdrSize;
            if (nGrmSize < nSize) {
                nSize = nGrmSize;
            }

            nGrmSize = nGrmSize - nSize;
            nGrmMdlHdrSize = MINI5_SPI_PAGE_SIZE;
            ivMemCopy(pRamBuffer, pWorkSpace, nSize);
        }
        EsSPIWriteData(pSPICache, (ivPointer)((ivAddress)pWorkSpace + nSize), nGrmSize);
        pRamBuffer = (ivPointer)((ivAddress)pOrgGrmMdlHdr + (sizeof(TGrmMdlHdr) - sizeof(TGrmDesc)) + i * sizeof(TGrmDesc));
        ivMemCopy(pRamBuffer, pOrgGrmDesc, sizeof(TGrmDesc));

        pSPIBuffer = (ivPointer)((ivAddress)pSPIBuffer + sizeof(TGrmDesc));
        EsSPIReadBuf((ivPointer)pOrgGrmDesc, pSPIBuffer, sizeof(TGrmDesc));
	} /* for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++) */

	pOrgGrmMdlHdr->nModelOffset = nOffset / sizeof(ivInt16);
	pNewModelHdr->nMixSModelNum += nTagStateNum;

    /* �µ�tagģ�ͷ���sil��fillerģ��ǰ��,���Ҫ�޸�sil��fillerģ�͵�ƫ��ֵ */
    nTagModelSize = (nTagStateNum * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel))) / sizeof(ivInt16);
    pNewModelHdr->nSilSModelOffset += nTagModelSize;
    pNewModelHdr->nFillerSModelOffset += nTagModelSize;
    if(pNewModelHdr->nOldFillerCnt > 0) {
        pNewModelHdr->nOldFillerSModelOffset += nTagModelSize;
    }

    /* ------------------------���¼���ģ��CRC.Begin-------------------------- */ 
	nModelCRC = 0;
	nModelCnt = 0;

    pData = (ivPInt16)((ivAddress)pNewModelHdr + sizeof(ivUInt32)*2);
    nSize = sizeof(TResHeader) - sizeof(ivUInt32)*2;
	ivMakeCRC(pData, nSize, &nModelCRC, &nModelCnt);

    pData = (ivPInt16)((ivAddress)pOrgModelHdrSPI+sizeof(TResHeader));
    nSize = (pNewModelHdr->nMixSModelNum - nTagStateNum - 2 - pNewModelHdr->nOldFillerCnt) * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel));
	EsSPICalcCRC(pWorkSpace,pData, nSize, &nModelCRC, &nModelCnt);
	
    pData = (ivCPointer)((ivAddress)pEngine->pTagCmdBuf + (sizeof(TTagCmd) - sizeof(TMixStateModel)));
    nSize = nTagStateNum * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel));
    EsSPICalcCRC(pWorkSpace, pData, nSize, &nModelCRC, &nModelCnt);

    ivAssert(pNewModelHdr->nSilSModelOffset < pNewModelHdr->nFillerSModelOffset);
    pData = (ivPInt16)((ivAddress)pOrgModelHdrSPI + nOrgSilSModelOffset * sizeof(ivInt16));
    nSize = (pNewModelHdr->nSilMixture + pNewModelHdr->nFillerMixture + pNewModelHdr->nOldFillerCnt * pNewModelHdr->nOldFillerMix) * sizeof(TGModel);
    EsSPICalcCRC(pWorkSpace, pData, nSize, &nModelCRC, &nModelCnt);	

    /* Model Info */
    nSize = sizeof(TGrmDesc)*(pOrgGrmMdlHdr->nTotalGrm-1) + sizeof(TGrmMdlHdr);
    nSize += (nModelCnt + nCounter)*sizeof(ivInt16) + sizeof(TResHeader);
    if( nSize> *pnNewResSize) {
        ivAssert(ivFalse);
        /* ʵ����SPI����С�ֽ�Ϊ (int)(nSize/MINI5_SPI_PAGE_SIZE + 1) * MINI5_SPI_PAGE_SIZE, �ȷ��ص�����Դ���ֽ����Դ�Щ*/
        *pnNewResSize = nSize;
        return EsErr_OutOfMemory;
    }

    pNewModelHdr->nCRC = nModelCRC;
	pNewModelHdr->nCRCCnt = nModelCnt;
    /* ------------------------���¼���ģ��CRC.End-------------------------- */

    /* ------------------------���������ģ��д��SPI��.Begin-------------------------- */ 
    pData = (ivPCInt16)pNewModelHdr;
    nSize = sizeof(TResHeader);
	ivMakeCRC(pData, nSize, &nCRC, &nCounter);
    EsSPIWriteData(pSPICache, pData, nSize);

    pData = (ivPCInt16)((ivAddress)pOrgModelHdrSPI+sizeof(TResHeader));
    nSize = (pNewModelHdr->nMixSModelNum - nTagStateNum - 2 - pNewModelHdr->nOldFillerCnt) * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel));
    EsSPIWriteSPIData(pSPICache, pWorkSpace, pData, nSize,&nCRC, &nCounter);

    pData = (ivCPointer)((ivAddress)pEngine->pTagCmdBuf + sizeof(TTagCmd) - sizeof(TMixStateModel));
    nSize = nTagStateNum * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel));
    EsSPIWriteSPIData(pSPICache, pWorkSpace, pData, nSize,&nCRC, &nCounter);

    ivAssert(pNewModelHdr->nSilSModelOffset < pNewModelHdr->nFillerSModelOffset);
    pData = (ivPCInt16)((ivAddress)pOrgModelHdrSPI + nOrgSilSModelOffset * sizeof(ivInt16));
    nSize = (pNewModelHdr->nSilMixture + pNewModelHdr->nFillerMixture + pNewModelHdr->nOldFillerCnt * pNewModelHdr->nOldFillerMix) * sizeof(TGModel);
    EsSPIWriteSPIData(pSPICache, pWorkSpace, pData, nSize, &nCRC, &nCounter);

    err = EsSPIWriteFlush(pSPICache);
    ivAssert(EsErrID_OK == err);

	pOrgGrmMdlHdr->nCRC = nCRC;
	pOrgGrmMdlHdr->nCRCCnt = nCounter;
	pOrgGrmMdlHdr->nModelSize += nTagStateNum * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture-1) * sizeof(TGModel));

    pSPICache->pSPIBufBase = (ivPointer)pNewRes;
    pSPICache->pSPIBuf = pSPICache->pSPIBufBase;
    pSPICache->pSPIBufEnd = (ivPointer)((ivAddress)pNewRes + MINI5_SPI_PAGE_SIZE);
    EsSPIWriteData(pSPICache, pOrgGrmMdlHdr, nGrmMdlHdrSize);
    ivAssert(0 == pSPICache->iCacheSPIWrite);

    /* �������Դ�Ĵ�С */
	*pnNewResSize = nGrmMdlHdrSize + nCounter*sizeof(ivInt16);

#ifdef TAG_LOG
	ParserGrm(pOrgRes, "ParserOrgLex");
	ParserGrm(pNewRes, "ParserTagLex");
#endif

	return EsErrID_OK;
}

/* ������� */
EsErrID ivCall ESRDelTagFromGrm(
                                ivPointer      pTagObj,            /* Tag Object */
                                ivSize ivPtr   pnTagObjSize,       /* [In/Out] Size of Tag object */
                                ivCPointer     pOrgRes,            /* [In] Original Resource */
                                ivPointer      pNewRes,            /* [In/Out] Original Resource */
                                ivPUInt32      pnNewResSize,       /* [In/Out]Size of pNewRes */ 
                                ivUInt16       nDelGrmID,          /* [In] Grammar ID to Delete VoiceTag */
                                ivUInt16       nDelTagID           /* [In] VoiceTag ID to Delete */
                                )
{
    PGrmMdlHdr pOrgGrmMdlHdr;
    ivSize nGrmMdlHdrSize;
    ivPointer pNewLexSPI;
    PGrmDesc pOrgGrmDesc;
    ivUInt16 i;

    ivUInt32 nCRC;
    ivUInt32 nCounter;

    ivPointer pWorkSpace;
    ivSize nWorkSpaceSize;
    ivUInt16 nGrmSize;
    ivUInt16 nSize;

    ivUInt16 nTagStateNum = 0;

    ivUInt32 nOffset;
    EsErrID err;
    ivCPointer pModelHdrSPI;
    PSPICache pSPICache;

    ivPointer pSPIBuffer;
    ivPointer pRamBuffer;

    if(ivNull == pTagObj || ivNull == pnTagObjSize || ivNull == pOrgRes || ivNull == pNewRes || ivNull == pnNewResSize){
        ivAssert(ivFalse);
        return ivESR_INVARG;
    }

    /* Memory align */
    pWorkSpace = (ivPointer)(((ivUInt32)pTagObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));
    nWorkSpaceSize = *pnTagObjSize;

    /* -----------��OrgRes����Դͷ��SPI�п������ڴ���.Begin------------------- */
    pOrgGrmMdlHdr = (PGrmMdlHdr)pWorkSpace;	
    err = EsSPIReadBuf((ivPointer)pOrgGrmMdlHdr, pOrgRes, sizeof(TGrmMdlHdr));
    ivAssert(EsErrID_OK == err);
    if (EsErrID_OK != err) {
        return EsErr_Failed;
    }
    if(DW_MINI_RES_CHECK != pOrgGrmMdlHdr->dwCheck){
        return ivESR_INVRESOURCE;
    }

    nGrmMdlHdrSize = sizeof(TGrmMdlHdr) + (pOrgGrmMdlHdr->nTotalGrm-1)*sizeof(TGrmDesc);
    if(nGrmMdlHdrSize > nWorkSpaceSize){
        *pnTagObjSize = nGrmMdlHdrSize;
        return ivESR_OUTOFMEMORY;
    }

    ivAssert(MINI5_SPI_PAGE_SIZE > nGrmMdlHdrSize);
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(MINI5_SPI_PAGE_SIZE));
    nWorkSpaceSize -= nGrmMdlHdrSize;

    pOrgGrmDesc = (PGrmDesc)pWorkSpace;
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(sizeof(TGrmDesc)));

    /* ����pTagObj��ΪSPICache */
    pSPICache = (PSPICache)pWorkSpace;
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(sizeof(TSPICache)));
    pSPICache->iCacheSPIWrite = 0;
    pSPICache->pCacheSPI = pWorkSpace;
    pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + ivGridSize(MINI5_SPI_PAGE_SIZE));
    /* -----------��OrgRes����Դͷ��SPI�п������ڴ���.End------------------- */

    /* ----------------------��SPI�е�OrgRes����CRC------------------------- */
    nCRC = 0;
    nCounter = 0;
    pSPIBuffer = (ivPointer)((ivAddress)pOrgRes+nGrmMdlHdrSize);
    err = EsSPICalcCRC(pWorkSpace, pSPIBuffer, pOrgGrmMdlHdr->nCRCCnt * sizeof(ivInt16), &nCRC, &nCounter);
    ivAssert(EsErrID_OK == err);
    if(nCRC != pOrgGrmMdlHdr->nCRC){
        return ivESR_INVRESOURCE;
    }

    pSPICache->pSPIBufBase = (ivPointer)((ivAddress)pNewRes + MINI5_SPI_PAGE_SIZE);
    pSPICache->pSPIBuf = pSPICache->pSPIBufBase;
    pSPICache->pSPIBufEnd = (ivPointer)((ivAddress)pNewRes + *pnNewResSize);

    /* ���ÿ���﷨������ص��ڴ��н��в��� */
    nCRC =0; 
    nCounter = 0;
    nOffset = nGrmMdlHdrSize;
    /* pNewRes����GrmMdlHdrͷ��Ϣ,������nCRC��Ҫȫ��������ŵõ�. ����ͷ��д */
    pNewLexSPI = (ivPointer)((ivAddress)pNewRes + nOffset);
    ivMemCopy(pOrgGrmDesc, pOrgGrmMdlHdr->tGrmDesc, sizeof(TGrmDesc));

    pSPIBuffer = (ivPointer)((ivAddress)pOrgRes + (sizeof(TGrmMdlHdr) - sizeof(TGrmDesc)));
    for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++) {
        ivCPointer pOrgLex;

        pOrgLex = (ivCPointer)((ivAddress)pOrgRes + pOrgGrmDesc->nGrmOffset * sizeof(ivInt16));
        nGrmSize = (ivUInt16)(pOrgGrmDesc->nGrmSize * sizeof(ivInt16));
        pOrgGrmDesc->nGrmOffset = nOffset / sizeof(ivInt16);

        if(nGrmSize > nWorkSpaceSize){
            ivAssert(ivFalse);
            *pnTagObjSize = nGrmMdlHdrSize + nGrmSize;
            return ivESR_OUTOFMEMORY;
        }

        /* ��SPI�е��﷨���翽�����ڴ��н���ɾ��tag�Ĳ��� */
        err= EsSPIReadBuf((ivPointer)pWorkSpace, pOrgLex, nGrmSize);
        ivAssert(EsErrID_OK == err);
        if (EsErrID_OK != err) {
            return EsErr_Failed;
        }

        /* ----------ɾ��һ��Tag------------ */
        if(pOrgGrmDesc->nGrmID == nDelGrmID){

            /* ���ܴ��ڶ��nDelTagID��tag.ȫ��ɾ�� */
            while(1){
                err = EsDelTagFromLex(pWorkSpace, nDelTagID, &nTagStateNum);
                if(ivESR_OK != err){
                    ivAssert(ivFalse);
                    return err;
                }

                if(0 == nTagStateNum){ /* �������Ѿ�û��Ҫɾ��Tag�� */
                    break;
                }

                nGrmSize -= (nTagStateNum*sizeof(TLexNode) + sizeof(TCmdDesc));				
            }

            if(nOffset + nGrmSize > *pnNewResSize) {
                ivAssert(ivFalse);
                return ivESR_OUTOFMEMORY;
            }


            pOrgGrmDesc->nGrmSize = nGrmSize / sizeof(ivInt16);	
        }

        ivMakeCRC(pWorkSpace, nGrmSize, &nCRC, &nCounter);
        nOffset += nGrmSize;

        nSize = 0;
        ivAssert(nGrmMdlHdrSize  < MINI5_SPI_PAGE_SIZE);
        if (nGrmMdlHdrSize < MINI5_SPI_PAGE_SIZE) {
            pRamBuffer = (ivPointer)((ivAddress)pOrgGrmMdlHdr + nGrmMdlHdrSize);
            nSize = MINI5_SPI_PAGE_SIZE - nGrmMdlHdrSize;
            if (nGrmSize < nSize) {
                nSize = nGrmSize;
            }

            nGrmSize = nGrmSize - nSize;
            nGrmMdlHdrSize = MINI5_SPI_PAGE_SIZE;
            ivMemCopy(pRamBuffer, pWorkSpace, nSize);
        }
        EsSPIWriteData(pSPICache, (ivPointer)((ivAddress)pWorkSpace + nSize), nGrmSize);
        pRamBuffer = (ivPointer)((ivAddress)pOrgGrmMdlHdr + (sizeof(TGrmMdlHdr) - sizeof(TGrmDesc)) + i * sizeof(TGrmDesc));
        ivMemCopy(pRamBuffer, pOrgGrmDesc, sizeof(TGrmDesc));

        pSPIBuffer = (ivPointer)((ivAddress)pSPIBuffer + sizeof(TGrmDesc));
        EsSPIReadBuf((ivPointer)pOrgGrmDesc, pSPIBuffer, sizeof(TGrmDesc));

    } /* for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++) */

    /* Model Info */
    if(nOffset + pOrgGrmMdlHdr->nModelSize > *pnNewResSize){
        ivAssert(ivFalse);
        return ivESR_OUTOFMEMORY;
    }

    pModelHdrSPI = (ivCPointer)((ivAddress)pOrgRes + pOrgGrmMdlHdr->nModelOffset * sizeof(ivInt16));
    EsSPIWriteSPIData(pSPICache, pWorkSpace, pModelHdrSPI, pOrgGrmMdlHdr->nModelSize, &nCRC, &nCounter);

    err = EsSPIWriteFlush(pSPICache);
    ivAssert(EsErrID_OK == err);

    pOrgGrmMdlHdr->nModelOffset = nOffset / sizeof(ivInt16);
    pOrgGrmMdlHdr->nCRC = nCRC;
    pOrgGrmMdlHdr->nCRCCnt = nCounter;	

    pSPICache->pSPIBufBase = (ivPointer)pNewRes;
    pSPICache->pSPIBuf = pSPICache->pSPIBufBase;
    pSPICache->pSPIBufEnd = (ivPointer)((ivAddress)pNewRes + MINI5_SPI_PAGE_SIZE);
    EsSPIWriteData(pSPICache, pOrgGrmMdlHdr, nGrmMdlHdrSize);
    ivAssert(0 == pSPICache->iCacheSPIWrite);

    *pnNewResSize = nGrmMdlHdrSize + nCounter * sizeof(ivInt16);

#ifdef TAG_LOG
    ParserGrm(pOrgRes, "ParserOrgLex");
    ParserGrm(pNewRes, "ParserTagLex");
#endif

    return ivESR_OK;
}

#else /* MINI5_SPI_MODE */

EsErrID ivCall ESRAddTagToGrm(
			ivPointer	pTagObj, 
			ivCPointer	pOrgRes, 
			ivPointer	pNewRes, 
			ivPUInt32	pnNewResSize, /* ��λΪƽ̨��С���ʵ�λ */ 
			ivPUInt16	pGrmIDLst, 
			ivUInt16	nGrmIDNum, 
			ivUInt16	nTagThresh
			)
{
	PEsrEngine pEngine;
	PTagCmd pTagCmdSPI;
	ivUInt16 nTagStateNum;
	ivPointer pWorkSpace;
	PGrmMdlHdr pOrgGrmMdlHdr;
	ivUInt16 nGrmMdlHdrSize;
	PResHeader pNewModelHdr;
	ivPointer pNewLexSPI;
	PGrmDesc pOrgGrmDesc;
	ivUInt16 i, j;
	ivUInt32 nCRC;
	ivUInt32 nCounter;
	ivUInt32 nWorkSpaceSize;
	ivBool bValidGrmID;
	ivUInt32 nOffset;
	ivUInt32 nGrmSize;
	EsErrID err;
	PResHeader pOrgModelHdrSPI;
	ivUInt16 nMixSModelWSize;
	ivUInt16 nMixSModelNum;
	TCmdDesc tTagCmdDesc;
	
	ivUInt32 nModelCRC;
	ivUInt32 nModelCnt;

	ivUInt32 nNewLexSize;

    ivUInt32 nSize;
    ivPCInt16 pData;
	ivUInt32 nTagModelSize;
    ivPointer pSPIBuffer;

	if(ivNull == pTagObj || ivNull == pOrgRes  || ivNull == pNewRes || ivNull == pGrmIDLst || ivNull == pnNewResSize){
		ivAssert(ivFalse);
		return EsErr_InvArg;
	}

	if(nTagThresh > 100){
		return ivESR_INVCAL;
	}

	/* Memory align */
	pTagObj = (ivPointer)(((ivUInt32)pTagObj+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));
	if(DW_OBJ_CHECK != *(ivPUInt32)pTagObj){
		return EsErr_InvCal;
	}

	pEngine = (PEsrEngine)((ivPUInt32)pTagObj + 1);
	pTagCmdSPI = pEngine->pTagCmdBuf;	 /* �洢TagCmd��SPIBuf��ַ */
    /* SPI Buf */
	if(ivNull == pTagCmdSPI) {
		ivAssert(ivFalse);
		return EsErr_InvCal;
	}

	if(DW_TAGCMD_CHECK != pTagCmdSPI->dwCheck) {
		ivAssert(ivFalse);
		return EsErr_InvCal;
	}

	nTagStateNum = pTagCmdSPI->nState;

	/* ����pTagObj�����ڴ� */	
	pWorkSpace = (ivPointer)((ivAddress)pEngine + sizeof(TEsrEngine));

	/* ��OrgRes����Դͷ��SPI�п������ڴ��� */
	pOrgGrmMdlHdr = (PGrmMdlHdr)pWorkSpace;
	nGrmMdlHdrSize = sizeof(TGrmMdlHdr);

	ivMemCopy(pOrgGrmMdlHdr, pOrgRes, nGrmMdlHdrSize); /* ��SPI Copy */

	if(DW_MINI_RES_CHECK != pOrgGrmMdlHdr->dwCheck) {
		return EsErr_InvRes;
	}
	if(pOrgGrmMdlHdr->nTotalGrm > 1) {
		/* ��SPI Copy */
        ivMemCopy((ivPointer)(pOrgGrmMdlHdr + 1), (ivPointer)((ivAddress)pOrgRes + nGrmMdlHdrSize), sizeof(TGrmDesc)*(pOrgGrmMdlHdr->nTotalGrm-1)); /* �����ϴε�SPIλ�ÿ��� */
        nGrmMdlHdrSize += sizeof(TGrmDesc)*(pOrgGrmMdlHdr->nTotalGrm-1);
	}
	pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + nGrmMdlHdrSize);
	
	pNewModelHdr = (PResHeader)pWorkSpace;
	pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + sizeof(TResHeader));
	nWorkSpaceSize = (ivAddress)pEngine->pBufferEnd - (ivAddress)pWorkSpace;	

	/* ��SPI�е�OrgRes����CRC */
	nCRC = 0;
	nCounter = 0;
    ivMakeCRC((ivPCInt16)((ivAddress)pOrgRes+nGrmMdlHdrSize), pOrgGrmMdlHdr->nCRCCnt * sizeof(ivInt16), &nCRC, &nCounter);
    if(nCRC != pOrgGrmMdlHdr->nCRC) {
		return (ivESRStatus)EsErr_InvRes;
	}

	/* �ڴ����.���Ƿ�����Ҫ���Tag�� */
	bValidGrmID = ivFalse;
	pOrgGrmDesc = pOrgGrmMdlHdr->tGrmDesc;
	for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++) {
		j = 0;
		while(j<nGrmIDNum) {
			if(pOrgGrmDesc[i].nGrmID == pGrmIDLst[j]) {
				/* Need to add tag cmd */
				bValidGrmID = ivTrue;
				break;
			} /* if */
			j++;
		} /* while(j<nGrmIDNum) */
		if(bValidGrmID) {
			break;	
		}
	} /* for (i = 0;...);*/
	
	/* û����Ҫ���Tag���﷨����,��ֱ�ӿ����˳� */
	if(!bValidGrmID) {
		*pnNewResSize = 0;
		return EsErr_InvCal;
	}

	tTagCmdDesc.nID = pEngine->nTagID;
	tTagCmdDesc.nCMThresh = nTagThresh;
	tTagCmdDesc.bTag = 1;

	/* ��SPI�е�ģ����Ϣ���������tagʱ��iSModelOffset���� */
	pOrgModelHdrSPI = (PResHeader)((ivAddress)pOrgRes + pOrgGrmMdlHdr->nModelOffset * sizeof(ivInt16));
	ivMemCopy(pNewModelHdr, pOrgModelHdrSPI, sizeof(TResHeader));

	nMixSModelNum = (ivUInt16)(pNewModelHdr->nMixSModelNum); /* ���ں���tag����ʱpLexNode->iSModeOffset */
	nMixSModelWSize = (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel)); /* word */
	
	nCRC =0; 
	nCounter = 0;
	nOffset = nGrmMdlHdrSize;
	/* pNewRes����GrmMdlHdrͷ��Ϣ,������nCRC��Ҫȫ��������ŵõ�. ����ͷ��д */
	pNewLexSPI = (ivPointer)((ivAddress)pNewRes + nOffset);
	for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++) {
		ivPointer pOrgLex;
		bValidGrmID = ivFalse;
		j = 0;
		while(j < nGrmIDNum) {
			if(pOrgGrmDesc->nGrmID == pGrmIDLst[j]) {
				/* Need to add tag cmd */
				bValidGrmID = ivTrue;
				break;
			}
			j++;
		} /* while(j < nGrmIDNum) */

		pOrgLex = (ivPointer)((ivAddress)pOrgRes + pOrgGrmDesc->nGrmOffset * sizeof(ivInt16));
		nGrmSize = pOrgGrmDesc->nGrmSize * sizeof(ivInt16);
		pOrgGrmDesc->nGrmOffset = nOffset / sizeof(ivInt16);

		if(nWorkSpaceSize < nGrmSize) {
			ivAssert(ivFalse);
			return EsErr_OutOfMemory;
		}

		/* ��SPI�е��﷨���翽�����ڴ��н������tag�Ĳ��� */
		ivMemCopy(pWorkSpace, pOrgLex, nGrmSize);

		if(bValidGrmID) {
			/* �����������tag */
			nNewLexSize = nGrmSize + nTagStateNum * sizeof(TLexNode) + sizeof(TCmdDesc);
			if(nWorkSpaceSize < nNewLexSize) {
				ivAssert(ivFalse);
				return EsErr_OutOfMemory;
			}

			if(nOffset + nNewLexSize > *pnNewResSize) {
				ivAssert(ivFalse);
				return EsErr_OutOfMemory;
			}

			err = EsAddTagToLex(pWorkSpace, nTagStateNum, &tTagCmdDesc, (ivUInt16)(nMixSModelNum - 2 - pNewModelHdr->nOldFillerCnt)); /* -2:sil,filler */
			if(ivESR_OK != err) {
				ivAssert(ivFalse);
				return err;
			}

			pOrgGrmDesc->nGrmSize = nNewLexSize / sizeof(ivInt16);

			nGrmSize = nNewLexSize;			
		} /* if(bValidGrmID) */
		else {
			if(nOffset + nGrmSize > *pnNewResSize) {
				ivAssert(ivFalse);
				return EsErr_OutOfMemory;
			}			
		} /* else */

		ivMakeCRC(pWorkSpace, nGrmSize, &nCRC, &nCounter);		
		ivMemCopy(pNewLexSPI, pWorkSpace, nGrmSize); /* nGrmSize��λ:word */

		nOffset += nGrmSize;

		pNewLexSPI = (ivPointer)((ivAddress)pNewRes + nOffset);

		pOrgGrmDesc++;
	} /* for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++) */

	pOrgGrmMdlHdr->nModelOffset = nOffset / sizeof(ivInt16);
	pNewModelHdr->nMixSModelNum += nTagStateNum;

    /* �µ�tagģ�ͷ���sil��fillerģ��ǰ��,���Ҫ�޸�sil��fillerģ�͵�ƫ��ֵ */
    nTagModelSize = (nTagStateNum * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel))) / sizeof(ivInt16);
    pNewModelHdr->nSilSModelOffset += nTagModelSize;
    pNewModelHdr->nFillerSModelOffset += nTagModelSize;
    if(pNewModelHdr->nOldFillerCnt > 0) {
        pNewModelHdr->nOldFillerSModelOffset += nTagModelSize;
    }

    /* ------------------------���¼���ģ��CRC.Begin-------------------------- */ 
	nModelCRC = 0;
	nModelCnt = 0;

    pData = (ivPCInt16)((ivUInt32)pNewModelHdr + sizeof(ivUInt32)*2);
    nSize = sizeof(TResHeader) - sizeof(ivUInt32)*2;
	ivMakeCRC(pData, nSize, &nModelCRC, &nModelCnt);

    pData = (ivPCInt16)((ivUInt32)pOrgModelHdrSPI+sizeof(TResHeader));
    nSize = (pNewModelHdr->nMixSModelNum - nTagStateNum - 2 - pNewModelHdr->nOldFillerCnt) * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel));
	ivMakeCRC(pData, nSize, &nModelCRC, &nModelCnt);
	
    pData = (ivPCInt16)pTagCmdSPI->pMixSModel;
    nSize = nTagStateNum * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel));
    ivMakeCRC(pData, nSize, &nModelCRC, &nModelCnt);	

    ivAssert(pNewModelHdr->nSilSModelOffset < pNewModelHdr->nFillerSModelOffset);
    pData = (ivPCInt16)((ivUInt32)pOrgModelHdrSPI + ((PResHeader)pOrgModelHdrSPI)->nSilSModelOffset * sizeof(ivInt16));
    nSize = (pNewModelHdr->nSilMixture + pNewModelHdr->nFillerMixture + pNewModelHdr->nOldFillerCnt * pNewModelHdr->nOldFillerMix) * sizeof(TGModel);
    ivMakeCRC(pData, nSize, &nModelCRC, &nModelCnt);	

    /* Model Info */
    nSize = sizeof(TGrmDesc)*(pOrgGrmMdlHdr->nTotalGrm-1) + sizeof(TGrmMdlHdr);
    nSize += (nModelCnt + nCounter)*sizeof(ivInt16) + sizeof(TResHeader);
    if( nSize> *pnNewResSize) {
        ivAssert(ivFalse);
        *pnNewResSize = nSize;
        return EsErr_OutOfMemory;
    }

    pNewModelHdr->nCRC = nModelCRC;
	pNewModelHdr->nCRCCnt = nModelCnt;
    /* ------------------------���¼���ģ��CRC.End-------------------------- */

    /* ------------------------���������ģ��д��SPI��.Begin-------------------------- */ 
    pData = (ivPCInt16)pNewModelHdr;
    nSize = sizeof(TResHeader);
	ivMakeCRC(pData, nSize, &nCRC, &nCounter);
	ivMemCopy(pNewLexSPI, pData, nSize);
	nOffset += nSize;
	pNewLexSPI = (ivPointer)((ivUInt32)pNewRes + nOffset);

    pData = (ivPCInt16)((ivUInt32)pOrgModelHdrSPI+sizeof(TResHeader));
    nSize = (pNewModelHdr->nMixSModelNum - nTagStateNum - 2 - pNewModelHdr->nOldFillerCnt) * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel));
    ivMakeCRC(pData, nSize, &nCRC, &nCounter);
    ivMemCopy(pNewLexSPI, pData, nSize);
    nOffset += nSize;
    pNewLexSPI = (ivPointer)((ivUInt32)pNewRes + nOffset);

    pData = (ivPCInt16)pTagCmdSPI->pMixSModel;
    nSize = nTagStateNum * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture - 1) * sizeof(TGModel));
    ivMakeCRC(pData, nSize, &nCRC, &nCounter);
    ivMemCopy(pNewLexSPI, pData, nSize);
    nOffset += nSize;
    pNewLexSPI = (ivPointer)((ivUInt32)pNewRes + nOffset);

    ivAssert(pNewModelHdr->nSilSModelOffset < pNewModelHdr->nFillerSModelOffset);
    pData = (ivPCInt16)((ivUInt32)pOrgModelHdrSPI + ((PResHeader)pOrgModelHdrSPI)->nSilSModelOffset * sizeof(ivInt16));
    nSize = (pNewModelHdr->nSilMixture + pNewModelHdr->nFillerMixture + pNewModelHdr->nOldFillerCnt * pNewModelHdr->nOldFillerMix) * sizeof(TGModel);
    ivMakeCRC(pData, nSize, &nCRC, &nCounter);
    ivMemCopy(pNewLexSPI, pData, nSize);
    nOffset += nSize;
    pNewLexSPI = (ivPointer)((ivUInt32)pNewRes + nOffset);

	pOrgGrmMdlHdr->nCRC = nCRC;
	pOrgGrmMdlHdr->nCRCCnt = nCounter;
	pOrgGrmMdlHdr->nModelSize += nTagStateNum * (sizeof(TMixStateModel) + (pNewModelHdr->nMixture-1) * sizeof(TGModel));

	ivMemCopy(pNewRes, pOrgGrmMdlHdr, nGrmMdlHdrSize);

	*pnNewResSize = nGrmMdlHdrSize + nCounter*sizeof(ivInt16);

#ifdef TAG_LOG
	ParserGrm(pOrgRes, "ParserOrgLex");
	ParserGrm(pNewRes, "ParserTagLex");
#endif

	return EsErrID_OK;
}

/* ������� */
EsErrID ivCall ESRDelTagFromGrm(
								ivPointer      pTagObj,            /* Tag Object */
								ivSize ivPtr   pnTagObjSize,       /* [In/Out] Size of Tag object */
								ivCPointer     pOrgRes,            /* [In] Original Resource */
								ivPointer      pNewRes,            /* [In/Out] Original Resource */
								ivPUInt32      pnNewResSize,       /* [In/Out]Size of pNewRes */ 
								ivUInt16       nDelGrmID,          /* [In] Grammar ID to Delete VoiceTag */
								ivUInt16       nDelTagID           /* [In] VoiceTag ID to Delete */
								)
{
	PGrmMdlHdr pOrgGrmMdlHdr;
	ivSize nGrmMdlHdrSize;
	ivPointer pNewLexSPI;
	PGrmDesc pOrgGrmDesc;
	ivUInt16 i;

	ivUInt32 nCRC;
	ivUInt32 nCounter;

    ivPointer pWorkSpace;
	ivSize nWorkSpaceSize;
	ivUInt16 nGrmSize;

	ivUInt16 nTagStateNum = 0;

	ivUInt32 nOffset;
	EsErrID err;
	PResHeader pModelHdrSPI;

	if(ivNull == pTagObj || ivNull == pnTagObjSize || ivNull == pOrgRes || ivNull == pNewRes || ivNull == pnNewResSize){
		ivAssert(ivFalse);
		return ivESR_INVARG;
	}

	/* Memory align */
	pWorkSpace = (ivPointer)(((ivUInt32)pWorkSpace+IV_PTR_GRID-1)&(~(IV_PTR_GRID-1)));
	nWorkSpaceSize = *pnTagObjSize;

	/* -----------��OrgRes����Դͷ��SPI�п������ڴ���.Begin------------------- */
	pOrgGrmMdlHdr = (PGrmMdlHdr)pWorkSpace;	
	ivMemCopy(pOrgGrmMdlHdr, pOrgRes, sizeof(TGrmMdlHdr)); /* ��SPI Copy */
	if(DW_MINI_RES_CHECK != pOrgGrmMdlHdr->dwCheck){
		return ivESR_INVRESOURCE;
	}

    nGrmMdlHdrSize = sizeof(TGrmMdlHdr) + (pOrgGrmMdlHdr->nTotalGrm-1)*sizeof(TGrmDesc);

    if(nGrmMdlHdrSize > nWorkSpaceSize){
        *pnTagObjSize = nGrmMdlHdrSize;
        return ivESR_OUTOFMEMORY;
    }

	pWorkSpace = (ivPointer)((ivAddress)pWorkSpace + nGrmMdlHdrSize);
	nWorkSpaceSize -= nGrmMdlHdrSize;
	/* -----------��OrgRes����Դͷ��SPI�п������ڴ���.End------------------- */

	/* ----------------------��SPI�е�OrgRes����CRC------------------------- */
	nCRC = 0;
	nCounter = 0;
	ivMakeCRC((ivPCInt16)((ivAddress)pOrgRes+nGrmMdlHdrSize), pOrgGrmMdlHdr->nCRCCnt * sizeof(ivInt16), &nCRC, &nCounter);
	if(nCRC != pOrgGrmMdlHdr->nCRC){
		return ivESR_INVRESOURCE;
	}

	/* ���ÿ���﷨������ص��ڴ��н��в��� */
	nCRC =0; 
	nCounter = 0;
	nOffset = nGrmMdlHdrSize;
	/* pNewRes����GrmMdlHdrͷ��Ϣ,������nCRC��Ҫȫ��������ŵõ�. ����ͷ��д */
	pNewLexSPI = (ivPointer)((ivAddress)pNewRes + nOffset);
	pOrgGrmDesc = pOrgGrmMdlHdr->tGrmDesc;
	for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++, pOrgGrmDesc++)
	{
		ivCPointer pOrgLex;
	
		pOrgLex = (ivCPointer)((ivAddress)pOrgRes + pOrgGrmDesc->nGrmOffset * sizeof(ivInt16));
		nGrmSize = (ivUInt16)(pOrgGrmDesc->nGrmSize * sizeof(ivInt16));
		pOrgGrmDesc->nGrmOffset = nOffset / sizeof(ivInt16);

		if(nGrmSize > nWorkSpaceSize){
			ivAssert(ivFalse);
			*pnTagObjSize = nGrmMdlHdrSize + nGrmSize;
			return ivESR_OUTOFMEMORY;
		}

		/* ��SPI�е��﷨���翽�����ڴ��н���ɾ��tag�Ĳ��� */
		ivMemCopy(pWorkSpace, pOrgLex, nGrmSize);
		
		/* ----------ɾ��һ��Tag------------ */
		if(pOrgGrmDesc->nGrmID == nDelGrmID){

			/* ���ܴ��ڶ��nDelTagID��tag.ȫ��ɾ�� */
			while(1){
				err = EsDelTagFromLex(pWorkSpace, nDelTagID, &nTagStateNum);
				if(ivESR_OK != err){
					ivAssert(ivFalse);
					return err;
				}

				if(0 == nTagStateNum){ /* �������Ѿ�û��Ҫɾ��Tag�� */
					break;
				}

				nGrmSize -= (nTagStateNum*sizeof(TLexNode) + sizeof(TCmdDesc));				
			}

			if(nOffset + nGrmSize > *pnNewResSize) {
				ivAssert(ivFalse);
				return ivESR_OUTOFMEMORY;
			}
			

			pOrgGrmDesc->nGrmSize = nGrmSize / sizeof(ivInt16);	
		}

		ivMakeCRC(pWorkSpace, nGrmSize, &nCRC, &nCounter);			
		ivMemCopy(pNewLexSPI, pWorkSpace, nGrmSize); /* nGrmSize��λ:word */

		nOffset += nGrmSize;
		pNewLexSPI = (ivPointer)((ivAddress)pNewRes + nOffset);
	} /* for(i=0; i<pOrgGrmMdlHdr->nTotalGrm; i++) */

	/* Model Info */
	if(nOffset + pOrgGrmMdlHdr->nModelSize > *pnNewResSize){
		ivAssert(ivFalse);
		return ivESR_OUTOFMEMORY;
	}

	pModelHdrSPI = (PResHeader)((ivAddress)pOrgRes + pOrgGrmMdlHdr->nModelOffset * sizeof(ivInt16));
	ivMakeCRC((ivPCInt16)pModelHdrSPI, pOrgGrmMdlHdr->nModelSize, &nCRC, &nCounter);
	ivMemCopy(pNewLexSPI, (ivCPointer)pModelHdrSPI, pOrgGrmMdlHdr->nModelSize);

	pOrgGrmMdlHdr->nModelOffset = nOffset / sizeof(ivInt16);
	pOrgGrmMdlHdr->nCRC = nCRC;
	pOrgGrmMdlHdr->nCRCCnt = nCounter;	

	ivMemCopy(pNewRes, pOrgGrmMdlHdr, nGrmMdlHdrSize);

	*pnNewResSize = nGrmMdlHdrSize + nCounter * sizeof(ivInt16);

#ifdef TAG_LOG
	ParserGrm(pOrgRes, "ParserOrgLex");
	ParserGrm(pNewRes, "ParserTagLex");
#endif

	return ivESR_OK;
}
#endif /* MINI5_SPI_MODE */

#endif /* #if MINI5_SPI_MODE && MINI5_TAG_SUPPORT */
