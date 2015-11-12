/*********************************************************************#
//	�ļ���		��EsSearch.h
//	�ļ�����	��Declare of structures for ESR Search corresponding
//	����		��Truman
//	����ʱ��	��2007��7��10��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/

#if !defined(ES_TEAM__2007_07_10__ESKERNEL_ESSEARCH__H)
#define ES_TEAM__2007_07_10__ESKERNEL_ESSEARCH__H

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#include "EsNetwork.h"
#include "EsSearchConst.h"
#include "ivMath.h"

/* ---------------------CMN���� ������.Begin--------------------------------- */
#define NOT_CMN_UPDATE              (1) //������CMN����
#define CMN_UPDATE_JUZI             (2) //ֻ���о�����
#define CMN_UPDATE_JUNEI            (3) //�����ڶ�����
#define TEST_CMN_UPDATE             (CMN_UPDATE_JUZI)  
/* ---------------------CMN���� ������.End---------------------------------- */

/* ---------------------Debug ������.Begin--------------------------------- */
#define LOAD_FLOAT_MDOEL            (0)  /* ������ģ��Ҳ������Դ�� */
#define LOAD_FLOAT_FEA              (0) /* Load�о�Ժfloat feature */

#define LOG_FIXED_FEATURE           (0) /* Log��Ʒ�������� */
#define LOG_SEARCH_SCORE            (0)
#define LOG_DEBUG_INFO				(0)
#define RATETEST_TOOL_LOG           (0)     /* ʶ���ʹ���log */
#define RATETSET_USE_VAD            (0)     /* ʶ���ʲ���֧�ֹ�VAD */

/* ---------------------Debug ������.End--------------------------------- */


/* --------------------Ͱ����ü� ������.Begin---------------------------- */
#define BEAM_THRESH_LOW					((1<<ESR_Q_SCORE)*200)
/* Bucket sort ���÷֣���֡��ǰ֡����40����,����Qֵ�� */
#define BUCKET_SCORE_STEP				(4*(1<<ESR_Q_SCORE))
#define BUCKET_EXCEED_MAX_SCORE			((40)*(1<<ESR_Q_SCORE))   /* ÿ��ʹ����֡�����ֵ���Ǳ�֡���ֵ���ܳ�����֡���ֵ�������� */
#define BUCKET_PLUS_SCORE_COUNT			(BUCKET_EXCEED_MAX_SCORE/BUCKET_SCORE_STEP) /* �÷ֱ����ֵ���Ͱ�� */
#define BUCKET_ALL_COUNT				(32 + (BEAM_THRESH_LOW+BUCKET_EXCEED_MAX_SCORE)/BUCKET_SCORE_STEP)  /* Ͱ����ע��!!!!!:���ֵ���ܳ���258,�ڴ��Ǹ��õ�front��FrameCache */
#define	BUCKET_BEAM_COUNT				(BEAM_THRESH_LOW/BUCKET_SCORE_STEP)
#define BUCKTE_PRUNE_MAX_NODE			(150) /* VAD��⵽������ʼ��Ĳü������node�� */

#define ESR_MAX_NBEST					 (3)
  
/* log0 */
#if LOAD_FLOAT_MDOEL
    #define LZERO						(-1.0E6) //(log0)
    #define LMINEXP						(-6.0)
    #define LSMALL						(-0.5E6)
#else
    #define LZERO                           (-0x7800)
#endif


#define DW_MINI_RES_CHECK               (0x20100107)
#define DW_MODEL_CHECK                  (0x20110107)
#define DW_TAG_MODEL_CHECK              (0x20130503)

/* ���grm������һ��������Model���ϲ��洢Ϊһ��bin */
typedef struct tagGrmDesc{
	ivUInt32  nGrmID;									 
	ivInt32   nSpeechTimeOutFrm;    /* ����ʱ������(֡��) */ 
	ivInt32   nMinSpeechFrm;	    /* �������ʱ��(֡),���ڻ��������жϵ� */
 	ivUInt32  nGrmOffset;			/* Unit:word. */					 
	ivUInt32  nGrmSize;		        /* Unit:word. */			  
}TGrmDesc, ivPtr PGrmDesc;

