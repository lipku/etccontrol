
#include <iostream>
#include <unistd.h>
#include <boost/thread.hpp>

#include "EtcRsu.h"
#include "AsioTcpServer.h"

using namespace std;
using namespace boost;

int main(int argc, char* argv[])
{

	int stdout_sfd = -1;
	int file_fd = -1;
	int stdout_nfd = -1;
	file_fd = open("./note.txt", O_CREAT | O_RDWR | O_APPEND, 00777);
	stdout_sfd = dup(STDOUT_FILENO);
	stdout_nfd = dup2(file_fd, STDOUT_FILENO);


	try {
		EtcRsu etcRsu;
		  etcRsu.SetComm("/dev/ttyS0"); //todo 设置串口名
		  etcRsu.SetPower(30); //todo
		  etcRsu.SetWaitTime(0); //todo
		//   char factors[] = {0xb1,0xb1,0xbe,0xa9,0xb1,0xb1,0xbe,0xa9};
		//    vector<unsigned char> cardfactor(factors, factors+sizeof(factors));
		//   etcRsu.SetCardFactor(cardfactor); //todo 卡一级分散因子
		etcRsu.init();
	
		//ReadConfigurationFile(etcRsu) ;
		AsioTcpServer tcpSrv(2000,&etcRsu);
		etcRsu.SetTcpSrvHandle(&tcpSrv);
		tcpSrv.StartRun();

		while(getchar()!='q')
			sleep(1);

		return 0;


	} catch(boost::system::system_error& e)
	{
		cout<<"Error: "<<e.what()<<endl;
		return 1;
	}
}
