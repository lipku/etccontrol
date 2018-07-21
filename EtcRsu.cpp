#include "EtcRsu.h"
#include <strings.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <time.h> 
#include "AsioTcpServer.h"

#include "tinyxml2.h"
using namespace tinyxml2;
using namespace std;

std::string B2_Issuerldentifier;
/*
 * 字符串转成bcd码，这个是正好偶数个数据的时候，如果是奇数个数据则分左靠还是右靠压缩BCD码
 */
static bool Str2Bcd(std::vector<unsigned char>& dest,const char *src)
{
	unsigned char temp;
	unsigned char destchar;
	while(*src !='\0')
	{
		temp = *src;
		destchar |= ((temp&0xf)<<4);
		src++;
		temp = *src;
		destchar |= (temp&0xf);
		src++;
		dest.push_back(destchar);
	}

	return true;
}
static bool Str2Bcdch(char *des, char *src)
{
	int c = 0;
	int index = 0;
	int i = 0;  
	unsigned char bytehignt;
	unsigned char bytelow;
	int len = strlen(src);
	int dlen= len/2;    

	for(i=0; i < dlen; i++) {
		index = i*2;
		bytehignt = src[index]-'0';
		bytelow = src[index+1]-'0';
		des[i] = (bytehignt<<4)|bytelow;
	}   
	if(len>dlen*2)
	{
		des[dlen] = (src[len-1]-'0')<<4;
	}

	return true;
}

static string Bin2Hex(const unsigned char* strBin, int binLen, bool bIsUpper = false)
{
	std::string strHex;
	strHex.resize(binLen * 2);
	for (size_t i = 0; i < binLen; i++)
	{
		uint8_t cTemp = strBin[i];
		for (size_t j = 0; j < 2; j++)
		{
			uint8_t cCur = (cTemp & 0x0f);
			if (cCur < 10)
			{
				cCur += '0';
			}
			else
			{
				cCur += ((bIsUpper ? 'A' : 'a') - 10);
			}
			strHex[2 * i + 1 - j] = cCur;
			cTemp >>= 4;
		}
	}

	return strHex;
}


///////////////////////////////////////////////////////////////////////////////////

EtcRsu::EtcRsu()
{
	mStoped = false;
	mIsRead = false;
	mCurrRSCTL = 0x08;

	m_currVehNumer = "";
	m_currPayMoney = 0;

	pthread_mutex_init(&m_vehMutex,NULL);

	m_tcpSrvHandle = NULL;
}

static void *readcomProc(void* lpParam)
{
	EtcRsu *pApp=(EtcRsu*)lpParam;
	pApp->run();
}

bool EtcRsu::init()
{
	mIsInit = false;
	bool result = this->OpenDrv(); 

	if(result)
	{
		//this->start();
		pthread_create(&readcom_thread, NULL, readcomProc, (void*)this);
		this->sendC0(0x80);
		/* for(int i= 0; i < 65; i++)//Log11  Log20
		   {
		   msleep(50);
		   if(mIsInit) return true;
		   }
		   if(mIsInit)
		   result = true;
		   else
		   result = false*/;
	}
	return result;
}

void EtcRsu::stop()
{
	mStoped = true;
	pthread_join(readcom_thread, NULL);
}

void EtcRsu::SetComm(string cName)
{
	mCommName = cName;
}
void EtcRsu::SetPower(unsigned char ThePower)
{
	mTxPower = ThePower;//功率级别
}

void EtcRsu::SetWaitTime(unsigned char TheTime)
{
	mWaitTime = TheTime;//最小重读时间
}

void EtcRsu::SetCardFactor(std::vector<unsigned char>& cardfactor)
{
	mCardFactor.swap(cardfactor);
}

bool EtcRsu::OpenDrv()
{
	this->mRsuComm.open(mCommName,115200);
	if (mRsuComm.isOpen())
	{
		return true;
	}
	else
	{
		return false;
	}
}

int EtcRsu::CloseDrv()
{
	this->mRsuComm.close();
	return 0;
}


int EtcRsu::AddVehCost(socket_handle socket, int sequence, std::string VehNumber, int money)
{
	pthread_mutex_lock(&m_vehMutex);
	m_currVehNumer.assign(VehNumber);// VehNumber;
	printf("vnumber = %s\n",VehNumber.c_str());
	m_currPayMoney = money;
    m_currSocketHandle = socket;
    m_currSequence = sequence;
	pthread_mutex_unlock(&m_vehMutex);
	return 0;
}

int EtcRsu::SetTcpSrvHandle(AsioTcpServer *tcpHandle)
{
	printf("tcpHandle=%x\n",tcpHandle);
	m_tcpSrvHandle = tcpHandle;
	return 0;
}
//-------------发送消息格式----------------------------------------------------------------

unsigned char EtcRsu::halfChange(unsigned char v)
{
	unsigned char h1 = v & 0x0f;
	unsigned char h2 = v & 0xf0;
	h1 = h1 << 4;
	h2 = h2 >> 4;
	return (h1|h2);
}

