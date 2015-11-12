/*********************************************************************#
//	�ļ���		��Search.cpp
//	�ļ�����	��Implements of ESR Search
//	����		��Truman
//	����ʱ��	��2007��7��10��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/

#include "EsKernel.h"
#include "EsSearch.h"
#include "EsEngine.h"
#include "EsSearchConst.h"


#if !LOAD_FLOAT_MDOEL

extern void EsUpdateEndFiller(PEsSearch pThis, PSearchNode pSearchNode, ivUInt16 nStateCount, ivUInt16 nWordID);

void EsGetPruneThresh(ivPUInt16 pnBucketVote, ivInt16 nPruneMaxNode, ivPInt16 pfScoreMaxPreFrm, ivPInt16 pfScoreThresh);

/* ������д��˶��ƽ̨������Ӧ 20101201 */

/*  ��ʶ��Ӱ�첻�󣬵��ǻᵼ�º�ԭʼC����Բ���
1.��ʹ��beam����ʱ��C�������õĵ�ǰ֡maxԤ���ģ��������ʹ����֡max�ģ���
2.��n = ivMin(i+BUCKET_BEAM_COUNT+1, BUCKET_ALL_COUNT);ȡ����BUCKET_ALL_COUNT����������Ͱ����������ü�node����ʱ�������*pfScoreThresh = fScoreBase - (i-    	  
1)*(ivUInt16)BUCKET_SCORE_STEP��C����Ҳ�ǲ�ƥ���
*/

void EsGetPruneThresh(ivPUInt16 pnBucketVote, ivInt16 nPruneMaxNode, ivPInt16 pfScoreMaxPreFrm, ivPInt16 pfScoreThresh) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ͬEsPruneByBucketSort��΢���ͨ������������Ϣ */
{
    ivInt16 i;
    ivUInt16 nCount;
    ivInt16 fScoreBase;
    ivInt16 n;

    fScoreBase = *pfScoreMaxPreFrm + BUCKET_EXCEED_MAX_SCORE;
    /* Get fScoreMax & fThreshScore */
    i = 0;
    while(pnBucketVote[i++] <= 0)
    {
        if (i>= BUCKET_ALL_COUNT )
        {
            *pfScoreThresh = LZERO;
            return;
        }
    }

    *pfScoreMaxPreFrm = fScoreBase - (i-1)*(ivInt16)BUCKET_SCORE_STEP;

    i--; /* ��һ����Ϊ�յ�Ͱ */
    nCount = 0;

    n = ivMin(i+BUCKET_BEAM_COUNT+1, BUCKET_ALL_COUNT);

    for(; i<n; i++){
        nCount = nCount + pnBucketVote[i];
        if(nCount >= nPruneMaxNode){
            break;
        }
    }

    if(BUCKET_ALL_COUNT == i){
        i--;	//Ϊ�˺ͻ�����Ӧ 20101220
    }
    *pfScoreThresh = fScoreBase - (i-1)*(ivUInt16)BUCKET_SCORE_STEP;
}

