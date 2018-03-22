/**
  ******************************************************************************
  * @file    MyIP_Enthernet.h
  * @author  mgdg
  * @version V1.0.0
  * @date    2018-02-28
  * @brief   
  ******************************************************************************
 **/
#ifndef _MYIP_ENTHERNET_H
#define _MYIP_ENTHERNET_H

#include "MyIP_TCPIP.h"

/**
  * @brief	Enthernet �ײ����
  * @param	*node������
  * @param	*remac�� Ŀ�ĵ�MAC
  * @param	*lcmac�� ����MAC
  * @param	type��00 IP��  06 ARP��

  * @return	bool	
  * @remark		
  */
bool EN_Head_Pack(LINKSTRUCT *node,const uint8_t *remac,const uint8_t *lcmac,uint8_t type);





#endif
