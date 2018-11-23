#include <AT89X52.h>
#include <Intrins.h>
#include <math.h>
#include <2pt.h>

typedef unsigned int u16;
typedef unsigned char u8;


//引脚定义
#define		DATA	P1          //LCD1602数据端口
sbit 		  DQ =  P2^7;			  //DS18B20数据端口
sbit 		  RS =	P2^0;       //LCD1602 RS
sbit 		  RW =	P2^1;       //LCD1602 RW
sbit 		  E  =	P2^2;       //LCD1602 E
sbit      hall = P3^2;      //霍尔水流量计输出检测引脚
sbit      led  = P2^5;      //霍尔水流量计输出一次脉冲LED取反一次


//温度数据存储结构
typedef struct tagTempData
{
	u8 					btThird;			//百位数据					
	u8 					btSecond;			//十位数据
	u8 					btFirst;			//个位数据
	u8 					btDecimal;		//小数点后一位数据
	u8					btNegative;		//是否为负数		
}TEMPDATA;
TEMPDATA m_TempData;


//DS18B20序列号
const u8 code ROMData1[8] = {0x28, 0x70, 0x62, 0x77, 0x91, 0x0b, 0x02, 0x26};	//Uin
const u8 code ROMData2[8] = {0x28, 0xe8, 0x8d, 0x77, 0x91, 0x19, 0x02, 0xfe};	//Uout


/*************************************水流量计相关函数************************************/
//相关变量定义
static u16 i;               //定时器0计数用
static u16 count=0;         //数水流量计1s内的脉冲个数
static u16 Hz=1000;         //记录1s内的脉冲个数 即频率

static float velocity;      //水的流速
static float power;         //计算出来的功率


//外部中断0检测霍尔水流量计输出脉冲
void Int0Init()
{
	IT0=1;                    //下降沿触发
	EX0=1;
	EA=1;
}

//计数接收到的脉冲 每计入一个led闪烁一次
void Int0()	interrupt 0
{
	count++;
	if(hall==0)
	{
		led=~led; 
	}
}


//定时器0检测1s内 hall 输出脉冲个数
void Timer0Init()
{
	TMOD=0x01;
	TH0=(65536-1000)/256;
	TL0=(65536-1000)%256;
	ET0=1;
	EA=1;
	TR0=1;
}

//每1ms*1000=1s记录一次本秒内的输出脉冲数Hz
void Timer0() interrupt 1
{
	TH0=(65536-1000)/256;
	TL0=(65536-1000)%256;
	i++;
	if(i==1000)
		{
			i=0;
			Hz=count; 
			count=0;			
		}
}
/*************************************水流量计相关函数************************************/


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
void WriteByte(u8 btData)
{
	u8 i, btBuffer;
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
u8 ReadByte()
{
	u8 i, btDest;
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
void MatchROM(const u8 *pMatchData)
{
	u8 i;
	Initialization();
	WriteByte(MATCH_ROM);
	for (i = 0; i < 8; i++) WriteByte(*(pMatchData + i));	
}

//读取温度值
static TEMPDATA TempData1,TempData2;

TEMPDATA ReadTemperature()
{
	TEMPDATA TempData;
	u16 iTempDataH;
	u8 btDot, iTempDataL;
	static u8 i = 0;

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

	switch (i)
	{
		case 1 : MatchROM(ROMData1); break;			//匹配1
		case 2 : MatchROM(ROMData2); break;			//匹配2
	}
	WriteByte(READ_MEMORY);						  //读数据
	iTempDataL = ReadByte();
	iTempDataH = ReadByte();	
	iTempDataH <<= 8;
	iTempDataH |= iTempDataL;

	if (iTempDataH & 0x8000)
	{
		TempData.btNegative = 1;
		iTempDataH = ~iTempDataH + 1;		  //负数求补
	}

	//为了省去浮点运算带来的开销，而采用整数和小数部分分开处理的方法(没有四舍五入)
	btDot = (u8)(iTempDataH & 0x000F);	//得到小数部分
	iTempDataH >>= 4;							      //得到整数部分
	btDot *= 5; 									      //btDot*10/16得到转换后的小数数据
	btDot >>= 3;

	//数据处理
	TempData.btThird   = (u8)iTempDataH / 100;
	TempData.btSecond  = (u8)iTempDataH % 100 / 10;
	TempData.btFirst   = (u8)iTempDataH % 10;
	TempData.btDecimal = btDot;	
	
	switch(i)
	{
		case 1 : TempData1=TempData; break;			//匹配1
		case 2 : TempData2=TempData; break;			//匹配2
	}
	
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
void WriteCommand(u8 btCommand)
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
void WriteData(u8 btData)
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
void DisplayOne(bit bRow, u8 btColumn, u8 btData, bit bIsNumber)
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
void DisplayString(bit bRow, u8 btColumn, u8 *pData)
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


//速度显示函数
void DisplayVelocity()
{
	int v10=(int)(velocity*10);
	u8 u16str[5];
	u16str[0] =  v10/10000+0x30;
	u16str[1] = (v10/1000)%10+0x30;
	u16str[2] = (v10/100)%10+0x30;
	u16str[3] = (v10/10)%10+0x30;
	u16str[4] =  v10%10+0x30;
	
	DisplayOne(1, 5, u16str[2], 0);
  DisplayOne(1, 6, u16str[3], 0);
	DisplayOne(1, 8, u16str[4], 0);
}

//功率显示函数
void DisplayPower()
{
	int k;
	int p10=(int)(power*10);
	u8 u16str[5];
	u16str[0] =  p10/10000+0x30;
	u16str[1] = (p10/1000)%10+0x30;
	u16str[2] = (p10/100)%10+0x30;
	u16str[3] = (p10/10)%10+0x30;
	u16str[4] =  p10%10+0x30;
	
	for(k=0;k<4;k++)
	{
		DisplayOne(1, 10+k, u16str[k], 0);
	} 
	DisplayOne(1, 15, u16str[4], 0);
}
/****************************************LCD相关函数****************************************/


//数据处理子程序（获取并显示温度、流速、功率）
float float_TempData=0;
float deltaTemp=0;

void DataProcess()
{
	m_TempData = ReadTemperature();	
        DisplayOne(1, 0, m_TempData.btSecond, 1);
	DisplayOne(1, 1, m_TempData.btFirst, 1);
	DisplayOne(1, 3, m_TempData.btDecimal, 1);

	float_TempData= ((int)(m_TempData.btSecond-'0'))*10 + (int)(m_TempData.btFirst-'0') + ((int)(m_TempData.btDecimal-'0'))/10.0;
	
	
	deltaTemp= 1.0;  //浮点类型温度差
	
	velocity=Hz/24.0;
	DisplayVelocity();
	
	power =C_water * RO_water * velocity * deltaTemp / 60.0 ;  // W  J/(kg*C)  g/mL  L/min  Centigrade
	DisplayPower();
}


/*****************************main*****************************/
void main()
{
	Int0Init();
	Timer0Init();
	Clear();
	LCDInit();
	DisplayString(0, 0, "T/C  L/mi P/W");
	DisplayOne(1, 2, '.', 0);
	DisplayOne(1, 7, '.', 0);
	DisplayOne(1, 14, '.', 0);
	
	while (1)
	{
		DataProcess();
	}
}
/*****************************main*****************************/
