#include <AT89X52.h>
#include <Intrins.h>
#include <2pt.h>


//引脚定义
#define		DATA	P1          //LCD1602数据端口
sbit 		  DQ =  P2^7;			  //DS18B20数据端口
sbit 		  RS =	P2^0;
sbit 		  RW =	P2^1;
sbit 		  E  =	P2^2;


//温度数据存储结构
typedef struct tagTempData
{
	unsigned char 					btThird;							//百位数据					
	unsigned char 					btSecond;							//十位数据
	unsigned char 					btFirst;							//个位数据
	unsigned char 					btDecimal;						//小数点后一位数据
	unsigned char					  btNegative;						//是否为负数		
}TEMPDATA;
TEMPDATA m_TempData;


//DS18B20序列号

const unsigned char code ROMData1[8] = {0x28, 0x70, 0x62, 0x77, 0x91, 0x0b, 0x02, 0x26};	//Uin
const unsigned char code ROMData2[8] = {0x28, 0xe8, 0x8d, 0x77, 0x91, 0x19, 0x02, 0xfe};	//Uout

/*
const unsigned char code ROMData1[8] = {0x28, 0xff, 0xdd, 0x0f, 0x54, 0x17, 0x04, 0x66};	//U1
const unsigned char code ROMData2[8] = {0x28, 0xff, 0x0b, 0x0c, 0xa2, 0x16, 0x03, 0x37};	//U2
*/
/*
const unsigned char code ROMData3[8] = {0x28, 0x31, 0xC5, 0xB8, 0x00, 0x00, 0x00, 0xB9};	//U3
const unsigned char code ROMData4[8] = {0x28, 0x32, 0xC5, 0xB8, 0x00, 0x00, 0x00, 0xE0};	//U4
const unsigned char code ROMData5[8] = {0x28, 0x34, 0xC5, 0xB8, 0x00, 0x00, 0x00, 0x52};	//U5
const unsigned char code ROMData6[8] = {0x28, 0x35, 0xC5, 0xB8, 0x00, 0x00, 0x00, 0x65};	//U6
const unsigned char code ROMData7[8] = {0x28, 0x36, 0xC5, 0xB8, 0x00, 0x00, 0x00, 0x3C};	//U7
const unsigned char code ROMData8[8] = {0x28, 0x37, 0xC5, 0xB8, 0x00, 0x00, 0x00, 0x0B};	//U8
*/

/**************************************DS18B20相关函数**************************************/
//芯片初始化函数
void Initialization()
{
	while(1)
	{
		DQ = 0;
		Delay480us();
		DQ = 1;
		Delay60us();
		if(!DQ)  				    //收到ds18b20的应答信号
		{	
			DQ = 1;
			Delay240us();		  //延时240us
			break;		
		}
	}
}

//从低位开始写一个字节
void WriteByte(unsigned char btData)
{
	unsigned char i, btBuffer;
	
	for (i = 0; i < 8; i++)
	{
		btBuffer = btData >> i;
		if (btBuffer & 1)
		{
			DQ = 0;
			_nop_();
			_nop_();
			DQ = 1;
			Delay60us();
		}
		else
		{
			DQ = 0;
			Delay60us();
			DQ = 1;			
		}
	}
}

//从低位开始读一个字节
unsigned char ReadByte()
{
	unsigned char i, btDest;

	for (i = 0; i < 8; i++)
	{
		btDest >>= 1;
		DQ = 0;
		_nop_();
		_nop_();
		DQ = 1;
		Delay16us();
		if (DQ) btDest |= 0x80; 
		Delay60us();
	}

	return btDest;
}


//序列号匹配函数
void MatchROM(const unsigned char *pMatchData)
{
	unsigned char i;

	Initialization();
	WriteByte(MATCH_ROM);
	for (i = 0; i < 8; i++) WriteByte(*(pMatchData + i));	
}


