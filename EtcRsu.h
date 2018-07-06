#ifndef ETCRSU_H
#define ETCRSU_H

#include <string>
#include <vector>
#include <pthread.h>
#include "BufferedAsyncSerial.h"
#include "RsuMsg.h"

using namespace std;

class EtcRsu
{
public:
    EtcRsu();
    bool init();
    void stop();
    void SetComm(string cName);
    void SetPower(unsigned char ThePower);
    void SetWaitTime(unsigned char TheTime);
    void SetCardFactor(std::vector<unsigned char>& cardfactor);

    void WriteSerial(std::vector<unsigned char>& buff);
    void run(); //process msg thread
private:
    //LoggerEvent *mLog;  ///* 日志文件
    BufferedAsyncSerial mRsuComm;
    string mCommName;
    std::vector<unsigned char> mCardFactor;
    unsigned char mTxPower;  /* 功率级别 */
    unsigned char mWaitTime; /* 最小重读时间 */

    bool mStoped;
    unsigned char mCurrRSCTL;

private:
    bool OpenDrv();
    int CloseDrv();
    int Read();
    //QByteArray GetOneMsg(bool* ok);
    //QByteArray beforDealRec(bool* ok,QByteArray msg);
    std::vector<unsigned char> beforDealSnd(std::vector<unsigned char>& msg);
    /* 高低半字节互换 */
    unsigned char halfChange(unsigned char v);

    bool mIsRead;
    bool mIsInit;
    void sendMsg2Logic(std::vector<unsigned char>& msg);
    void openAntenna(bool open);
    int send4C(unsigned char rsctl,unsigned char ante);
    int sendC0(unsigned char rsctl);
    int sendC1(unsigned char rsctl,unsigned char* OBUID, std::vector<unsigned char>& cardDivFactor);
    int sendC2(unsigned char rsctl, char stopType,unsigned char* OBUID);
    int sendC6(unsigned char rsctl,unsigned char* OBUId,int TradeMoney,time_t TradeTime,File0019 f0019,std::vector<unsigned char>& cardDivFactor);

    pthread_t readcom_thread;

//signals:
    void receiveB0(std::vector<unsigned char>& buff);
    void receiveB2(std::vector<unsigned char>& buff);
    void receiveB3(std::vector<unsigned char>& buff);
    void receiveB4(std::vector<unsigned char>& buff);
    void receiveB5(std::vector<unsigned char>& buff);
    

};

#endif // ETCRSU_H
