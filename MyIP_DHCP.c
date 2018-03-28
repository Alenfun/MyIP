/**
  ******************************************************************************
  * @file    MyIP_DHCP.c
  * @author  mgdg
  * @version V1.0.0
  * @date    2018-02-28
  * @brief   DHCP���ݰ����
  ******************************************************************************
 **/
#include "MyIP_DHCP.h"
#include "MyIP_UDP.h"

#define DHCP_DEBUGOUT(...)	TCPDEBUGOUT(__VA_ARGS__)


//�ֶ���DHCP�����е�ƫ�Ƶ�ַ
#define DHCP_OP			((uint8_t)0)			//����client�͸�server�ķ������Ϊ1������Ϊ2
#define DHCP_HTYPE		((uint8_t)1)			//Ӳ�����ethernetΪ1
#define DHCP_HLEN		((uint8_t)2)			//Ӳ�����ȣ�ethernetΪ6
#define DHCP_HOPS		((uint8_t)3)			//�����ݰ��辭��router���ͣ�ÿվ��1������ͬһ���ڣ�Ϊ0
#define DHCP_XID		((uint8_t)4)			//����ID���Ǹ�����������ڿͻ��ͷ�����֮��ƥ���������Ӧ��Ϣ
#define DHCP_SECS		((uint8_t)8)			//���û�ָ����ʱ�䣬ָ��ʼ��ַ��ȡ�͸��½��к��ʱ��
#define DHCP_FLAGS		((uint8_t)10)			//��0-15bits������һbitΪ1ʱ��ʾserver���Թ㲥��ʽ���ͷ���� client��������δʹ��
#define DHCP_CIADDR		((uint8_t)12)			//�û�IP��ַ
#define DHCP_YIADDR		((uint8_t)16)			//�ͻ�IP��ַ(���),�����Client�Ŀ���IP��
#define DHCP_SIADDR		((uint8_t)20)			//����bootstrap�����е�IP��ַ��
#define DHCP_GIADDR		((uint8_t)24)			//ת���������أ�IP��ַ��
#define DHCP_CHADDR		((uint8_t)28)			//client��MAC��ַ
#define DHCP_SNAME		((uint8_t)44)			//server������(��ѡ)����0x00��β
#define DHCP_FILE		((uint8_t)108)			//�����ļ���(��ѡ)����0x00��β
#define AHCP_OPTIONS	((uint8_t)236)			//��ѡ�Ĳ����ֶ�

/*
 Option 53 (DHCP Message Type)
 Description 
 This option is used to convey the type of the DHCP message. The code for this option is 53, and its length is 1. Legal values for this option are: 
*/
#define	DHCP_MSG_DISCOVER			((uint8_t)1)			//Client��ʼDHCP���̵ĵ�һ������	[RFC2132] 
#define DHCP_MSG_OFFER				((uint8_t)2)			//Server��Discover���ĵ���Ӧ		[RFC2132] 
#define DHCP_MSG_REQUEST			((uint8_t)3)			//Client��Offer���ĵĻ�Ӧ��������Client����IP��ַ����ʱ�����ı��� [RFC2132] 
#define DHCP_MSG_DECLINE			((uint8_t)4)			//Client����server���������ip�޷�ʹ��ʱ�����ñ���֪ͨserver��ֹʹ�ø�ip	[RFC2132] 
#define DHCP_MSG_ACK				((uint8_t)5)			//server��client��request���ĵ�ȷ����Ӧ��client�յ��ñ��Ĳ��������IP��ַ�����������Ϣ	[RFC2132] 
#define DHCP_MSG_NAK				((uint8_t)6)			//server��client��request���ľܾ���Ӧ��client�յ��ñ��ĺ����¿�ʼDHCP����	[RFC2132] 
#define DHCP_MSG_RELEASE			((uint8_t)7)			//client�����ͷ�server�����IP��ַ��server�յ��ñ��ĺ���ո�ip	[RFC2132] 
#define DHCP_MSG_INFORM				((uint8_t)8)			//client�Ѿ����ip�������˱��ĵ�Ŀ���Ǵ�DHCP Server���������������Ϣ����route DNS�ȵ�[RFC2132] 
#define DHCP_MSG_FORCERENEW			((uint8_t)9)			//[RFC3203]
#define DHCP_MSG_LEASEQUERY			((uint8_t)10)			//[RFC4388]
#define DHCP_MSG_LEASEUNASSIGNED	((uint8_t)11)			//[RFC4388]
#define DHCP_MSG_LEASEUNKNOWN		((uint8_t)12)			//[RFC4388]
#define DHCP_MSG_LEASEACTIVE		((uint8_t)13)			//[RFC4388]

