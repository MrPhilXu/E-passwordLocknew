#include <REG52.h>
#include<intrins.h>
#define LCM_Data  P0
#define uchar unsigned char 
#define uint  unsigned int

sbit lcd1602_rs=P2^7;
//sbit lcd1602_rw=P2^6;
sbit lcd1602_en=P2^6;

sbit Scl=P2^0;			//24C02串行时钟
sbit Sda=P2^1;			//24C02串行数据

sbit ALAM = P3^6;		//报警	
sbit KEY = P3^2;		//开锁


bit  operation=0;		//操作标志位
bit  pass=0;			//密码正确标志
bit  ReInputEn=0;		//重置输入充许标志	
bit  s3_keydown=0;		//3秒按键标志位
bit  key_disable=0;		//锁定键盘标志

unsigned char countt0,second;	//t0中断计数器,秒计数器

//解码变量
unsigned char Im[4]={0x00,0x00,0x00,0x00};


//全局变量
uchar f;
unsigned long m,Tc;
unsigned char IrOK;


unsigned char code a[]={0xFE,0xFD,0xFB,0xF7}; 											//控盘扫描控制表

unsigned char code start_line[]	= {"password:       "};
unsigned char code name[] 	 	= {"===Coded Lock==="};												//显示名称
unsigned char code Correct[] 	= {"     correct    "};			 								//输入正确
unsigned char code Error[]   	= {"      error     "};  											//输入错误
unsigned char code codepass[]	= {"      pass      "}; 
unsigned char code LockOpen[]	= {"      open      "};												//OPEN
unsigned char code SetNew[] 	= {"SetNewWordEnable"};
unsigned char code Input[]   	= {"input:          "};												//INPUT
unsigned char code ResetOK[] 	= {"ResetPasswordOK "};
unsigned char code initword[]	= {"Init password..."};
unsigned char code Er_try[]		= {"error,try again!"};
unsigned char code again[]		= {"input again     "};

unsigned char InputData[6];																//输入密码暂存区
unsigned char CurrentPassword[6]={1,3,1,4,2,0}; 														//当前密码值
unsigned char TempPassword[6];
unsigned char N=0;				//密码输入位数记数
unsigned char ErrorCont;			//错误次数计数
unsigned char CorrectCont;			//正确输入计数
unsigned char ReInputCont; 			//重新输入计数
unsigned char code initpassword[6]={0,0,0,0,0,0};


//=====================5ms延时==============================
void Delay5Ms()
{
	unsigned int TempCyc = 5552;
	while(TempCyc--);
}	

//===================400ms延时==============================
void Delay400Ms()
{
 unsigned char TempCycA = 5;
 unsigned int TempCycB;
 while(TempCycA--)
 {
  TempCycB=7269;
  while(TempCycB--);
 }
}

//=============================================================================================
//================================24C02========================================================
//=============================================================================================

void mDelay(uint t) //延时
{ 
	uchar i;
   	while(t--)
   	{
   		for(i=0;i<125;i++)
   		{;}
   	}
}
   

void Nop()		  //空操作
{
 	_nop_();
 	_nop_();
 	_nop_();
 	_nop_();
}


/*起始条件*/

void Start(void)
{
 	Sda=1;
 	Scl=1;
 	Nop();
 	Sda=0;
 	Nop();
}


 /*停止条件*/
void Stop(void)
{
 	Sda=0;
 	Scl=1;
 	Nop();
 	Sda=1;
 	Nop();
}

/*应答位*/
void Ack(void)
{
	Sda=0;
	Nop();
	Scl=1;
	Nop();
	Scl=0;
}

/*反向应答位*/
void NoAck(void)
{
 	Sda=1;
 	Nop();
 	Scl=1;
 	Nop();
 	Scl=0;
}	

 /*发送数据子程序，Data为要求发送的数据*/
void Send(uchar Data)
{
   	uchar BitCounter=8;
   	uchar temp;
   	do
   	{
   		temp=Data;
   		Scl=0;
   		Nop();
   		if((temp&0x80)==0x80)
   		Sda=1;
   		else 
   		Sda=0;
   		Scl=1;
   		temp=Data<<1;
   		Data=temp;
   		BitCounter--;
   	}
   	while(BitCounter);
   	Scl=0;
}

