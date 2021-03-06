#pragma once
#include <iostream>
#include "AsioTcp.h"
#include <boost/make_shared.hpp>

class EtcRsu;
class AsioTcpServer : public INetCallback
{

public:
  AsioTcpServer(int port, EtcRsu* pEtcRsu);

  int StartRun();

  int SendMsg(socket_handle socket, const char* pData, unsigned int nDataSize );

  virtual void OnNewConnection( socket_handle newSocket );

  virtual void OnRecvData(socket_handle socket, const char* pData, unsigned int nDataSize );

private:
 CAsioTcp* m_pAsioTcp;
 EtcRsu* m_pEtcRsu;
 
};