static TEMPDATA TempData1,TempData2;

	
//读取温度值
TEMPDATA ReadTemperature()
{
	TEMPDATA TempData;
	unsigned int iTempDataH;
	unsigned char btDot, iTempDataL;
	static unsigned char i = 0;

	TempData.btNegative = 0;						//温度为正
	i++;
	if (i==3)                           //N个18B20这里条件就为i==N+1  
		i = 1;
	Initialization();
	WriteByte(SKIP_ROM);							  //跳过ROM匹配
	WriteByte(TEMP_SWITCH);							//启动转换
	Delay500ms();  									    //调用一次就行	
	Delay500ms(); 	 		
	Initialization();

	//多个芯片的时候用MatchROM(ROMData)换掉WriteByte(SKIP_ROM)
	switch (i)
	{
		case 1 : MatchROM(ROMData1); break;			//匹配1
		case 2 : MatchROM(ROMData2); break;			//匹配2
		/*
		case 3 : MatchROM(ROMData3); break;			//匹配3
		case 4 : MatchROM(ROMData4); break;			//匹配4	
		case 5 : MatchROM(ROMData5); break;			//匹配5
		case 6 : MatchROM(ROMData6); break;			//匹配6
		case 7 : MatchROM(ROMData7); break;			//匹配7
		case 8 : MatchROM(ROMData8); break;			//匹配8
		*/
	}
	//WriteByte(SKIP_ROM);							//跳过ROM匹配(单个芯片时用这句换掉上面的switch)
	WriteByte(READ_MEMORY);							//读数据
	iTempDataL = ReadByte();
	iTempDataH = ReadByte();	
	iTempDataH <<= 8;
	iTempDataH |= iTempDataL;

	if (iTempDataH & 0x8000)
	{
		TempData.btNegative = 1;
		iTempDataH = ~iTempDataH + 1;				//负数求补
	}

	//为了省去浮点运算带来的开销，而采用整数和小数部分分开处理的方法(没有四舍五入)
	btDot = (unsigned char)(iTempDataH & 0x000F);	//得到小数部分
	iTempDataH >>= 4;							//得到整数部分
	btDot *= 5; 									//btDot*10/16得到转换后的小数数据
	btDot >>= 3;

	//数据处理
	TempData.btThird   = (unsigned char)iTempDataH / 100;
	TempData.btSecond  = (unsigned char)iTempDataH % 100 / 10;
	TempData.btFirst   = (unsigned char)iTempDataH % 10;
	TempData.btDecimal = btDot;	
	
	switch(i)
	{
		case 1 : TempData1=TempData; break;			//匹配1
		case 2 : TempData2=TempData; break;			//匹配2
	}
	
	//考虑在这个地方将温度代入公式算出功率作为返回值
	
	
	
	return TempData;
}
/**************************************DS18B20相关函数**************************************/


/****************************************LCD相关函数****************************************/
//判断忙函数
void Busy()
{
	DATA = 0xff;
	RS = 0;
	RW = 1;
   	while(DATA & 0x80){
			E = 0;
   		E = 1;
   	}
   	E = 0;
}

//写指令函数
void WriteCommand(unsigned char btCommand)
{
	Busy();
	RS = 0;
	RW = 0;
	E = 1;
	DATA = btCommand;
	E = 0;
}

//芯片初始化函数
void LCDInit()
{
	WriteCommand(0x0c);	//开显示,无光标显示
	WriteCommand(0x06);	//文字不动，光标自动右移
	WriteCommand(0x38);	//设置显示模式:8位2行5x7点阵
}

//写数据函数
void WriteData(unsigned char btData)
{
	Busy();
	RS = 1;
	RW = 0;
	E = 1;
	DATA = btData;
	E = 0;
}

//清屏函数
void Clear(){
	WriteCommand(1);
}

//单个字符显示函数
void DisplayOne(bit bRow, unsigned char btColumn, unsigned char btData, bit bIsNumber)
{
	if (bRow) 		
		WriteCommand(0xc0 + btColumn);  //显示在第1行
	else 
		WriteCommand(0x80 + btColumn);  //显示在第0行

	if (bIsNumber)
		WriteData(btData + 0x30);
	else
		WriteData(btData);
}

//字符串显示函数
void DisplayString(bit bRow, unsigned char btColumn, unsigned char *pData)
{
	while (*pData != '\0')
   	{
   		if (bRow) 
				WriteCommand(0xc0 + btColumn);	//显示在第1行
   		else  	  
				WriteCommand(0x80 + btColumn);	//显示在第0行
		WriteData(*(pData++));			        //要显示的数据
		btColumn++;									        //列数加一
   	}
}

////无符号整型（u16）显示函数  也可传入int型
//void Display_u16(bit bRow, u8 btColumn, u16 x)
//{
//	int k;
//	u8 u16str[5];
//	u16str[0] =  x/10000+0x30;
//	u16str[1] = (x/1000)%10+0x30;
//	u16str[2] = (x/100)%10+0x30;
//	u16str[3] = (x/10)%10+0x30;
//	u16str[4] =  x%10+0x30;
//	
//	for(k=0;k<5;k++)
//	{
//		DisplayOne(bRow, btColumn+k, u16str[k], 0);
//	} 
//}
/****************************************LCD相关函数****************************************/




//数据处理子程序（获取温度并通过LCD显示）
void DataProcess()
{
	m_TempData = ReadTemperature();
	
	if (m_TempData.btNegative)
		DisplayOne(1, 6, '-', 0);
	else 
		DisplayOne(1, 6, m_TempData.btThird, 1);

	DisplayOne(1, 7, m_TempData.btSecond, 1);
	DisplayOne(1, 8, m_TempData.btFirst, 1);
	DisplayOne(1, 10, m_TempData.btDecimal, 1);
}



/*****************************main*****************************/
void main()
{
	Clear();
	LCDInit();
	DisplayString(0, 0, "  Temperature");
	DisplayOne(1, 9, '.', 0);
	
	while (1)
	{
		DataProcess();
	}
}
/*****************************main*****************************/