/*读一字节的数据，并返回该字节值*/
uchar Read()
{
    uchar temp=0;
	uchar temp1=0;
	uchar BitCounter=8;
	Sda=1;
	do{
	Scl=0;
	Nop();
	Scl=1;
	Nop();
	if(Sda)
	temp=temp|0x01;
	else
	temp=temp&0xfe;
	if(BitCounter-1)
	{
	temp1=temp<<1;
	temp=temp1;
	}
	BitCounter--;
	}
	while(BitCounter);
	return(temp);
	}

void WrToROM(uchar Data[],uchar Address,uchar Num)
{
  uchar i;
  uchar *PData;
  PData=Data;
  for(i=0;i<Num;i++)
  {
  Start();
  Send(0xa0);
  Ack();
  Send(Address+i);
  Ack();
  Send(*(PData+i));
  Ack();
  Stop();
  mDelay(20);
  }
}

void RdFromROM(uchar Data[],uchar Address,uchar Num)
{
  uchar i;
  uchar *PData;
  PData=Data;
  for(i=0;i<Num;i++)
  {
  Start();
  Send(0xa0);
  Ack();
  Send(Address+i);
  Ack();
  Start();
  Send(0xa1);
  Ack();
  *(PData+i)=Read();
  Scl=0;
  NoAck();
  Stop();
  }
}


//==================================================================================================
//=======================================LCD1602====================================================
//==================================================================================================

#define yi 0x80 //LCD第一行的初始位置,因为LCD1602字符地址首位D7恒定为1（100000000=80）
#define er 0x80+0x40 //LCD第二行初始位置（因为第二行第一个字符位置地址是0x40）


//----------------延时函数，后面经常调用----------------------
void delay(uint xms)//延时函数，有参函数
{
	uint x,y;
	for(x=xms;x>0;x--)
	 for(y=110;y>0;y--);
}

//--------------------------写指令---------------------------
void write_1602com(uchar com)//****液晶写入指令函数****
{
	lcd1602_rs=0;//数据/指令选择置为指令
//	lcd1602_rw=0; //读写选择置为写
	P0=com;//送入数据
	delay(1);
	lcd1602_en=1;//拉高使能端，为制造有效的下降沿做准备
	delay(1);
	lcd1602_en=0;//en由高变低，产生下降沿，液晶执行命令
}

//-------------------------写数据-----------------------------
void write_1602dat(uchar dat)//***液晶写入数据函数****
{
	lcd1602_rs=1;//数据/指令选择置为数据
//	lcd1602_rw=0; //读写选择置为写
	P0=dat;//送入数据
	delay(1);
	lcd1602_en=1; //en置高电平，为制造下降沿做准备
	delay(1);
	lcd1602_en=0; //en由高变低，产生下降沿，液晶执行命令
}

//-------------------------初始化-------------------------
void lcd_init()
{
	write_1602com(0x38);//设置液晶工作模式，意思：16*2行显示，5*7点阵，8位数据
	write_1602com(0x0c);//开显示不显示光标
	write_1602com(0x06);//整屏不移动，光标自动右移
	write_1602com(0x01);//清显示
}
//========================================================================================
//=========================================================================================




//==============将按键值编码为数值=========================
unsigned char coding(unsigned char hh)	 
{
	unsigned char k;

		switch(hh)
		{
			case (0): k=1;break;
			case (1): k=2;break;
			case (2): k=3;break;
			case (3): k='A';break;
			case (4): k=4;break;
			case (5): k=5;break;
			case (6): k=6;break;
			case (7): k='B';break;
			case (8): k=7;break;
			case (9): k=8;break;
			case (10): k=9;break;
			case (11): k='C';break;
			case (12): k='*';break;
			case (13): k=0;break;
			case (14): k='#';break;
			case (15): k='D';break;
		}


	return(k);
}




unsigned char KeyMemory;

