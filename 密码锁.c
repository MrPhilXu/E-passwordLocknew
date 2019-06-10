#include <REG52.h>
#include<intrins.h>
#define LCM_Data  P0
#define uchar unsigned char 
#define uint  unsigned int

sbit lcd1602_rs=P2^7;
//sbit lcd1602_rw=P2^6;
sbit lcd1602_en=P2^6;

sbit Scl=P2^0;			//24C02����ʱ��
sbit Sda=P2^1;			//24C02��������

sbit ALAM = P3^6;		//����	
sbit KEY = P3^2;		//����


bit  operation=0;		//������־λ
bit  pass=0;			//������ȷ��־
bit  ReInputEn=0;		//������������־	
bit  s3_keydown=0;		//3�밴����־λ
bit  key_disable=0;		//�������̱�־

unsigned char countt0,second;	//t0�жϼ�����,�������

//�������
unsigned char Im[4]={0x00,0x00,0x00,0x00};


//ȫ�ֱ���
uchar f;
unsigned long m,Tc;
unsigned char IrOK;


unsigned char code a[]={0xFE,0xFD,0xFB,0xF7}; 											//����ɨ����Ʊ�

unsigned char code start_line[]	= {"password:       "};
unsigned char code name[] 	 	= {"===Coded Lock==="};												//��ʾ����
unsigned char code Correct[] 	= {"     correct    "};			 								//������ȷ
unsigned char code Error[]   	= {"      error     "};  											//�������
unsigned char code codepass[]	= {"      pass      "}; 
unsigned char code LockOpen[]	= {"      open      "};												//OPEN
unsigned char code SetNew[] 	= {"SetNewWordEnable"};
unsigned char code Input[]   	= {"input:          "};												//INPUT
unsigned char code ResetOK[] 	= {"ResetPasswordOK "};
unsigned char code initword[]	= {"Init password..."};
unsigned char code Er_try[]		= {"error,try again!"};
unsigned char code again[]		= {"input again     "};

unsigned char InputData[6];																//���������ݴ���
unsigned char CurrentPassword[6]={1,3,1,4,2,0}; 														//��ǰ����ֵ
unsigned char TempPassword[6];
unsigned char N=0;				//��������λ������
unsigned char ErrorCont;			//�����������
unsigned char CorrectCont;			//��ȷ�������
unsigned char ReInputCont; 			//�����������
unsigned char code initpassword[6]={0,0,0,0,0,0};


//=====================5ms��ʱ==============================
void Delay5Ms()
{
	unsigned int TempCyc = 5552;
	while(TempCyc--);
}	

//===================400ms��ʱ==============================
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

void mDelay(uint t) //��ʱ
{ 
	uchar i;
   	while(t--)
   	{
   		for(i=0;i<125;i++)
   		{;}
   	}
}
   

void Nop()		  //�ղ���
{
 	_nop_();
 	_nop_();
 	_nop_();
 	_nop_();
}


/*��ʼ����*/

void Start(void)
{
 	Sda=1;
 	Scl=1;
 	Nop();
 	Sda=0;
 	Nop();
}


 /*ֹͣ����*/
void Stop(void)
{
 	Sda=0;
 	Scl=1;
 	Nop();
 	Sda=1;
 	Nop();
}

/*Ӧ��λ*/
void Ack(void)
{
	Sda=0;
	Nop();
	Scl=1;
	Nop();
	Scl=0;
}

/*����Ӧ��λ*/
void NoAck(void)
{
 	Sda=1;
 	Nop();
 	Scl=1;
 	Nop();
 	Scl=0;
}	

 /*���������ӳ���DataΪҪ���͵�����*/
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

/*��һ�ֽڵ����ݣ������ظ��ֽ�ֵ*/
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

#define yi 0x80 //LCD��һ�еĳ�ʼλ��,��ΪLCD1602�ַ���ַ��λD7�㶨Ϊ1��100000000=80��
#define er 0x80+0x40 //LCD�ڶ��г�ʼλ�ã���Ϊ�ڶ��е�һ���ַ�λ�õ�ַ��0x40��


//----------------��ʱ���������澭������----------------------
void delay(uint xms)//��ʱ�������вκ���
{
	uint x,y;
	for(x=xms;x>0;x--)
	 for(y=110;y>0;y--);
}

//--------------------------дָ��---------------------------
void write_1602com(uchar com)//****Һ��д��ָ���****
{
	lcd1602_rs=0;//����/ָ��ѡ����Ϊָ��
//	lcd1602_rw=0; //��дѡ����Ϊд
	P0=com;//��������
	delay(1);
	lcd1602_en=1;//����ʹ�ܶˣ�Ϊ������Ч���½�����׼��
	delay(1);
	lcd1602_en=0;//en�ɸ߱�ͣ������½��أ�Һ��ִ������
}