int EtcRsu::sendC0(unsigned char rsctl)
{
	std::vector<unsigned char> buff(sizeof(MSG_C0),0);

	MSG_C0* msgC0;
	msgC0 = (MSG_C0* )buff.data();

	msgC0->RSCTL=halfChange(rsctl);

	msgC0->CMDType = 0xc0;


	//时间戳
	//QDateTime curTime=QDateTime::currentDateTime();
	time_t timep; 
	time(&timep);
	unsigned int timeStamp = (unsigned int)timep; //curTime.secsTo(QDateTime::fromString("1970-01-01 00:00:00","yyyy-MM-dd hh:mm:ss") );
	//unsigned int timeStamp = curTime.toTime_t();
	unsigned char* p=(unsigned char*)&timeStamp;
	msgC0->UnixSeconds[0] =*(3+p);
	msgC0->UnixSeconds[1] =*(2+p);
	msgC0->UnixSeconds[2] =*(1+p);
	msgC0->UnixSeconds[3] =*(p);

	//时间BCD码
	char sCurTime[64];
	char sCurTime1[64];
	strftime(sCurTime, sizeof(sCurTime), "%Y%m%d%H%M%S",localtime(&timep) );

	bool ok=false;
	std::vector<unsigned char> bCurrTime;
	// ok = Str2Bcd(bCurrTime,sCurTime);
	ok = Str2Bcdch(sCurTime1,sCurTime);
	for (int i=0;i<7;i++)
	{
		msgC0->CurrTime[i] = sCurTime1[i];
		// msgC0->CurrTime[i] = bCurrTime.at(i);
	}

	msgC0->LaneMode = 3;

	msgC0->WaitTime =this->mWaitTime;//最小重读时间室外200 室内1

	msgC0->TxPower =this->mTxPower;//功率级数 室外28 室内5
	msgC0->PLLChannelID=0;//信到号
	msgC0->SendFlagInfo=1;//是否处理表示站信息1不处理 0处理

	unsigned char bcc = buff.at(0);
	for (int i = 1;i<buff.size()-1;i++)
	{
		bcc = bcc^buff.at(i);
	}
	msgC0->BCC = bcc;
	printf("%d\n",buff.size());
	/*vector<unsigned char>::iterator it;
	  for(it = buff.begin();it!=buff.end();it++)
	  {
	  printf("%02X",*it);
	  }*/

	WriteSerial(buff);


	return 0;
}

int EtcRsu::sendC1(unsigned char rsctl, unsigned char* OBUID, std::vector<unsigned char>& cardDivFactor)
{
	std::vector<unsigned char> buff(sizeof(MSG_C1),0);
	MSG_C1* msgC1;
	msgC1 = (MSG_C1* )buff.data();
	msgC1->RSCTL=halfChange(rsctl);
	msgC1->CMDType = 0xc1;
	for (int i=0;i<4;i++)
	{
		msgC1->OBUId[i] = OBUID[i];
	}
	/* 一级分散因子  */
	memcpy(msgC1->OBUDivFactor,cardDivFactor.data(),cardDivFactor.size());
	//msgC1->OBUDivFactor = cardDivFactor;
	unsigned char bcc = buff.at(0);
	for (int i = 1;i<buff.size()-1;i++)
	{
		bcc = bcc^buff.at(i);
	}
	msgC1->BCC = bcc;
	WriteSerial(buff);
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER, "C1 -> " + mTool->bin2hex((char*)msgC1,sizeof(MSG_C1)));
	printf("sendC1 done \n");
	return 0;
}

int EtcRsu::sendC2(unsigned char rsctl, char stopType, unsigned char* OBUID)
{
	std::vector<unsigned char> buff(sizeof(MSG_C2),0);
	MSG_C2* msgC2;
	msgC2 = (MSG_C2*)buff.data();
	msgC2->RSCTL=halfChange(rsctl);
	msgC2->CMDType = 0xc2;
	for (int i=0;i<4;i++)
	{
		msgC2->OBUId[i] = OBUID[i];
	}
	msgC2->StopType = stopType;

	unsigned char bcc = buff.at(0);
	for (int i = 1;i<buff.size()-1;i++)
	{
		bcc = bcc^buff.at(i);
	}

	msgC2->BCC = bcc;
	WriteSerial(buff);
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,"C2 -> " + mTool->bin2hex((char*)msgC2,sizeof(MSG_C2)));
	return 0;
}

int EtcRsu::sendC6(unsigned char rsctl, unsigned char* OBUId, int TradeMoney, time_t TradeTime, File0019 f0019, std::vector<unsigned char>& cardDivFactor)
{
	std::vector<unsigned char> buff(sizeof(MSG_C6),0);
	MSG_C6* msgC6;
	msgC6 = (MSG_C6* )buff.data();
	msgC6->RSCTL=halfChange(rsctl);
	msgC6->CMDType = 0xc6;
	for (int i=0;i<4;i++){
		msgC6->OBUID[i] = OBUId[i];
	}
	//交易金额
	unsigned char* p=(unsigned char*)&TradeMoney;
	msgC6->ConsumeMoney[0] =*(3+p);
	msgC6->ConsumeMoney[1] =*(2+p);
	msgC6->ConsumeMoney[2] =*(1+p);
	msgC6->ConsumeMoney[3] =*(p);

	/* 卡一级分散因子 */
	memcpy(msgC6->CardDivFactor,cardDivFactor.data(),cardDivFactor.size());

	//交易时间BCD码
	char sCurTime[64];
	char sCurTime1[64];
	strftime(sCurTime, sizeof(sCurTime), "%Y%m%d%H%M%S",localtime(&TradeTime) );
	printf("sCurTime=%s\n",sCurTime);
	bool ok=false;
	std::vector<unsigned char> bCurrTime;
	//ok = Str2Bcd(bCurrTime,sCurTime);
	ok = Str2Bcdch(sCurTime1,sCurTime);
	for (int i=0;i<7;i++){
		msgC6->PurchaseTime[i] = sCurTime1[i];
		//msgC6->PurchaseTime[i] = bCurrTime.at(i);
	}
	char* pos0019 = (char*)&f0019;
	memcpy(&(msgC6->f0019),pos0019+3,sizeof(f0019)-3 );
	unsigned char bcc = buff.at(0);
	for (int i = 1;i<buff.size()-1;i++){
		bcc = bcc^buff.at(i);
	}
	msgC6->BCC = bcc;
	this->WriteSerial(buff);
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER," C6 -> " + mTool->bin2hex((char*)msgC6,sizeof(MSG_C6)));
	return 0;
}