//enum NET_STAT Pre_DHCPStat = CLOSE;
//enum NET_STAT Cur_DHCPStat = CLOSE;
bool DHCP_FinishFlg = false;
static bool ReRequestFlg = false;
static uint32_t IP_Geted_Time;											//��¼��ȡ��IP��ʱ�䣬��λs
uint32_t IP_Lease_Time = 2*60*60;										//IP��Լʱ�䣬��λs��Ĭ����Ϊ2��Сʱ
static uint8_t DHCP_Server_IP[4]={0,0,0,0};								//��DHCP��������ȡ���ķ�����ip
//static uint8_t DHCP_Client_IP[4]={0,0,0,0};							//��DHCP��������ȡ���Ŀͻ���ip
static const uint8_t Transaction_ID[4] = {0x45,0x78,0x33,0xF5};			//�����
static uint8_t DHCP_Option[]={0x63,0x82,0x53,0x63,						//Magic cookie: DHCP,Ϊ����BOOTP����
							  0x35,0x01,0x00,							//Option: (35)DHCP Msg Type
							  0x0c,0x0d,0x4d,0x47,0x44,0x47,0x5f,0x4d,0x79,0x54,0x43,0x50,0x2F,0x49,0x50,		//Option: (12) Host Name 
							  0x32,0x04,0x00,0x00,0x00,0x00				//Option: (50) Requested IP Address
							  };

/**
  * @brief	����DHCP Discover
  * @param	void

  * @return	void	
  * @remark
  */
void DHCP_Send_Discover(void)
{
	uint8_t data[350];

	//������0
	memset(data,0x00,(sizeof(data)/sizeof(data[0])) );

	//DHCP���ݴ��
	data[DHCP_OP] = 0x01;					//Message type: Boot Request (1)
	data[DHCP_HTYPE] = 0x01;				//Hardware type: Ethernet (0x01)
	data[DHCP_HLEN] = 0x06;					//Hardware address length: 6
	data[DHCP_HOPS] = 0x00;					//Hops: 0
	memcpy(data+DHCP_XID,Transaction_ID,4);		//Transaction ID: 0x457833f5
	data[DHCP_SECS] = 0x00;					//Seconds elapsed: 0
	data[DHCP_SECS+1] = 0x00;				//Seconds elapsed: 0
	data[DHCP_FLAGS] = 0x80;				//�㲥
	//DHCP_CIADDR��Client IP address: 0.0.0.0
	//DHCP_YIADDR��Your (client) IP address: 0.0.0.0
	//DHCP_SIADDR��Next server IP address: 0.0.0.0
	//DHCP_GIADDR��Relay agent IP address: 0.0.0.0
	memcpy(data+DHCP_CHADDR,My_MAC,6);		//����MAC Client MAC address
	//DHCP_SNAME��Server host name not given
	//DHCP_FILE��Boot file name not given

	//���Option�ֶ�
	memcpy(&data[AHCP_OPTIONS],DHCP_Option,22);
	data[AHCP_OPTIONS+6] 	= DHCP_MSG_DISCOVER;			//DHCPDISCOVER
	data[AHCP_OPTIONS+22] 	= 0xff;			//End


	//���ñ���IPΪ0.0.0.0
	memset(MyIP_LoaclIP,0x00,sizeof(MyIP_LoaclIP)/sizeof(MyIP_LoaclIP[0]));
	
	//�������Ӳ���
	UDPSTRUCT tempUdp;
	memcpy(tempUdp.Re_IP,My_MACIP,4);				//Զ��IPΪ�㲥IP��255.255.255.255��
	memcpy(tempUdp.Re_MAC,My_MACIP,6);				//Զ��MAC����Ϊ�㲥MAC
	tempUdp.Lc_Port = 68;							//���ؿͻ��˶˿�
	tempUdp.Re_Port = 67;							//Զ�̷������˿�

	//����UDP���ݰ�
	Send_UDP_Bag(&MyNet[0],&tempUdp,data,259);
}

/**
  * @brief	����DHCP Request
  * @param	void

  * @return	broadcast�� 0Ϊ�㲥������Ϊ����	
  * @remark
  */
