#ifndef RSUMSG_H
#define RSUMSG_H
//#include <QString>
//#include <QByteArray>
//#include <QDateTime>

//struct File0019{
//    unsigned char ComplexFlag;
//    unsigned char RecordSize;//0x25
//    unsigned char LockFlag;
//    unsigned char RoadNet[2];//
//    unsigned char Plaza[2];//
//    unsigned char Lane;
//    unsigned char Time[4];//unix 从1970-1-1 00:00:00开始
//    unsigned char VehicleClass;//
//    unsigned char CardFlowStatus;// 01 mtc 入 03 ETC 入 04 ETC出
//    unsigned char FlagSite[9];//标识站
//    unsigned char CollectorID[3];//
//    unsigned char CollectorShift;//入口班次
//    unsigned char VehiclePlateNumber[12];//车牌
//    unsigned char other[4];

//};

struct File0019{
    unsigned char ComplexFlag;
    unsigned char TotalLength;
    unsigned char LockFlag;
    unsigned char EnRoadNet[2];//
    unsigned char EnPlaza[2];//
    unsigned char EnLane;
    unsigned char EnTime[4];//unix 从1970-1-1 00:00:00开始
    unsigned char EnVehicleClass;//
    unsigned char CardFlowStatus;// 01 mtc 入 03 ETC 入 04 ETC出
    unsigned char FlagSite[9];//标识站
    unsigned char EnOpreatorID[3];//
    unsigned char EnShift;//入口班次
    unsigned char VehiclePlateNumber[12];//车牌
};

struct File0015{
    unsigned char ContractProvider[8];//发行商代码
    unsigned char CardType;
    unsigned char CardVer;
    unsigned char CardNetNo[2];
    unsigned char CardID[8];//
    unsigned char ContractDate[4];//启用日
    unsigned char ExpiredDate[4];//过期日
    unsigned char VehiclePlateNumber[12];//车牌
    unsigned char UserType;//0,6:免费用户
    unsigned char other[2];
};

struct MSG_4C{
    unsigned char RSCTL;
    unsigned char CMDType;
    unsigned char Ante;        /* 天线开关 1远区天线  2近区天线 */
    unsigned char BCC;
};

struct MSG_C0{
    unsigned char RSCTL;
    unsigned char CMDType;
    unsigned char UnixSeconds[4];
    unsigned char CurrTime[7];
    unsigned char LaneMode;
    unsigned char WaitTime;      /* 最小重读时间 */
    unsigned char TxPower;       /* 功率级数 */
    unsigned char PLLChannelID;  /* 信到号 */
    unsigned char SendFlagInfo;  /* 是否处理表示站信息 0不处理 1处理 */
    unsigned char BCC;
};

struct MSG_C1{
    unsigned char RSCTL;
    unsigned char CMDType;
    unsigned char OBUId[4];
    unsigned char OBUDivFactor[8];  /* 分散因子 */
    unsigned char BCC;
};

struct MSG_C2{
    unsigned char RSCTL;
    unsigned char CMDType;
    unsigned char OBUId[4];
    unsigned char StopType;
    unsigned char BCC;
};

struct MSG_C3{
    unsigned char RSCTL;
    unsigned char CMDType;
    unsigned char OBUId[4];
    unsigned char CardDivFactor[8];
    unsigned char TransSerial[4];
    unsigned char PurchaseTime[7];
    unsigned char f0009[36];
    unsigned char BCC;
};

struct MSG_C6{
    unsigned char RSCTL;
    unsigned char CMDType;
    unsigned char OBUID[4];//
    unsigned char CardDivFactor[8];
    unsigned char TransSerial[4];
    unsigned char ConsumeMoney[4];
    unsigned char PurchaseTime[7];
    unsigned char f0019[36];
    unsigned char BCC;
};

