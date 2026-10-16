#include "HDC2080.h"
#include <stdio.h>
#define HDC2080_ADDR 0x40//addr���Žӵ�

HDC2080RH HDC2080RH_SENRH;

//#define RH_SDA_IN()     {RH_SDA_GPIO_Port->CRH&=0xFFFFFF0F;RH_SDA_GPIO_Port->CRH|=8<<4;}       //PB9����ģʽ
//#define RH_SDA_OUT()    {RH_SDA_GPIO_Port->CRH&=0xFFFFFF0F;RH_SDA_GPIO_Port->CRH|=3<<4;} 	    //PB9���ģ�?

void RH_SDA_IN(void){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = RH_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(RH_SCL_GPIO_Port, &GPIO_InitStruct);
}

void RH_SDA_OUT(void){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = RH_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(RH_SCL_GPIO_Port, &GPIO_InitStruct);
}

static void CPU_Delay_US(uint16_t n)
{
	delay_us(n);
}

void IIC_Init(void)
{
	RH_IIC_SDA(1);
  RH_IIC_SCL(1);  
}

void IIC_Start(void) 
{
	RH_SDA_OUT();
  RH_IIC_SDA(1);
  RH_IIC_SCL(1);
  CPU_Delay_US(4);
  RH_IIC_SDA(0);
  CPU_Delay_US(4);
  RH_IIC_SCL(0);
}

void IIC_Stop(void) 
{
	RH_SDA_OUT();
  RH_IIC_SCL(0);
  RH_IIC_SDA(0);
  CPU_Delay_US(4);
  RH_IIC_SCL(1);
  CPU_Delay_US(4);
  RH_IIC_SDA(1);
}

uint8_t IIC_Wait_Ack(void) 
{
	uint16_t ucErrTime = 0;
  RH_SDA_IN();
  RH_IIC_SDA(1);
  CPU_Delay_US(1);
  RH_IIC_SCL(1);
  CPU_Delay_US(1);
  while(RH_READ_SDA){
		ucErrTime++;
    if (ucErrTime > 250){
			IIC_Stop();
      return 1;
		}
	}
  RH_IIC_SCL(0);
  return 0;
}

void IIC_Ack(void) 
{
	RH_IIC_SCL(0);
  RH_SDA_OUT();
  RH_IIC_SDA(0);
  CPU_Delay_US(2);
  RH_IIC_SCL(1);
  CPU_Delay_US(2);
  RH_IIC_SCL(0);
}

void IIC_NAck(void) 
{
	RH_IIC_SCL(0);
  RH_SDA_OUT();
  RH_IIC_SDA(1);
  CPU_Delay_US(2);
  RH_IIC_SCL(1);
  CPU_Delay_US(2);
  RH_IIC_SCL(0);
}

void IIC_Send_Byte(uint8_t txd) 
{
	uint8_t t;
  RH_SDA_OUT();
  RH_IIC_SCL(0);
  for (t = 0; t < 8; t++) {
		RH_IIC_SDA((txd & 0x80) >> 7);
    txd <<= 1;
    CPU_Delay_US(2);
    RH_IIC_SCL(1);
    CPU_Delay_US(2);
    RH_IIC_SCL(0);
    CPU_Delay_US(2);
	}
}

uint8_t IIC_Read_Byte(unsigned char ack) 
{
	unsigned char i, receive = 0;
  RH_SDA_IN();
  for (i = 0; i < 8; i++) {
		RH_IIC_SCL(0);
    CPU_Delay_US(2);
    RH_IIC_SCL(1);
    receive <<= 1;
    if (RH_READ_SDA) 
			receive++;
    CPU_Delay_US(1);
	}
  if (!ack)
		IIC_NAck();
  else
    IIC_Ack();
	return receive;
}

