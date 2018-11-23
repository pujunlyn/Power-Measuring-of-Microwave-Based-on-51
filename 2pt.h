#define		DATA	P1          //LCD1602数据端口

/************************************DS18B20 ROM操作命令************************************/
#define             SEARCH_ROM              0xF0                  //搜索ROM
#define             READ_ROM               	0x33                  //读取ROM 
#define             MATCH_ROM               0x55                  //匹配ROM
#define             SKIP_ROM               	0xCC                  //跳过ROM 
#define             ALARM_SEARCH            0xEC                  //告警搜索
/************************************DS18B20 ROM操作命令************************************/

/**************************************存储器操作命令**************************************/
#define             READ_POWER              0xB4                  //读18B20供电方式
#define             TEMP_SWITCH             0x44                  //启动温度变换 
#define             READ_MEMORY             0xBE                  //读暂存存储器
#define             WRITE_MEMORY            0x4E                  //写暂存存储器
#define             COPY_MEMORY             0x48                  //复制暂存存储器
#define             ANEW_MOVE             	0xB8                  //重新调出E^2PROM中的数据
/**************************************存储器操作命令***************************************/


/****************一系列延时函数****************/
void Delay16us()
{
  unsigned char a;
	for (a = 0; a < 4; a++);
}

void Delay60us()
{
	unsigned char a;
	for (a = 0; a < 18; a++);
}

void Delay480us()
{
	unsigned char a;
	for (a = 0; a < 158; a++);
}

void Delay240us()
{
	unsigned char a;
	for (a = 0; a < 78; a++);
}

void Delay500ms()
{
	unsigned char a, b, c;
	for (a = 0; a < 250; a++)
	for (b = 0; b < 3; b++)
	for (c = 0; c < 220; c++);
}
/****************一系列延时函数****************/