#if 0
/* ��Ͱ�������м��㵱ǰ֡fScoreMax��fScorThresh,����֡ʹ�� */
void EsGetPruneThreshXXX(ivPUInt16 pnBucketVote, ivInt16 nPruneMaxNode, ivPInt16 pfScoreMaxPreFrm, ivPInt16 pfScoreThresh) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ͬEsPruneByBucketSort��΢���ͨ������������Ϣ */
{
    ivInt16 i;
    ivUInt16 nCount;
    ivInt16 fScoreThresh = 0;
    ivInt16 fScoreMaxPreFrm;
    ivBool bGotScoreMax = ivFalse;

    fScoreMaxPreFrm = *pfScoreMaxPreFrm;
    /* Get fScoreMax & fThreshScore */
    nCount = 0;
    for(i=0; i<BUCKET_ALL_COUNT; i++){
        nCount = nCount + pnBucketVote[i];

        if(!bGotScoreMax && pnBucketVote[i] > 0){
            /* ���¸���ͶͰʱ��ʽ����õ� iIndex = (ivInt16)BUCKET_PLUS_SCORE_COUNT + (fScoreMaxPreFrame -  pStaScoreCache[i])/(ivInt16)BUCKET_SCORE_STEP;	 */
            *pfScoreMaxPreFrm += ((ivInt16)BUCKET_PLUS_SCORE_COUNT - i)* (ivInt16)BUCKET_SCORE_STEP;
            fScoreThresh = *pfScoreMaxPreFrm - (ivInt16)BEAM_THRESH_LOW; /* ���ڵ���С�ڲü��������ֵʱ,��BeamThresh��Ϊ���� */
            bGotScoreMax = ivTrue;
        }

        if(nCount >= nPruneMaxNode){
            /* ȡi-1��Ͱ��Ϊ���� */
            ivInt16 fTemp = fScoreMaxPreFrm + ((ivInt16)BUCKET_PLUS_SCORE_COUNT - i + 1)* (ivInt16)BUCKET_SCORE_STEP;
            if (fTemp > fScoreThresh)
            {
                fScoreThresh = fTemp;
            }			
            break;
        }
    }

    *pfScoreThresh = fScoreThresh;
}
#endif

void EsGetPruneThreshByBucket(PEsSearch pThis) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: �����ӿ� */
{
    EsGetPruneThresh(pThis->pnBucketCacheLast, (ivInt16)BUCKTE_PRUNE_MAX_NODE, &pThis->fScoreMaxLast, &pThis->fScoreThreshLast);
}

/* return Q4 */
ivInt32 LAdd(ivInt32 x, ivInt32 y)
{
    ivInt32 diff, n32Ret;

    //log(1+exp(-x)) Q4. 0<x<4, ��0~4��������ȵķ�Ϊ64�Σ�ȡÿ�ε��м�ֵ.
    /*	ivStatic ivConst ivInt32 g_fLaddTab[44] = {
    10, // 0.0313
    10, // 0.0938
    9, // 0.1563
    9, // 0.2188
    8, // 0.2813
    8, // 0.3438
    8, // 0.4063
    7, // 0.4688
    7, // 0.5313
    7, // 0.5938
    6, // 0.6563
    6, // 0.7188
    6, // 0.7813
    5, // 0.8438
    5, // 0.9063
    5, // 0.9688
    4, // 1.0313
    4, // 1.0938
    4, // 1.1563
    4, // 1.2188
    3, // 1.2813
    3, // 1.3438
    3, // 1.4063
    3, // 1.4688
    3, // 1.5313
    2, // 1.5938
    2, // 1.6563
    2, // 1.7188
    2, // 1.7813
    2, // 1.8438
    2, // 1.9063
    2, // 1.9688
    1, // 2.0313
    1, // 2.0938
    1, // 2.1563
    1, // 2.2188
    1, // 2.2813
    1, // 2.3438
    1, // 2.4063
    1, // 2.4688
    1, // 2.5313
    1, // 2.5938
    1, // 2.6563
    1 // 2.7188
    };
    */
    ivStatic ivConst ivInt32 g_fLaddTab[28] = {
        11,
        10,
        9,
        8,
        7,
        7,
        6,
        5,
        5,
        4,
        4,
        3,
        3,
        3,
        2,
        2,
        2,
        2,
        2,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1
    };

    ivAssert(4 == ESR_Q_SCORE);
    if(y>x){
        diff = y-x;	/* Q4 */
        diff >>= 1;
        if(diff >= 28){
            return y;
        }
        else{
            n32Ret = y+g_fLaddTab[diff];
            return n32Ret;
        }
    }
    else{
        diff = x-y;
        diff >>= 1;
        if(diff >= 28){
            return x;
        }
        else{
            n32Ret = x+g_fLaddTab[diff];
            return n32Ret;
        }
    }
}

