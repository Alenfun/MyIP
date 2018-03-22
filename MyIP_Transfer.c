/**
  ******************************************************************************
  * @file    MyIP_Transfer.c
  * @author  mgdg
  * @version V1.0.0
  * @date    2018-02-28
  * @brief   �������ݴ���
  ******************************************************************************
 **/
#include "MyIP_Transfer.h"
#include "MyIP_TCPIP.h"
#include "enc28j60.h"

uint16_t MyIP_PacketReceive(uint8_t* packet,uint16_t maxlen)	 //�������ݰ���������󳤶� ����
{
	extern uint16_t NextPacketPtr;
	static uint16_t NextPackLen=0,ReadedPackLen=0;		//�´��Ƿ��������
	static uint16_t CurrentPacketPtr;	  	//���浱ǰ����λ��
	uint16_t rxstat;
    uint16_t len;
	
	if(packet == NULL)
		return 0;

	if(NextPackLen==0)		//������յİ��ڻ��淶Χ��
	{
		// ��黺���Ƿ�һ�����Ѿ��յ�
		if( !(enc28j60Read(EIR) & EIR_PKTIF) )
	    {
	        // ͨ���鿴EPKTCNT�Ĵ����ٴμ���Ƿ��յ���
			if (enc28j60Read(EPKTCNT) == 0)
	            return 0;
	    }
	    
		//���ý��յ������ݰ���ָ�뿪ʼ
		enc28j60Write(ERDPTL, (NextPacketPtr));
	    enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
		CurrentPacketPtr=NextPacketPtr;

	    // ��һ�������ָ��
		NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	    NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	
	    // ��ȡ���ĳ���
		len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	    len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
		len-=4;// �Ƴ�CRC�ֶεĳ���������MAC�����泤��
	
		// ��ȡ�������ݰ���״̬
		rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
		rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	    //if ((rxstat & 0x80)==0)	return 0; 	    // invalid
	
	    // ����ʵ�����ݳ���
		if(len>maxlen)
		{
			NextPackLen=len-maxlen;	 		//ʣ���ֽ�
			len=maxlen;
			// copy the packet from the receive buffer
		    enc28j60ReadBuffer(len, packet);
			packet[16]=(len-42+28)>>8; 		//�޸�IP���ѽ��հ�����,����ֻ�н���UDP��ʱ�Ż���ֳ�������
			packet[17]=len-42+28;			//,���õ���TCP,��ΪTCP�д���С����,���ᳬ������
		}
		else
		{
			NextPackLen=0;
			// copy the packet from the receive buffer
		    enc28j60ReadBuffer(len, packet);
		}

		ReadedPackLen=len;		//�Ѷ�����

	}
	else 		//������ϴ�û������İ�
	{
		if(NextPackLen>maxlen-42)		//����UDP��ͷ42�ֽڣ�TCP�޴�����
		{
			NextPackLen=NextPackLen-(maxlen-42);
			len=maxlen-42;
		}
		else
		{
			len=NextPackLen;
			NextPackLen=0;
		}

		//���ý��յ������ݰ���ָ�뿪ʼ
		enc28j60Write(ERDPTL, (CurrentPacketPtr+6));	   	//����ǰ��6�ֽڵĳ�����Ϣ
	    enc28j60Write(ERDPTH, (CurrentPacketPtr+6)>>8);
		enc28j60ReadBuffer(42, packet);		//������ͷ��Ŀǰ֧��UDP��ͷΪ42�ֽڣ�TCP��������������
		packet[16]=(len+28)>>8; 		//�޸�IP���ѽ��հ�����,����ֻ�н���UDP��ʱ�Ż���ֳ�������
		packet[17]=len+28;				//,���õ���TCP,��ΪTCP�д���С����,���ᳬ������

		enc28j60Write(ERDPTL, (CurrentPacketPtr+6+ReadedPackLen));	   	//�����ϴζ����λ�ô���ʼ��
	    enc28j60Write(ERDPTH, (CurrentPacketPtr+6+ReadedPackLen)>>8); 	
		enc28j60ReadBuffer(len, packet+42);		//����ʣ�µ�����
		ReadedPackLen+=len;

		len+=42;
	}
	
	if(NextPackLen==0)		 		//���������ռģ��Ļ���ռ�
	{
	    // ERXRDPT��������ָ��
		// ENC28J60��һֱд����ָ��֮ǰ��һ��ԪΪֹ
	    enc28j60Write(ERXRDPTL, (NextPacketPtr));
	    enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8);
	    // Errata workaround #13. Make sure ERXRDPT is odd

	    // ���ݰ������ݼ�λEPKTCNT��1
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);  
	}

    return len;
}

