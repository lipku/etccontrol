#include "AsioTcp.h"
#include <boost/thread.hpp>

using namespace boost::asio;

CAsioTcp::CAsioTcp(INetCallback* pINetCallback, int port)
 : m_pINetCallback(pINetCallback),
 m_pAcceptor(m_ioservice, ip::tcp::endpoint(ip::tcp::v4(), port))
{
 StartAccept();
}


CAsioTcp::~CAsioTcp(void)
{
}

void CAsioTcp::StartAccept()
{
 CTcpSession* pTcpSession = new CTcpSession(m_ioservice, m_pINetCallback);
 m_pAcceptor.async_accept(pTcpSession->GetSocket(), boost::bind(&CAsioTcp::AcceptHandler, this, boost::asio::placeholders::error, pTcpSession));
}

void CAsioTcp::AcceptHandler( const boost::system::error_code& ec, CTcpSession* pTcpSession )
{
 if (ec)
 {
  delete pTcpSession;
 }
 else
 {

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
 (pTcpSession)->SendMsg(pData, nDataSize);
}

void CAsioTcp::Start()
{
 m_ioservice.run();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

CTcpSession::CTcpSession(boost::asio::io_service& ioService, INetCallback* pINetCallback)
 : m_socket(ioService)
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
    delete this;
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
 boost::asio::async_write(m_socket,
  boost::asio::buffer(pData, nDataSize),
  boost::bind(&CTcpSession::HandleWrite, this,
  boost::asio::placeholders::error));

}

void CTcpSession::HandleWrite( const boost::system::error_code& ec )
{

}