#include "icKey.h"
/*----------------------------------------------------------------------------------------
������: InitKey
����:
		None
����ֵ:
		None
����:
		����GPIO��ʼ��
----------------------------------------------------------------------------------------*/
void InitKey(void)
{
	DrvGPIO_Open(GPB,2,IO_QUASI);
	DrvGPIO_Open(GPB,3,IO_QUASI);
	DrvGPIO_Open(GPB,5,IO_QUASI);
	GPIOB->DOUT |= (1 << 2);
	GPIOB->DOUT |= (1 << 3);
	GPIOB->DOUT |= (1 << 5);

	DrvGPIO_Open(GPA,7,IO_QUASI);	//��ʼ�����������
	DrvGPIO_Open(GPA,15,IO_INPUT); 	//��ʼ��USB��Դ����
	GPIOA->DOUT |= (1 << 7);
}
/*----------------------------------------------------------------------------------------
������: InitKey
����:
		None
����ֵ:
		None
����:
		��ȡ����״̬
----------------------------------------------------------------------------------------*/
uint16_t GetKeyStatus(void)
{
	uint8_t key = 0;
	key |= ((~DrvGPIO_GetBit(GPB,2) & 0x01) << 0);		//���ػ�/������ͣ
	key |= ((~DrvGPIO_GetBit(GPB,3) & 0x01) << 1);		//¼��/��¼��
	key |= ((~DrvGPIO_GetBit(GPB,5) & 0x01) << 2);		//ħ��->��ħ��/ʶ��
	key &= 0x07;
	return key;
}
/*----------------------------------------------------------------------------------------
������: InitKey
����:
		None
����ֵ:
		None
����:
		��ȡ������Ϣ(�кܴ��ȱ�ݣ�û��ʹ��)
----------------------------------------------------------------------------------------*/
//int* Get_Key_Message()
//{
//	int code, i;
//	static int last_code=0, last_valid_code=0,count[KEY_COUNT],Key_Flag[KEY_COUNT];
////	Key_Flag[0] = 0;
//	code = GetKeyStatus();
//	
//	if(code == last_code){
//		for(i=0; i<KEY_COUNT; i++){
//			if(code & (1<<i)){
//				if(last_valid_code & (1<<i)){
//					count[i]++;
////					if(count[i] == 50)
////						Key_Flag[i] = 2;
//					if((count[i]) % 50 == 0)	//����DOWN HOLD
//						Key_Flag[i] = 2;
//				}else{
//					count[i] = 0;
//					Key_Flag[i] = 1;		//����DOWN
//					last_valid_code |= (1<<i);
//				}	
//			}else{
//				if(last_valid_code & (1<<i)){
//					Key_Flag[i] = 3;		//����UP
//					last_valid_code &= ~(1<<i);	
//				}
//			}
//		}
//	}else{
//		for(i=0; i<KEY_COUNT; i++){
//			if(last_valid_code & (1<<i)){
//				count[i]++;
//				if(count[i] % 25 == 0)	//����DOWN HOLD
//					Key_Flag[i] = 2;
//			}	
//		}			
//	}
//	last_code = code;
//
//	return Key_Flag;
//}
