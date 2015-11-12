/*********************************************************************#
//	�ļ���		��EsNetwork.h
//	�ļ�����	��ʶ��������������ݽṹ�Ķ���
//	����		��Truman
//	����ʱ��	��2007��7��10��
//	��Ŀ����	��EsKernel
//	��ע		��
/---------------------------------------------------------------------#
//	��ʷ��¼��
//	���	����		����	��ע
#**********************************************************************/

#ifndef IFLY_ES_TEAM__2007_07_30__ESRNETWORK__H
#define IFLY_ES_TEAM__2007_07_30__ESRNETWORK__H

#include "EsFrontParam.h"

#define DW_LEXHEADER_CHECK	(0x20080114)

/* ��������洢ʱ��֤����Ҷ�ӽڵ㿿��洢����������֪����Щ����Դ�����EndFiller��ȥ,ʡȥnResult�ֶ�;����洢���õ���洢��pLexRootָ�����һ���ڵ� */
typedef struct tagLexiconNode	 
{	
	ivUInt16 iParent;			    /* ��1��ʼ��0��ʾ�޸��ڵ� */
	
	ivUInt16 iSModeIndex;		    /* �ýڵ��Ӧ��ģ��״̬ID */
	ivUInt16 nStateCount;           /* Ҷ�ӽڵ��ϴ洢���Ǹ�����ʵ�state����.��Ҷ�ӽڵ�=0 */
}TLexNode, ivPtr PLexNode;

/* Lexicon */
typedef struct tagLexiconHeader
{
	ivUInt16 nTotalNodes;		
	ivUInt16 nExtendNodes;		/* ��ʼ�����ɽڵ㣬��δ洢��������ǵ�VAD��Ϊ����δ��ʼʱ����չ�Ľڵ� */
	ivUInt32 dwCheck;			
	ivUInt32 nCmdNum;			/* ����ʸ���,���ڼ��ESRSetCmdActiveʱnID����Ч�Ե� 20101022 */
	ivUInt32 nLexRootOffset;	
	ivUInt32 nCmdDescOffset;	
}TLexiconHeader,ivPtr PLexiconHeader;
typedef TLexiconHeader ivCPtr PCLexiconHeader;

typedef struct tagCmdDesc
{
	ivUInt16 nID;							
	ivInt16  nCMThresh;     /* ��ʶ���� */		
	ivUInt16 bTag;		    /* �Ƿ���voicetag */
}TCmdDesc, ivPtr PCmdDesc;

#endif /* IFLY_ES_TEAM__2007_07_30__ESRNETWORK__H */