void EsUpdateNBest(PEsSearch pThis)
{
    ivUInt16 i;
    ivInt16 fSilScore = pThis->fSilScore;
    ivInt16 fScoreThresh = pThis->fScoreThreshLast;
    PSearchNode pSearchNode;

#if MINI5_USE_NEWCM_METHOD
    # if MINI5_SPI_MODE
    PLexNode pLexNode = (PLexNode)pThis->pnBucketCacheLast;

    ivAssert((TRANSFORM_FFTNUM_DEF+2) * sizeof(ivInt16) >= pThis->nCmdNum * sizeof(TLexNode));
    EsSPIReadBuf((ivPointer)pLexNode, (ivPointer)pThis->pLexRoot, pThis->nCmdNum * sizeof(TLexNode));
    pLexNode = pLexNode + (pThis->nCmdNum - 1);
    #else
    PLexNode pLexNode = pThis->pLexRoot + (pThis->nCmdNum - 1);
    #endif /* MINI5_SPI_MODE */
#endif /* MINI5_USE_NEWCM_METHOD */

    ivAssert(pThis->tNBest[0].fScore + fSilScore >= -32768 && pThis->tNBest[0].fScore + fSilScore <= 32767);
    pThis->tNBest[0].fScore = pThis->tNBest[0].fScore + fSilScore;
    if(pThis->tNBest[0].fScore <= LZERO){
        pThis->tNBest[0].fScore = LZERO;
    }

    ivAssert(pThis->tNBest[1].fScore + fSilScore >= -32768 && pThis->tNBest[1].fScore + fSilScore <= 32767);
    pThis->tNBest[1].fScore = pThis->tNBest[1].fScore + fSilScore;
    if(pThis->tNBest[1].fScore <= LZERO){
        pThis->tNBest[1].fScore = LZERO;
    }

    ivAssert(pThis->tNBest[2].fScore + fSilScore >= -32768 && pThis->tNBest[2].fScore + fSilScore <= 32767);
    pThis->tNBest[2].fScore = pThis->tNBest[2].fScore + fSilScore;
    if(pThis->tNBest[2].fScore <= LZERO){
        pThis->tNBest[2].fScore = LZERO;
    }

    pSearchNode = pThis->pSearchNodeLast + pThis->nTotalNodes - pThis->nCmdNum + 1;

    /* Ҷ�ӽڵ���ܴ���EndFiller */
    for(i = 1; i <= pThis->nCmdNum; i++, pSearchNode++){
        if(pSearchNode->fScore > fScoreThresh){
#if MINI5_USE_NEWCM_METHOD
            ivAssert(pLexNode->nStateCount > 0);
            EsUpdateEndFiller(pThis, pSearchNode, pLexNode->nStateCount, i);
#else
            EsUpdateEndFiller(pThis, pSearchNode, 0, i);
#endif /* MINI5_USE_NEWCM_METHOD */
        }
#if MINI5_USE_NEWCM_METHOD
        pLexNode--;
#endif /* MINI5_USE_NEWCM_METHOD */
    }
}

#if LOG_SEARCH_SCORE
extern FILE *g_fpOutlike;
#endif

#if !MINI5_SPI_MODE
ivInt16 EsCalcOutLike(PEsSearch pThis, PCMixSModel pState, ivUInt16 nMixtures) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ��΢���ͨ����������,����ֵ��int32��Ϊint16 */
{
    ivInt16 i;
    ivInt32 fOutLike;
    PCGModel pGaussian;

    pGaussian = pState->tGModel;

    fOutLike = LZERO;
    for(i=0;i < nMixtures;++i,++pGaussian){
        ivInt16 fLike;

        fLike = EsCal1GaussOutlike(pThis, pGaussian);

        fOutLike = LAdd(fOutLike, (ivInt32)fLike);
    }

    ivAssert(fOutLike <= 32767 && fOutLike >= -32768);

#if LOG_SEARCH_SCORE
    {
        fprintf(g_fpOutlike, "%s : %.2f\r\n", "  ", fOutLike*1.0/(1<<ESR_Q_SCORE));       
        fflush(g_fpOutlike);
    }
#endif

    return (ivInt16)fOutLike;
}