//按键行定义
sbit KeyLine_1	=	P1^7;
sbit KeyLine_2	=	P1^5;
sbit KeyLine_3	=	P1^3;
sbit KeyLine_4	=	P1^1;
//按键列定义        
sbit Keylist_1	=	P1^0;	
sbit Keylist_2	=	P1^2;	
sbit Keylist_3	=	P1^4;	
sbit Keylist_4	=	P1^6;	
void KeyOut(unsigned char i)
{
	KeyLine_1 = 1;
	KeyLine_2 = 1;
	KeyLine_3 = 1;
	KeyLine_4 = 1;
	switch(i)
	{
		case 0: KeyLine_1 = 0; break;
		case 1: KeyLine_2 = 0; break;
		case 2: KeyLine_3 = 0; break;
		case 3: KeyLine_4 = 0; break;
	}	
}

unsigned char KeyIn(unsigned char i)
{
	static unsigned char a=1;
	switch(i)
	{
		case 0: a=Keylist_1; break;
		case 1: a=Keylist_2; break;
		case 2: a=Keylist_3; break;
		case 3: a=Keylist_4; break;
	}	
	return a;
}
//========================================================================
// 函数: u8 update_key(void)													
// 应用: key=u8 update_key();                         
// 描述: 扫描并返回按下的按键值,                       	
// 参数:                                              
// 返回: 按下的键值,0-15;无按下返回0xff;               
// 版本: VER1.0                                       
// 日期: 2013-4-1                                     
// 备注: 
//========================================================================
unsigned char keynum(void)
{
	unsigned char key_rt=0xff;//按键返回值
	unsigned char i, j;
	for(i = 0; i < 4; i++)             //i是输出口，依次置高电平
	{
		KeyOut(i);
		for(j = 0; j < 4; j++)            //j是输入口，当键按下时导通被置为高电平  //在1路输出高电平的时候输入扫描,并储存
		{
			if(KeyIn(j) == 0)		//如果有按键按下
			{
				Delay5Ms();
				Delay5Ms();
				if(KeyIn(j) == 0)		//如果有按键按下
				{
					KeyMemory=(1<<i)*16+(1<<j);
				}
			}
			while(KeyIn(j) == 0);	//松手检测,假如有按键按下则等待,没有按下或松开则通过
		}
	}
	for(i = 0; i < 4; i++)         
	{
		if((KeyMemory/16)>>i==0x01)
		{
			for(j = 0; j < 4; j++)           
			{	
				if((KeyMemory%16)>>j==0x01)
				{
					KeyMemory=0;
					key_rt=i*4+j;
				}
			}
		}
	}
	return key_rt;
}



//=======================一声提示音，表示有效输入========================
void OneAlam()
{
	ALAM=0;
	Delay5Ms();
    ALAM=1;
}

//========================二声提示音，表示操作成功========================
void TwoAlam()
{
	ALAM=0;
	Delay5Ms();
    ALAM=1;
    Delay5Ms();
	ALAM=0;
	Delay5Ms();
    ALAM=1;
}

//========================三声提示音,表示错误========================
void ThreeAlam()
{
	ALAM=0;
	Delay5Ms();
    ALAM=1;
    Delay5Ms();
	ALAM=0;
	Delay5Ms();
    ALAM=1;
    Delay5Ms();
	ALAM=0;
	Delay5Ms();
    ALAM=1;

}


//=======================显示提示输入=========================
void DisplayChar()
{
	unsigned char i;
	if(pass==1)
	{
		write_1602com(er);
		for(i=0;i<16;i++)
		{
			write_1602dat(LockOpen[i]);	
		}
	}
	else
	{
		if(N==0)
		{
			write_1602com(er);
			for(i=0;i<16;i++)
			{
				write_1602dat(Error[i]);	
			}
		}
		else
		{
			write_1602com(er);
			for(i=0;i<16;i++)
			{
				write_1602dat(start_line[i]);	
			}
		}
	}
}


