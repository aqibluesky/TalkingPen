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
#include "ESSearchConst.h"

#if LOAD_FLOAT_MDOEL

extern void EsUpdateEndFiller(PEsSearch pThis, PSearchNode pSearchNode, ivUInt16 nStateCount, ivUInt16 nWordID);

extern void EsUpdateNBest(PEsSearch pThis);

void EsGetPruneThresh(ivPUInt16 pnBucketVote, ivInt16 nPruneMaxNode, float * pfScoreMaxPreFrm, float * pfScoreThresh);

/* ������д��˶��ƽ̨������Ӧ 20101201 */

/*  ��ʶ��Ӱ�첻�󣬵��ǻᵼ�º�ԭʼC����Բ���
1.��ʹ��beam����ʱ��C�������õĵ�ǰ֡maxԤ���ģ��������ʹ����֡max�ģ���
2.��n = ivMin(i+BUCKET_BEAM_COUNT+1, BUCKET_ALL_COUNT);ȡ����BUCKET_ALL_COUNT����������Ͱ����������ü�node����ʱ�������*pfScoreThresh = fScoreBase - (i-    	  
1)*(ivUInt16)BUCKET_SCORE_STEP��C����Ҳ�ǲ�ƥ���
*/
void EsGetPruneThresh(ivPUInt16 pnBucketVote, ivInt16 nPruneMaxNode, float * pfScoreMaxPreFrm, float * pfScoreThresh) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ͬEsPruneByBucketSort��΢���ͨ������������Ϣ */
{
    ivInt16 i;
    ivUInt16 nCount;
    float fScoreBase;
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
        nCount += pnBucketVote[i];
        if(nCount >= nPruneMaxNode){
            break;
        }
    }

    if(BUCKET_ALL_COUNT == i){
        i--;	//Ϊ�˺ͻ�����Ӧ 20101220
    }
    *pfScoreThresh = fScoreBase - (i-1)*(ivUInt16)BUCKET_SCORE_STEP;
}

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

void EsGetPruneThreshByBucket(PEsSearch pThis) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: �����ӿ� */
{
    EsGetPruneThresh(pThis->pnBucketCacheLast, (ivInt16)BUCKTE_PRUNE_MAX_NODE, &pThis->fScoreMaxLast, &pThis->fScoreThreshLast);
}

void EsUpdateNBest(PEsSearch pThis)
{
    ivUInt16 i;
    float fSilScore = pThis->fSilScore;
    float fScoreThresh = pThis->fScoreThreshLast;
    PSearchNode pSearchNode;
    PLexNode pLexNode = pThis->pLexRoot;

    pThis->tNBest[0].fScore = pThis->tNBest[0].fScore + fSilScore;
    if(pThis->tNBest[0].fScore <= LZERO){
        pThis->tNBest[0].fScore = LZERO;
    }

   
    pThis->tNBest[1].fScore = pThis->tNBest[1].fScore + fSilScore;
    if(pThis->tNBest[1].fScore <= LZERO){
        pThis->tNBest[1].fScore = LZERO;
    }

    pThis->tNBest[2].fScore = pThis->tNBest[2].fScore + fSilScore;
    if(pThis->tNBest[2].fScore <= LZERO){
        pThis->tNBest[2].fScore = LZERO;
    }

    pSearchNode = pThis->pSearchNodeLast + pThis->nTotalNodes - pThis->nCmdNum + 1;
    pLexNode += (pThis->nCmdNum - 1);

    /* Ҷ�ӽڵ���ܴ���EndFiller */
    for(i = 1; i <= pThis->nCmdNum; i++, pSearchNode++, pLexNode--){
        if(pSearchNode->fScore > fScoreThresh){
            ivAssert(pLexNode->nStateCount > 0);
            EsUpdateEndFiller(pThis, pSearchNode, pLexNode->nStateCount, i);
        }
    }
}

#include <math.h>
double LAddFloat(double x, double y)
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

float ESCalcFillerOutLike(PEsSearch pThis, PCMixSModel pFillerState, ivUInt16 nMixtures) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ��΢���ͨ����������,����ֵ��int32��Ϊint16 */
{
    ivInt16 i;
    double fOutLike;
    PCGModel pGaussian;

    pGaussian = pFillerState->tGModel;

    fOutLike = LZERO;
    for(i=0;i < nMixtures;++i,++pGaussian){
        float fLike;

        fLike = EsCal1GaussOutlike(pThis, pGaussian);

        fOutLike = LAddFloat(fOutLike, fLike);
    }

    return (float)fOutLike;   //�����������6
}

#if LOG_SEARCH_SCORE
extern FILE *g_fpOutlike;
#endif
float ESCalcOutLike(PEsSearch pThis, PCMixSModel pState, ivUInt16 nMixtures) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ǰ����ͬ������������ģ�����ļ��� */
{
    ivInt16 i;
    double fOutLike;

    fOutLike = LZERO;
    for(i=0; i < nMixtures; ++i){
        float fLike;
        fLike = EsCal1GaussOutlike(pThis, pState->tGModel + i);
        fOutLike = LAddFloat(fOutLike, fLike);
    }

#if LOG_SEARCH_SCORE
    {
        fprintf(g_fpOutlike, "%s : %.2f\r\n", pState->szName, fOutLike);       
        fflush(g_fpOutlike);
    }
#endif

    return (float)fOutLike;
}