void ARP_Packet_Send(const uint8_t *EN_Head,const uint8_t *ARP_Head)
{
	if(EN_Head==NULL || ARP_Head==NULL)
		return;
	
	// ����дָ�뿪ʼ�Ĵ��仺������
	enc28j60Write(EWRPTL, (uint8_t)TXSTART_INIT);
    enc28j60Write(EWRPTH, TXSTART_INIT>>8);

    // ����TXNDָ���Ӧ�ڸ��������ݰ���С
	enc28j60Write(ETXNDL, (uint8_t)(TXSTART_INIT+42));//14Ϊ��̫��ͷ+28 ARP���ݰ�����
    enc28j60Write(ETXNDH, (TXSTART_INIT+42)>>8);

    // дÿ�����Ŀ�����
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

    // TODO, fix this up
	    // ��̫��ͷ �����仺��
		enc28j60WriteBuffer(14, EN_Head);
		// ARP���ݰ� �����仺��
		enc28j60WriteBuffer(28, ARP_Head);
	    

	// ����̫�����ƼĴ���ECON1����λ ��1���Է��ͻ���������
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

void UDP_Packet_Send(const uint8_t *EN_Head,const uint8_t *IP_Head,const uint8_t *UDP_Head,const uint8_t *DATA,uint16_t len)
{
	if(EN_Head==NULL || IP_Head==NULL || UDP_Head==NULL)
		return;
	
	if(DATA == NULL && len != 0)
		return;
	
	// ����дָ�뿪ʼ�Ĵ��仺������
	enc28j60Write(EWRPTL, (uint8_t)TXSTART_INIT);
    enc28j60Write(EWRPTH, TXSTART_INIT>>8);

    // ����TXNDָ���Ӧ�ڸ��������ݰ���С
	enc28j60Write(ETXNDL, (TXSTART_INIT+42+len));//14Ϊ��̫��ͷ+20IPͷ+8UDPͷ+len���ݰ�����
    enc28j60Write(ETXNDH, (TXSTART_INIT+42+len)>>8);

    // дÿ�����Ŀ�����
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

    // TODO, fix this up

    
        // ��̫��ͷ �����仺��
		enc28j60WriteBuffer(14, EN_Head);
		// IPͷ �����仺��
		enc28j60WriteBuffer(20, IP_Head);
		// UDPͷ �����仺��
		enc28j60WriteBuffer(8, UDP_Head);
		// UDP���� �����仺��
		if(len != 0)
			enc28j60WriteBuffer(len, DATA);
    

	// ����̫�����ƼĴ���ECON1����λ ��1���Է��ͻ���������
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

void TCP_Packet_Send(const uint8_t *EN_Head,const uint8_t *IP_Head,const uint8_t *TCP_Head,const uint8_t *DATA,uint16_t len)
{
	if(EN_Head==NULL || IP_Head==NULL || TCP_Head==NULL)
		return;
	
	if(DATA == NULL && len != 0)
		return;
	
	uint8_t tcp_head_len = (((TCP_Head[12])>>2)&0x3C);
	
	// ����дָ�뿪ʼ�Ĵ��仺������
	enc28j60Write(EWRPTL, (uint8_t)TXSTART_INIT);
    enc28j60Write(EWRPTH, TXSTART_INIT>>8);

    // ����TXNDָ���Ӧ�ڸ��������ݰ���С
	uint16_t Totail_len = (TXSTART_INIT+14+20+tcp_head_len+len);		//TXSTART_INIT+54+len
	enc28j60Write(ETXNDL, (uint8_t)Totail_len);//14Ϊ��̫��ͷ+20IPͷ+20TCPͷ+len���ݰ�����
    enc28j60Write(ETXNDH, Totail_len>>8);

    // дÿ�����Ŀ�����
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

    // TODO, fix this up
        // ��̫��ͷ �����仺��
		enc28j60WriteBuffer(14, EN_Head);
		// IPͷ �����仺��
		enc28j60WriteBuffer(20, IP_Head);
		// TCPͷ �����仺��
		enc28j60WriteBuffer(tcp_head_len, TCP_Head);
		// UDP���� �����仺��
		if(len != 0)
			enc28j60WriteBuffer(len,DATA);

    

	// ����̫�����ƼĴ���ECON1����λ ��1���Է��ͻ���������
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

void ICMP_Ping_Packet_Send(const uint8_t *EN_Head,const uint8_t *IP_Head,const uint8_t *ICMP_Head,uint16_t ICMP_len)
{
	if(EN_Head==NULL || IP_Head==NULL || ICMP_Head==NULL)
		return;
	
	// ����дָ�뿪ʼ�Ĵ��仺������
	enc28j60Write(EWRPTL, (uint8_t)TXSTART_INIT);
    enc28j60Write(EWRPTH, TXSTART_INIT>>8);

    // ����TXNDָ���Ӧ�ڸ��������ݰ���С
	enc28j60Write(ETXNDL, (uint8_t)(TXSTART_INIT+34+ICMP_len));//14Ϊ��̫��ͷ+20IPͷ+40ICMP_Ping���ݰ�����
    enc28j60Write(ETXNDH, (TXSTART_INIT+34+ICMP_len)>>8);

    // дÿ�����Ŀ�����
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

    // TODO, fix this up
        // ��̫��ͷ �����仺��
		enc28j60WriteBuffer(14, EN_Head);
		// IPͷ �����仺��
		enc28j60WriteBuffer(20, IP_Head);
		// ICMP �����仺��
		enc28j60WriteBuffer(ICMP_len, ICMP_Head);


	// ����̫�����ƼĴ���ECON1����λ ��1���Է��ͻ���������
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

/**
  * @brief	UDP���ݽ���
  * @param	sockfd: �Ѿ�������ĳ������
  * @param	*data: �յ�����Ч����
  * @param	len�� ��Ч���ݵĳ���

  * @return	void	
  * @remark	�����յ������ݴ������
  */
void UDP_Data_Recev(uint16_t sockfd,const uint8_t *data,uint16_t len)
{
	printf("udp(%d) recev: ",sockfd);
	for(size_t i=0;i<len;i++)
		printf("%c",data[i]);
	printf("\r\n");
}

/**
  * @brief	TCP���ݽ���
  * @param	sockfd: �Ѿ�������ĳ������
  * @param	*data: �յ�����Ч����
  * @param	len�� ��Ч���ݵĳ���

  * @return	uint16_t	
  * @remark	�����յ������ݴ�����У�����ʵ�ʱ���ɹ������ݴ�С
  */
uint16_t TCP_Data_Recev(uint16_t sockfd,const uint8_t *data,uint16_t len)
{
	printf("tcp(%d) recev: ",sockfd);
	for(uint16_t i=0;i<len;i++)
		printf("%c",data[i]);
	printf("\r\n");
	return len;
}
