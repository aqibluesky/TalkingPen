/******************************************************************************
* File Name		       : ivMemory.c
* Description          : InterSound �ڴ����Դ�ļ�
* Author               : pingbu
* Date Of Creation     : 2004-05-21
* Platform             : Any
* Modification History : 
*------------------------------------------------------------------------------
* Date        Author     Modifications
*------------------------------------------------------------------------------
* 2004-05-21  pingbu     Created
******************************************************************************/

#include "ivMemory.h"
#include "ivDebug.h"


#if !IV_ANSI_MEMORY


/* ��ʼ���ڴ�� */
/* add by huxiong 2008-3-13 */
void ivCall ivMemZero( ivPointer pBuffer0, ivSize nSize )
{
	ivUInt16 nUnitBytes = sizeof(ivUInt16); /* �ڴ������Ԫ�ֽ��� */
	if(2 == nUnitBytes){
		ivPInt16 pBuffer;
		pBuffer = (ivPInt16)pBuffer0;
		ivAssert(pBuffer);

		ivAssert(0 == (0x01 & nSize));

		while(nSize>0)
		{
			*pBuffer++ = 0;
			nSize -= 2;
		}	
	}
	else if(1 == nUnitBytes){
		ivPInt16 pBuffer;
		pBuffer = (ivPInt16)pBuffer0;
		ivAssert(pBuffer);

		while ( nSize -- )
			*pBuffer++ = 0;
	}
	else{
		ivAssert(ivFalse);
	}
	
}

void ivCall ivMemCopy( ivPointer pDesc0, ivCPointer pSrc0, ivSize nSize )
{
	ivUInt16 nUnitBytes = sizeof(ivUInt16); /* �ڴ������Ԫ�ֽ��� */
	if(2 == nUnitBytes){
		ivPInt16 pDesc;
		ivPCInt16 pSrc;
		pDesc = (ivPInt16)pDesc0;
		pSrc = (ivPCInt16)pSrc0;
		ivAssert(pDesc && pSrc);

		ivAssert(0 == (0x01 & nSize));

		while(nSize>0){
			*pDesc++ = *pSrc++;
			nSize -= 2;
		}
	}
	else if(1 == nUnitBytes){
		ivPInt16 pDesc;
		ivPCInt16 pSrc;
		pDesc = (ivPInt16)pDesc0;
		pSrc = (ivPCInt16)pSrc0;
		ivAssert(pDesc && pSrc);

		while ( nSize -- )
			*pDesc++ = *pSrc++;
	}
	else{
		ivAssert(ivFalse);
	}
}

#endif