void DHCP_Send_Request(int broadcast)
{
	uint8_t data[350];

	//������0
	memset(data,0x00,(sizeof(data)/sizeof(data[0])) );

	//DHCP���ݴ��
	data[DHCP_OP] = 0x01;						//����
	data[DHCP_HTYPE] = 0x01;					//10M
	data[DHCP_HLEN] = 0x06;						//����
	data[DHCP_HOPS] = 0x00;						//����
	memcpy(data+DHCP_XID,Transaction_ID,4);		//�����
	
	if(broadcast != 0)
	{
		data[DHCP_FLAGS] = 0x00;					//����
		memcpy(data+DHCP_CIADDR,MyIP_LoaclIP,4);	//����ip
	}
	else
	{
		data[DHCP_FLAGS] = 0x80;					//�㲥
	}
	memcpy(data+DHCP_CHADDR,My_MAC,6);			//����MAC

	//���Option�ֶ�
	memcpy(data+AHCP_OPTIONS,DHCP_Option,28);
	data[AHCP_OPTIONS+6] 	= DHCP_MSG_REQUEST;			//DHCP REQUEST
//	memcpy(data+AHCP_OPTIONS+24,DHCP_Client_IP,4);		//Requested IP Address
	data[AHCP_OPTIONS+28] 	= 0xff;						//End

	//�������Ӳ���
	UDPSTRUCT tempUdp;
	if(broadcast == 0)
		memcpy(tempUdp.Re_IP,My_MACIP,4);		//Զ��IPΪ�㲥IP��255.255.255.255��
	else
		memcpy(tempUdp.Re_IP,DHCP_Server_IP,4);	//Զ��IPΪDHCP������IP��255.255.255.255��
	memcpy(tempUdp.Re_MAC,My_MACIP,6);			//Զ��MAC����Ϊ�㲥MAC
	tempUdp.Lc_Port = 68;						//���ؿͻ��˶˿�
	tempUdp.Re_Port = 67;						//Զ�̷������˿�

	//����UDP���ݰ�
	Send_UDP_Bag(&MyNet[0],&tempUdp,data,265);
}

/**
  * @brief	����DHCP Release
  * @param	void

  * @return	void	
  * @remark
  */
void DHCP_Send_Release(void)
{
	uint8_t data[350];

	//������0
	memset(data,0x00,(sizeof(data)/sizeof(data[0])) );

	//DHCP���ݴ��
	data[DHCP_OP] = 0x01;						//����
	data[DHCP_HTYPE] = 0x01;					//10M
	data[DHCP_HLEN] = 0x06;						//����
	data[DHCP_HOPS] = 0x00;						//����
	memcpy(data+DHCP_XID,Transaction_ID,4);		//�����
	data[DHCP_FLAGS] = 0x00;					//����
	//��伴�����ͷŵ�ip��ַ
	memcpy(data+DHCP_CIADDR,MyIP_LoaclIP,4);		//����ip
	memcpy(data+DHCP_CHADDR,My_MAC,6);			//����MAC

	//���Option�ֶ�
	
	memcpy(data+AHCP_OPTIONS,DHCP_Option,7);
	data[AHCP_OPTIONS+6] 	= DHCP_MSG_RELEASE;			//DHCP RELEASE
	
	data[AHCP_OPTIONS+7] 	= 54;						//option 54 DHCP��������ַ
	data[AHCP_OPTIONS+8] 	= 4;						//len 4
	memcpy(data+AHCP_OPTIONS+9,DHCP_Server_IP,4);
	
	data[AHCP_OPTIONS+13] 	= 61;						//Option: (61) Client identifier
	data[AHCP_OPTIONS+14] 	= 0x07;						//Length: 7
	data[AHCP_OPTIONS+15]	= 0x01;						//Hardware type: Ethernet (0x01)
	memcpy(data+AHCP_OPTIONS+16,My_MAC,6);				//Client MAC address
	
	data[AHCP_OPTIONS+22] 	= 0xff;						//End


	//�������Ӳ���
	UDPSTRUCT tempUdp;
	memcpy(tempUdp.Re_IP,DHCP_Server_IP,4);
	memcpy(tempUdp.Re_MAC,My_MACIP,6);
	tempUdp.Lc_Port = 68;
	tempUdp.Re_Port = 67;
	
	//����UDP���ݰ�
	Send_UDP_Bag(&MyNet[0],&tempUdp,data,259);
}


/**
  * @brief	DHCP option�ֶδ���,��ȡ��Ϣ����
  * @param	*data: Option����ʼ��ַ
  * @param	len:  Option�����ݳ���

  * @return	void	
  * @remark
  */
