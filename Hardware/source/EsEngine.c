/*********************************************************************#
//	�ļ���		��ESREngine.c
//	�ļ�����	��
//	����		��Truman
//	����ʱ��	��2007��8��1��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/

#include "EsKernel.h"
#include "EsEngine.h"
#if !MINI5_USE_NEWCM_METHOD
#include "EsSearchConst.h"
#endif


#if LOG_FIXED_FEATURE
#include <stdio.h>
char g_szFeaFile[256];
#endif


//hbtao  ����ֱ�ӵ������� begin
#if LOAD_FLOAT_FEA
#include <stdio.h>
char g_szFloatFea[256];
#if LOAD_FLOAT_MDOEL
float (ivPtr g_ppFeature)[FEATURE_DIMNESION] = ivNull;
#else
ivInt16 (ivPtr g_ppFeature)[FEATURE_DIMNESION] = ivNull;
#endif
int g_nFrameCnt;

extern ivPInt16        g_ps16MeanCoef;
extern ivPInt16        g_ps16MeanCoefQ;   /* MeanCoef�ĸ�ά���� */
#endif
//hbtao  ����ֱ�ӵ������� end

#if RATETSET_USE_VAD
ivUInt16 g_bDisableVad = 0;
#endif /* RATETSET_USE_VAD */

ivUInt16 EsCMScoreCDFMatch(ivInt32 nCMScore); /* normalize result cm score to 0-100 score 20100728 */

ivUInt16 g_bChkAmbientNoise = ES_DEFAULT_AMBIENTNOISE;

ivUInt16 g_nPriorResultID = 0;	/* ��¼��һ�ε�ʶ����ID������CMN���²��ԣ����ֱ���ʶ����ID���ϴ�һ���Ͳ�����CMN���£���ֹһֱ˵���CMN���¹��� 20110107 */

#define EsErr_Restart	((EsErrID)20) /* ��VAD���0.4s��������CM�÷ֵ������޵÷�(̧��)����VAD��Ϊ���0.75s�жϽ��� 20110121 */

EsErrID EsInit(PEsrEngine pEngine,ivCPointer pModel) /* ��˶��ƽ̨AitalkMini1.0������ͬ */
{
	EsErrID err;

	pEngine->dwCheck = DW_ENGINE_CHECK;
	pEngine->bRun = ivFalse;

	err = EsFrontInit(&pEngine->tFront, pEngine);
#if MINI5_API_PARAM_CHECK
    ivAssert(EsErrID_OK == err);
	if(EsErrID_OK != err){
		return err;
	}
#endif

	err = EsSearchInit(&pEngine->tSearch,pModel, pEngine);
#if MINI5_API_PARAM_CHECK
    ivAssert(EsErrID_OK == err);
	if(EsErrID_OK != err){
		return err;
	}
#endif

	return EsErrID_OK;
}

#if MINI5_API_PARAM_CHECK
EsErrID	EsValidate(PEsrEngine pEngine) /* ��˶��ƽ̨AitalkMini1.0������ͬ */
{
	ivAssert(ivNull != pEngine);
	if(ivNull == pEngine){
		return EsErr_InvCal;
	}
	ivAssert(DW_ENGINE_CHECK == pEngine->dwCheck);
	if(DW_ENGINE_CHECK != pEngine->dwCheck){
		ivAssert(ivFalse);
		return EsErr_InvCal;
	}

	return EsErrID_OK;
}
#endif

EsErrID EsReset(PEsrEngine pEngine) /* ��˶��ƽ̨AitalkMini1.0������ͬ */
{
	EsErrID err;

#if MINI5_API_PARAM_CHECK
	/* Check Engine object */
	err = EsValidate(pEngine);
	if(EsErrID_OK != err){
		return err;
	}
#endif

	EsFrontReset(&pEngine->tFront);
	
	EsSearchReset(&pEngine->tSearch);

	pEngine->bRun = ivFalse;

	return EsErrID_OK;
}

/* normalize result score to 0-100 score 20100728 */
#if MINI5_USE_NEWCM_METHOD
ivUInt16 EsCMScoreCDFMatch(ivInt32 nScore)
{
	ivInt32 nThresh;

	//�ڲ�CMScore����Ϊ0-100�ĵ÷֡�׼��:...-47->29, -48->30, -47->31,....
	nThresh = nScore + 78; //nScore - (-48) + 30;
	if(nThresh >= 100){
		return 100;
	}
	else if(nThresh <= 0){
		return 0;
	}
	
	return (ivUInt16)nThresh;
}
#else
ivUInt16 EsCMScoreCDFMatch(ivInt32 nScore)  /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: �����ӿ� */
{
    ivUInt16 i;

    if(nScore < g_s16CMScore[0]){
        return 0;
    }

    for(i=0; i<100; i++){
        if(nScore < g_s16CMScore[i]){
            return i;
        }
    }

    return i;
}

#endif

#if RATETEST_TOOL_LOG
float g_fCMScore = 1000;
int g_nSearchStartMS, g_nSearchEndMS;

void LogResult(PEsrEngine pEngine, float fCMScore)
{
	g_fCMScore = fCMScore;

	g_nSearchStartMS = ivMax(0, pEngine->tFront.m_iSpeechBegin - SPEECH_BEGIN_MARGIN)*10;
	g_nSearchEndMS = (int)(pEngine->tFront.m_iSpeechEnd*10);

    /* g_nSearchStartMS = (int)(pEngine->tFront.m_iSpeechBegin*10);
    g_nSearchEndMS = (int)(pEngine->tFront.m_iSpeechEnd*10); */

}
#endif