typedef struct tagGrmMdlHeader	
{
    ivUInt32 nCRC;	        /* ��CRC�Ƕ�ȥ��sizeof(TGrmMdlHdr)��CRC */			
    ivUInt32 nCRCCnt;      /* Unit:word */
	ivUInt32 dwCheck;			
	ivUInt32 nModelOffset;	/* Unit:word */	
	ivUInt32 nModelSize;						
	ivUInt32 nTotalGrm;			
	TGrmDesc tGrmDesc[1];		
}TGrmMdlHdr, ivPtr PGrmMdlHdr;

typedef struct tagResHeader						
{
	ivUInt32    nCRC;	        /* ��ȥ��nCRC��nCRCSize�ֶκ������size����CRC */				
	ivUInt32    nCRCCnt;		/* Unit:word */		
	ivUInt32    dwCheck;                /* У���֣�Ϊ�˺��ں˴����Ӧ�� */	
    ivUInt32    nMixSModelOffset;       /* Unit:word */
    ivUInt32    nSilSModelOffset;       /* Unit:word */
    ivUInt32    nFillerSModelOffset;    /* Unit:word. �¾�ʶ������fillerģ�� */

    ivUInt32    nOldFillerSModelOffset; /* Unit:word. �Ͼ�ʶ������fillerģ�� �¼�*/
    ivUInt32    nOldFillerCnt;          /* En_Child_filler,En_Adult_filler,Cn_Child_filler,Cn_Adult_filler */
    ivUInt32    nOldFillerMix;

    ivUInt32    nSilMixture;           
    ivUInt32    nFillerMixture;
    ivUInt32    nMixture;              /* ����sil��filller������ģ�͵�mixture��һ���� */
    ivUInt32    nMixSModelNum;         /* ����sil��filler */
    
    ivInt16     s16MeanCoef[FEATURE_DIMNESION];
    ivInt16     s16MeanCoefQ[FEATURE_DIMNESION];

}TResHeader,ivPtr PResHeader;
typedef TResHeader ivCPtr PCResHeader;

typedef struct tagGaussianModel		
{
	ivInt16		fGConst;			/* Together with GWeight, Q8 */
	ivInt16		fMeanVar[FEATURE_DIMNESION]; 
#if LOAD_FLOAT_MDOEL    /* ������Ҳ�����Դ hbtao*/
    float       fltGConst;
    float       fltWeight;
    float       fltMean[FEATURE_DIMNESION];
    float       fltVar[FEATURE_DIMNESION];
#endif
}TGaussianModel,TGModel,ivPtr PGaussianModel,ivPtr PGModel;
typedef TGaussianModel ivCPtr PCGModel;

typedef struct tagMixStateModel{		
	/* Gauss Model */
#if LOAD_FLOAT_MDOEL
    char            szName[256];
#endif
	TGaussianModel tGModel[1];			/* �䳤����,ʵ�ʸ�������ģ�����ȷ��. */
}TMixStateModel, ivPtr PMixStateModel;
typedef TMixStateModel ivCPtr PCMixSModel;

/* Word Result */
typedef struct tagNBest		
{
	ivUInt16	nWordID;
#if !LOAD_FLOAT_MDOEL
	ivInt16		fScore;
#else
    float       fScore;
#endif
#if MINI5_USE_NEWCM_METHOD
#if !LOAD_FLOAT_MDOEL
	ivInt16		fCMScore;
#else
    float       fCMScore;
#endif
#endif
}TNBest,ivPtr PNBest;

#if MINI5_SPI_MODE
#define MINI5_SPI_PAGE_SIZE         ((ivUInt16)256)       /* ��ע9160ƽ̨SPIһҳ��С.��λ:�ֽ� */
#endif

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT
/*------------------------------------------------------------------------------------------*/
/* ------------------------------Ч������������----------------------------- */
//#define TAG_LOG						/* Log */
//#define TAG_SEARCH_ADD_SIL		/* �Ƿ��ڽ���ʱ�ͼ����ѡsil������� */
/* ------------------------------------------------------------------------- */

#define DW_TAGCMD_CHECK				(0x20120503) /* ���ɵ�tag��У���� */
#define MAXNUM_TAGCMD_STATENUM		(160)		/* ������������ǩ����ʵ����state���� */
#define STATE_LPF_PUNISH_SCORE		(3*(1<<ESR_Q_SCORE))		/* ��ֹstate������������ʱ����ĳ˷��÷� */

