#ifndef PRP_APP_KEY_H_
#define PRP_APP_KEY_H_

#include "Driver\DrvGPIO.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvTimer.h"

#define KEY_COUNT 2

/*������ʼ��--------------------------------*/
void InitKey(void);

/*��ȡ����״̬-------------------------------*/
uint16_t GetKeyStatus(void);

/*��ȡ������Ϣ-------------------------------*/
//int* Get_Key_Message(void);

#endif
