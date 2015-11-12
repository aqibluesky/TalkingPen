
#include "hw.h"
#include "pwm_drv.h"

typedef struct pwm_struct
{
	uint32_t cnr0; //pwmͨ��0�Ķ�ʱ��������������pwm������
	uint32_t duty0;//pwmͨ��0��ռ�ձ�
	uint32_t cnr1;
	uint32_t duty1;
} PWM_STRUCT;

PWM_STRUCT pwm_struct;

//��ʼ��pwm����Ҫ�޸�pwm��Ƶ�ʵ�ʱ����Ҫ���øú����������õ�Ƶ�ʷ�ΧΪ��1-240KHz
//channel:Ҫ��ʼ����ͨ��ͨ��0��ӦPA12, ͨ��1��ӦPA13
//freq   :pwm�����Ƶ��,��λ��KHz,���ֵ��240,��Ϊ����Ƶ�ʵ�ʱ��ռ�ձ��޷�����100�������ֶ�
//duty   :pwm�ĳ�ʼռ�ձȣ���ʼ����ɺ���������ռ�ձȵ�pwm�ź�
//����ֵ :�������õ�pwmƵ��		
uint32_t pwm_init(uint8_t channel, uint32_t freq, uint32_t duty)
{
	uint32_t cnr, cmr;
	SYSCLK->APBCLK.PWM01_EN = 1;
	SYSCLK->CLKSEL1.PWM01_S = 3;//ʹ��48MHz	
	PWMA->PPR.CP01 = 1; //1+1 ��Ƶ
	if(freq > 240)
		freq = 240;
	cnr = 24000 * 100 / freq;
	cnr = cnr / 100;
	cmr = cnr * duty / 100;
	if(channel == 0)
	{	
		SYS->GPA_ALT.GPA12 = 1; //PWM0
		PWMA->CSR.CSR0 = 4; //1��Ƶ pwmʱ�� = 48M / 2 / 1 = 24MHz
		PWMA->PCR.CH0MOD = 1; //Auto-reload mode
		PWMA->PCR.CH0INV = 0; //����
		PWMA->CNR0 = cnr;
		PWMA->CMR0 = cmr; 			
		PWMA->POE.PWM0 = 1;
		PWMA->PCR.CH0EN = 1;
		pwm_struct.cnr0 = cnr;
		pwm_struct.duty0 = duty;
	}
	else
	{	
		SYS->GPA_ALT.GPA13 = 1; //PWM1
		PWMA->CSR.CSR1 = 4; //1��Ƶ pwmʱ�� = 48M / 2 / 1 = 24MHz
		PWMA->PCR.CH1MOD = 1; //Auto-reload mode
		PWMA->PCR.CH1INV = 0; //����
		PWMA->CNR1 = cnr;
		PWMA->CMR1 = cmr;	
		PWMA->POE.PWM1 = 1;
		PWMA->PCR.CH1EN = 1;
		pwm_struct.cnr1 = cnr;
		pwm_struct.duty1 = duty;		
	}
	return freq;
}

//��ʼ����pwm����Ե��øú���������ռ�ձ�
//channel : Ҫ���õ�ͨ�� 0:��ӦPA12 1:��ӦPA13
//duty    : Ҫ�����ռ�ձ�
//off_stat: ������õ�ռ�ձ�Ϊ0��ʱ�򣬶�Ӧͨ������õ�ƽ��0������͵�ƽ��1������ߵ�ƽ
uint32_t pwm_ctrl(uint8_t channel, uint8_t duty, uint8_t off_stat)
{
	uint32_t cmr;
	if(duty == 0)
	{
		if(channel == 0)
		{
			SYS->GPA_ALT.GPA12 = 0;
			if(off_stat) //ռ�ձ�Ϊ0��ʱ���ǹر�pwm�����������ó�һ��״̬
			{
				GPIOA->DOUT |= 1 << 12;	
			}
			else
			{
				GPIOA->DOUT &= ~(1 << 12);
			}	
			GPIOA->PMD.PMD12 = IO_OUTPUT;			
		}
		else
		{
			SYS->GPA_ALT.GPA13 = 0;
			if(off_stat)
			{
				GPIOA->DOUT |= 1 << 13;
			}
			else
			{
				GPIOA->DOUT &= ~(1 << 13);
			}
			GPIOA->PMD.PMD13 = IO_OUTPUT;
		}
	}
	else
	{
		if(channel == 0)
		{
			SYS->GPA_ALT.GPA12 = 1; //PWM0
			cmr = pwm_struct.cnr0 * duty / 100;
			PWMA->CMR0 = cmr;
			pwm_struct.duty0 = duty;
			PWMA->PCR.CH0EN = 1;	
		}
		else
		{
			SYS->GPA_ALT.GPA13 = 1; //PWM1	
			cmr = pwm_struct.cnr1 * duty / 100;
			PWMA->CMR1 = cmr;
			pwm_struct.duty1 = duty;
			PWMA->PCR.CH1EN = 1;
		}
	}
	return duty;
}





//add by wgtbupt

void pwm_led_init(void)
{
	pwm_init(0, 120, 0);
	pwm_init(1, 120, 0);
}


void pwm_led_set(uint8_t led0_duty, uint8_t led1_duty)
{
#if   0
	if(100 == led0_duty)
	{
		pwm_ctrl(0, 0, 1);	
	}

	if(100 == led1_duty)
	{
		pwm_ctrl(1, 0, 1);	
	}
#endif

	pwm_ctrl(0, led0_duty, 0);	
	pwm_ctrl(1, led1_duty, 0);
}

