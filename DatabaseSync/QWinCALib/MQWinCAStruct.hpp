//-----------------------------------------------------------------------------------------------------------------------------
//                `-.   \    .-'
//        ,-"`````""-\__ |  /							文件名称：MQWinCAStruct
//         '-.._    _.-'` '-o,							文件描述：QWinCA数据结构
//             _>--:{{<   ) |)							文件编写：Lumy
//         .-''      '-.__.-o`							创建日期：2017.04.03
//        '-._____..-/`  |  \							更新日期：2017.04.03
//                ,-'   /    `-.
//-----------------------------------------------------------------------------------------------------------------------------
#ifndef __QWinCA_MQWinCAStruct_HPP__
#define __QWinCA_MQWinCAStruct_HPP__
//-----------------------------------------------------------------------------------------------------------------------------
#pragma pack(1)
//.............................................................................................................................
typedef struct											//QWIN产品信息记录
{
	unsigned int				uiProductID;			//产品编号
	char						szProductName[32];		//产品名称
	char						szRightInfo[300];		//权限描述【以逗号','分隔】
	unsigned int				uiStartDate;			//开始日期【YYYYMMDD格式】
	unsigned int				uiEndDate;				//结束日期【YYYYMMDD格式】
	unsigned int				uiLimitCount;			//限制数量【最大在线终端数量】
	char						szMarket[64];			//支持市场【以逗号','分隔】
														//1:沪深交易所现货（包括股票、基金、债券等）
														//2:沪深交易所期权（包括ETF期权）
														//3:中金所股指期货
														//4:中金所股指期权
														//5:国内商品交易所商品期货
														//6:国内商品交易所商品期权
	char						szReserved[100];		//保留
} tagQWinCA_ProductRecord;
//.............................................................................................................................
typedef struct											//QWIN产品信息 
{
	tagQWinCA_ProductRecord		sProductRecord[15];		//产品信息
	unsigned int				uiProductRecordCount;	//产品数量
	char						szCheckCode[16];		//产品信息校验
} tagQWinCA_ProductInfo;
//.............................................................................................................................
typedef struct											//QWIN证书 
{
	unsigned int				uiVersion;				//证书版本
	unsigned int				uiCustomID;				//客户编号
	char						szCustomName[64];		//客户名称
	unsigned int				uiCreateDate;			//证书创建日期【YYYYMMDD格式】
	unsigned int				uiCreateTime;			//证书创建时间【HHMMSS格式】
	unsigned int				uiDeadDate;				//证书有效日期【YYYYMMDD格式】
	unsigned int				uiMachineCode;			//机器码
	tagQWinCA_ProductInfo		sProductInfo;			//产品信息
	char						szReserved[388];		//保留
	char						szCheckCode[16];		//证书校验
} tagQWinCA_Certificate;
//.............................................................................................................................
typedef struct											//QWIN硬件锁信息
{
	unsigned int				uiVersion;				//信息版本
	unsigned int				uiCustomID;				//客户编号
	char						szCustomName[64];		//客户名称
	unsigned int				uiCreateDate;			//证书创建日期【YYYYMMDD格式】
	unsigned int				uiCreateTime;			//证书创建时间【HHMMSS格式】
	char						szPassword[16];			//客户秘钥
	char						szReserved[16];			//保留
	char						szCheckCode[16];		//证书校验
} tagQWinCA_HardLockInfo;
//.............................................................................................................................
#pragma pack()
//-----------------------------------------------------------------------------------------------------------------------------
class MQWinCA_Interface									//QWinCA提供接口
{
public:
	//获取本机机器码
	virtual unsigned int GetMachineCode(void) = 0;
	//获取当前QWinCA证书信息
	virtual bool GetQWinCA(const char * szQWinPath,tagQWinCA_Certificate * lpOut,char * szErrorString,unsigned int uiErrorSize) = 0;
	//获取硬件锁信息
	virtual bool ReadHardLockInfo(tagQWinCA_HardLockInfo * lpOut,char * szErrorString,unsigned int uiErrorSize) = 0;
};
//.............................................................................................................................
//创建一个QWINCA接口对象
extern MQWinCA_Interface * QWinCA_CreateObject(void);
//释放一个QWinCA接口对象
extern void QWinCA_DeleteObject(MQWinCA_Interface * lpObject);
//-----------------------------------------------------------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------------------------------------------------------