//===============================================================================================================



std::vector<unsigned char> EtcRsu::beforDealSnd(std::vector<unsigned char>& msg)
{
	std::vector<unsigned char> ret;
	std::vector<unsigned char> aFF(2,0);
	std::vector<unsigned char> aFE(2,0);
	aFF[0] = 0xfe;
	aFF[1]=0x01;
	aFE[0] = 0xfe;
	aFE[1]=0x00;
	unsigned char value;
	for (int i=0;i<msg.size();i++){
		value = msg.at(i);
		if (value==0xff){
			ret.insert(ret.end(), aFF.begin(), aFF.end());
		}
		else if (value== 0xfe){
			ret.insert(ret.end(), aFE.begin(), aFE.end());
		}
		else
			ret.push_back(value);
	}
	return ret;
}


void EtcRsu::WriteSerial(std::vector<unsigned char>& buff){

	//mSerialLock.lockForWrite();
	std::vector<unsigned char> tmp = beforDealSnd(buff);
	std::vector<unsigned char> tmp2;
	tmp2.push_back(0xff) ;
	tmp2.push_back(0xff) ;
	tmp2.insert(tmp2.end(), tmp.begin(), tmp.end()) ;
	tmp2.push_back(0xff) ;
	/* for(int i = 0;i<tmp2.size();i++)
	   {
	   printf("%02X ",tmp2[i]);
	   }*/
	printf("\n");
	mRsuComm.write(tmp2);
	//   mSerialLock.unlock();
}

//------------接收消息处理-------------------------------------------------------------------
void EtcRsu::receiveB0(std::vector<unsigned char>& buff)
{
	MSG_B0* msgB0;
	msgB0 = (MSG_B0*) buff.data();
	if (msgB0->RSCTL == 0x98)
	{
		sendC0(msgB0->RSCTL);
		openAntenna(true);
	}  
	else {  
		openAntenna(true);     
		//sendC1(msgB0->RSCTL);
	}
	m_RSUTerminalld = Bin2Hex(msgB0->RSUTerminalld1, sizeof(msgB0->RSUTerminalld1));
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,QString("天线 PSAM1:%1 PSAM2:%2").arg(mTool->bin2hex((char*)(msgB0->RSUTerminalld1),sizeof(msgB0->RSUTerminalld1))).arg(mTool->bin2hex((char*)(msgB0->RSUTerminalld2),sizeof(msgB0->RSUTerminalld2))));
}

