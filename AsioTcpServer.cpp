#include "AsioTcpServer.h"
#include "EtcRsu.h"

#include "tinyxml2.h"
using namespace tinyxml2;

AsioTcpServer::AsioTcpServer(int port, EtcRsu* pEtcRsu)
 {
  m_pAsioTcp = new CAsioTcp(this, port);

  m_pEtcRsu = pEtcRsu;

  m_currConn = NULL;
 }

int AsioTcpServer::StartRun()
{
  m_pAsioTcp->Start();
  return 0;
}

 int AsioTcpServer::SendMsg(const char* pData, unsigned int nDataSize )
 {
  m_pAsioTcp->SendMsg(m_currConn, pData, nDataSize);
  return 0;
 }


void AsioTcpServer::OnNewConnection( socket_handle newSocket )
 {
  std::cout << "OnNewConnection" << std::endl;
 }

void AsioTcpServer::OnRecvData( socket_handle socket, const char* pData, unsigned int nDataSize )
 {
  std::cout << "OnRecvData:" << pData  << " size=" << nDataSize << std::endl;

  //m_pAsioTcp->SendMsg(socket, "echo", 4);
  m_currConn=socket;

  //解析xml
  XMLDocument docXml;
  XMLError errXml = docXml.Parse( pData );
  if (XML_SUCCESS == errXml)
  {
    XMLElement* elmtRoot = docXml.RootElement();
    if (elmtRoot)
    {
       //printf( "elmtRoot===true\n");
        const XMLElement* vehElem = elmtRoot->FirstChildElement("vehplateNo");
        const XMLElement* moneyElem = elmtRoot->FirstChildElement("money");
        if(vehElem && moneyElem)
        {
	//printf( "vehElem===true\n");
          const char* vehNo = vehElem->GetText();
          int money=0;
          moneyElem->QueryIntText( &money );

          printf( "vehNo=%s, money=%d\n", vehNo, money);
	        if(m_pEtcRsu == NULL)
          {
             printf( "m_pEtcRsu===null\n");
	        }
	        else
          {
	           m_pEtcRsu->AddVehCost(string(vehNo),money);
          }		
	  
          
        }
    }
  }


 }