#if !MINI5_USE_NEWCM_METHOD
ivInt32 LAddQ10(ivInt32 x, ivInt32 y) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: �����ӿڡ�ͰLAdd,���Ȳ�ͬ */
{
	ivInt32 diff;

	ivStatic ivConst ivUInt32 g_fLaddTab[16] =
	{
		5182/8,
		4285/8,
		3511/8,
		2854/8,
		2303/8,
		1846/8,
		1472/8,
		1168/8,
		924/8,
		728/8,
		572/8,
		449/8,
		352/8,
		275/8,
		215/8,
		168/8
	};


	if(y>x){
		diff = (y-x) >> 8;	/* Q10->Q2 */
		if(diff >= 16){
			return y;
		}
		else{
			return y+g_fLaddTab[diff];
		}
	}
	else{
		diff = (x-y) >> 8;
		if(diff >= 16){
			return x;
		}
		else{
			return x+g_fLaddTab[diff];
		}
	}
}
#endif

#if LOAD_FLOAT_MDOEL
#include <math.h>
double LAddFlt(double x, double y)
{
    double temp,diff,z;
    if (x < y)
    {
        temp = x; x = y; y = temp;
    }
    diff = y - x;
    if (diff < LMINEXP)
    {
        return (x < LSMALL)?LZERO:x;
    }
    else 
    {
        z = exp(diff);
        return x + log(1.0 + z);
    }
}
#endif
void EsOutputResult(PEsrEngine pEngine, PEsrResult pResult)
{
	ivInt16     nConfidence;
    PCmdDesc    pCmdDesc;
#if !MINI5_USE_NEWCM_METHOD
    ivInt32		fCMScore = LZERO;
    ivInt32		fScoreTmp;
#endif

#if MINI5_SPI_MODE
    ivPointer   pSPIDesc;
#endif

	PEsSearch   pEsSearch = &pEngine->tSearch;

    pEngine->iEngineStatus = ES_STATUS_RESULT;
    pResult->nConfidenceScore = 0;
    pResult->nResultID = 32768;

    if (0 == pEsSearch->tNBest[0].nWordID ) {
        pResult->nResultType = ES_TYPE_RESULTNONE;
        EsReset(pEngine);

        return;
    }

#if MINI5_SPI_MODE
    pSPIDesc = (ivPointer)((ivAddress)pEsSearch->pCmdDesc + (pEsSearch->tNBest[0].nWordID-1)*sizeof(TCmdDesc));
    pCmdDesc = (PCmdDesc)((ivAddress)pEsSearch->pnBucketCacheLast + ivGridSize(sizeof(TEsrResult)));
    EsSPIReadBuf((ivPointer)pCmdDesc, pSPIDesc, sizeof(TCmdDesc));
#else
    pCmdDesc = &pEsSearch->pCmdDesc[pEsSearch->tNBest[0].nWordID-1];
#endif

#if MINI5_USE_NEWCM_METHOD
    nConfidence = EsCMScoreCDFMatch(pEsSearch->tNBest[0].fCMScore);
#else
    /*
    ԭ��CM����,֡ƽ��ֵ
    CM=0.2(pResult[0].fOutlike - fFillerScore) + 0.8(pResult[0].fOutlike - LAdd(pResult[0].fOutlike, pResult[1].fOutlike, pResult[2].fOutlike)

    LAdd(x1,x2) = t + LAdd(x1-t,x2-t)

    = 0.2(pResult[0].fOutlike - fFillerScore) + 0.8(pResult[0].fOutlike - fFillerScore - LAdd(pResult[0].fOutlike-fFillerScore, pResult[1].fOutlike-fFillerScore, pResult[2].fOutlike-fFillerScore)
    = (pResult[0].fOutlike - fFillerScore) - 0.8*LAdd(pResult[0].fOutlike-fFillerScore, pResult[1].fOutlike-fFillerScore, pResult[2].fOutlike-fFillerScore)
    ->��Ʒ:pEsSearch->tNBest[0].fScore/pEsSearch->iFrameIndex - 0.8LAdd(pEsSearch->tNBest[0].fScore/pEsSearch->iFrameIndex, pEsSearch->tNBest[1].fScore/pEsSearch->iFrameIndex,pEsSearch->tNBest[2].fScore/pEsSearch->iFrameIndex)
    */

    /* ����CM = NBest*0.8 + fillerScore*0.2 */
    fScoreTmp = LAddQ10((ivInt32)pEsSearch->tNBest[0].fScore*(1<<6)/(ivInt32)pEsSearch->iFrameIndexLast, (ivInt32)pEsSearch->tNBest[1].fScore*(1<<6)/(ivInt32)pEsSearch->iFrameIndexLast);
    fScoreTmp = LAddQ10(fScoreTmp, (ivInt32)pEsSearch->tNBest[2].fScore*(1<<6)/(ivInt32)pEsSearch->iFrameIndexLast);
    fCMScore = (ivInt32)pEsSearch->tNBest[0].fScore*(1<<6)/(ivInt32)pEsSearch->iFrameIndexLast - (fScoreTmp*4)/5;
    nConfidence = EsCMScoreCDFMatch(fCMScore);
#if LOAD_FLOAT_MDOEL
    {
        int i;
        float fFillerScore = LZERO, fBestScore = LZERO, fNBestScore = LZERO, fConfidence = LZERO, fOneBestCM, fBest1Outlike = LZERO;

        fFillerScore = pEsSearch->fFillerScoreSum / pEsSearch->iFrameIndexLast;
        for(i=0; i<ESR_MAX_NBEST; i++)
        {
            if(0 ==pEsSearch->tNBest[i].nWordID)
            {
                break;
            }
            fBestScore = pEsSearch->tNBest[i].fScore / pEsSearch->iFrameIndexLast;
            if(0 == i)
            {
                fBest1Outlike = fBestScore;
                fConfidence = fBestScore - fFillerScore;
            }

            fNBestScore = LAddFlt(fNBestScore, fBestScore);
        }

        fOneBestCM = fConfidence * 0.2 + (fBest1Outlike - fNBestScore) * 0.8;

        printf("fCM=%.2f\r\n", fOneBestCM);
    }
#endif
#endif

#if RATETEST_TOOL_LOG   /* For Log */
#if MINI5_USE_NEWCM_METHOD
    LogResult(pEngine, pEsSearch->tNBest[0].fCMScore);
#else
    LogResult(pEngine, fCMScore/1024.0);
#endif
#endif

    if(0 == pEsSearch->iFrameIndexLast){
        pResult->nResultType = ES_TYPE_RESULTNONE;
    }

#if ESR_ENAHNCE_VAD
	/* Get speech frame num */
	if(0 == pEngine->tFront.m_iSpeechEnd){
		nSpeechFrame = 0;
	}
	else{
		nSpeechFrame = pEngine->tFront.m_iSpeechEnd - pEngine->tFront.m_iSpeechBegin - (ivInt32)SPEECH_END_MARGIN;
	}
#endif

	if(nConfidence < pCmdDesc->nCMThresh){

#if ESR_ENAHNCE_VAD
		if(nSpeechFrame > pEngine->tFront.m_nSpeechTimeOut){
			/* Speech time out & ResultNone */
			iStatus = EsErr_SpeechTimeOut;
		}
		else if(nSpeechFrame >= (ivInt32)MINIMUM_SPEECH_FRAMENUM && nSpeechFrame <= (ivInt32)MINIMUM2_SPEECH_FRAMENUM){
			/* 0.25s<SpeechTime<0.35s & ResultNone */
			iStatus = EsErr_UnbelievablySpeech;
		}
		else{
			iStatus = EsErr_ResultNone;		
		}
#else
		pResult->nResultType = ES_TYPE_RESULTNONE;	
#endif
		
	}
#if ESR_ENAHNCE_VAD
	else if((ivInt32)EHIGH_CHKEND_FRAMENUM == pEngine->tFront.m_nChkEndFrameNum && fCMScore <pCmdDesc->nCMThresh + 102){
		/* VAD�����0.4s�״ν�������CM�÷���������趨����+0.1��(̧��)�����ٵ�0.75-0.4s */
		iStatus = EsErr_Restart;
		return iStatus;
	}
	//else if(g_bChkAmbientNoise  && (pEngine->tFront.m_iSpeechEnd > 0) && (pEngine->tFront.m_iSpeechEnd - pEngine->tFront.m_iSpeechBegin < MINIMUM_SPEECH_FRAMENUM+SPEECH_END_MARGIN)){
	else if(g_bChkAmbientNoise  &&   nSpeechFrame < (ivInt32)MINIMUM_SPEECH_FRAMENUM){
#else
	else if(g_bChkAmbientNoise  && (pEngine->tFront.m_iSpeechEnd > 0) && (pEngine->tFront.m_iSpeechEnd - pEngine->tFront.m_iSpeechBegin < pEngine->nMinSpeechFrm)){
#endif
 		pResult->nResultType = ES_TYPE_AMBIENTNOISE;
 	}
	else{		
		pResult->nResultID = pCmdDesc->nID;	
		pResult->nConfidenceScore = nConfidence;
		pResult->nResultType = ES_TYPE_RESULT;

		/* ��ǰCM�÷ֱ����޵÷ָ�0.1*(1<<10)�Ÿ���CMN����ֹ����ʶ��ʱ�м���˵��CMN���²��� 20100706 */
#if RATETSET_USE_VAD
        if(!g_bDisableVad) {
            if(nConfidence - pCmdDesc->nCMThresh > 10 && g_nPriorResultID != pEsSearch->tNBest[0].nWordID){			
                EsFrontUpdateMean(&pEngine->tFront);
            }
            g_nPriorResultID = pEsSearch->tNBest[0].nWordID;
        }
#endif /* RATETSET_USE_VAD */
		
	}

#if RATETSET_USE_VAD
    if(g_bDisableVad) {
        EsFrontUpdateMean(&pEngine->tFront);
    }
#endif /* RATETSET_USE_VAD */

	EsReset(pEngine);
}
//hbtao  ����ֱ�ӵ������� begin
#if LOAD_FLOAT_FEA

void Swap32(ivUInt32 *pData)
{
    char bLow, bMLow, bMHigh, bHigh;
    char * pU8Data;

    pU8Data = (char *)(pData);
    bLow = pU8Data[0];
    bMLow = pU8Data[1];
    bMHigh = pU8Data[2];
    bHigh = pU8Data[3];

    pU8Data[0] = bHigh;
    pU8Data[1] = bMHigh;
    pU8Data[2] = bMLow;
    pU8Data[3] = bLow;
}
void Swap16(ivUInt16 *pData)
{
    char bLow, bHigh;
    char * pU8Data;

    pU8Data = (char *)(pData);
    bLow = pU8Data[0];
    bHigh = pU8Data[1];

    pU8Data[0] = bHigh;
    pU8Data[1] = bLow;
}
void LoadFloatFea(PEsrEngine pEngine)
{

    FILE *fp ;
    int i, j, nSize;
    ivUInt32 nFrameCnt,nFlag, iFrame;
    ivUInt16 nVec, nFlag2;
    float pfMFCC[FEATURE_DIMNESION];
    
    fp = fopen(g_szFloatFea, "rb");
    if(NULL == fp)
    {
        ivAssert(ivFalse);
        return;
    }

    fread(&nFrameCnt, 4, 1, fp);
    Swap32((ivPUInt32)&nFrameCnt);	

    fread(&nFlag, 4, 1, fp);
    Swap32((ivPUInt32)&nFlag);

    fread(&nVec, 2, 1, fp);
    Swap16((ivPUInt16)&nVec);

    fread(&nFlag2, 2, 1, fp);
    Swap16((ivPUInt16)&nFlag2);


    if(NULL != g_ppFeature)
    {
        free(g_ppFeature);
        g_ppFeature = NULL;
    }

#if LOAD_FLOAT_MDOEL
    g_ppFeature = malloc(nFrameCnt * sizeof(float) * FEATURE_DIMNESION);
#else
    g_ppFeature = malloc(nFrameCnt * sizeof(ivInt16) * FEATURE_DIMNESION);
#endif
    ivAssert(NULL != g_ppFeature);

    g_nFrameCnt = nFrameCnt;

    nSize = 12;

    iFrame = 0;
    while(!feof(fp)){       

        for(i=0; i<FEATURE_DIMNESION; i++){
            fread(&pfMFCC[i], 4, 1, fp);
            Swap32((ivPUInt32)(&pfMFCC[i]));
        }
        nSize += FEATURE_DIMNESION * sizeof(ivInt32);			

        for ( i = 0; i < FEATURE_DIMNESION ; ++i )
        {
#if LOAD_FLOAT_MDOEL
            g_ppFeature[iFrame][i] = pfMFCC[i];
#else
            double fea;				

            if(i>=8)
            {
                fea = pfMFCC[i]*(1<<ESR_Q_MEAN_MANUAL) * 10 * g_ps16MeanCoef[i]/(1<<g_ps16MeanCoefQ[i]); //Q:ESR_Q_MEAN_MANUAL
            }
            else
            {
                fea = pfMFCC[i]*(1<<ESR_Q_MEAN_MANUAL) * g_ps16MeanCoef[i]/(1<<g_ps16MeanCoefQ[i]); //Q:ESR_Q_MEAN_MANUAL
            }
            
            ivAssert(fea >=-32768 && fea <=32767);
            if(fea >= 0){
                g_ppFeature[iFrame][i] = (ivInt16)(fea + 0.5); //Q14
            }
            else{
                g_ppFeature[iFrame][i] = (ivInt16)(fea - 0.5);
            }

            g_ppFeature[iFrame][i] <<= ESR_VAR_QUANT_BIT;
#if (5 == ESR_VAR_QUANT_BIT && 6 == ESR_MEAN_QUANT_BIT)
            g_ppFeature[iFrame][i] &= 0xFFE0;
            /* ���б��ͣ���֤mean�ڶ��귶Χ�ڲ�����.mean��6bit��ʾ,��Ч��ֵΪ5bit. 2011-01-20 */
            if(g_ppFeature[iFrame][i] > (ivInt16)0x03E0){
                g_ppFeature[iFrame][i] = (ivInt16)0x03E0;
            }
            else if(g_ppFeature[iFrame][i] < (ivInt16)0xFC00){
                g_ppFeature[iFrame][i] = (ivInt16)0xFC00;
            }
#elif (6 == ESR_VAR_QUANT_BIT && 6 == ESR_MEAN_QUANT_BIT)  
            g_ppFeature[iFrame][i] &= 0xFFC0;
            /* ���б��ͣ���֤mean�ڶ��귶Χ�ڲ�����.mean��6bit��ʾ,��Ч��ֵΪ5bit. 2011-01-20 */
            if(g_ppFeature[iFrame][i] > (ivInt16)0x09C0){
                g_ppFeature[iFrame][i] = (ivInt16)0x09C0;
            }
            else if(g_ppFeature[iFrame][i] < (ivInt16)0xF800){
                g_ppFeature[iFrame][i] = (ivInt16)0xF800;
            }  
#elif (5 == ESR_VAR_QUANT_BIT && 7 == ESR_MEAN_QUANT_BIT)  
            g_ppFeature[iFrame][i] &= 0xFFE0;
            /* ���б��ͣ���֤mean�ڶ��귶Χ�ڲ�����.mean��6bit��ʾ,��Ч��ֵΪ5bit. 2011-01-20 */
            if(g_ppFeature[iFrame][i] > (ivInt16)0x07E0){
                g_ppFeature[iFrame][i] = (ivInt16)0x07E0;
            }
            else if(g_ppFeature[iFrame][i] < (ivInt16)0xF800){
                g_ppFeature[iFrame][i] = (ivInt16)0xF800;
            } 
#elif (6 == ESR_VAR_QUANT_BIT && 7 == ESR_MEAN_QUANT_BIT)  
            g_ppFeature[iFrame][i] &= 0xFFC0;
            /* ���б��ͣ���֤mean�ڶ��귶Χ�ڲ�����.mean��6bit��ʾ,��Ч��ֵΪ5bit. 2011-01-20 */
            if(g_ppFeature[iFrame][i] > (ivInt16)0x2FC0){
                g_ppFeature[iFrame][i] = (ivInt16)0x2FC0;
            }
            else if(g_ppFeature[iFrame][i] < (ivInt16)0xF000){
                g_ppFeature[iFrame][i] = (ivInt16)0xF000;
            } 
#else
            ivAssert(ivFalse);
#endif
            g_ppFeature[iFrame][i] -= 0x800; /* ֻ�е���ֵ�����Ϊ4KWʱ����Ҫ�˲� */
#endif
        }
        iFrame ++;
        ivAssert(iFrame <= nFrameCnt);
        if(iFrame >= nFrameCnt)
        {
            g_nFrameCnt = iFrame;
            break;
        }
    } 

    fclose(fp);

    _CrtCheckMemory();
}
/* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ��΢���,ֻ��pResult�ṹ��if(pFront->m_fSpeechEnd || pSearch->nSpeechFrame > ESR_MAX_SPEECH_FRAMENUM){��ͬ */
EsErrID EsRunStep(PEsrEngine pEngine, ivUInt32 dwMessage, ivESRStatus ivPtr pStatus, PCEsrResult ivPtr ppResult)
{
    EsErrID err = EsErrID_OK;
    PESRFront pFront = &pEngine->tFront;
    PEsSearch pSearch = &pEngine->tSearch;
    int i;

    ivAssert(!pEngine->bRun);
    if(pEngine->bRun){
        return EsErr_ReEnter;
    }
    pEngine->bRun = 1;

    /* �൱��Mini4.0��֮ǰ�汾��ESREndAppendData */
    switch(dwMessage) {
        case ES_MSG_ENDAPPENDDATA:
            pFront->m_bEndAppendData = ivTrue;
            break;
        case ES_MSG_RESERTSEARCHR:
            /* ����δ��ʼʱ�����˽���˵�RAM,���Դ˴���ҪReset�� */
            EsSearchReset(&pEngine->tSearch);
            pEngine->iEngineStatus = 0;
            break;
    }

    /* �ҵ���ʼ�㣬�ȴ����������ڴ��ͷ� */
    if (ES_STATUS_FINDSTART == pEngine->iEngineStatus) {
        *pStatus = pEngine->iEngineStatus;
        pEngine->bRun = 0;
        return ivESR_OK;
    }

    /* check whether set grammar and malloc score buffer */
#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT
    if(ivNull == pEngine->tSearch.pStaScoreCacheLast){
            ivAssert(ivFalse);
            pEngine->bRun = 0;
            return (ivESRStatus)EsErr_InvCal;
    }
#endif

    if(ivNull == pEngine->tSearch.pSearchNodeLast){
            ivAssert(ivFalse);
            pEngine->bRun = 0;
            return (ivESRStatus)EsErr_InvCal;
    }

    if(0 == pSearch->iFrameIndexLast)
    {
        LoadFloatFea(pEngine);
    }

    for(i=0; i<g_nFrameCnt; i++)
    {

        int j;
        for(j=0; j<FEATURE_DIMNESION; j++)
        {
            pSearch->fFeature[j] = g_ppFeature[i][j];
        }

#ifdef ESR_TAG_SUPPORT
        if(ESENGINE_TAG == pEngine->iEngineType){
            err = EsTagSearchFrameStep(pSearch);
        }
        else
#endif
        {
            err = EsSearchFrameStep(pSearch);
        }
    }


    /* ������ʶ��VAD�������������Ϊ������ʼ����Ϊ���������ˣ��������������ʶ�� */
    if(1) {
        ivBool bSpeechTimeOut = pFront->m_bSpeechTimeOut;

#if MINI5_ENHANCE_FRONT
        EsGetEnhanceFrontResult(pFront);
#endif

#ifdef ESR_TAG_SUPPORT
        if(ESENGINE_TAG == pEngine->iEngineType){
            PEsrResult pResult;

            err = EsSPIWriteFlush(&pSearch->tSPICache);
            if(EsErrID_OK != err)
            {
                return err;
            }

            err = EsTagOutputResult(pEngine);
            //������ǰ�˵Ľṹͨ��pResult�׳�
            pResult = (PEsrResult)pSearch->pStaScoreCacheLast;
            pResult->bSpeechEnergyLower = pFront->m_bSpeechLowEnergy;
            pResult->bSNRLower = pFront->m_bSpeechLowSNR;
            pResult->bSpeechCut = pFront->m_bSpeechCut;
            *ppResult = pResult;
        }
        else
#endif
        {
            PEsrResult  pResult;
            /* ������û��ʶ������������pResult,���ڴ�������ǰ�˼���� */
            pResult = (PEsrResult)pSearch->pnBucketCacheLast;
            pResult->nConfidenceScore = 0;
            pResult->nResultID = 32768;
#if MINI5_ENHANCE_FRONT            
            pResult->bSpeechCut = pEngine->tFront.m_bSpeechCut;
            pResult->bSNRLower = pEngine->tFront.m_bSpeechLowSNR;
            pResult->bSpeechEnergyLower = pEngine->tFront.m_bSpeechLowEnergy;
#endif
            EsOutputResult(pEngine, pResult);

            if(bSpeechTimeOut && ES_TYPE_RESULT == pResult->nResultType){
                pResult->nResultType = ES_TYPE_FORCERESULT;
            }
            *ppResult = pResult;
        }

        *pStatus = pEngine->iEngineStatus;
        pEngine->bRun = ivFalse;

#if ESR_ENAHNCE_VAD
        if(EsErr_Restart == err){
            pFront->m_nChkEndFrameNum = (ivInt32)EHIGH2_CHKEND_FRAMENUM;
            pFront->m_iVADState = (ivUInt16)ESVAD_CHECK_END;
            pFront->m_fSpeechEnd = 0;
        }
        else{
            return err;
        }
#else
        pEngine->bRun = 0;
        return err;
#endif
    }

 
    pEngine->bRun = 0;

    return err;
}
#else
//hbtao  ����ֱ�ӵ������� end
EsErrID EsRunStep(PEsrEngine pEngine, ivUInt32 dwMessage, ivESRStatus ivPtr pStatus, PCEsrResult ivPtr ppResult)
{
    EsErrID err = EsErrID_OK;
    PESRFront pFront = &pEngine->tFront;
    PEsSearch pSearch = &pEngine->tSearch;
    PEsrResult  pResult;

    ivAssert(!pEngine->bRun);
    if(pEngine->bRun){
        return EsErr_ReEnter;
    }
    pEngine->bRun = 1;

    /* �൱��Mini4.0��֮ǰ�汾��ESREndAppendData */
    if (ES_MSG_ENDAPPENDDATA == dwMessage) {
        pFront->m_bEndAppendData = ivTrue;
    }
    if (ES_MSG_RESERTSEARCHR == dwMessage) {
        /* ����δ��ʼʱ�����˽���˵�RAM,���Դ˴���ҪReset�� */
        EsSearchReset(&pEngine->tSearch);
        pEngine->iEngineStatus = 0;
    }

    /* check whether set grammar and malloc score buffer */
#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT
    if(ESENGINE_TAG == pEngine->iEngineType && ivNull == pEngine->tSearch.pStaScoreCacheLast) {
        ivAssert(ivFalse);
        pEngine->bRun = 0;
        return EsErr_InvCal;
    }
#endif

    if(ESENGINE_REC == pEngine->iEngineType && ivNull == pEngine->tSearch.pSearchNodeLast){
        ivAssert(ivFalse);
        pEngine->bRun = 0;
        return EsErr_InvCal;
    }

    /* ������ʶ��VAD�������������Ϊ������ʼ����Ϊ���������ˣ��������������ʶ�� */
    if(pFront->m_fSpeechEnd || pFront->m_bEndAppendData) {
        ivBool bSpeechTimeOut = pFront->m_bSpeechTimeOut;

#if MINI5_ENHANCE_FRONT
        EsGetEnhanceFrontResult(pFront);
#endif
        /* ������û��ʶ������������pResult,���ڴ�������ǰ�˼���� */
        pResult = (PEsrResult)pSearch->pnBucketCacheLast;
#if MINI5_ENHANCE_FRONT
        pResult->bSpeechCut = pEngine->tFront.m_bSpeechCut;
        pResult->bSNRLower = pEngine->tFront.m_bSpeechLowSNR;
        pResult->bSpeechEnergyLower = pEngine->tFront.m_bSpeechLowEnergy;
#endif

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT
        if(ESENGINE_TAG == pEngine->iEngineType) {
            err = EsSPIWriteFlush(&pSearch->tSPICache);
            if(EsErrID_OK != err) {
                return err;
            }

            err = EsTagOutputResult(pEngine);
        }
        else
#endif
        {
            EsOutputResult(pEngine, pResult);

            if(bSpeechTimeOut && ES_TYPE_RESULT == pResult->nResultType) {
                pResult->nResultType = ES_TYPE_FORCERESULT;
            }
        }

        *ppResult = pResult;
        *pStatus = pEngine->iEngineStatus;
        pEngine->bRun = 0;

#if ESR_ENAHNCE_VAD
        if(EsErr_Restart == err){
            pFront->m_nChkEndFrameNum = (ivInt32)EHIGH2_CHKEND_FRAMENUM;
            pFront->m_iVADState = (ivUInt16)ESVAD_CHECK_END;
            pFront->m_fSpeechEnd = 0;
        }
        else{
            return err;
        }
#else
        return err;
#endif
    }

    err = EsFrontGetFeature(pFront,pSearch->fFeature);
    if (EsErr_BufferEmpty == err) {
        pEngine->bRun = ivFalse;
        return err;
    }
    else if(EsErr_FindStart == err ) {
        pEngine->iEngineStatus = ES_STATUS_FINDSTART;
    }

#if LOG_DEBUG_INFO
    {
        int i;
        static FILE *g_fpSearchFea = NULL;
        if(NULL == g_fpSearchFea)
        {
            g_fpSearchFea = fopen("E:\\SearchFea_mini5.log", "wb");
        }
        fprintf(g_fpSearchFea, "iFrame=%d: ", pEngine->tSearch.iFrameIndexLast);
        for(i=0; i<FEATURE_DIMNESION; i++)
        {
            fprintf(g_fpSearchFea, "%d, ", pEngine->tSearch.fFeature[i]);
        }
        fprintf(g_fpSearchFea, "\r\n");
        fflush(g_fpSearchFea);
    }
#endif

#if LOG_FIXED_FEATURE
    {
        static FILE *g_fpLogFea = NULL;
        if(0 == pSearch->iFrameIndexLast)
        {
            if(NULL != g_fpLogFea)
            {
                fclose(g_fpLogFea);
            }
            g_fpLogFea = fopen(g_szFeaFile, "wb");
        }
        fwrite(pSearch->fFeature, FEATURE_DIMNESION*sizeof(ivInt16), 1, g_fpLogFea);
        fflush(g_fpLogFea);
    }
#endif

    /* �ҵ���ʼ�㣬�ȴ����������ڴ��ͷ� */
    if (ES_STATUS_FINDSTART == pEngine->iEngineStatus) {
        *pStatus = pEngine->iEngineStatus;
        pEngine->bRun = 0;

        return ivESR_OK;
    }

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT
    if(ESENGINE_TAG == pEngine->iEngineType) {
        err = EsTagSearchFrameStep(pSearch);
    }
    else
#endif
    {
        err = EsSearchFrameStep(pSearch);
    }

    pEngine->bRun = 0;

    return err;
}
//hbtao  ����ֱ�ӵ������� begin
#endif
//hbtao  ����ֱ�ӵ������� end
ivPUInt16 EsAlloc(PEsrEngine pEngine,ivUInt16 nSize)  /* ��˶��ƽ̨AitalkMini1.0������ͬ */
{
	ivPUInt16 p;

	nSize = (nSize+3)&(~(4-1));

	ivAssert((ivUInt32)pEngine->pBuffer+nSize <= (ivUInt32)pEngine->pBufferEnd);
	if((ivUInt32)pEngine->pBuffer+nSize > (ivUInt32)pEngine->pBufferEnd){
		return ivNull;
	}
	p = pEngine->pBuffer;	
	pEngine->pBuffer = (ivPUInt16)((ivUInt32)pEngine->pBuffer + nSize);

	return p;
}

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT

#if LOG_DEBUG_INFO
void LogTagModelInfo(PEsrEngine pEngine)
{
    PTagCmd pTagCmd;
    PMixStateModel pState;
    int i, j, k;

    FILE *fpTag  = NULL;
    
    fpTag = fopen("E:\\TagModel_mini5.log", "wb");
    
    pTagCmd = (PTagCmd)pEngine->pTagCmdBuf;

    fprintf(fpTag, "nID=%d, nMix=%d, nState=%d\r\n", pTagCmd->nID, pTagCmd->nMixture, pTagCmd->nState);
    
    pState = pTagCmd->pMixSModel;
    for(i=0; i<pTagCmd->nState; i++)
    {
        fprintf(fpTag, "iState=%d\r\n", i);
        for(j=0; j<pTagCmd->nMixture; j++)
        {
            fprintf(fpTag, " iGModel=%d: fGconst=%d, fMeanVar=", j, pState->tGModel[j].fGConst);
            for(k=0; k<FEATURE_DIMNESION; k++)
            {
                fprintf(fpTag, "%d, ", pState->tGModel[j].fMeanVar[k]);
            }
            fprintf(fpTag, "\r\n");
        }

        pState = (PMixStateModel)((ivUInt32)pState + sizeof(TMixStateModel) + (pTagCmd->nMixture-1)*sizeof(TGModel));
    }

    fclose(fpTag);
}
#endif

EsErrID EsTagOutputResult(PEsrEngine pEngine)
{
	ivUInt16 nFrame, nState, nMixSModelSize; 
	int i, j, k ;
	ivInt16 fFea, fMean2, fVar;
	ivPInt16 pfFeature, pfFeatureTmp;
	PMixStateModel pDstState;
#if MINI5_SPI_MODE
	ivPointer pSrcState;
#else
	PMixStateModel pSrcState;
#endif
	PGModel pSrcGModel, pDstGModel;
	PPathItem pPathTmp;
	PTagPathDesc pTagPath;
	PTagCmd pTagCmd;
	ivUInt16 nRAMSize;
	ivUInt16 nDataSize;
	EsErrID err;

	PEsSearch pEsSearch;

	pEsSearch = &pEngine->tSearch;

    pEngine->iEngineStatus = ES_STATUS_TAGFAILED;

	/* �ڴ渴�� */
	/* pPathTmp MaxBuf:MAXNUM_TAGCMD_STATENUM * sizeof(TPathItem)= 60 * 4 = 240Bytes */
	pPathTmp = (PPathItem)((ivAddress)pEngine->pBufferBase + ivGridSize(MAXNUM_TAGCMD_STATENUM * sizeof(TPathItem)));
	pfFeature = (ivPInt16)pPathTmp;
    pfFeatureTmp = (ivPInt16)((ivAddress)pfFeature + ivGridSize(FEATURE_DIMNESION * sizeof(ivInt16)));
#if MINI5_SPI_MODE
    /* pSrcGModel: sizeof(TGModel), 34Bytes*/
    pSrcGModel = (PGModel)((ivAddress)pfFeatureTmp + ivGridSize(FEATURE_DIMNESION * sizeof(ivInt16)));
	/* pTagCmd MaxBuf: ��FA��tag��ģ���ļ����ΪMAXNUM_TAGCMD_STATENUM * sizeof(TMixStateModel),1mix��Լ60*17*2+8 =2KByte */
	pTagCmd = (PTagCmd)((ivAddress)pSrcGModel + ivGridSize(sizeof(TGModel)));
#else
    pTagCmd = (PTagCmd)((ivAddress)pfFeatureTmp + ivGridSize(FEATURE_DIMNESION * sizeof(ivInt16)));
#endif
	nRAMSize = (ivUInt16)((ivAddress)pEngine->pBufferEnd - (ivAddress)pTagCmd);

	nFrame = 0;
	nState = 0;

	pTagPath = ((PTagPathDesc)pEsSearch->tSPICache.pSPIBufBase) + pEsSearch->iFrameIndexLast - 1;

	/* �����������SPI. pTagPath��SPI�е�ַ */
	while(nFrame < pEsSearch->iFrameIndexLast){
		--pPathTmp;

        EsSPIReadBuf((ivPointer)pPathTmp, (ivPointer)(pTagPath->tPath), sizeof(TPathItem));
		//pPathTmp->iState = pTagPath->tPath->iState;
		//pPathTmp->nRepeatFrm = pTagPath->tPath->nRepeatFrm;

		nFrame = nFrame + pPathTmp->nRepeatFrm;
		pTagPath = pTagPath - pPathTmp->nRepeatFrm;

		if(++nState > MAXNUM_TAGCMD_STATENUM){
			ivAssert(ivFalse);
            return EsErrID_OK;
		}
	}

	if(nFrame != pEsSearch->iFrameIndexLast){
		ivAssert(ivFalse);
        return EsErrID_OK;
	}

    if(nRAMSize < sizeof(TTagCmd) + (pEsSearch->nMixture - 1) * sizeof(TGModel)) {
        return EsErr_OutOfMemory;
    }

    /* ��Ȼʹ��pCacheSPIʵ�ִ洢��FA��voicetagģ�ʹ���ҳ��SPIдһ�� */
    pEsSearch->tSPICache.iCacheSPIWrite = 0; 
    ivAssert(0 == ((ivAddress)pEsSearch->tSPICache.pSPIBuf - (ivAddress)pEsSearch->tSPICache.pSPIBufBase)%MINI5_SPI_PAGE_SIZE); /* ģ�͵�SPI�洢��ַ��Ҫ��ҳ����� */
    pEngine->pTagCmdBuf = pEsSearch->tSPICache.pSPIBuf;

	/* pTagCmd�Ǹ���pEngine�ڴ�� */
	pTagCmd->dwCheck = DW_TAGCMD_CHECK;
	pTagCmd->nID = pEngine->nTagID;
	pTagCmd->nState = nState;
	pTagCmd->nMixture = pEsSearch->nMixture;
    nDataSize = sizeof(TTagCmd) - sizeof(TMixStateModel);
    err = EsSPIWriteData(&pEsSearch->tSPICache, (ivPInt8)pTagCmd, nDataSize);
    if(EsErrID_OK != err) {
        return err;
    }	

	/* ��ȡSPI��feature��Ϣ��ģ�͵ľ�ֵ����FA,����ר���ڸ�tag����ģ�� */
	nFrame = 0;
	pTagPath = (PTagPathDesc)pEsSearch->tSPICache.pSPIBufBase;    
    pDstState = pTagCmd->pMixSModel;
    nMixSModelSize = sizeof(TMixStateModel) + (pEsSearch->nMixture-1) * sizeof(TGModel);
	while(nFrame < pEsSearch->iFrameIndexLast)
	{
		/* �Ƕ�������,����ֱ�����±����� */
#if MINI5_SPI_MODE
		pSrcState = (ivPointer)((ivAddress)pEsSearch->pState + pPathTmp->iState * nMixSModelSize);		
		err = EsSPIReadBuf((ivPointer)pSrcGModel,pSrcState, sizeof(TGModel));
        ivAssert(EsErrID_OK == err);
#else
        pSrcState = (PMixStateModel)((ivAddress)pEsSearch->pState + pPathTmp->iState * nMixSModelSize);		
        pSrcGModel = pSrcState->tGModel;
#endif
		pDstGModel = pDstState->tGModel;

		for(i=0; i<pEsSearch->nMixture; i++, pDstGModel++){
			pDstGModel->fGConst = pSrcGModel->fGConst;

			for(j=0; j<FEATURE_DIMNESION; j++){
				pfFeature[j] = 0;
			}

			for(j=0; j<pPathTmp->nRepeatFrm; j++){

                /* pTagPath��SPIBuf */
#if MINI5_SPI_MODE
                ivPointer pSPIPath = (ivPointer)((ivAddress)pTagPath + sizeof(TTagPathDesc) * j + sizeof(TPathItem));
                EsSPIReadBuf((ivPointer)pfFeatureTmp, (pSPIPath), FEATURE_DIMNESION * sizeof(ivInt16));
#else
                EsSPIReadBuf((ivPointer)pfFeatureTmp, (ivPointer)(pTagPath[j].pFeature), FEATURE_DIMNESION * sizeof(ivInt16));
#endif
				for(k=0; k<FEATURE_DIMNESION; k++){
					pfFeature[k] += ((pfFeatureTmp[k] +0x800) & 0xFFE0);
				}
			}

			for(j=0; j<FEATURE_DIMNESION; j++){				
				pfFeature[j] /= pPathTmp->nRepeatFrm;
				pfFeature[j] &= 0xFFE0;

				//�ٽ��þ�ֵ��ģ�͵�ԭʼ��ֵƽ����,ͳ���µ�ģ��
				fFea = pSrcGModel->fMeanVar[j];
				

				fVar = fFea & 0x1F;
				fMean2 = fFea & 0xFFE0;

				//fFea = (pfFeature[j] + fMean2)>>1;
				fFea = (5*pfFeature[j] + 11*fMean2)/16;
				fFea &= 0xFFE0;

				fFea += fVar;

				pDstGModel->fMeanVar[j] = fFea;

				/* ���ԡ���ȫʹ����ģ�͵���Ϣ������FA */
				//pDstGModel->fMeanVar[j] = pSrcGModel->fMeanVar[j];
			}
#if MINI5_SPI_MODE
            pSrcState = (ivPointer)((ivAddress)pSrcState + sizeof(TGModel));
            err = EsSPIReadBuf((ivPointer)pSrcGModel,pSrcState, sizeof(TGModel));
            ivAssert(EsErrID_OK == err);
#else
            pSrcGModel++;
#endif
		}	

		pTagPath = (PTagPathDesc)((ivAddress)pTagPath + pPathTmp->nRepeatFrm * sizeof(TTagPathDesc));
		nFrame += pPathTmp->nRepeatFrm;
		pPathTmp ++;

        /* �����ɵ�voicetagģ��д��SPI�� */
        err = EsSPIWriteData(&pEsSearch->tSPICache, (ivPInt8)pDstState, nMixSModelSize);
        if(EsErrID_OK != err) {
            return err;
        }		
	}

    err = EsSPIWriteFlush(&pEsSearch->tSPICache);
    if(EsErrID_OK != err) {
        return err;
    }

#if LOG_DEBUG_INFO
   LogTagModelInfo(pEngine);
#endif

	EsReset(pEngine);

    pEngine->iEngineStatus = ES_STATUS_TAGSUCCESS;

	return EsErrID_OK;
}

#endif