typedef struct tagPathItem{		
    ivUInt16 iState;		    /* ��ǰ����state */
    ivUInt16 nRepeatFrm;		/* ��ǰ����state�Ѿ��ظ��˶���֡ */
}TPathItem ,ivPtr PPathItem;

typedef struct tagTagPathDesc{	
	TPathItem   tPath[1];
	ivInt16     pFeature[FEATURE_DIMNESION];		/* ��֡������ */
}TTagPathDesc ,ivPtr PTagPathDesc;
typedef TTagPathDesc ivCPtr PCTagPathDesc;

typedef struct tagTagCmdDesc		
{
	ivUInt32		dwCheck;		
    ivUInt16		nID;			
    ivUInt16		nState;			
	ivUInt16        nMixture;		
	TMixStateModel	pMixSModel[1];  /* ����������,ʵ�ʸ���ΪnState�� */
}TTagCmd, ivPtr PTagCmd;

/*------------------------------------------------------------------------------------------*/
#endif

/* Declare of ASR object */
typedef struct tagEsrEngine TEsrEngine,ivPtr PEsrEngine;

/* �µ�CM����:��������·��: */
/*           s1---s1---s1---s2---s2-----s3-----s4--- */
/*           |-----------|  |-----|    |--|    |--| */
/*  nCMScore =   (min1     +  min2  +  min3 +  min4) / 4  */
/*  ����min1��ʾ״̬s1����֡����С��stateoutlike-filleroutlike */
typedef struct tagSearchNode
{
#if !LOAD_FLOAT_MDOEL
    ivInt16 fScore;               /* ��������÷ִ��� */
#else
    float fScore;               /* ��������÷ִ��� */
#endif
#if MINI5_USE_NEWCM_METHOD
#if !LOAD_FLOAT_MDOEL
	ivInt16 fCMScore;             /* ���浱ǰstate֮ǰ����state��·���ϵ�nCMScoreSum(��û�г���nStateCount) */
	ivInt16 fCurStateCMScore;     /* ��ǰstate��outlike-FillerOutlike�������ǰ״̬�����ظ���֡����ȡ��Сֵ. */
#else
    float fCMScore;             /* ���浱ǰstate֮ǰ����state��·���ϵ�nCMScoreSum(��û�г���nStateCount) */
    float fCurStateCMScore;     /* ��ǰstate��outlike-FillerOutlike�������ǰ״̬�����ظ���֡����ȡ��Сֵ. */
#endif
#endif  /* #if MINI5_USE_NEWCM_METHOD */

}TSearchNode, ivPtr PSearchNode;

typedef struct tagSPICache
{
    ivPointer	pSPIBufBase;
    ivPointer   pSPIBuf;            /* ҳ�����SPI��ǰ���õ�ַ */
    ivPointer   pSPIBufEnd;

    ivPInt8		pCacheSPI;		/* RAM.������Ҫ�洢��SPI������,����һҳдһ��SPI,����SPIд������ */
    ivUInt16	iCacheSPIWrite; 
}TSPICache, ivPtr PSPICache;

typedef struct tagEsSearch			
{
	ivUInt16    iFrameIndexLast;	
	
    /* Model */
    ivUInt16    nMixture;	
    ivUInt16    nSilMixture;
    ivUInt16    nFillerMixture;
#if !MINI5_USE_NEWCM_METHOD
    ivUInt16    nFillerCnt;
#endif
    PCMixSModel pState;		
    PCMixSModel pSilState;
    PCMixSModel pFillerState;

	/* Cache for Calculation of all State */
	PSearchNode pSearchNodeLast;
	/*-------------------------------------*/

#if !LOAD_FLOAT_MDOEL
	/* Frame data */
	ivInt16		fFeature[FEATURE_DIMNESION];		
	/*------------*/

	/* For Filler */
	ivInt16		fSilScore;		/* ÿ֡��sil �÷� */	
	ivInt16     fFillerScore;	/* ÿ֡��Filler�÷� */		
#else
    /* Frame data */
    float		fFeature[FEATURE_DIMNESION];		
    /*------------*/

    /* For Filler */
    float		fSilScore;		/* ÿ֡��sil �÷� */	
    float       fFillerScore;	/* ÿ֡��Filler�÷� */		
    double      fFillerScoreSum;
#endif
	/* Network */
	ivUInt16	nTotalNodes;						
	ivUInt16    nExtendNodes;						
	PLexNode    pLexRoot;							
	PCmdDesc	pCmdDesc;							

	/* ��¼endfiller�ϵ�WordID */
	TNBest		tNBest[ESR_MAX_NBEST];					
		
	/* Ͱ����ü� */
	ivPUInt16	pnBucketCacheLast;	

#if !LOAD_FLOAT_MDOEL
	ivInt16     fScoreMaxLast;					
	ivInt16		fScoreThreshLast;				
#else
    float     fScoreMaxLast;					
    float		fScoreThreshLast;				
#endif

	ivUInt16	nCmdNum;						

    ivUInt16	iEngineType;
#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT

    TSPICache   tSPICache;
    ivUInt16    nClusters;

    ivPInt16	pStaScoreCacheLast;		
	ivPInt16	pnRepeatFrmCacheLast;	/* ��¼ÿ��state�ظ����ֵ�֡�� */
	ivInt16     iMaxStateLast;			/* ��¼�÷�max���ĸ�state */

#endif
}TEsSearch,ivPtr PEsSearch;