ivInt16 EsCal1GaussOutlike(PEsSearch pThis, PCGModel pGModel) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ��䶯 */
{
    ivInt16 i;

    ivInt16 fValue;
    ivInt32 f32Like;

    ivPCInt16 pfFeature;

    ivPCInt16 pfMeanVar;

    pfFeature = pThis->fFeature; /*  16λ����High-Low  | 5bit������չ | 7bit fea (Q6) | 0000 | */

    /* ԭʼ���㹫ʽ 1/2[-fConst - ((fea - mean)^2)/var]	 */
    /*    �任: ((fea - mean)^2)/var = [(fea*meancoef - mean*meancoef)*sqrt(1/(var*meancoef*meancoef))]^2     AAA */
    /*    ȫ�־�ֵ�����g_s16MeanVarTable�д洢�ľ��ǹ�ʽAAA�Ľ��,(fea*meancoef - mean*meancoef)ռ8bit, var'ռ4bit���ñ���2^12=2028��С�ı� */
    /* Ϊ��ʵ��mean7bit,var4bit�洢���ڴ��ģ��ʱ���˴���,�洢��mean' = mean*meancoef, var'=ln(var*meancoef*meancoef)*16/varcoef */

    pfMeanVar = pGModel->fMeanVar; /* 16λ����High-Low  | 5bit������չ | 7bit mean (Q6) | 4bit Var (Q0) | */ 

    f32Like = (ivInt32)pGModel->fGConst;  /* Q8 */
    ivAssert(8 == ESR_Q_WEIGHT_GCOSNT_MANUAL);    

    for(i = FEATURE_DIMNESION; i; i -= 4){
        fValue = (*pfMeanVar++) -  (*pfFeature++); 
        ivAssert(fValue < (1<<(ESR_VAR_QUANT_BIT+ESR_MEAN_QUANT_BIT+1)));
        f32Like -= g_s16MeanVarTable[fValue];  /* Q8 */

        fValue = (*pfMeanVar++) -  (*pfFeature++); 
        ivAssert(fValue < (1<<(ESR_VAR_QUANT_BIT+ESR_MEAN_QUANT_BIT+1)));
        f32Like -= g_s16MeanVarTable[fValue];  /* Q8 */

        fValue = (*pfMeanVar++) -  (*pfFeature++); 
        ivAssert(fValue < (1<<(ESR_VAR_QUANT_BIT+ESR_MEAN_QUANT_BIT+1)));
        f32Like -= g_s16MeanVarTable[fValue];  /* Q8 */

        fValue = (*pfMeanVar++) -  (*pfFeature++); 
        ivAssert(fValue < (1<<(ESR_VAR_QUANT_BIT+ESR_MEAN_QUANT_BIT+1)));
        f32Like -= g_s16MeanVarTable[fValue];  /* Q8 */
    }

    ivAssert(ESR_Q_WEIGHT_GCOSNT_MANUAL == ESR_Q_MEANVARTAB_MANUAL);

    f32Like >>= (ESR_Q_MEANVARTAB_MANUAL+1-ESR_Q_SCORE);	/* Q8->ESR_Q_SCORE and *0.5 = (8-ESR_Q_SCORE+1) */

    ivAssert(f32Like >= -32768 && f32Like <= 32767);

    return (ivInt16)f32Like;
}

