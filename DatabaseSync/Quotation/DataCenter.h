#ifndef __DATA_CENTER_H__
#define __DATA_CENTER_H__


#pragma warning(disable:4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "../QuoteCltDef.h"
#include "../Configuration.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"
#include "../Infrastructure/LoopBuffer.h"


#pragma pack(1)
/**
 * @class						T_TICK_LINE
 * @brief						Tick线
 */
typedef struct
{
	unsigned char				Type;					///< 类型
	char						eMarketID;				///< 市场ID
	char						Code[16];				///< 商品代码
	unsigned int				Date;					///< YYYYMMDD（如20170705）
	unsigned int				Time;					///< HHMMSSmmm（如093005000)
	double						PreClosePx;				///< 昨收价
	double						PreSettlePx;			///< 昨结价
	double						OpenPx;					///< 开盘价
	double						HighPx;					///< 最高价
	double						LowPx;					///< 最低价
	double						ClosePx;				///< 收盘价
	double						NowPx;					///< 现价
	double						SettlePx;				///< 结算价
	double						UpperPx;				///< 涨停价
	double						LowerPx;				///< 跌停价
	double						Amount;					///< 成交额
	unsigned __int64			Volume;					///< 成交量(股/张/份)
	unsigned __int64			OpenInterest;			///< 持仓量(股/张/份)
	unsigned __int64			NumTrades;				///< 成交笔数
	double						BidPx1;					///< 委托买盘一价格
	unsigned __int64			BidVol1;				///< 委托买盘一量(股/张/份)
	double						BidPx2;					///< 委托买盘二价格
	unsigned __int64			BidVol2;				///< 委托买盘二量(股/张/份)
	double						AskPx1;					///< 委托卖盘一价格
	unsigned __int64			AskVol1;				///< 委托卖盘一量(股/张/份)
	double						AskPx2;					///< 委托卖盘二价格
	unsigned __int64			AskVol2;				///< 委托卖盘二量(股/张/份)
	double						Voip;					///< 基金模净、权证溢价
	char						TradingPhaseCode[12];	///< 交易状态
	char						PreName[8];				///< 证券前缀
} T_TICK_LINE;

/**
 * @class						T_LINE_PARAM
 * @brief						基础参数s
 */
typedef struct
{
	///< ---------------------- 参考基础信息 ----------------------------------
	char						Code[16];				///< 商品代码
	char						Name[32];				///< 商品名称
	unsigned short				ExchangeID;				///< 交易所ID
	unsigned char				Type;					///< 类型
	unsigned int				LotSize;				///< 手比率(最小交易数量)
	unsigned int				ContractUnit;			///< 合约单位
	unsigned int				ContractMulti;			///< 合约乖数
	double						PriceTick;				///< 最小价格变动单位
	double						UpperPrice;				///< 涨停价
	double						LowerPrice;				///< 跌停价
	double						PreClosePx;				///< 昨收
	double						PreSettlePx;			///< 昨结
	double						PrePosition;			///< 昨持
	char						PreName[8];				///< 前缀
	double						PriceRate;				///< 放大倍数
	double						FluctuationPercent;		///< 涨跌幅度(用收盘价计算)
	unsigned __int64			Volume;					///< 成交量
	unsigned __int64			TradingVolume;			///< 现量
	short						IsTrading;				///< 是否交易标记
	unsigned int				TradingDate;			///< 交易日期
	char						ClassID[12];			///< 分类ID
} T_LINE_PARAM;
#pragma pack()


typedef	std::map<enum XDFMarket,int>					TMAP_MKID2STATUS;			///< 各市场模块状态
const	unsigned int									MAX_WRITER_NUM = 128;		///< 最大落盘文件句柄
typedef std::map<std::string,T_LINE_PARAM>				T_MAP_QUO;					///< 行情数据缓存
extern unsigned int										s_nNumberInSection;			///< 一个市场有可以缓存多少个数据块(可配)


/**
 * @class						CacheAlloc
 * @brief						日线缓存
 */
class CacheAlloc
{
private:
	/**
	 * @brief					构造函数
	 * @param[in]				eMkID			市场ID
	 */
	CacheAlloc();

public:
	~CacheAlloc();

	/**
	 * @brief					获取单键
	 */
	static CacheAlloc&			GetObj();

	/**
	 * @brief					获取缓存地址
	 */
	char*						GetBufferPtr();

	/**
	 * @brief					获取缓存中有效数据的长度
	 */
	unsigned int				GetDataLength();