static uint8_t DHCP_GetMsgType(const uint8_t *data,uint16_t len)
{
	if(len < 8)
		return 0;

	for(uint16_t i=4;i<len-2;)
	{
		if(data[i]==0x35 && data[i+1]==0x01)
		{
			return data[i+2];
		}
		else
		{
			i+=data[i+1]+2;
		}
	}

	return 0;
}

/**
  * @brief	DHCP option�ֶδ���,��ȡ��������ַ
  * @param	*data: Option����ʼ��ַ
  * @param	len:  Option�����ݳ���

  * @return	void	
  * @remark Option��ʽ ���� ���� ����
  */
static void DHCP_GetOption(const uint8_t *data,uint16_t len)
{
	if(len < 8)
		return;
	
	for(uint16_t i=4;i<len-2;)
	{
		//������Ϣ����
//		if(data[i]==0x35 && data[i+1]==0x01)
//		{
//			DHCP_DEBUGOUT("1 DHCP Msg Type: %d\r\n",data[i+2]);
//		}
//		else 
		//��ȡIP��Լʱ��
		if(data[i]==0x33 && data[i+1]==0x04)
		{
			IP_Lease_Time = (((uint32_t)data[i+2])<<24) | (((uint32_t)data[i+3])<<16) | (((uint32_t)data[i+4])<<8) | (data[i+5]);
			IP_Geted_Time = MyIP_GetNowTime();			//��¼��ȡ��IPʱ��ʱ��
//			DHCP_DEBUGOUT("DHCP ack IP Addr Lease Time: %u s\r\n",IP_Lease_Time);
		}
		//��ȡ��������
		else if(data[i]==0x01 && data[i+1]==0x04)
		{
			memcpy(MyIP_SubnetMask,&data[i+2],4);
//			DHCP_DEBUGOUT("DHCP ack subnet mask: %d.%d.%d.%d\r\n",MyIP_SubnetMask[0],MyIP_SubnetMask[1],MyIP_SubnetMask[2],MyIP_SubnetMask[3]);
		}
		//��ȡ��������ַ
		else if(data[i]==0x36 && data[i+1]==0x04)
		{
			memcpy(DHCP_Server_IP,&data[i+2],4);
//			DHCP_DEBUGOUT("DHCP ack server ip: %d.%d.%d.%d\r\n",DHCP_Server_IP[0],DHCP_Server_IP[1],DHCP_Server_IP[2],DHCP_Server_IP[3]);
		}
		//��ȡ·������ַ�����ص�ַ��
		else if(data[i]==0x03 && data[i+1]==0x04)
		{
			memcpy(MyIP_GateWay,&data[i+2],4);
//			DHCP_DEBUGOUT("DHCP ack GateWay: %d.%d.%d.%d\r\n",MyIP_GateWay[0],MyIP_GateWay[1],MyIP_GateWay[2],MyIP_GateWay[3]);
		}

		if(data[i+1] != 0xFF)
			i+=data[i+1]+2;
		else
			break;
	}
}

/**
  * @brief	DHCP���ݰ�����
  * @param	*data: ���յ�����������
  * @param	len: ���ݳ���

  * @return	void	
  * @remark
  */