EsErrID EsUpdateAllScore(PEsSearch pThis)
{ 
    ivInt16 fSilScore;
    ivInt16 i;

    PLexNode pLexNode;	
    PSearchNode pSearchNode;
    ivInt16 fScoreThresh;
    ivInt16 fScore;

    ivInt16 iIndex;
    ivInt16 fFillerScore;
    ivPUInt16 pnBucketVote;
    ivInt16 fScoreMaxPreFrame;

    ivInt16 fOutlike;
    PCMixSModel pState;

#if MINI5_USE_NEWCM_METHOD
    ivBool bTranpLPR;
#endif
#if MINI5_SPI_MODE
    ivPointer pSPIBuffer;
#endif

    fSilScore = pThis->fSilScore;
    fFillerScore = pThis->fFillerScore;

    pSearchNode = pThis->pSearchNodeLast;
    pnBucketVote = pThis->pnBucketCacheLast;
    fScoreThresh = pThis->fScoreThreshLast;
    fScoreMaxPreFrame = pThis->fScoreMaxLast;

    EsUpdateNBest(pThis);
    ivMemZero(pnBucketVote, sizeof(ivInt16)*BUCKET_ALL_COUNT);

    pLexNode = pThis->pLexRoot;
    for(i = pThis->nTotalNodes; i>=1; i--,pLexNode++)
    {
        fScore = pSearchNode[pLexNode->iParent].fScore;
#if MINI5_USE_NEWCM_METHOD
        bTranpLPR = ivFalse;
#endif
        if(pSearchNode[i].fScore > fScoreThresh)
        {
            /* ���ڵ�ǰ����������PK */
            if(pSearchNode[i].fScore > fScore)
            {
                fScore = pSearchNode[i].fScore;
#if MINI5_USE_NEWCM_METHOD
                bTranpLPR = ivTrue;
#endif
            }
        }
        else{  //if(pStaScoreCache[i] > fScoreThresh) No
            /* �����ڵ� */
            if(fScore <= fScoreThresh)
            {				
                pSearchNode[i].fScore = LZERO;
                continue;
            }
        }			

        pState = (PCMixSModel)((ivUInt32)pThis->pState + pLexNode->iSModeIndex * sizeof(TGModel) * pThis->nMixture);
        fOutlike = EsCalcOutLike(pThis, pState, pThis->nMixture);
        pSearchNode[i].fScore = EsSatInt16(fScore - fFillerScore + fOutlike);
        ivAssert(pSearchNode[i].fScore == fScore - fFillerScore + fOutlike);

#if MINI5_USE_NEWCM_METHOD
        /* ��CM������� */
        ivAssert(0 == pSearchNode[1].fCMScore);
        fScore = fOutlike - fFillerScore;
        if(bTranpLPR){				
            if(fScore < pSearchNode[i].fCurStateCMScore){
                pSearchNode[i].fCurStateCMScore = fScore;
            }
        }
        else{
            pSearchNode[i].fCurStateCMScore = fScore;
            pSearchNode[i].fCMScore = pSearchNode[pLexNode->iParent].fCMScore + pSearchNode[pLexNode->iParent].fCurStateCMScore;	
        }
#endif
        /* Bucket Sort  ����֡MaxScoreΪ��׼���бȽ�, ��ID=0��Ͱ��ʼ:|...|12|8|4|0|-4|-8|-12|...����ĳ��ֵx����(-4,0],��Ͷ���0���Ǹ�Ͱ */	
        ivAssert(64 == BUCKET_SCORE_STEP);
        iIndex = (ivInt16)BUCKET_PLUS_SCORE_COUNT + ((fScoreMaxPreFrame -  pSearchNode[i].fScore)>>6);			
        if(iIndex < 0){
            iIndex = 0; /* ��ֹ���ε÷ֱ��ϴ���ߵ÷�ƫ��̫�࣬дԽ��!!! */
        }
        if(iIndex < BUCKET_ALL_COUNT){ /*  ����ý��÷���Ԥ��Ͱ��Χ��,�����ͶͰ���� */
            pnBucketVote[iIndex]++;	
        }
    }//for(; i>=1; i--,pLexNode--){

    pSearchNode[0].fScore = EsSatInt16(pSearchNode[0].fScore + fSilScore);
#if MINI5_USE_NEWCM_METHOD
    pSearchNode[0].fCMScore = 0; //sil CM�÷ֲ�����
    pSearchNode[0].fCurStateCMScore = 0;
#endif

#if LOG_DEBUG_INFO
    {
        static FILE *g_fpSearchScore = NULL;
        if(1 == pThis->iFrameIndexLast)
        {
            g_fpSearchScore = fopen("E:\\SearchScore_mini5.log", "wb");
        }
        fprintf(g_fpSearchScore, "iFrame = %d\r\n", pThis->iFrameIndexLast);
        fprintf(g_fpSearchScore, " fSilScore=%d, fFillerScore=%d\r\n", pThis->fSilScore, pThis->fFillerScore);

        for(i=0; i<=pThis->nTotalNodes; i++)
        {
#if MINI5_USE_NEWCM_METHOD
            fprintf(g_fpSearchScore, " %d: fScore=%d, fCMScore=%d, fCurStateCMScore=%d\r\n", i, pSearchNode[i].fScore, pSearchNode[i].fCMScore, pSearchNode[i].fCurStateCMScore);   
#else
            fprintf(g_fpSearchScore, " %d: fScore=%d\r\n", i, pSearchNode[i].fScore);   
#endif
        }
        fflush(g_fpSearchScore);
    }
#endif

#if LOG_SEARCH_SCORE
    {
        static FILE *g_fpSearchScoreFlt = NULL;
        if(1 == pThis->iFrameIndexLast)
        {
            g_fpSearchScoreFlt = fopen("E:\\SearchScore_mini5.log", "wb");
        }
        fprintf(g_fpSearchScoreFlt, "iFrame = %d\r\n", pThis->iFrameIndexLast);
        fprintf(g_fpSearchScoreFlt, " fSilScore=%.2f, fFillerScore=%.2f\r\n", pThis->fSilScore*1.0/(1<<ESR_Q_SCORE), pThis->fFillerScore*1.0/(1<<ESR_Q_SCORE));

        for(i=0; i<=pThis->nTotalNodes; i++)
        {
#if  MINI5_USE_NEWCM_METHOD
            fprintf(g_fpSearchScoreFlt, " %d: fScore=%.2f, fCMScore=%.2f, fCurStateCMScore=%.2f\r\n", 
                i, pSearchNode[i].fScore*1.0/(1<<ESR_Q_SCORE), pSearchNode[i].fCMScore*1.0/(1<<ESR_Q_SCORE), pSearchNode[i].fCurStateCMScore*1.0/(1<<ESR_Q_SCORE));   
#else
            fprintf(g_fpSearchScoreFlt, " %d: fScore=%.2f\r\n", i, pSearchNode[i].fScore*1.0/(1<<ESR_Q_SCORE));   
#endif
        }
        fflush(g_fpSearchScoreFlt);
    }
#endif

    return EsErrID_OK;
}

