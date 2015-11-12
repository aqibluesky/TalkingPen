/*********************************************************************#
//	�ļ���		��ESSearchConst.h
//	�ļ�����	��ESSearch��ʹ�õĳ�������
//	����		��qungao    
//	����ʱ��	��2013��5��17��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/

#if !defined(ES_TEAM__2013_05_17__ESSEARCHCONST__H)
#define ES_TEAM__2013_05_17__ESSEARCHCONST__H

#include "ivESR.h"

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#define ESR_VAR_QUANT_BIT               (5)     /* ��������bit�� */
#define ESR_MEAN_QUANT_BIT              (6)     /* ��ֵ����bit�� */

#define ESR_Q_MEAN_MANUAL               (ESR_MEAN_QUANT_BIT)     /* ��ֵ����ֵ. ģ�Ͷ��㻯��������ֵ�����ʹ��. */
#define ESR_Q_VAR_MANUAL                (0)     /* �����ֵ. ģ�Ͷ��㻯��������ֵ�����ʹ��. */
#define ESR_Q_WEIGHT_GCOSNT_MANUAL	    (8)     /*  */ 
#define ESR_Q_MEANVARTAB_MANUAL         (8)     /* ��ֵ�������ֵ�Ķ���ֵ */
#define ESR_Q_SCORE						(4)     /*  */
extern ivConst ivInt16 g_s16MeanVarTable[1<<(ESR_VAR_QUANT_BIT+ESR_MEAN_QUANT_BIT+1)];	

#if !MINI5_USE_NEWCM_METHOD 
/* �洢����Ϊ0-100֮��÷ֵ�CM�÷ֵ���ʵֵ, Q10 */
extern ivConst ivInt16 g_s16CMScore[101];
#endif

#endif /* !defined(ES_TEAM__2013_05_17__ESSEARCHCONST__H) */