//-------------------------д����-----------------------------
void write_1602dat(uchar dat)//***Һ��д�����ݺ���****
{
	lcd1602_rs=1;//����/ָ��ѡ����Ϊ����
//	lcd1602_rw=0; //��дѡ����Ϊд
	P0=dat;//��������
	delay(1);
	lcd1602_en=1; //en�øߵ�ƽ��Ϊ�����½�����׼��
	delay(1);
	lcd1602_en=0; //en�ɸ߱�ͣ������½��أ�Һ��ִ������
}

//-------------------------��ʼ��-------------------------
void lcd_init()
{
	write_1602com(0x38);//����Һ������ģʽ����˼��16*2����ʾ��5*7����8λ����
	write_1602com(0x0c);//����ʾ����ʾ���
	write_1602com(0x06);//�������ƶ�������Զ�����
	write_1602com(0x01);//����ʾ
}
//========================================================================================
//=========================================================================================




//==============������ֵ����Ϊ��ֵ=========================
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

//�����ж���
sbit KeyLine_1	=	P1^7;
sbit KeyLine_2	=	P1^5;
sbit KeyLine_3	=	P1^3;
sbit KeyLine_4	=	P1^1;
//�����ж���        
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
// ����: u8 update_key(void)													
// Ӧ��: key=u8 update_key();                         
// ����: ɨ�貢���ذ��µİ���ֵ,                       	
// ����:                                              
// ����: ���µļ�ֵ,0-15;�ް��·���0xff;               
// �汾: VER1.0                                       
// ����: 2013-4-1                                     
// ��ע: 
//========================================================================
unsigned char keynum(void)
{
	unsigned char key_rt=0xff;//��������ֵ
	unsigned char i, j;
	for(i = 0; i < 4; i++)             //i������ڣ������øߵ�ƽ
	{
		KeyOut(i);
		for(j = 0; j < 4; j++)            //j������ڣ���������ʱ��ͨ����Ϊ�ߵ�ƽ  //��1·����ߵ�ƽ��ʱ������ɨ��,������
		{
			if(KeyIn(j) == 0)		//����а�������
			{
				Delay5Ms();
				Delay5Ms();
				if(KeyIn(j) == 0)		//����а�������
				{
					KeyMemory=(1<<i)*16+(1<<j);
				}
			}
			while(KeyIn(j) == 0);	//���ּ��,�����а���������ȴ�,û�а��»��ɿ���ͨ��
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



//=======================һ����ʾ������ʾ��Ч����========================
void OneAlam()
{
	ALAM=0;
	Delay5Ms();
    ALAM=1;
}

//========================������ʾ������ʾ�����ɹ�========================
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

//========================������ʾ��,��ʾ����========================
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


//=======================��ʾ��ʾ����=========================
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


//========================��������==================================================
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
						if(TempPassword[i]==InputData[i])	//��������������������Ա�
							i++;
						else
						{
							write_1602com(er);
							for(j=0;j<16;j++)
							{
								write_1602dat(Error[j]);	
							}
							ThreeAlam();			//������ʾ	
							pass=0;
							ReInputEn=0;			//�ر����ù��ܣ�
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

						TwoAlam();				//�����ɹ���ʾ
					 	WrToROM(TempPassword,0,6);		//��������д��24C02�洢
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
						TempPassword[i]=InputData[i];		//����һ������������ݴ�����						
					}
				}

			N=0;						//��������λ������������
		   }
	    }
	}

}



//=======================����������󳬹���������������������======================
void Alam_KeyUnable()
{
	P1=0x00;
	{
		ALAM=~ALAM;
		Delay5Ms();
	}
}


//=======================ȡ�����в���============================================
void Cancel()
{	
	unsigned char i;
	unsigned char j;
	write_1602com(er);
	for(j=0;j<16;j++)
	{
		write_1602dat(start_line[j]);	
	}
	TwoAlam();				//��ʾ��
	for(i=0;i<6;i++)
	{
		InputData[i]=0;
	}
	KEY=1;					//�ر���
	ALAM=1;					//������
	operation=0;			//������־λ����
	pass=0;					//������ȷ��־����
	ReInputEn=0;			//������������־����
	CorrectCont=0;			//������ȷ�����������
	ReInputCont=0;			//������������������� 
	s3_keydown=0;
	key_disable=0;
	N=0;					//����λ������������
}


