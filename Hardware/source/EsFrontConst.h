/*********************************************************************#
//	�ļ���		��ESTransformConst.h
//	�ļ�����	��ESTransform��ʹ�õĳ�������
//	����		��Truman
//	����ʱ��	��2007��9��19��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/

#if !defined(ES_TEAM__2007_09_19__ESTRANSFORMCONST__H)
#define ES_TEAM__2007_09_19__ESTRANSFORMCONST__H

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#include "EsFrontParam.h"


extern ivConst ivInt16 g_s16CMNCoef[TRANSFORM_CEPSNUM_DEF+1];

/* For CMN Floor */
extern ivConst ivInt16 g_s16CMNCoefMax[TRANSFORM_CEPSNUM_DEF+1];
extern ivConst ivInt16 g_s16CMNCoefMin[TRANSFORM_CEPSNUM_DEF+1];

#if (8000 == MINI5_SAMPLERATE)
extern ivConst ivInt16 g_pwFilterbankInfo[290]; /* Ϊ˶��ƽ̨��໯���� �����FFTStartIndex, count, weight(num=count) */

/*Ham Window Table(0.16 format)  f=2*PI/(200-1), X=0.54-0.46*cos(f*i) */
extern ivConst ivUInt16 g_sHamWindow[200];

#if ESR_CPECTRAL_SUB
extern ivConst ivInt32 g_nNoiseInitValue[129];  /* 8K������ʼֵ */
#endif /* ESR_CPECTRAL_SUB */

#elif (16000 == MINI5_SAMPLERATE)
extern ivConst ivInt16 g_pwFilterbankInfo[530]; /* Ϊ˶��ƽ̨��໯���� �����FFTStartIndex, count, weight(num=count) */

/*Ham Window Table(0.16 format)  f=2*PI/(200-1), X=0.54-0.46*cos(f*i) */
extern ivConst ivUInt16 g_sHamWindow[400];

#if ESR_CPECTRAL_SUB
extern ivConst ivInt32 g_nNoiseInitValue[257];  /* 16k ������ʼֵ */
#endif /* ESR_CPECTRAL_SUB */

#else

#endif
/* Discrete reverse cosine transform Q2.13 */
extern ivConst ivInt16 g_wFBToMFCCTable[288];

extern ivConst ivUInt16 g_s16SimpleLnTable[512]; /* Q16, [1-2)��lnֵ����[0,1)�ֳ�2^9�� */

#endif /* !defined(ES_TEAM__2007_09_19__ESTRANSFORMCONST__H) */