void EtcRsu::receiveB2(std::vector<unsigned char>& buff)
{
	MSG_B2 * msgB2;
	msgB2 = (MSG_B2*)buff.data();
	//QDateTime qTmpTime = QDateTime::currentDateTime();
	//mRsuHardTime = qTmpTime;
	//mLog->loggerInfo(qTmpTime,MOUDLE_ASSISTANT_LOGGER,QString("VST---错误代码:%1;").arg(msgB2->ErrCode));
	if (msgB2->ErrCode == 0x80)
	{       
	}
	else if (msgB2->ErrCode == 0)
	{
		/*  pthread_mutex_lock(&m_vehMutex);
		    if (m_currVehNumer.empty()) //当前没收到扣费请求
		    {
		    printf("not received pay request\n");
		    pthread_mutex_unlock(&m_vehMutex);
		    sendC2(msgB2->RSCTL,1,msgB2->OBUID);
		    return;
		    }
		    pthread_mutex_unlock(&m_vehMutex);
		    */
		unsigned char tmp=0;
		unsigned char obuStatus = msgB2->OBUStatus[0];
		tmp = obuStatus &(unsigned char) 0x80;
		/// * 生成卡一级分散因子*/
		//unsigned char* pVehFactor = mCardFactor.data();
		//memcpy(pVehFactor,msgB2->Issuerldentifier,4);
		//memcpy(pVehFactor+4,msgB2->Issuerldentifier,4);
		if(mCardFactor.size()!=0)
		{
			mCardFactor.clear();
		}
		for(int i = 0;i<4;i++)
		{
			mCardFactor.push_back(msgB2->Issuerldentifier[i]);
		}
		for(int i = 0;i<4;i++)
		{
			mCardFactor.push_back(msgB2->Issuerldentifier[i]);
		}

		if (tmp >0)       ///* 判断OBU是否有卡 */
		{
			//	        printf("send C2 trance\n");
			sendC2(msgB2->RSCTL,1,msgB2->OBUID);
			return;
		}

		//	    printf("send C1 trance\n");
		m_currVehInfo.sOBUID = Bin2Hex(msgB2->OBUID, sizeof(msgB2->OBUID));
		m_currVehInfo.sIssuerldentifier = Bin2Hex(msgB2->Issuerldentifier, sizeof(msgB2->Issuerldentifier));
        m_currVehInfo.sPlateNumber = "";
        m_currVehInfo.CardType = 0;
        m_currVehInfo.PhysicalCardType = 0;
        m_currVehInfo.TransType = 0;
        m_currVehInfo.beforeBlance = 0;
        m_currVehInfo.sCardID = "";
        m_currVehInfo.sCardSerialNo = "";
        m_currVehInfo.sCardNetNo = "";
        m_currVehInfo.sPSAMNo = "";
        m_currVehInfo.sTransTime = "";
        m_currVehInfo.iTAC = 0;
        m_currVehInfo.iICCPayserial = 0;
        m_currVehInfo.iPSAMTransSerial = 0;
        m_currVehInfo.afterBlance = 0;

		///* 判断OBU过期 */ //todo
		std::string  Enable_Data = Bin2Hex(msgB2->Dateoflssue,sizeof(msgB2->Dateoflssue)); 
		std::string  Expiration_Data = Bin2Hex(msgB2->DateofExpire,sizeof(msgB2->DateofExpire)); 
		std::string nowtime = get_local_Time();
		B2_Issuerldentifier = Bin2Hex(msgB2->Issuerldentifier, sizeof(msgB2->Issuerldentifier));    //B2和0015的发卡方做对比
		cout <<"B2_Issuerldentifier:"<<B2_Issuerldentifier<<endl;
		cout << "B2  Enable_Data:" <<Enable_Data <<"          Expiration_Data:" <<Expiration_Data <<"             nowtime:"<<nowtime<<endl;
		if(Enable_Data.compare(nowtime) >= 0 || Expiration_Data.compare(nowtime) <= 0)
		{
			cout <<"OBU已过期"<<endl;
			sendC2(msgB2->RSCTL,1,msgB2->OBUID);
		}	


		log_Time()  ;	    
		sendC1(msgB2->RSCTL,msgB2->OBUID,mCardFactor);  //todo

		//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,QString("VST---标签号:%1;发行号:%2;起始时间:%3;终止时间:%4;错误码:%5;").arg(vehB2OBUID).arg(qVeh->mOBUSerialNo).arg(mTool->getDateTimeByBCD(qVeh->mB2.Dateoflssue,4).toString("yyyy-MM-dd")).arg(mTool->getDateTimeByBCD(qVeh->mB2.DateofExpire,4).toString("yyyy-MM-dd")).arg(msgB2->ErrCode));

	}
	else
	{
		sendC2(msgB2->RSCTL,1,msgB2->OBUID);
	}
}

void EtcRsu::receiveB3(std::vector<unsigned char>& buff)
{
	MSG_B3 * msgB3;
	msgB3 = (MSG_B3*)buff.data();
	///* 日志
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,QString("OBU----标签号:%1;车牌号:%2; 车型:%3").arg(vehB3OBUID).arg(qVeh->mOBUPlate).arg(qVeh->mOBUCarType));
	if(msgB3->ErrorCode != 0) //交易失败
	{
		sendC2(msgB3->RSCTL,1,msgB3->OBUID);
		return;
	}
	//todo 保存车辆信息
	printf("car number=%s\n", msgB3->PlateNumber);
	m_currVehInfo.sPlateNumber = std::string((const char*)msgB3->PlateNumber);//Bin2Hex(msgB3->PlateNumber, sizeof(msgB3->PlateNumber));

	//if(!m_currVehInfo.sPlateNumber.compare(m_currVehNumer)) //todo
	sendC1(msgB3->RSCTL,msgB3->OBUID,mCardFactor);//////////////

	//   pthread_mutex_lock(&m_vehMutex);
	//   m_currVehNumer = "";
	//  pthread_mutex_unlock(&m_vehMutex);

	return;
}