//========================重置密码==================================================
//==================================================================================
void ResetPassword()
{
	unsigned char i;	
	unsigned char j;
	if(pass==0)
	{
		pass=0;
		DisplayChar();
		ThreeAlam();
	}
	else
	{
    	if(ReInputEn==1)
		{
			if(N==6)
			{
				ReInputCont++;				
				if(ReInputCont==2)
				{
					for(i=0;i<6;)
					{
						if(TempPassword[i]==InputData[i])	//将两次输入的新密码作对比
							i++;
						else
						{
							write_1602com(er);
							for(j=0;j<16;j++)
							{
								write_1602dat(Error[j]);	
							}
							ThreeAlam();			//错误提示	
							pass=0;
							ReInputEn=0;			//关闭重置功能，
							ReInputCont=0;
							DisplayChar();
							break;
						}
					} 
					if(i==6)
					{
						write_1602com(er);
						for(j=0;j<16;j++)
						{
							write_1602dat(ResetOK[j]);	
						}

						TwoAlam();				//操作成功提示
					 	WrToROM(TempPassword,0,6);		//将新密码写入24C02存储
						ReInputEn=0;
					}
					ReInputCont=0;
					CorrectCont=0;
				}
				else
				{
					OneAlam();
					write_1602com(er);
					for(j=0;j<16;j++)
					{
						write_1602dat(again[j]);	
					}					
					for(i=0;i<6;i++)
					{
						TempPassword[i]=InputData[i];		//将第一次输入的数据暂存起来						
					}
				}

			N=0;						//输入数据位数计数器清零
		   }
	    }
	}

}



//=======================输入密码错误超过三过，报警并锁死键盘======================
void Alam_KeyUnable()
{
	P1=0x00;
	{
		ALAM=~ALAM;
		Delay5Ms();
	}
}


//=======================取消所有操作============================================
void Cancel()
{	
	unsigned char i;
	unsigned char j;
	write_1602com(er);
	for(j=0;j<16;j++)
	{
		write_1602dat(start_line[j]);	
	}
	TwoAlam();				//提示音
	for(i=0;i<6;i++)
	{
		InputData[i]=0;
	}
	KEY=1;					//关闭锁
	ALAM=1;					//报警关
	operation=0;			//操作标志位清零
	pass=0;					//密码正确标志清零
	ReInputEn=0;			//重置输入充许标志清零
	CorrectCont=0;			//密码正确输入次数清零
	ReInputCont=0;			//重置密码输入次数清零 
	s3_keydown=0;
	key_disable=0;
	N=0;					//输入位数计数器清零
}


//==========================确认键，并通过相应标志位执行相应功能===============================
void Ensure()
{	
	unsigned char i,j;
	RdFromROM(CurrentPassword,0,6); 					//从24C02里读出存储密码
    if(N==6)
	{
	    if(ReInputEn==0)							//重置密码功能未开启
		{
			for(i=0;i<6;)
   			{					
				if(CurrentPassword[i]==InputData[i])
				{
					i++;
				}
				else 
				{
					i=7;			
					ErrorCont++;
					if(ErrorCont>=3&&KEY==1)			//错误输入计数达三次时，报警并锁定键盘
					{
						write_1602com(er);
						for(i=0;i<16;i++)
						{
							write_1602dat(Error[i]);	
						}
						Alam_KeyUnable();
						TR0=1;				//开启定时
						key_disable=1;			//锁定键盘
						pass=0;
						break;	
					}
				}  
			}

			if(i==6)
			{
				CorrectCont++;
				if(CorrectCont==1)				//正确输入计数，当只有一次正确输入时，开锁，
				{
					write_1602com(er);
					for(j=0;j<16;j++)
					{
						write_1602dat(LockOpen[j]);	
					}
					TwoAlam();			//操作成功提示音
					ErrorCont=0;
					KEY=0;											//开锁
					pass=1;											//置正确标志位
					TR0=1;											//开启定时
					for(j=0;j<6;j++)								//将输入清除
					{
						InputData[i]=0;
					}
				}	
				else												//当两次正确输入时，开启重置密码功能
				{
					write_1602com(er);
					for(j=0;j<16;j++)
					{
						write_1602dat(SetNew[j]);	
					}
					TwoAlam();									    //操作成功提示
					ReInputEn=1;									//允许重置密码输入
					CorrectCont=0;									//正确计数器清零
				}
	  		}
	
			else			//=========================当第一次使用或忘记密码时可以用131420对其密码初始化============
			{
				if((InputData[0]==1)&&(InputData[1]==3)&&(InputData[2]==1)&&(InputData[3]==4)&&(InputData[4]==2)&&(InputData[5]==0))
		  	 	{
					WrToROM(initpassword,0,6); 				//强制将初始密码写入24C02存储
					write_1602com(er);
					for(j=0;j<16;j++)
					{
						write_1602dat(initword[j]);	
					}
					TwoAlam();
					Delay400Ms();
					TwoAlam();
					N=0;
				}
				else
				{
					write_1602com(er);
					for(j=0;j<16;j++)
					{
						write_1602dat(Error[j]);	
					}
 					ThreeAlam();										//错误提示音
					pass=0;	
				}
			}
		}

		else											//当已经开启重置密码功能时，而按下开锁键，
		{
			write_1602com(er);
			for(j=0;j<16;j++)
			{
				write_1602dat(Er_try[j]);	
			}
			ThreeAlam();
		}
	}

	else
	{
		write_1602com(er);
		for(j=0;j<16;j++)
		{
			write_1602dat(Error[j]);	
		}

 		ThreeAlam();										//错误提示音
		pass=0;	
	}
	
	N=0;													//将输入数据计数器清零，为下一次输入作准备

	operation=1;
}