struct MSG_B0
{
    unsigned char RSCTL;
    unsigned char RSUStatus;
    unsigned char PSAMNum;
    unsigned char RSUTerminalld1[6];    /* PSAM卡终端机编号，国标卡PSAM卡号 */
    unsigned char RSUTerminalld2[6];    /* PSAM卡终端编号，一卡通PSAM卡号 */
    unsigned char RSUAlgld;             /* 算法标识 */
    unsigned char RSUManulD[2];         /* RSU厂商代码,16进制表示 */
    unsigned char RSUID[2];             /* RSU编号，16进制表示 */
    unsigned char RSUVersion[2];        /* RUS 版本号 16进制表示 */
    unsigned char Reserved[5];          /* 保留字节 */
    unsigned char BCC;
};
struct MSG_B2
{
    unsigned char RSCTL;
    unsigned char FrameType;
    unsigned char OBUID[4];
    unsigned char ErrCode;
    unsigned char Issuerldentifier[8]; /* 发行商代码 */
    unsigned char SerialNumber[8];     /* 应用序列号 */
    unsigned char Dateoflssue[4];      /* 启用日期 */
    unsigned char DateofExpire[4];     /* 过期日期 */
    unsigned char EquitmentStatus;  /* 设备类型 */
    unsigned char OBUStatus[2];
    unsigned char BCC;
};

struct MSG_B3
{
    unsigned char RSCTL;
    unsigned char FrameType;   /* B3 */
    unsigned char OBUID[4];
    unsigned char ErrorCode;
    unsigned char PlateNumber[12]; /* 车牌号码 */
    unsigned char PlateColor[2];   /*  车牌颜色 */
    unsigned char VehicleClass;    /* 车种 */
    unsigned char VehicieUserType; /* 车辆用户类型 */
    unsigned char BCC;
};
struct MSG_B4
{
    unsigned char RSCTL;
    unsigned char FrameType;          /* B4 */
    unsigned char OBUID[4];
    unsigned char ErrorCode;          /* 0x00有效 */
    unsigned char CardType;           /* 00国标卡   01一卡通 */
    unsigned char PhysicalCardType;   /* 物理卡类型，0x00：国标CPU卡   0x01：MIFARE 1(S50) 卡   0x02: MIFARE PRO(或兼容)卡   0x03:Ultra Light  0x05: MIFARE 丁 (S70)卡  */
    unsigned char TransType;          /* 交易类型， 00-传统交易， Ox10】复合父易 只用于国标 CPU 卡，一卡迪为 00 */
    unsigned char CardRestMoney[4];   /* 卡余额，高位在前，低位在后 */
    unsigned char CardID[4];          /* 一卡迪和 CPU 卡的物理卡号 C CPU 卡可以没有，暂填 0 */
    File0015 f0015;
    File0019 f0019;
    unsigned char BCC;
};
struct MSG_B5
{
    unsigned char RSCTL;
    unsigned char FrameType;         /* B5 */
    unsigned char OBUID[4];
    unsigned char ErrorCode;
    unsigned char WrFileTime[4];    /* 写文件时间 */
    unsigned char PSAMNo[6];        /* PSAM 卡终端机编号 */
    unsigned char TransTime[7];     /* 交易时间，格式 YYYYMMDDHHMMSS */
    unsigned char TransType;        /* 交易类型 */
    unsigned char TAC[4];
    unsigned char ICCPayserial[2];
    unsigned char PSAMTransSerial[4];
    unsigned char CardBalance[4];   /* PSAM 卡交 易序号，记 11段卡填充 0; */
    unsigned char BCC;
};

/*
struct CardTradeInfo   //MTC、ETC刷卡交易使用
{
    QDateTime   TradeTime;
    uint        TAC;
    QString     CardCSN;//IC卡物理编号
    QString     CardNetId;
    QString     CardId;//用户卡内部编号
    QString     IssuerIdTrade;//交易用
    int         CardVer;
    uint         blance;//读卡时的
    uint         lastBlance;
    int         CardTradeNum;//卡交易计数
    QString     PSamId;
    int         PSamIdTradeNum;
    QByteArray  f0019;
    QByteArray  f0015;

    int TradeType;//交易类型
    bool ModifyTransaction;//修改交易
};
*/

#endif // RSUMSG_H