void EtcRsu::receiveB4(std::vector<unsigned char>& buff)
{
	MSG_B4 * msgB4;
	msgB4 = (MSG_B4*)buff.data();
	//QString sOBUID = mTool->obuID2String(msgB4->OBUID);
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,QString("IC---错误代码:%1;").arg(msgB4->ErrorCode));
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,QString("IC卡信息加入队列,OBUID:%1;").arg(sOBUID));  ///*日志
	//
	pthread_mutex_lock(&m_vehMutex);
	if (m_currVehNumer.empty()) //当前没收到扣费请求
	{
		printf("not received pay request\n");
		pthread_mutex_unlock(&m_vehMutex);
		sendC2(msgB4->RSCTL,1,msgB4->OBUID);
		return;
	}
	pthread_mutex_unlock(&m_vehMutex);


	if(msgB4->ErrorCode != 0) //交易失败
	{
		sendC2(msgB4->RSCTL,1,msgB4->OBUID);
		return;
	}

	time_t timep; 
	time(&timep);
	//int tradeMoney=100;  //todo


	///* 检查状态名单  - 黑名单

	///* 获取余额
	unsigned char cardblance[4];
	cardblance[0] = msgB4->CardRestMoney[3];
	cardblance[1] = msgB4->CardRestMoney[2];
	cardblance[2] = msgB4->CardRestMoney[1];
	cardblance[3] = msgB4->CardRestMoney[0];
	uint* itmp  = (uint*)&cardblance;
	m_currVehInfo.beforeBlance =  *itmp;

	m_currVehInfo.CardType = msgB4->f0015.CardType;
	m_currVehInfo.PhysicalCardType = msgB4->PhysicalCardType;
	m_currVehInfo.sCardSerialNo = Bin2Hex((unsigned char*)msgB4->f0015.CardID,sizeof(msgB4->f0015.CardID));
	m_currVehInfo.sCardID = Bin2Hex(msgB4->CardID,sizeof(msgB4->CardID));
	m_currVehInfo.sCardNetNo = Bin2Hex((unsigned char*)msgB4->f0015.CardNetNo,sizeof(msgB4->f0015.CardNetNo));


	//根据0015文件判断卡片是否过期
	std::string  Enable_Data = Bin2Hex((unsigned char*)msgB4->f0015.ContractDate,sizeof(msgB4->f0015.ContractDate)); 
	std::string  Expiration_Data = Bin2Hex((unsigned char*)msgB4->f0015.ExpiredDate,sizeof(msgB4->f0015.ExpiredDate)); 
	std::string nowtime = get_local_Time();
	cout << "B4  Enable_Data:" <<Enable_Data <<"          Expiration_Data:" <<Expiration_Data <<"             nowtime:"<<nowtime<<endl;
	if(Enable_Data.compare(nowtime) >= 0 || Expiration_Data.compare(nowtime) <= 0)
	{
		cout<<"0015文件判断卡片过期"<<endl;
		sendC2(msgB4->RSCTL,1,msgB4->OBUID);

	}

	//根据0015和B2的发卡方进行比较
	std::string  B4_0015_Issuerldentifier = Bin2Hex((unsigned char*)msgB4->f0015.ContractProvider,sizeof(msgB4->f0015.ContractProvider)); 
//	std::string  B4_0015_Issuerldentifier = Bin2Hex(msgB4->f0015.ContractProvider,sizeof(msgB4->f0015.ContractProvider)); 
	cout <<"B4_0015_Issuerldentifier :"<<B4_0015_Issuerldentifier <<"         B2_Issuerldentifier:"<<B2_Issuerldentifier<<endl;
	if(B2_Issuerldentifier.compare(B4_0015_Issuerldentifier) != 0 && B4_0015_Issuerldentifier.compare("0000000000000000") != 0)
	{
		cout <<"B4_0015_Issuerldentifier not Not equal to B2_Issuerldentifier"<<endl;
		sendC2(msgB4->RSCTL,1,msgB4->OBUID);

	}

//	std::string  B4_0015_VehiclePlateNumber = Bin2Hex((unsigned char*)msgB4->f0015.VehiclePlateNumber,sizeof(msgB4->f0015.VehiclePlateNumber)); 
    //    cout << "B4_0015_VehiclePlateNumber:"<<<<endl;
 printf("B4car number=%s\n", msgB4->f0015.VehiclePlateNumber);



	//  sendC6(msgB4->RSCTL,msgB4->OBUID,1,timep,msgB4->f0019,mCardFactor);
	sendC6(msgB4->RSCTL,msgB4->OBUID,m_currPayMoney,timep,msgB4->f0019,mCardFactor);
	pthread_mutex_lock(&m_vehMutex);
	m_currVehNumer = "";
	pthread_mutex_unlock(&m_vehMutex); 
	return;

}

