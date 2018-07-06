
#ifndef CPUFILE_H
#define CPUFILE_H
struct File0015{
    unsigned char ContractProvider[8];//发行商代码
    unsigned char CardType;//22储值卡
    unsigned char CardVer;
    unsigned char CardNetNo[2];
    unsigned char CardID[8];//
    unsigned char ContractDate[4];//启用日
    unsigned char ExpiredDate[4];//过期日
    unsigned char VehiclePlateNumber[12];//车牌
    unsigned char UserType;//0:普通用户 1：车卡绑定
    unsigned char FCI[9];
};


struct File0019{
    unsigned char ComplexFlag;
    unsigned char RecordSize;//0x25
    unsigned char LockFlag;
    unsigned char RoadNet[2];//
    unsigned char Plaza[2];//

    unsigned char Lane;
    unsigned char Time[4];//unix 从1970-1-1 00:00:00开始
    unsigned char VehicleClass;//
    unsigned char CardFlowStatus;// 01 mtc 入 03 ETC 入 04 ETC出
    unsigned char FlagSite[9];//标识站
    unsigned char CollectorID[3];//
    unsigned char CollectorShift;//入口班次
    unsigned char VehiclePlateNumber[12];//车牌
    unsigned char other[4];
};
#endif // CPUFILE_H