#endif /* !MINI5_SPI_MODE */

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT

#if LOG_DEBUG_INFO
LogTagSearchScore(PEsSearch pThis)
{
    int i;
    FILE *fp;
    if(0 == pThis->iFrameIndexLast)
    {
        fp = fopen("E:\\TagSearchScore_mini5.log", "wb");
    }
    else
    {
        fp = fopen("E:\\TagSearchScore_mini5.log", "ab");
    }
    fprintf(fp, "iFrame=%d\r\n", pThis->iFrameIndexLast);
    fprintf(fp, "   fSilScore=%d, fFillerScore=%d\r\n", pThis->fSilScore, pThis->fFillerScore);
    for(i=0; i<pThis->nClusters+2; i++)
    {
        fprintf(fp, "   fScore=%d, nRepeatFrm=%d\r\n", pThis->pStaScoreCacheLast[i], pThis->pnRepeatFrmCacheLast[i]);
    }

    fclose(fp);
}
#endif

EsErrID EsTagUpdateAllScore(PEsSearch pThis) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: �䶯�϶࣬VADwδ��⵽����ʱ�����˲ü� */
{
    ivInt16 fScore, fScoreMaxPre, fScoreMaxCur, fOutLike;
    ivInt16 i, iMaxPre, iMaxCur, nLastMaxRepeat;
    ivPInt16 pScoreCacheLast, pnRepeatFrmCacheLast;
    TLexNode tLexNode;
    ivInt16 fSilScore;
    ivInt16 fFillerScore;
    TPathItem tPathTmp;
    EsErrID err;
    PCMixSModel pState;

    fSilScore = pThis->fSilScore;
    fFillerScore = pThis->fFillerScore;

    pScoreCacheLast = pThis->pStaScoreCacheLast;
    pnRepeatFrmCacheLast = pThis->pnRepeatFrmCacheLast;

    fScoreMaxPre = pThis->fScoreMaxLast;
    iMaxPre = pThis->iMaxStateLast;

    /* Update BeginSil */
    pScoreCacheLast[0] += fSilScore;

    /* Update EndSil */
    fScore = ivMax(pScoreCacheLast[pThis->nClusters+1], fScoreMaxPre);
    pScoreCacheLast[pThis->nClusters+1] = fScore + fSilScore;

#ifdef TAG_SEARCH_ADD_SIL
    if(pScoreCacheLast[0] > fScoreMaxPre){
        fScoreMaxPre = pScoreCacheLast[0];
        iMaxPre = 0;
    }
#endif

    fScoreMaxCur = LZERO;
    fScoreMaxPre -= STATE_LPF_PUNISH_SCORE; //���ڵ���������ͷ� 
    iMaxCur = 1;
    nLastMaxRepeat = pnRepeatFrmCacheLast[iMaxPre]+1;
    for(i=1; i<=pThis->nClusters; i++){
        if(pScoreCacheLast[i] > fScoreMaxPre){
            /* ���� */
            fScore = pScoreCacheLast[i];
            pnRepeatFrmCacheLast[i]++;
        }
        else{
            fScore = fScoreMaxPre;
            pnRepeatFrmCacheLast[i] = 1; 
        }

        tLexNode.iSModeIndex = i-1;
        pState = (PCMixSModel)((ivUInt32)pThis->pState + tLexNode.iSModeIndex * sizeof(TGModel) * pThis->nMixture);
        fOutLike = EsCalcOutLike(pThis, pState, pThis->nMixture);

        pScoreCacheLast[i] = EsSatInt16(fScore + fOutLike - fFillerScore);
        ivAssert(pScoreCacheLast[i] == fScore + fOutLike - fFillerScore);
        if(pScoreCacheLast[i] > fScoreMaxCur){
            fScoreMaxCur = pScoreCacheLast[i];
            iMaxCur = i;
        }
    }

    pnRepeatFrmCacheLast[iMaxPre] = nLastMaxRepeat; /* ���ڵ�������.������ */

    pThis->fScoreMaxLast = fScoreMaxCur;
    pThis->iMaxStateLast = iMaxCur;

    /* ��һ֡��path����.�Ȼ��浽256�ֽڵ�ram�У����ram����256�ֽڣ�����SPIдһ�� */
    tPathTmp.iState = iMaxCur - 1;
    tPathTmp.nRepeatFrm = pnRepeatFrmCacheLast[iMaxCur];
    err = EsSPIWriteData(&pThis->tSPICache, (ivPInt8)(&tPathTmp), sizeof(tPathTmp));
    if(EsErrID_OK != err)
    {
        return err;
    }
    err = EsSPIWriteData(&pThis->tSPICache, (ivPInt8)pThis->fFeature, sizeof(pThis->fFeature));
    if(EsErrID_OK != err)
    {
        return err;
    }

#if LOG_DEBUG_INFO
    LogTagSearchScore(pThis);
#endif

    return EsErrID_OK;
}
#endif

#endif