void EtcRsu::receiveB5(std::vector<unsigned char>& buff)
{
	MSG_B5 * msgB5;
	msgB5 = (MSG_B5*)buff.data();
	//QString sOBUID = mTool->obuID2String(msgB5->OBUID);;
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,QString("收到B5交易结果,OBUID:%1;").arg(sOBUID));

	if(msgB5->ErrorCode == 0)
	{
		m_currVehInfo.sPSAMNo = Bin2Hex((unsigned char*) (msgB5->PSAMNo),sizeof(msgB5->PSAMNo));  
		m_currVehInfo.sTransTime = Bin2Hex((unsigned char*) (msgB5->TransTime),sizeof(msgB5->TransTime));  
		m_currVehInfo.TransType = msgB5->TransType;

		///* IC卡交易总数
		unsigned char* tmpValue = (unsigned char *)&(m_currVehInfo.iICCPayserial);
		*(tmpValue+3) = 0;
		*(tmpValue+2) = 0;
		*(tmpValue+1) = msgB5->ICCPayserial[0];
		*(tmpValue)   = msgB5->ICCPayserial[1];

		///* 转换TAC码
		tmpValue = (unsigned char *)&(m_currVehInfo.iTAC);
		*(tmpValue+3) = msgB5->TAC[0];
		*(tmpValue+2) = msgB5->TAC[1];
		*(tmpValue+1) = msgB5->TAC[2];
		*(tmpValue)   = msgB5->TAC[3]; 

		///* PSAM 卡交易总数
		tmpValue = (unsigned char *)&(m_currVehInfo.iPSAMTransSerial);
		*(tmpValue+3) = msgB5->PSAMTransSerial[0];
		*(tmpValue+2) = msgB5->PSAMTransSerial[1];
		*(tmpValue+1) = msgB5->PSAMTransSerial[2];
		*(tmpValue)   = msgB5->PSAMTransSerial[3];
		///* 获取交易后的余额
		unsigned char cardblance[4];
		cardblance[0] = msgB5->CardBalance[3];
		cardblance[1] = msgB5->CardBalance[2];
		cardblance[2] = msgB5->CardBalance[1];
		cardblance[3] = msgB5->CardBalance[0];
		uint* itmp  = (uint*)&cardblance;
		m_currVehInfo.afterBlance = *itmp;

		ResonseTcpSrv(&m_currVehInfo,0);
		//memset(&m_currVehInfo,0,sizeof(VehInfo));

		sendC1(msgB5->RSCTL,msgB5->OBUID,mCardFactor);
		/*qVeh->mWorkName = "交易成功";
		  qVeh->mWorkMode = WMNormal;
		  if(qVeh->mIsLowValue)
		  {
		  qVeh->mWorkName = "余额低值";
		  }
		// IC卡交易总数
		unsigned char* tmpValue = (unsigned char *)&(qVeh->mICCPayserial);
		 *(tmpValue+3) = 0;
		 *(tmpValue+2) = 0;
		 *(tmpValue+1) = msgB5->ICCPayserial[0];
		 *(tmpValue)   = msgB5->ICCPayserial[1];

		// 转换TAC码
		tmpValue = (unsigned char *)&(qVeh->mTAC);
		 *(tmpValue+3) = msgB5->TAC[0];
		 *(tmpValue+2) = msgB5->TAC[1];
		 *(tmpValue+1) = msgB5->TAC[2];
		 *(tmpValue)   = msgB5->TAC[3];
		// 转换TAC码
		qVeh->mTranType = msgB5->TransType;
		// PSAM 卡交易总数
		tmpValue = (unsigned char *)&(qVeh->mPSamTranSerial);
		 *(tmpValue+3) = msgB5->PSAMTransSerial[0];
		 *(tmpValue+2) = msgB5->PSAMTransSerial[1];
		 *(tmpValue+1) = msgB5->PSAMTransSerial[2];
		 *(tmpValue)   = msgB5->PSAMTransSerial[3];
		// 获取交易后的余额
		unsigned char cardblance[4];
		cardblance[0] = msgB5->CardBalance[3];
		cardblance[1] = msgB5->CardBalance[2];
		cardblance[2] = msgB5->CardBalance[1];
		cardblance[3] = msgB5->CardBalance[0];
		uint* itmp  = (uint*)&cardblance;
		qVeh->mLastBlance =  *itmp; */
		///* 交易入库
		///* 如果之前入库，证明MID已经存在则生成新的MID进行入库

	}else{ //交易失败
		printf("B5 fail\n");

	}
	printf("B5 Done\n");
	log_Time(); 
}
//=========================================================================================
static std::vector<unsigned char>::iterator replaceBytesInVector(
		std::vector<unsigned char>& v, const std::vector<unsigned char>& s, const std::vector<unsigned char>& r)
{
	if(s.size()==0) return v.end();

	vector<unsigned char>::iterator it=v.begin();
	for(;;)
	{
		vector<unsigned char>::iterator result=find(it,v.end(),s[0]);
		if(result==v.end()) return v.end();//If not found return

		for(size_t i=0;i<s.size();i++)
		{
			vector<unsigned char>::iterator temp=result+i;
			if(temp==v.end()) return v.end();
			if(s[i]!=*temp) goto mismatch;
		}
		//Found
		if(s.size()>r.size())
		{
			*result=r[0];
			vector<unsigned char>::iterator temp=result+1;
			v.erase(temp);
		}

mismatch:
		it=result+1;
	}
}


bool EtcRsu::beforDealRec(std::vector<unsigned char>& result)
{
	bool ok = false;
	vector<unsigned char> origin,replaced;  //protocol 4.3 特殊字节转义处理
	origin.push_back(0xfe);
	origin.push_back(0x01);
	replaced.push_back(0xff);
	replaceBytesInVector(result,origin,replaced);

	origin.clear();
	replaced.clear();
	origin.push_back(0xfe);
	origin.push_back(0x00);
	replaced.push_back(0xfe);
	replaceBytesInVector(result,origin,replaced);

	unsigned char bcc = result[0];  //异或校验
	for (int i=1;i<result.size()-1;i++){
		bcc = bcc ^ result[i];
	}
	unsigned char bcc1 = result[result.size()-1];
	if (bcc==bcc1)
		ok = true;
	else
	{
		printf("bcc!=bcc1\n");
	}

	return ok;
}

void EtcRsu::run()
{
	bool ok;
	//初始化发C0 由主业务逻辑控制开闭天线
	while(!mStoped)
	{
		std::vector<unsigned char> oneMsg;
		ok = mRsuComm.GetOneMsg(oneMsg);
		//            mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,QString("获取到oneMsg-RSU数据"));


		if (ok)
		{
			ok = this->beforDealRec(oneMsg);
			if (ok)
			{
				/*   for(int i = 0;i<oneMsg.size();i++)
				     {
				     printf("%02X ",oneMsg[i]);
				     }
				     printf("\n");*/
				this->sendMsg2Logic(oneMsg);
			}
		}
		//   else{
		//printf("check err\n");
		usleep(1*1000);
		//  }

	}
	CloseDrv();
}