uint8_t DHCP_Data_Process(const uint8_t *data,uint16_t len)
{
	if(len < 286)
		return 1;			//С����СDHCP���ĳ��� 14EN+20IP+8UDP+236DHCP+8DHCP_Option
	
	//���ʶ���룬���Ǳ����ĺ���
	//ʶ����ƫ��λ�� 14EN + 20IP + 8UDP + XID(4)
	if(memcmp(Transaction_ID,data+46,4) != 0)
		return 2;
	
	//��ȡ��Ϣ����
	//ƫ��λ�� 14EN + 20IP + 8UDP + AHCP_OPTIONS = 278
	const uint16_t offset = 278;	//AHCP_OPTIONS+14+20+8;
	uint8_t DHCP_Msg_Type = DHCP_GetMsgType(data+offset,len-offset);
	
//	DHCP_DEBUGOUT("DHCP Msg Type: %d\r\n",DHCP_Msg_Type);

	switch(DHCP_Msg_Type)
	{
		case DHCP_MSG_OFFER:
		{
			//�յ���DHCP������������OFFER
			if(MyNet[0].Cur_Stat == DHCP_DISCOVER)
			{
				//��������������IP
				//��ʱ����ģ���ͻ���request���õ���������ACK�����ȷ�ϸ�IP��ʹ��Ȩ
				//ֱ�ӱ��浽DHCP��ѡ���ֶ��У�����request�ķ���
				memcpy((uint8_t *)(DHCP_Option+24),data+42+DHCP_YIADDR,4);
				//����OFFER״̬����״̬����ִ�У�����REQUEST
				MyNet[0].Cur_Stat = DHCP_OFFER;
			}
		}
		break;

		case DHCP_MSG_ACK:
		{
			if(ReRequestFlg)
			{
				//��Լ����涨ʱ�䣬����request�󷵻ص�ACK
				ReRequestFlg = false;
				DHCP_GetOption(data+offset,len-offset);
			}
			//REQUEST״̬���յ���DHCP������������ACK
			//��ʾIP����ɹ�
			else if(MyNet[0].Cur_Stat == DHCP_REQUEST)
			{
				//��������������IP
				memcpy(MyIP_LoaclIP,data+42+DHCP_YIADDR,4);
//				DHCP_DEBUGOUT("DHCP ack client ip: %d.%d.%d.%d\r\n",MyIP_LoaclIP[0],MyIP_LoaclIP[1],MyIP_LoaclIP[2],MyIP_LoaclIP[3]);
				DHCP_GetOption(data+offset,len-offset);
				//ip��ȡ��ϣ�����ACK״̬
				MyNet[0].Cur_Stat = DHCP_ACK;
			}
		}
		break;

		case DHCP_MSG_NAK:
		{
			if(!ReRequestFlg)
			{
				//IP����ʧ���ˣ���Ҫ��������
				//����NACK״̬���·���Discover
				MyNet[0].Cur_Stat = DHCP_NACK;
			}
		}
		break;

#if 0
		case DHCP_MSG_REQUEST:
		break;

		case DHCP_MSG_DECLINE:
		break;
		
		case DHCP_MSG_DISCOVER:
		break;
		
		case DHCP_MSG_RELEASE:
		break;

		case DHCP_MSG_INFORM:
		break;

		case DHCP_MSG_FORCERENEW:
		break;

		case DHCP_MSG_LEASEQUERY:
		break;

		case DHCP_MSG_LEASEUNASSIGNED:
		break;

		case DHCP_MSG_LEASEUNKNOWN:
		break;

		case DHCP_MSG_LEASEACTIVE:
		break;
#endif
	}
	return 0;
}

/**
  * @brief	ip���ڴ���
  * @param	ElapsedTime��ÿ�ε��øú���ʱ���Ѿ��ȹ���ʱ��

  * @return	void	
  * @remark 1����ʹ�����ڵ�50%��client��server��������DHCPREQUEST���������ڡ�
  * @remark 2��server��ͬ�⣬����DHCPACK��client��ʼһ���µ��������ڣ�����ͬ�⣬����DHCPNAK
  * @remark 3��client��������û�б�ͬ�⣬�����ڹ�ȥ87.5��ʱ�̴���client��server�㲥����
  * @remark 4��server��ͬ�⣬����DHCPACK��client��ʼһ���µ��������ڣ�����ͬ�⣬����DHCPNAK�����ڵ��ں�client�������IP�����»�ȡIP��
  */
void MyIP_IPLeaseTimeProc(void)
{
	static uint8_t flg = 0;
	uint32_t ElapsedTime;

	if(!DHCP_FinishFlg)
		return;
	
	ElapsedTime = MyIP_GetElapsedTime(IP_Geted_Time);
	
	//��Լ���ڣ����»�ȡIP��ַ
	if(ElapsedTime >= IP_Lease_Time)
	{
		if(flg != 1)
		{
			flg = 1;
			DHCP_FinishFlg = false;
			MyNet[0].Cur_Stat = DHCP_DISCOVER;
		}
	}
	//���ڰٷ�87.5
	else if(ElapsedTime > ((IP_Lease_Time>1) + (IP_Lease_Time>2)) )		
	{
		if(flg != 2)
		{
			flg = 2;
			//�㲥����
			DHCP_Send_Request(0);
			ReRequestFlg = true;
		}
	}
	//���ڰٷ�50
	else if(ElapsedTime > (IP_Lease_Time>1))		
	{
		if(flg != 3)
		{
			flg = 3;
			//��������
			DHCP_Send_Request(1);
			ReRequestFlg = true;
		}
	}
	else
	{
		if(flg != 0)
		{
			flg = 0;
			ReRequestFlg = false;
		}
	}
}