	/**
	 * @brief					获取缓存最大长度
	 */
	unsigned int				GetMaxBufLength();

	/**
	 * @brief					为一个商品代码分配缓存对应的专用缓存
	 * @param[in]				eMkID			市场ID
	 * @param[out]				nOutSize		输出为这一个商品分配的缓存的大小
	 */
	char*						GrabCache( enum XDFMarket eMkID, unsigned int& nOutSize );

	/**
	 * @brief					释放缓存
	 */
	void						FreeCaches();

private:
	CriticalObject				m_oLock;					///< 临界区对象
	unsigned int				m_nMaxCacheSize;			///< 行情缓存总大小
	unsigned int				m_nAllocateSize;			///< 当前使用的缓存大小
	char*						m_pDataCache;				///< 行情数据缓存地址
};


/**
 * @brief					连接路径和文件名(处理了是否路径有带分隔符的问题)
 * @param[in]				sPath				保存路径
 * @param[in]				sFileName			文件名
 * @detail					不需要考虑sPath是否以/或\结束
 */
std::string	JoinPath( std::string sPath, std::string sFileName );


/**
 * @brief					生成文件名
 * @param[in]				sFolderPath			保存路径
 * @param[in]				sFileName			文件名
 * @param[in]				nMkDate				日期( 通过日期生成 -> 文件扩展名(星期的索引值) )
 * @note					本函数生成的文件名，可以通过“星期”，保证最多只生成七个文件名，保证不占用过多磁盘空间
 */
std::string	GenFilePathByWeek( std::string sFolderPath, std::string sFileName, unsigned int nMkDate );


/**
 * @class						QuotationData
 * @brief						行情数据存储类
 */
class QuotationData
{
public:
	QuotationData();
	~QuotationData();

	/**
	 * @brief					初始化
	 * @return					==0				成功
								!=				失败
	 */
	int							Initialize( void* pQuotation );

	/**
	 * @brief					释放资源
	 */
	void						Release();

	/**
	 * @brief					更新模块工作状态
	 * @param[in]				cMarket			市场ID
	 * @param[in]				nStatus			状态值
	 */
	void						UpdateModuleStatus( enum XDFMarket eMarket, int nStatus );

	/**
	 * @brief					获取市场模块状态
	 */
	short						GetModuleStatus( enum XDFMarket eMarket );

public:
	/**
	 * @brief					更新商品的参数信息
	 * @param[in]				eMarket			市场ID
	 * @param[in]				sCode			商品代码
	 * @param[in]				refParam		行情参数
	 * @return					==NULL			建立失败
								!=NULL			建立成功，返回对应的结构指针
	 */
	T_LINE_PARAM*				BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam );

	/**
	 * @brief					更新前缀
	 * @param[in]				eMarket			市场ID
	 * @param[in]				pszPreName		前缀内容
	 * @param[in]				nSize			内容长度
	 */
	int							UpdatePreName( enum XDFMarket eMarket, std::string& sCode, char* pszPreName, unsigned int nSize );

	/**
	 * @brief					更新日线信息
	 * @param[in]				pSnapData		快照指针
	 * @param[in]				nSnapSize		快照大小
	 * @param[out]				refTickLine		输出tick数据
	 * @param[in]				nTradeDate		交易日期
	 * @return					==0				成功
	 */
	int							UpdateTickLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, T_TICK_LINE& refTickLine, unsigned int nTradeDate = 0 );

protected:
	static void*	__stdcall	ThreadSyncSnapshot( void* pSelf );			///< 日线落盘线程

protected:
	TMAP_MKID2STATUS			m_mapModuleStatus;				///< 模块状态表
	CriticalObject				m_oLock;						///< 临界区对象
	void*						m_pQuotation;					///< 行情对象
protected:
	T_MAP_QUO					m_mapSHL1;						///< 上证L1
	T_MAP_QUO					m_mapSHOPT;						///< 上证期权
	T_MAP_QUO					m_mapSZL1;						///< 深证L1
	T_MAP_QUO					m_mapSZOPT;						///< 深证期权
	T_MAP_QUO					m_mapCFF;						///< 中金期货
	T_MAP_QUO					m_mapCFFOPT;					///< 中金期权
	T_MAP_QUO					m_mapCNF;						///< 商品期货(上海/郑州/大连)
	T_MAP_QUO					m_mapCNFOPT;					///< 商品期权(上海/郑州/大连)
protected:
	SimpleThread				m_oThdTickDump;					///< Tick落盘数据
};




#endif