//==========================ȷ�ϼ�����ͨ����Ӧ��־λִ����Ӧ����===============================
void Ensure()
{	
	unsigned char i,j;
	RdFromROM(CurrentPassword,0,6); 					//��24C02������洢����
    if(N==6)
	{
	    if(ReInputEn==0)							//�������빦��δ����
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
					if(ErrorCont>=3&&KEY==1)			//�����������������ʱ����������������
					{
						write_1602com(er);
						for(i=0;i<16;i++)
						{
							write_1602dat(Error[i]);	
						}
						Alam_KeyUnable();
						TR0=1;				//������ʱ
						key_disable=1;			//��������
						pass=0;
						break;	
					}
				}  
			}

			if(i==6)
			{
				CorrectCont++;
				if(CorrectCont==1)				//��ȷ�����������ֻ��һ����ȷ����ʱ��������
				{
					write_1602com(er);
					for(j=0;j<16;j++)
					{
						write_1602dat(LockOpen[j]);	
					}
					TwoAlam();			//�����ɹ���ʾ��
					ErrorCont=0;
					KEY=0;											//����
					pass=1;											//����ȷ��־λ
					TR0=1;											//������ʱ
					for(j=0;j<6;j++)								//���������
					{
						InputData[i]=0;
					}
				}	
				else												//��������ȷ����ʱ�������������빦��
				{
					write_1602com(er);
					for(j=0;j<16;j++)
					{
						write_1602dat(SetNew[j]);	
					}
					TwoAlam();									    //�����ɹ���ʾ
					ReInputEn=1;									//����������������
					CorrectCont=0;									//��ȷ����������
				}
	  		}
	
			else			//=========================����һ��ʹ�û���������ʱ������131420���������ʼ��============
			{
				if((InputData[0]==1)&&(InputData[1]==3)&&(InputData[2]==1)&&(InputData[3]==4)&&(InputData[4]==2)&&(InputData[5]==0))
		  	 	{
					WrToROM(initpassword,0,6); 				//ǿ�ƽ���ʼ����д��24C02�洢
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
 					ThreeAlam();										//������ʾ��
					pass=0;	
				}
			}
		}

		else											//���Ѿ������������빦��ʱ�������¿�������
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

 		ThreeAlam();										//������ʾ��
		pass=0;	
	}
	
	N=0;													//���������ݼ��������㣬Ϊ��һ��������׼��

	operation=1;
}


//==============================������===============================
void main()
{
 	unsigned char KEY,NUM;
	unsigned char i,j;
 	P1=0xFF; 
	EA=1;
	TMOD=0x11;
	IT1=1;//�½�����Ч
	EX1=1;//�ⲿ�ж�1��
	   
	TH0=0;//T0����ֵ
	TL0=0;
	TR0=0;//t0��ʼ��ʱ
 	TL1=0xB0;
 	TH1=0x3C;
 	ET1=1;	
 	TR1=0;
 	Delay400Ms(); 	//�����ȴ�����LCM���빤��״̬
 	lcd_init(); 	//LCD��ʼ��
	write_1602com(yi);//������ʾ�̶����Ŵӵ�һ�е�0��λ��֮��ʼ��ʾ
	for(i=0;i<16;i++)
	{
		write_1602dat(name[i]);//��Һ����д������ʾ�Ĺ̶����Ų���
	}
	write_1602com(er);//ʱ����ʾ�̶�����д��λ�ã��ӵ�2��λ�ú�ʼ��ʾ
	for(i=0;i<16;i++)
	{
		write_1602dat(start_line[i]);//д��ʾʱ��̶����ţ�����ð��
	}
	write_1602com(er+9);	//���ù��λ��
	write_1602com(0x0f);	//���ù��Ϊ��˸
 	Delay5Ms(); //��ʱƬ��(�ɲ�Ҫ)

 	N=0;														//��ʼ����������λ��
 	while(1)
 	{	

		if(key_disable==1)
			Alam_KeyUnable();
		else
			ALAM=1;								//�ر���

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
						case ('D'): ResetPassword();		break;      //������������
						case ('*'): Cancel();				break;      //ȡ����ǰ����
						case ('#'): Ensure(); 				break;   	//ȷ�ϼ���
						default: 
						{	
							write_1602com(er);
							for(i=0;i<16;i++)
							{
								write_1602dat(Input[i]);
							}
						    operation=0;
							if(N<6)                   					//���������������6λʱ���������벢���棬����6λʱ����Ч��
							{  
								OneAlam();								//������ʾ��						
						 		for(j=0;j<=N;j++)
								{
									write_1602com(er+6+j);
									write_1602dat('*');
								}
								InputData[N]=NUM;
								N++;
							}
							else										//��������λ������6�󣬺�������
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