EsErrID EsSearchInit(PEsSearch pThis, ivCPointer pModel,PEsrEngine pEngine);

EsErrID EsSetLexicon(PEsSearch pThis,ivCPointer pLexicon, PEsrEngine pEngine);

void EsSearchReset(PEsSearch pThis);

EsErrID EsSearchFrameStep(PEsSearch pThis);

void EsUpdateNBest(PEsSearch pThis);

ivInt32 LAdd(ivInt32 x, ivInt32 y);

#if MINI5_SPI_MODE && MINI5_TAG_SUPPORT
EsErrID EsTagSearchFrameStep(PEsSearch pThis);
EsErrID EsSPIWriteData(PSPICache pThis, ivCPointer pData, ivUInt32 nDataSize);
EsErrID EsSPIWriteSPIData(PSPICache pThis, ivPointer pRAMBuf,ivCPointer pData, ivUInt32 nDataSize, ivPUInt32 pnCRC,ivPUInt32 piCounter);
EsErrID EsSPIWriteFlush(PSPICache pThis);
void    EsSPIWriteOnePage(ivPointer pSPIBuf, ivPointer pRAMBuf);
#endif  /* MINI5_SPI_MODE && MINI5_TAG_SUPPORT */

/* SPI: See EsPlatform.c */
#if MINI5_SPI_MODE
#define EsCal1GaussOutlike  EsSPICal1GaussOutlike
#define EsCalcOutLike       EsSPICalcOutLike
#define EsUpdateAllScore    EsSPIUpdateAllScore
#endif

#if MINI5_SPI_MODE
EsErrID EsSPIReadBuf(ivPointer pRAMBuf, ivCPointer pSPIBuf, ivUInt16 nSize);
ivInt16 EsSPICal1GaussOutlike(PEsSearch pThis, PCGModel pGModel);
ivInt16 EsSPICalcOutLike(PEsSearch pThis, PCMixSModel pState, ivUInt16 nMixtures);
EsErrID EsSPIUpdateAllScore(PEsSearch pThis);

/* SPI����CRCУ�飬ÿ�ζ�һҳ��С��RAM�м���CRC */
EsErrID EsSPICalcCRC(ivPointer pRAMBuf, ivCPointer pSPIData,ivUInt32 nSize,ivPUInt32 pnCRC,ivPUInt32 piCounter);
#else
#if !LOAD_FLOAT_MDOEL
ivInt16 EsCal1GaussOutlike(PEsSearch pThis, PCGModel pGModel);
ivInt16 EsCalcOutLike(PEsSearch pThis, PCMixSModel pState, ivUInt16 nMixtures);
EsErrID EsUpdateAllScore(PEsSearch pThis);
#else /* MINI5_SPI_MODE */
float EsCal1GaussOutlike(PEsSearch pThis, PCGModel pGModel);
float EsCalcOutLike(PEsSearch pThis, PCMixSModel pState, ivUInt16 nMixtures);
EsErrID EsUpdateAllScore(PEsSearch pThis);
#endif
#endif /* MINI5_SPI_MODE */

#endif /* !defined(ES_TEAM__2007_07_10__ESKERNEL_ESSEARCH__H) */
