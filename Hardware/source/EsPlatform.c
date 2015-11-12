/*********************************************************************#
//	�ļ���		��EsPlatform.c
//	�ļ�����	����ƽ̨��صĺ���.����SPI����.
//	����		��qungao
//	����ʱ��	��2013��5��14��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/

#include "EsKernel.h"
#include "EsSearch.h"

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT
void EsSPIWriteOnePage(ivPointer pSPIBuf, ivPointer pRAMBuf)
{
	ivMemCopy(pSPIBuf, pRAMBuf, MINI5_SPI_PAGE_SIZE);	/* SPIд���� */
}
#endif

#if MINI5_SPI_MODE
EsErrID EsSPIReadBuf(ivPointer pRAMBuf, ivCPointer pSPIBuf, ivUInt16 nSize)
{
	ivMemCopy(pRAMBuf, pSPIBuf, nSize);

	return EsErrID_OK;
}

ivInt16 EsSPICal1GaussOutlike(PEsSearch pThis, PCGModel pGModel) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ��䶯 */
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

	for(i = FEATURE_DIMNESION; i; i--){
		fValue = (*pfMeanVar++) -  (*pfFeature++); 
		ivAssert(fValue < 4096);
		f32Like -= g_s16MeanVarTable[fValue];  /* Q8 */
	}

	f32Like >>= (ESR_Q_MEANVARTAB_MANUAL+1-ESR_Q_SCORE);	/* Q8->ESR_Q_SCORE and *0.5 = (8-ESR_Q_SCORE+1) */

	ivAssert(f32Like >= -32768 && f32Like <= 32767);

	return (ivInt16)f32Like;
}

ivInt16 EsSPICalcOutLike(PEsSearch pThis, PCMixSModel pState, ivUInt16 nMixtures) /* ��˶��ƽ̨AitalkMini1.0����Ƚ�: ��΢���ͨ����������,����ֵ��int32��Ϊint16 */
{
	ivInt16 i;
	ivInt32 fOutLike;
	PCGModel pGauss;

	pGauss = pState->tGModel;
	fOutLike = LZERO;

	for(i = 0; i < nMixtures; ++i){
		ivInt16 fLike;
		fLike = EsSPICal1GaussOutlike(pThis, pGauss + i);
		fOutLike = LAdd(fOutLike, (ivInt32)fLike);
	}

	ivAssert(fOutLike <= 32767 && fOutLike >= -32768);

	return (ivInt16)fOutLike;
}

EsErrID EsSPIUpdateAllScore(PEsSearch pThis)
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

    // ivAssert(BUCKET_ALL_COUNT + sizeof(TLexNode)/sizeof(ivInt16) < ivGridSize(TRANSFORM_FFTNUM_DEF+2));
    pLexNode = (PLexNode)((ivAddress)pThis->pnBucketCacheLast + ivGridSize(BUCKET_ALL_COUNT * sizeof(ivInt16)));
    pSPIBuffer = pThis->pLexRoot;

    for(i = pThis->nTotalNodes; i>=1; i--)
    {
        EsSPIReadBuf((ivPointer)pLexNode, pSPIBuffer, sizeof(TLexNode));
        pSPIBuffer = (ivPointer)((ivUInt32)pSPIBuffer + sizeof(TLexNode));

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

    return EsErrID_OK;
}

#endif
