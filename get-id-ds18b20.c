/******************************************************************
基于51单片机读取DS18B20的序列号，用LCD1602显示

具体连接见引脚定义
******************************************************************/

#include <reg52.h>
#define uchar unsigned char
#define uint unsigned int
/********************************************************************/
//LCD引脚定义
#define DATA P1
sbit RS = P2^0;    
sbit RW = P2^1; 
sbit E = P2^2;
//DS18B20引脚定义
sbit DQ = P2^7;
/********************************************************************/
void  delay(uint z);                          
void  DS18B20_Reset(void);           //初始化DS18B20
bit   DS18B20_Readbit(void);         //读一位数据
uchar DS18B20_ReadByte(void);        //读一字节数据
void  DS18B20_WriteByte(uchar dat);  //写一字节数据
void  LCD_WriteCom(uchar com);       //LCD指令写入
void  LCD_WriteData(uchar dat);      //LCD数据写入     
void  LCD_Init();                    //LCD初始化
void  Display18B20Rom(char Rom);     //显示DS18B20序列号
/**********************************************/
/*     主函数                                */
/**********************************************/
void main()
{        
  uchar a,b,c,d,e,f,g,h;
  LCD_Init();
  RW = 0;
  DS18B20_Reset();
  delay(1);
  DS18B20_WriteByte(0x33);
  delay(1);
  a = DS18B20_ReadByte();
  b = DS18B20_ReadByte();
  c = DS18B20_ReadByte();
  d = DS18B20_ReadByte();
  e = DS18B20_ReadByte();
  f = DS18B20_ReadByte();
  g = DS18B20_ReadByte();
  h = DS18B20_ReadByte();
  LCD_WriteCom(0x80+0x40);
  Display18B20Rom(h);
  Display18B20Rom(g);
  Display18B20Rom(f);
  Display18B20Rom(e);
  Display18B20Rom(d);
  Display18B20Rom(c);
  Display18B20Rom(b);
  Display18B20Rom(a);
  while(1);
}
/**********************************************/



//上述函数具体定义
void delay(uint z)
{
	uint x,y;
	for( x = z; x > 0; x-- )
	for( y = 110; y > 0; y-- );
}

void DS18B20_Reset(void)
{
	uint i;
	DQ = 0;
	i = 103;
	while( i > 0 ) i--;
	DQ = 1;
	i = 4;
	while( i > 0 ) i--;
}

bit DS18B20_Readbit(void)
{
	uint i;
	bit dat;
	DQ = 0;
	i++;    
	DQ = 1;
	i++;
	i++;
	dat = DQ;
	i = 8;
	while( i > 0 )i--;
	return(dat);
}


uchar DS18B20_ReadByte(void) 
{
	uchar i,j,dat;
	dat = 0;
	for( i = 1; i <= 8; i++ )
	{
		j = DS18B20_Readbit();
		dat = ( j << 7 ) | ( dat >> 1 );
	}
	return(dat);
}

void DS18B20_WriteByte(uchar dat)
{
	uint i;
	uchar j;
	bit testb;
	for( j=1; j<=8; j++)
	{
		testb = dat&0x01;
		dat= dat>>1;
		if(testb)
			{
				DQ = 0;
				i++;i++;
				DQ = 1;
				i = 8;
				while(i>0)i--;
			}
			else
				{
					DQ = 0;
					i = 8;while(i>0)i--;
					DQ = 1;
					i++;i++;
				}
	}
} 

void LCD_WriteCom(uchar com)
{
	RS = 0;
	RW = 0;
	DATA = com;
	delay(5);
	E = 0;
	delay(5);
	E = 1;
	delay(5);
	E = 0;
}

void LCD_WriteData(uchar dat)
{
	RS = 1;
	RW = 0;
	E = 0;
	DATA = dat;
	delay(5);                           
	E = 1;           
	delay(5);
	E = 0;
	delay(5);
}

void LCD_Init()
{
	LCD_WriteCom(0x38);
	delay(15);
	LCD_WriteCom(0x08);
	delay(3);
	LCD_WriteCom(0x01);
	delay(3);
	LCD_WriteCom(0x06);
	delay(3);
	LCD_WriteCom(0x0c);
}

void Display18B20Rom(char Rom)
{
	uchar h,l;
	l = Rom & 0x0f;
	h = Rom & 0xf0;
	h >>= 4;
	if( ( h >= 0x00 )&&( h <= 0x09 ) )
		LCD_WriteData(h+0x30);
	else  
		LCD_WriteData(h+0x37); 
	if( ( l >= 0x00 )&&( l <= 0x09 ) )
		LCD_WriteData(l+0x30); 
	else  
		LCD_WriteData(l+0x37);
} 
