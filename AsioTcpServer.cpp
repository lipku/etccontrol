#include "AsioTcpServer.h"
#include "EtcRsu.h"
#include "RsuMsg.h"
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "tinyxml2.h"
using namespace tinyxml2;

AsioTcpServer::AsioTcpServer(int port, EtcRsu* pEtcRsu)
{
	m_pAsioTcp = new CAsioTcp(this, port);

	m_pEtcRsu = pEtcRsu;

}

int AsioTcpServer::StartRun()
{
	m_pAsioTcp->Start();
	return 0;
}

int AsioTcpServer::SendMsg(socket_handle socket, const char* pData, unsigned int nDataSize )
{
	m_pAsioTcp->SendMsg(socket, pData, nDataSize);
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

	if(nDataSize<=sizeof(MSG_Header))
	{
		printf("recv data len:%d is too short\n", nDataSize);
		return;
	}

	MSG_Header *header = (MSG_Header*)pData;
	printf("header info, msgsize=%d,seq=%d,msgcmd=%d\n",ntohl(header->msglen),ntohl(header->sequence), ntohl(header->msgcmd));
	if(nDataSize!=ntohl(header->msglen))
	{
		printf("recv data size:%d != msgsize:%d\n",nDataSize,ntohl(header->msglen));
		return;
	}

	if(ntohl(header->msgcmd) == 0x1) //扣费请求
	{
		//解析xml
		XMLDocument docXml;
		XMLError errXml = docXml.Parse( pData+sizeof(MSG_Header) );
		if (XML_SUCCESS == errXml)
		{
			XMLElement* elmtRoot = docXml.RootElement();
			if (elmtRoot)
			{
				XMLElement* elmtData = elmtRoot->FirstChildElement("Data");
				if(!elmtData)
					return;
				//printf( "elmtRoot===true\n");
				const XMLElement* vehElem = elmtData->FirstChildElement("VehplateNo");
				const XMLElement* moneyElem = elmtData->FirstChildElement("money");
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
						m_pEtcRsu->AddVehCost(socket,ntohl(header->sequence),string(vehNo),money);




						char sql[200] = {0};
						char  guid[40];
						random_uuid(guid);
						char  *localtime = (char*)malloc(sizeof(char)* 30); 
						localtime = getNowTime();
						cout <<"localtime---------------------------:"<<localtime<<endl;;

						string time;
						stringstream ss;
						ss<<localtime;
						ss>>time;

                                                string suuid;
						ss<<guid;
 						suuid = guid;

                                                string head = "INSERT INTO T_transaction (local_serial_no , record_time , request_serial_no , trans_time ,trans_amount ) VALUES (\"";

                                                string sql123 = head + suuid + "\"" + ",\"" + time + "\"";
 						cout <<"------------sql:"<<sql123<<endl;



                                               //INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)
                                               //VALUES (1, 'Paul', 32, 'California', 20000.00 );



					}		

				}
			}
		}
	}
	else if(ntohl(header->msgcmd) == 0x2) //当前扫描车牌
	{
		//解析xml
		XMLDocument docXml;
		XMLError errXml = docXml.Parse( pData+sizeof(MSG_Header) );
		if (XML_SUCCESS == errXml)
		{
			if(m_pEtcRsu)
				m_pEtcRsu->AddVehNumber(socket,ntohl(header->sequence));
		}
	}


}
