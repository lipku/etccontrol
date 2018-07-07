
#include <iostream>
#include <unistd.h>
#include <boost/thread.hpp>

#include "EtcRsu.h"
#include "AsioTcpServer.h"

using namespace std;
using namespace boost;

int main(int argc, char* argv[])
{
    try {
        EtcRsu etcRsu;
        etcRsu.SetComm("/dev/ttyS0"); //todo 设置串口名
        etcRsu.SetPower(5); //todo
        etcRsu.SetWaitTime(1); //todo
     //   char factors[] = {0xb1,0xb1,0xbe,0xa9,0xb1,0xb1,0xbe,0xa9};
    //    vector<unsigned char> cardfactor(factors, factors+sizeof(factors));
     //   etcRsu.SetCardFactor(cardfactor); //todo 卡一级分散因子
        etcRsu.init();

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
