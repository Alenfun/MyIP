/**
  ******************************************************************************
  * @file    MyIP_DHCP.h
  * @author  mgdg
  * @version V1.0.0
  * @date    2018-02-28
  * @brief   
  ******************************************************************************
 **/
#ifndef _MYIP_DHCP_H
#define _MYIP_DHCP_H

#include "MyIP_TCPIP.h"


void DHCP_Send_Discover(void);

void DHCP_Send_Request(int broadcast);

void DHCP_Send_Release(void);

uint8_t DHCP_Data_Process(const uint8_t *data,uint16_t len);

#endif