int EtcRsu::send4C(unsigned char rsctl,unsigned char ante)
{
	std::vector<unsigned char> buff(sizeof(MSG_4C),0);
	MSG_4C* msg4C;
	msg4C =(MSG_4C* ) buff.data();

	msg4C->RSCTL = halfChange(rsctl);
	msg4C->CMDType = 0x4c;
	msg4C->Ante = ante;

	unsigned char bcc = buff.at(0);
	for (int i = 1;i<buff.size()-1;i++){
		bcc = bcc^buff.at(i);
	}
	msg4C->BCC = bcc;
	this->WriteSerial(buff);
	return 0;
}


void EtcRsu::sendMsg2Logic(std::vector<unsigned char>& msg)
{
	unsigned char msgType =  msg.at(1);
	this->mCurrRSCTL = msg.at(0);
	printf("msgtype---%X\n",msg.at(1));
	//mLog->loggerInfo(QDateTime::currentDateTime(),MOUDLE_ASSISTANT_LOGGER,mTool->bin2hex((char*)&msgType,1)+ " -> " + mTool->Array2HexStr(msg));
	if(!mIsRead)
	{
		printf("mIsRead=== false\n");
		if(msgType == 0xb0)
		{
			receiveB0(msg);
			mIsInit = true;

		}
		/*else if((msgType == 0xb2) && ((unsigned char)msg.at(6) == 0x80))
		  {
		  emit receiveB2(msg);
		  }*/
		return;
	}
	switch (msgType)
	{
		case 0xb0:
			receiveB0(msg);
			break;
		case 0xb2:
			receiveB2(msg);
			break;
		case 0xb3:
			receiveB3(msg);
			break;
		case 0xb4:        
			receiveB4(msg);
			break;
		case 0xb5:
			receiveB5(msg);
			break;
		default:
			break;
	}
}

void EtcRsu::openAntenna(bool open)
{
	mIsRead = open;
	//    if (open)
	//    {
	//        this->send4C(this->mCurrRSCTL,0x01);
	//    }
	//    else
	//    {
	//        this->send4C(this->mCurrRSCTL,0x00);
	//    }
}

int EtcRsu::ResonseTcpSrv(VehInfo* vehinfo,int errcode)
{
	char buffer [30];

	XMLDocument* doc = new XMLDocument();
	XMLNode* rootElem = doc->InsertEndChild( doc->NewElement( "MessageInfo" ) );

    XMLNode* typeElem = rootElem->InsertEndChild( doc->NewElement( "sType" ) );
    typeElem->InsertFirstChild(doc->NewText("Transaction"));

    memset(buffer,0,sizeof(buffer));
    sprintf(buffer,"%d",errcode);
    XMLNode* codeElem = rootElem->InsertEndChild( doc->NewElement( "rCode" ) );
    codeElem->InsertFirstChild(doc->NewText(buffer));

    XMLNode* tTypeElem = rootElem->InsertEndChild( doc->NewElement( "tType" ) );
    tTypeElem->InsertFirstChild(doc->NewText("01"));

    XMLNode* dataElem = rootElem->InsertEndChild( doc->NewElement( "Data" ) );

	XMLNode* timeElem = dataElem->InsertEndChild( doc->NewElement( "time" ) );
	timeElem->InsertFirstChild(doc->NewText(vehinfo->sTransTime.c_str()));

	XMLNode* issueridElem = dataElem->InsertEndChild( doc->NewElement( "cardIssuerID" ) );
	issueridElem->InsertFirstChild(doc->NewText(vehinfo->sIssuerldentifier.c_str()));

	XMLNode* chipnoElem = dataElem->InsertEndChild( doc->NewElement( "cardChipNo" ) );
	chipnoElem->InsertFirstChild(doc->NewText(vehinfo->sCardSerialNo.c_str()));

	XMLNode* termElem = dataElem->InsertEndChild( doc->NewElement( "terminalId" ) );
	termElem->InsertFirstChild(doc->NewText(m_RSUTerminalld.c_str()));

	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",vehinfo->iICCPayserial);
	XMLNode* cardSerialElem = dataElem->InsertEndChild( doc->NewElement( "cardSerialNo" ) );
	cardSerialElem->InsertFirstChild(doc->NewText(buffer));

	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",vehinfo->iPSAMTransSerial);
	XMLNode* psamSerialElem = dataElem->InsertEndChild( doc->NewElement( "psamSerialNo" ) );
	psamSerialElem->InsertFirstChild(doc->NewText(buffer));

	XMLNode* cardRndElem = dataElem->InsertEndChild( doc->NewElement( "cardRnd" ) );
	cardRndElem->InsertFirstChild(doc->NewText(""));  //todo

	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%x",vehinfo->iTAC);
	XMLNode* tacElem = dataElem->InsertEndChild( doc->NewElement( "tac" ) );
	tacElem->InsertFirstChild(doc->NewText(buffer));

	XMLNode* cardNetElem = dataElem->InsertEndChild( doc->NewElement( "cardNetNo" ) );
	cardNetElem->InsertFirstChild(doc->NewText(vehinfo->sCardNetNo.c_str()));

	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",vehinfo->beforeBlance);
	XMLNode* transBeforeElem = dataElem->InsertEndChild( doc->NewElement( "transBeforeBalance" ) );
	transBeforeElem->InsertFirstChild(doc->NewText(buffer));

	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",vehinfo->afterBlance);
	XMLNode* balanceElem = dataElem->InsertEndChild( doc->NewElement( "balance" ) );
	balanceElem->InsertFirstChild(doc->NewText(buffer));

	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%02x",vehinfo->TransType);
	XMLNode* transTypeElem = dataElem->InsertEndChild( doc->NewElement( "transType" ) );
	transTypeElem->InsertFirstChild(doc->NewText(buffer));

	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",vehinfo->CardType);
	XMLNode* cardTypeElem = dataElem->InsertEndChild( doc->NewElement( "cardType" ) );
	cardTypeElem->InsertFirstChild(doc->NewText(buffer));

	XMLNode* originalTransactionDateElem = dataElem->InsertEndChild( doc->NewElement( "originalTransactionDate" ) );
	originalTransactionDateElem->InsertFirstChild(doc->NewText(""));

	XMLPrinter printer;
	doc->Print( &printer );
	log_Time();
	printf("m_tcpSrvHandle=%x, send msg:%s\n", m_tcpSrvHandle, printer.CStr());
	if(m_tcpSrvHandle)
    {
        unsigned char* buffer=new unsigned char[printer.CStrSize()+sizeof(MSG_Header)];
        MSG_Header *header = (MSG_Header*)buffer;
        header->identifer = 0xffffffff;
        header->sequence = htonl(m_currSequence);
        header->msglen = htonl(printer.CStrSize()+sizeof(MSG_Header));
        header->msgcmd = htonl(0x1);
        memcpy(buffer+sizeof(MSG_Header), printer.CStr(), printer.CStrSize());
		m_tcpSrvHandle->SendMsg(m_currSocketHandle, (const char*)buffer, printer.CStrSize()+sizeof(MSG_Header));
        delete buffer;
    }

	delete doc;

	return 0;
}