void HDC2080_Write_Byte(uint8_t Address, uint8_t Data) 
{
	IIC_Start();
  IIC_Send_Byte((HDC2080_ADDR << 1) | 0);
  IIC_Wait_Ack();
  IIC_Send_Byte(Address);
  IIC_Wait_Ack();
  IIC_Send_Byte(Data);
  IIC_Wait_Ack();
  IIC_Stop();
}

void HDC2080_SendCommand(uint16_t command) 
{
	IIC_Start();
  IIC_Send_Byte((HDC2080_ADDR << 1) | 0x00);
  IIC_Wait_Ack();
  IIC_Send_Byte(command);
  IIC_Wait_Ack();
  IIC_Stop();
}

void HDC2080_Read_Bytes(uint8_t Address, uint8_t *buf, uint8_t len) 
{
	IIC_Start();
  IIC_Send_Byte((HDC2080_ADDR << 1) | 0);
  IIC_Wait_Ack();
  IIC_Send_Byte(Address);
  IIC_Wait_Ack();
 
  IIC_Start();
  IIC_Send_Byte((HDC2080_ADDR << 1) | 1);
  IIC_Wait_Ack();
  while (len) {
		if (len == 1)
			*buf = IIC_Read_Byte(0);
    else
			*buf = IIC_Read_Byte(1);
		buf++;
    len--;
	}
  IIC_Stop();
}
 
uint8_t HDC2080_Read_B(uint8_t Address) 
{
	uint8_t Data_buf;
  IIC_Start();
  IIC_Send_Byte((HDC2080_ADDR << 1) | 0x01);
  IIC_Wait_Ack();
  IIC_Send_Byte(Address);
  IIC_Wait_Ack();
    
  IIC_Start();
  IIC_Send_Byte((HDC2080_ADDR << 1) | 1);
  IIC_Wait_Ack();
  Data_buf = IIC_Read_Byte(0);
  IIC_Stop();
  return Data_buf;
}

double Humidity_Value = 0;
double Temperature_Value = 0;

void HDC2080_ReadTemperatureHumidity(HDC2080RH *Sen) 
{
	uint8_t data[4];
  uint8_t TimeFlag = 0;
  Sen->TempValue_state = 0;
    
  HDC2080_Write_Byte(0x0F, 0x01);
    
  while((HDC2080_Read_B(0x0F) & 0x01)&& (TimeFlag++<250)){};
  HAL_Delay(10);   
    
  HDC2080_Read_Bytes(0x00, data, 4);
  uint16_t tempRaw = (uint16_t)((data[1] << 8) | data[0]);   /* TEMP MSB:LSB, D1/D0 = 0 */
  int16_t temp14 = (int16_t)(tempRaw >> 2);                      /* 14-bit signed, sign-extended */
  Sen->TempValue = ((double)temp14 / 16384.0) * 165.0 - 40.0;    /* TI drivers_hdc2080.c standard decode */
	Temperature_Value = Sen->TempValue;
	//printf("Temperature:%.2f\r\n",Sen->TempValue);
  if(Sen->TempValue < 0) 
		Sen->TempValue_state = 1;
    
  uint16_t humiRaw = (uint16_t)((data[3] << 8) | data[2]);   /* RH MSB:LSB, D1/D0 = 0 */
  Sen->HumiValue = ((double)humiRaw / 65536.0) * 100.0;   /* Eq.3, 12-bit unsigned */
	Humidity_Value = Sen->HumiValue;
	//printf("Humidity:%.2f\r\n",Sen->HumiValue);
}

void HDC2080_Humi_Compensation(uint8_t Compensation){
	HDC2080_Write_Byte(HDC2080_HUMI_COMPENSATIONA_ADDR, Compensation);
}

void HDC2080_Init(void){
	IIC_Init();
	HAL_Delay(3000);
  HDC2080_Write_Byte(REG_CFG, 0x00);
	HDC2080_Humi_Compensation(HDC2080_HUMI_COMPENSATIONB_DATA);
}

void HDC2080_GETVALUE(void){
	HDC2080_ReadTemperatureHumidity(&HDC2080RH_SENRH);
}
