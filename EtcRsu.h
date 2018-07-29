#ifndef ETCRSU_H
#define ETCRSU_H
#include <sqlite3.h>
#include <string>
#include <vector>
#include <pthread.h>
#include "BufferedAsyncSerial.h"
#include "RsuMsg.h"
char *random_uuid( char buf[37] );
typedef void* socket_handle;
//线程 写天线的状态
void aerial_state_fail();
static sqlite3 *db = NULL;
//写天线的状态 成功的情况
void aerial_state_suc() ;

using namespace std;
char*   log_Time(void);
char*  getNowTime();
string  get_local_Time(void) ;
int blacklist_lookup(std::string blacklist);
struct VehInfo
{
    //B2
    std::string   sOBUID;
    std::string   sIssuerldentifier; /* 发行商代码 */
    //B3
    std::string   sPlateNumber; /* 车牌号码 */
    //B4
    unsigned char CardType;           /* 00国标卡   01一卡通 */
    unsigned char PhysicalCardType;   /* 物理卡类型，0x00：国标CPU卡   0x01：MIFARE 1(S50) 卡   0x02: MIFARE PRO(或兼容)卡   0x03:Ultra Light  0x05: MIFARE 丁 (S70)卡  */
    unsigned char TransType;          /* 交易类型， 00-传统交易， Ox10】复合父易 只用于国标 CPU 卡，一卡迪为 00 */
    unsigned int  beforeBlance;   /* 卡余额，高位在前，低位在后 */
    std::string   sCardID;          /* 一卡迪和 CPU 卡的物理卡号 C CPU 卡可以没有，暂填 0 */
    std::string   sCardSerialNo; 
    std::string   sCardNetNo;
    //B5
    std::string   sPSAMNo;        /* PSAM 卡终端机编号 */
    std::string   sTransTime;     /* 交易时间，格式 YYYYMMDDHHMMSS */
    unsigned int  iTAC;
    unsigned int  iICCPayserial;   //CPU卡交易序号
    unsigned int  iPSAMTransSerial;   /* PSAM 卡交 易序号，记 11段卡填充 0; */
    unsigned int  afterBlance;   //交易后余额 高位在前，低位在后

};

class AsioTcpServer;
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

    int AddVehCost(socket_handle socket, int sequence, std::string VehNumber, int money);
	int AddVehNumber(socket_handle socket, int sequence);
    int SetTcpSrvHandle(AsioTcpServer *tcpHandle);
    
private:
    //LoggerEvent *mLog;  ///* 日志文件
    BufferedAsyncSerial mRsuComm;
    string mCommName;
    std::vector<unsigned char> mCardFactor;
    unsigned char mTxPower;  /* 功率级别 */
    unsigned char mWaitTime; /* 最小重读时间 */

    VehInfo m_currVehInfo;
    std::string m_RSUTerminalld;

    bool mStoped;
    unsigned char mCurrRSCTL;

    std::string m_currVehNumer;
    int m_currPayMoney;
    socket_handle m_currSocketHandle;
    int m_currSequence;
	unsigned int m_lastRecvTime;
    pthread_mutex_t m_vehMutex;

	socket_handle m_numberSocketHandle;
    int m_numberSequence;
	unsigned int m_numberRecvTime;

    AsioTcpServer *m_tcpSrvHandle;

private:
    bool OpenDrv();
    int CloseDrv();
    int Read();

    int ResonseVehCost(VehInfo* vehinfo,int errcode);
	int ResonseVehNumber(VehInfo* vehinfo,int errcode);
    //QByteArray GetOneMsg(bool* ok);
    bool beforDealRec(std::vector<unsigned char>& result);
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