float EsCal1GaussOutlike(PEsSearch pThis, PCGModel pGModel) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ��䶯 */
{
    ivInt16 i;

    float fValue;
    double f32Like;

    float * pfFeature;

    float * pfMean;
    float * pfVar;

    pfFeature = pThis->fFeature; /*  16λ����High-Low  | 5bit������չ | 7bit fea (Q6) | 0000 | */

    /* ԭʼ���㹫ʽ 1/2[-fConst - ((fea - mean)^2)/var]	 */
    /*    �任: ((fea - mean)^2)/var = [(fea*meancoef - mean*meancoef)*sqrt(1/(var*meancoef*meancoef))]^2     AAA */
    /*    ȫ�־�ֵ�����g_s16MeanVarTable�д洢�ľ��ǹ�ʽAAA�Ľ��,(fea*meancoef - mean*meancoef)ռ8bit, var'ռ4bit���ñ���2^12=2028��С�ı� */
    /* Ϊ��ʵ��mean7bit,var4bit�洢���ڴ��ģ��ʱ���˴���,�洢��mean' = mean*meancoef, var'=ln(var*meancoef*meancoef)*16/varcoef */

    pfMean = pGModel->fltMean; /* 16λ����High-Low  | 5bit������չ | 7bit mean (Q6) | 4bit Var (Q0) | */ 
    pfVar = pGModel->fltVar;
    f32Like = 2.0*(pGModel->fltWeight) - pGModel->fltGConst;  /* Q8 */

    for(i = FEATURE_DIMNESION; i; i--){
        fValue = (*pfMean++) -  (*pfFeature++); 
        
        f32Like -= (fValue*fValue)/(*pfVar++);  /* Q8 */
    }

    f32Like = 0.5 * f32Like;

    /* f32Like >>= (9-ES_Q_TRANSP); */	/* Q8->ES_Q_TRANSP and *0.5 = (8-ES_Q_TRANSP+1) */

    /*ivAssert(f32Like >= -32768 && f32Like <= 32767);*/

    return (double)f32Like;
}

#include <stdio.h>
EsErrID EsUpdateAllScore(PEsSearch pThis)
{ 
    float fSilScore;
    ivInt16 i;

    PLexNode pLexNode;	
    PSearchNode pSearchNode;
    float fScoreThresh;
    float fScore;

    ivInt16 iIndex;
    float fFillerScore;
    ivPUInt16 pnBucketVote;
    float fScoreMaxPreFrame;

    float fOutlike;

    PCMixSModel pState;

#if MINI5_USE_NEWCM_METHOD
    ivBool bTranpLPR;
#endif

    fSilScore = pThis->fSilScore;
    fFillerScore = pThis->fFillerScore;

    pLexNode = pThis->pLexRoot;
    pSearchNode = pThis->pSearchNodeLast;
    pnBucketVote = pThis->pnBucketCacheLast;
    fScoreThresh = pThis->fScoreThreshLast;
    fScoreMaxPreFrame = pThis->fScoreMaxLast;

    fScoreThresh = LZERO;

    EsUpdateNBest(pThis);

    ivMemZero(pnBucketVote, sizeof(ivInt16)*BUCKET_ALL_COUNT);

    i = pThis->nTotalNodes;

    for(; i>=1; i--,pLexNode++)
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

        pState = (PCMixSModel)((ivUInt32)pThis->pState + pLexNode->iSModeIndex * (sizeof(TMixStateModel) + sizeof(TGModel) * (pThis->nMixture-1)));
        fOutlike = ESCalcOutLike(pThis, pState, pThis->nMixture);
        pSearchNode[i].fScore = fScore - fFillerScore + fOutlike;

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
        iIndex = (ivInt16)BUCKET_PLUS_SCORE_COUNT + ((fScoreMaxPreFrame -  pSearchNode[i].fScore)/4);			
        if(iIndex < 0){
            iIndex = 0; /* ��ֹ���ε÷ֱ��ϴ���ߵ÷�ƫ��̫�࣬дԽ��!!! */
        }
        if(iIndex < BUCKET_ALL_COUNT){ /*  ����ý��÷���Ԥ��Ͱ��Χ��,�����ͶͰ���� */
            pnBucketVote[iIndex]++;	
        }
    }//for(; i>=1; i--,pLexNode--){

    pSearchNode[0].fScore = pSearchNode[0].fScore + fSilScore;
#if MINI5_USE_NEWCM_METHOD
    pSearchNode[0].fCMScore = 0; //sil CM�÷ֲ�����
    pSearchNode[0].fCurStateCMScore = 0;
#endif

#if LOAD_FLOAT_MDOEL && LOG_SEARCH_SCORE
    {
        static FILE *g_fpSearchScoreFlt = NULL;
        if(1 == pThis->iFrameIndexLast)
        {
            g_fpSearchScoreFlt = fopen("E:\\SearchScoreFlt_mini5.log", "wb");
        }
        fprintf(g_fpSearchScoreFlt, "iFrame = %d\r\n", pThis->iFrameIndexLast);
        fprintf(g_fpSearchScoreFlt, " fSilScore=%.2f, fFillerScore=%.2f\r\n", pThis->fSilScore, pThis->fFillerScore);

        for(i=0; i<=pThis->nTotalNodes; i++)
        {
#if  MINI5_USE_NEWCM_METHOD
            fprintf(g_fpSearchScoreFlt, " %d: fScore=%.2f, fCMScore=%.2f, fCurStateCMScore=%.2f\r\n", i, pSearchNode[i].fScore, pSearchNode[i].fCMScore, pSearchNode[i].fCurStateCMScore);   
#else
            fprintf(g_fpSearchScoreFlt, " %d: fScore=%.2f\r\n", i, pSearchNode[i].fScore);   
#endif
        }
        fflush(g_fpSearchScoreFlt);
    }
#endif

    return EsErrID_OK;
}

#endif