char*   log_Time(void)
{
	/*        struct  tm      *ptm;
		  struct  timeb   stTimeb;
		  static  char    szTime[19];

		  ftime(&stTimeb);
		  ptm = localtime(&stTimeb.time);
		  printf("%02d-%02d %02d:%02d:%02d.%03d",
		  ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, stTimeb.millitm);
		  szTime[18] = 0;
		  */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t ts = (int64_t)tv.tv_sec*1000 + tv.tv_usec/1000; 
	printf("now time is %ld\n",ts);
}

string  get_local_Time(void)                                                                                                                                                           
{
	struct  tm      *ptm;
	struct  timeb   stTimeb;
	static  char    szTime[19];

	ftime(&stTimeb);
	ptm = localtime(&stTimeb.time);

	int year = ptm->tm_year+1900;
	int month = ptm->tm_mon+1;
	int day = ptm->tm_mday;

	char chyear[10];
	char chmonth[10];
	char chday[10];

	sprintf(chyear, "%d", year);
	sprintf(chmonth, "%2d", month);
	sprintf(chday, "%2d", day);

	string syear(chyear);
	string smonth(chmonth);
	string sday(chday);

	string nowtime = syear  + smonth  + sday;
	return nowtime;
}


/*
 //读配置文件初始化配置
void ReadConfigurationFile(EtcRsu etcRsu)  
{  
	XMLDocument doc;  
	doc.LoadFile("bcspftp.xml");  
	XMLElement *scene=doc.RootElement();  
	XMLElement *surface=scene->LastChildElement("SysConfigure");  
	XMLElement *surfaceChild=surface->FirstChildElement();  


	const char* devicechuan;  
	const char* aerialtype; //天线类型 
	const char* aerialpower; //天线功率 
	const char* aerialrate;//天线波特率  
	const char* serialnumber; //天线串口号 
	const char* cardserialnumber; //读卡器串口号 


	const XMLAttribute *attributeOfSurface = surface->FirstAttribute();  
	cout<< attributeOfSurface->Name() << ":" << attributeOfSurface->Value() << endl;  

	//总揽
	devicechuan=surfaceChild->GetText();  
	surfaceChild=surfaceChild->NextSiblingElement();  
	cout<<devicechuan<<endl;  


	//天线类型
	aerialtype=surfaceChild->GetText();  
	surfaceChild=surfaceChild->NextSiblingElement();  
	cout<<aerialtype<<endl;  

	//天线功率
	aerialpower=surfaceChild->GetText();  
	surfaceChild=surfaceChild->NextSiblingElement();  
	cout<<aerialpower<<endl;  

	//天线波特率
	aerialrate=surfaceChild->GetText();  
	surfaceChild=surfaceChild->NextSiblingElement();  
	cout<<aerialrate<<endl;  


	//天线串口号
	serialnumber=surfaceChild->GetText();  
	surfaceChild=surfaceChild->NextSiblingElement();  
	cout<<serialnumber<<endl;  


	//读卡器串口号
	cardserialnumber=surfaceChild->GetText();  
	cout<<cardserialnumber<<endl;  



}*/ 
