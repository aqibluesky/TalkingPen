/*----------------------------------------------+
 |												|
 |	ivPlatform.h - Platform Config				|
 |												|
 |		Platform: Win32 (X86)					|
 |												|
 |		Copyright (c) 1999-2009, iFLYTEK Ltd.	|
 |		All rights reserved.					|
 |												|
 +----------------------------------------------*/


/*
 *	TODO: ���������Ŀ��ƽ̨������Ҫ�Ĺ���ͷ�ļ�
 */

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <crtdbg.h>



/*
 *	TODO: ����Ŀ��ƽ̨�����޸����������ѡ��
 */

#define IV_PTR_GRID				4			/* ���ָ�����ֵ */

#define IV_PTR_PREFIX						/* ָ�����ιؼ���(����ȡֵ�� near | far, ����Ϊ��) */
#define IV_CONST				const		/* �����ؼ���(����Ϊ��) */
#define IV_STATIC				static		/* ��̬�����ؼ���(����Ϊ��) */
#define IV_INLINE				//__inline	/* �����ؼ���(����ȡֵ�� inline, ����Ϊ��) */
#define IV_CALL_STANDARD		//__stdcall	/* ��ͨ�������ιؼ���(����ȡֵ�� stdcall | fastcall | pascal, ����Ϊ��) */

#define IV_TYPE_INT8			char		/* 8λ�������� */
#define IV_TYPE_INT16			short		/* 16λ�������� */
#define IV_TYPE_INT32			long		/* 32λ�������� */

#if 0 /* 48/64 λ���������ǿ�ѡ��, ��Ǳ�Ҫ��Ҫ����, ��ĳЩ 32 λƽ̨��, ʹ��ģ�ⷽʽ�ṩ�� 48/64 λ������������Ч�ʺܵ� */
#define IV_TYPE_INT48			__int64		/* 48λ�������� */
#define IV_TYPE_INT64			__int64		/* 64λ�������� */
#endif

#define IV_TYPE_SIZE			unsigned long		/* ��С�������� */

#define IV_ANSI_MEMORY			0			/* �Ƿ�ʹ�� ANSI �ڴ������ */

#define IV_ASSERT(exp)			//_ASSERT(exp)/* ���Բ���(����Ϊ��) */
#define IV_YIELD				Sleep(0)	/* ���в���(��Э��ʽ����ϵͳ��Ӧ����Ϊ�����л�����, ����Ϊ��) */

/* ����ƽ̨����ѡ������Ƿ�֧�ֵ��� */
#if defined(DEBUG) || defined(_DEBUG)
	#define IV_DEBUG			1			/* �Ƿ�֧�ֵ��� */
#else
	#define IV_DEBUG			0			/* �Ƿ�֧�ֵ��� */
#endif

/* ����ƽ̨����ѡ������Ƿ��� Unicode ��ʽ���� */
#if defined(UNICODE) || defined(_UNICODE)
	#define IV_UNICODE			1			/* �Ƿ��� Unicode ��ʽ���� */
#else
	#define IV_UNICODE			0			/* �Ƿ��� Unicode ��ʽ���� */
#endif
