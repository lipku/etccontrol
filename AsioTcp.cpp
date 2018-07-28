#include "AsioTcp.h"
#include <boost/thread.hpp>
#include <algorithm>

using namespace std;
using namespace boost::asio;

CAsioTcp::CAsioTcp(INetCallback* pINetCallback, int port)
 : m_pINetCallback(pINetCallback),
 m_pAcceptor(m_ioservice, ip::tcp::endpoint(ip::tcp::v4(), port))
{
 StartAccept();
 pthread_mutex_init(&m_connMutex,NULL);
}


CAsioTcp::~CAsioTcp(void)
{
}

void CAsioTcp::StartAccept()
{
 CTcpSession* pTcpSession = new CTcpSession(m_ioservice, this, m_pINetCallback);
 m_pAcceptor.async_accept(pTcpSession->GetSocket(), boost::bind(&CAsioTcp::AcceptHandler, this, boost::asio::placeholders::error, pTcpSession));
}

void CAsioTcp::AcceptHandler( const boost::system::error_code& ec, CTcpSession* pTcpSession )
{
 if (ec)
 {
  //delete pTcpSession;
  RemoveConn(pTcpSession);
 }
 else
 {

   pthread_mutex_lock(&m_connMutex);
   mConnList.push_back(pTcpSession);
   pthread_mutex_unlock(&m_connMutex);

   if (m_pINetCallback != NULL)
   {
     m_pINetCallback->OnNewConnection(pTcpSession);
   }
   pTcpSession->StartRead();
 }

 StartAccept();
 
}

void CAsioTcp::SendMsg( socket_handle socket, const char* pData, unsigned int nDataSize )
{
  CTcpSession* pTcpSession = reinterpret_cast<CTcpSession*>(socket);
 //printf("pTcpSession=%x\n",pTcpSession);
  pthread_mutex_lock(&m_connMutex);
  vector<CTcpSession*>::iterator result=find(mConnList.begin(),mConnList.end(),pTcpSession);
  if(result == mConnList.end())
  {
    pthread_mutex_unlock(&m_connMutex);
    printf("pTcpSession=%x not found\n",pTcpSession);
    return;
  }
  pthread_mutex_unlock(&m_connMutex);
 (pTcpSession)->SendMsg(pData, nDataSize);
}

void CAsioTcp::Start()
{
 //m_ioservice.run();
  boost::thread t(boost::bind(&boost::asio::io_service::run, &m_ioservice));
  backgroundThread.swap(t);
}

int CAsioTcp::RemoveConn(CTcpSession* pTcpSession)
{
  if(pTcpSession==NULL)
    return -1;
  delete pTcpSession;

  pthread_mutex_lock(&m_connMutex);
  vector<CTcpSession*>::iterator result=find(mConnList.begin(),mConnList.end(),pTcpSession);
  if(result != mConnList.end())
    mConnList.erase(result);
  pthread_mutex_unlock(&m_connMutex);
  return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

CTcpSession::CTcpSession(boost::asio::io_service& ioService, CAsioTcp* pParent, INetCallback* pINetCallback)
 : m_socket(ioService)
 ,m_pParent(pParent)
 , m_pINetCallback(pINetCallback)
{

}

void CTcpSession::HandleRead( const boost::system::error_code& ec, size_t bytes_transferred )
{
 if (!ec)
 {
  if (m_pINetCallback != NULL)
  {
   m_pINetCallback->OnRecvData(this, m_dataRecvBuff, bytes_transferred);
  }
  StartRead();
 }
 else
 {
    printf("close connection============================\n");
    //delete this;
    m_pParent->RemoveConn(this);
 }
}

void CTcpSession::StartRead()
{
 
 m_socket.async_read_some(boost::asio::buffer(m_dataRecvBuff, max_length),
  boost::bind(&CTcpSession::HandleRead, this,
  boost::asio::placeholders::error,
  boost::asio::placeholders::bytes_transferred));
}

void CTcpSession::SendMsg( const char* pData, unsigned int nDataSize )
{
 printf("CTcpSession::SendMsg\n");
 boost::asio::async_write(m_socket,
  boost::asio::buffer(pData, nDataSize),
  boost::bind(&CTcpSession::HandleWrite, this,
  boost::asio::placeholders::error));

}

void CTcpSession::HandleWrite( const boost::system::error_code& ec )
{

}
