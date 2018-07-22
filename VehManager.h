#pragma once
#include <iostream>
#include <pthread.h>

class VehManager
{

public:
  VehManager(int port, EtcRsu* pEtcRsu);
  ~VehManager();

  int StartRun();

  int SendMsg(const char* pData, unsigned int nDataSize );

  virtual void OnNewConnection( socket_handle newSocket );

  virtual void OnRecvData( socket_handle socket, const char* pData, unsigned int nDataSize );

private:
 CAsioTcp* m_pAsioTcp;
 EtcRsu* m_pEtcRsu;
 socket_handle m_currConn;
};