//==============================主函数===============================
void main()
{
 	unsigned char KEY,NUM;
	unsigned char i,j;
 	P1=0xFF; 
	EA=1;
	TMOD=0x11;
	IT1=1;//下降沿有效
	EX1=1;//外部中断1开
	   
	TH0=0;//T0赋初值
	TL0=0;
	TR0=0;//t0开始计时
 	TL1=0xB0;
 	TH1=0x3C;
 	ET1=1;	
 	TR1=0;
 	Delay400Ms(); 	//启动等待，等LCM讲入工作状态
 	lcd_init(); 	//LCD初始化
	write_1602com(yi);//日历显示固定符号从第一行第0个位置之后开始显示
	for(i=0;i<16;i++)
	{
		write_1602dat(name[i]);//向液晶屏写日历显示的固定符号部分
	}
	write_1602com(er);//时间显示固定符号写入位置，从第2个位置后开始显示
	for(i=0;i<16;i++)
	{
		write_1602dat(start_line[i]);//写显示时间固定符号，两个冒号
	}
	write_1602com(er+9);	//设置光标位置
	write_1602com(0x0f);	//设置光标为闪烁
 	Delay5Ms(); //延时片刻(可不要)

 	N=0;														//初始化数据输入位数
 	while(1)
 	{	

		if(key_disable==1)
			Alam_KeyUnable();
		else
			ALAM=1;								//关报警

		KEY=keynum();
		if(KEY!=0xff||IrOK==1)
		{	

			if(key_disable==1)
			{
				second=0;
			}
			else
			{
				NUM=coding(KEY);
				{
					switch(NUM)
					{
						case ('A'): 	; 					break;
						case ('B'):		;     				break;
						case ('C'): 	; 					break;
						case ('D'): ResetPassword();		break;      //重新设置密码
						case ('*'): Cancel();				break;      //取消当前输入
						case ('#'): Ensure(); 				break;   	//确认键，
						default: 
						{	
							write_1602com(er);
							for(i=0;i<16;i++)
							{
								write_1602dat(Input[i]);
							}
						    operation=0;
							if(N<6)                   					//当输入的密码少于6位时，接受输入并保存，大于6位时则无效。
							{  
								OneAlam();								//按键提示音						
						 		for(j=0;j<=N;j++)
								{
									write_1602com(er+6+j);
									write_1602dat('*');
								}
								InputData[N]=NUM;
								N++;
							}
							else										//输入数据位数大于6后，忽略输入
							{
								N=6;
						 		break;
							}
						}
					}
				}
			}
	 	} 
	}
}

