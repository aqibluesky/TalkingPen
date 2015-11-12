/*----------------------------------------------+
 |												|
 |	ivDefine.h - Basic Definitions				|
 |												|
 |		Copyright (c) 1999-2008, iFLYTEK Ltd.	|
 |		All rights reserved.					|
 |												|
 +----------------------------------------------*/

#ifndef IFLYTEK_VOICE__DEFINE__H
#define IFLYTEK_VOICE__DEFINE__H


#include "ivPlatform.h"

/*
 *	���η�
 */

#define ivConst		IV_CONST
#define ivStatic	IV_STATIC
#ifdef IV_INLINE
#define ivInline	IV_STATIC IV_INLINE
#else
#define ivInline	IV_STATIC
#endif
/* #define ivExtern	IV_EXTERN */

#define ivPtr		IV_PTR_PREFIX*
#define ivCPtr		ivConst ivPtr
#define ivPtrC		ivPtr ivConst
#define ivCPtrC		ivConst ivPtr ivConst

#define ivCall		IV_CALL_STANDARD	/* �ǵݹ��(���������) */
///* #define ivReentrant	IV_CALL_REENTRANT	/ * �ݹ��(�������) * /*/
///* #define ivVACall	IV_CALL_VAR_ARG */		/* ֧�ֱ�ε� */

#define ivProc		ivCall ivPtr


/*
 *	����
 */

#define	ivNull	(0)

/* �������ͼ���ֵ(�����ƽ̨����) */
typedef	int ivBool;

#define	ivTrue	(~0)
#define	ivFalse	(0)

/* �Ա����ͼ���ֵ */
typedef int ivComp;

#define ivGreater	(1)
#define ivEqual		(0)
#define ivLesser	(-1)

#define ivIsGreater(v)	((v)>0)
#define ivIsEqual(v)	(0==(v))
#define ivIsLesser(v)	((v)<0)


/*
 *	��������
 */

/* ����ֵ���� */
typedef	signed IV_TYPE_INT8		ivInt8;	/* 8-bit */
typedef	unsigned IV_TYPE_INT8	ivUInt8;	/* 8-bit */

typedef	signed IV_TYPE_INT16	ivInt16;	/* 16-bit */
typedef	unsigned IV_TYPE_INT16	ivUInt16;	/* 16-bit */

typedef	signed IV_TYPE_INT32	ivInt32;	/* 32-bit */
typedef	unsigned IV_TYPE_INT32	ivUInt32;	/* 32-bit */

#ifdef IV_TYPE_INT48
typedef	signed IV_TYPE_INT48	ivInt48;	/* 48-bit */
typedef	unsigned IV_TYPE_INT48	ivUInt48;	/* 48-bit */
#endif

#ifdef IV_TYPE_INT64
typedef	signed IV_TYPE_INT64	ivInt64;	/* 64-bit */
typedef	unsigned IV_TYPE_INT64	ivUInt64;	/* 64-bit */
#endif

/* ��Ӧ��ָ������ */
typedef	ivInt8		ivPtr ivPInt8;		/* 8-bit */
typedef	ivUInt8		ivPtr ivPUInt8;		/* 8-bit */

typedef	ivInt16		ivPtr ivPInt16;		/* 16-bit */
typedef	ivUInt16	ivPtr ivPUInt16;	/* 16-bit */

typedef	ivInt32		ivPtr ivPInt32;		/* 32-bit */
typedef	ivUInt32	ivPtr ivPUInt32;	/* 32-bit */

#ifdef IV_TYPE_INT48
typedef	ivInt48		ivPtr ivPInt48;		/* 48-bit */
typedef	ivUInt48	ivPtr ivPUInt48;	/* 48-bit */
#endif

#ifdef IV_TYPE_INT64
typedef	ivInt64		ivPtr ivPInt64;		/* 64-bit */
typedef	ivUInt64	ivPtr ivPUInt64;	/* 64-bit */
#endif



/* ����ָ������ */
/* typedef	ivInt8		ivCPtr ivPCInt8; */		/* 8-bit */
/* typedef	ivUInt8		ivCPtr ivPCUInt8; */	/* 8-bit */

typedef	ivInt16		ivCPtr ivPCInt16;	/* 16-bit */
typedef	ivUInt16	ivCPtr ivPCUInt16;	/* 16-bit */


typedef	ivInt32		ivCPtr ivPCInt32;	/* 32-bit */
typedef	ivUInt32	ivCPtr ivPCUInt32;	/* 32-bit */

#ifdef IV_TYPE_INT48
typedef	ivInt48		ivCPtr ivPCInt48;	/* 48-bit */
typedef	ivUInt48	ivCPtr ivPCUInt48;	/* 48-bit */
#endif

#ifdef IV_TYPE_INT64
typedef	ivInt64		ivCPtr ivPCInt64;	/* 64-bit */
typedef	ivUInt64	ivCPtr ivPCUInt64;	/* 64-bit */
#endif

/* �߽�ֵ���� */
#define IV_MAX_INT16	(+32767)
#define IV_MAX_UINT16	(+65535)
#define IV_INT_MAX		(+8388607L)
#define IV_MAX_INT32	(+2147483647L)

/* #define IV_SBYTE_MIN	(-IV_SBYTE_MAX - 1) */
#define IV_MIN_INT16	(-IV_MAX_INT16 - 1)
#define IV_INT_MIN		(-IV_INT_MAX - 1)
#define IV_MIN_INT32	(-IV_MAX_INT32 - 1)


/* �ڴ������Ԫ */
/* typedef	ivUInt8		ivByte; */
/* typedef	ivPUInt8	ivPByte; */
/* typedef	ivPCUInt8	ivPCByte; */

typedef	signed			ivInt;
typedef	signed ivPtr	ivPInt;
typedef	signed ivCPtr	ivPCInt;

typedef	unsigned		ivUInt;
typedef	unsigned ivPtr	ivPUInt;
typedef	unsigned ivCPtr	ivPCUInt;

/* ָ�� */
typedef	void ivPtr	ivPointer;
typedef	void ivCPtr	ivCPointer;

/* ��� */
typedef ivPointer ivHandle;

/* ��ַ���ߴ����� */
typedef	IV_TYPE_SIZE	ivSize;
typedef IV_TYPE_ADDRESS ivAddress;



typedef void (ivProc PCBGetResult)(
								   ivPInt16 pData,
								   ivInt16 nDataSize
								   );

typedef struct tagUser
{
	PCBGetResult	lpfnGetRusult;
}TUser,ivPtr PUser;

#endif /* !IFLYTEK_VOICE__DEFINE__H */
