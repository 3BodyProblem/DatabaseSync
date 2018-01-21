#ifndef __DATABASE_DBOPERATION_H__
#define __DATABASE_DBOPERATION_H__


#pragma warning(disable:4786)
#include <windows.h>
#include <winsock.h>
#include <string>
#include <fstream>
#include "../Infrastructure/DateTime.h"
#include <mysql.h>


/**
 * @class			QuotationDatabase
 * @brief			行情数据库更新类
 * @author			barry
 */
class QuotationDatabase
{
private:
	QuotationDatabase();

public:
	~QuotationDatabase();
	static QuotationDatabase&	GetDbObj();

	/**
	 * @brief					初始化数据库对象
	 * @return					==0						初始化成功
								!=0						失败
	 */
	int							Initialize();

	/**
	 * @brief					释放资源
	 */
	void						Release();

	/**
	 * @brief					建立数据库连接
	 * @return					==0						成功
								!=0						失败
	 */
	int							EstablishConnection();

public:
	/**
	 * @brief					新增一条商品记录
	 * @param[in]				nType					商品类型
	 * @param[in]				nExchangeID				市场代码, '0'：未知 '1'：SSE（上海证劵交易所） '2'：SZSE（深圳证劵交易所） '3'：cffEX（中国金融期货交易） '4'：dcE （大连商品期货交易所） '5'：ZcE（郑州商品期货交易所） '6'：SHfE （上海期货交易所）
	 * @param[in]				pszCode					商品代码
	 * @param[in]				pszName					商品名称
	 * @param[in]				nLotSize				最小交易数量（手比率）
	 * @param[in]				nContractMulti			合约乖数
	 * @param[in]				dPriceTick				最小价格变动单位
	 * @param[in]				dPreClose				昨收价
	 * @param[in]				dPreSettle				昨结价
	 * @param[in]				dUpperPrice				涨停价格
	 * @param[in]				dLowerPrice				跌停价格
	 * @param[in]				dPrice					当前价格
	 * @param[in]				dOpenPrice				开盘价格
	 * @param[in]				dSettlePrice			今结价格
	 * @param[in]				dClosePrice				今收价格
	 * @param[in]				dBid1Price				买一价
	 * @param[in]				dAsk1Price				卖一价
	 * @param[in]				dHighPrice				最高价
	 * @param[in]				dLowPrice				最低价
	 * @param[in]				dFluctuationPercent		涨跌幅度(用收盘价计算)
	 * @param[in]				nVolume					成交量
	 * @param[in]				nTradingVolume			现量
	 * @param[in]				dAmount					成交金额
	 * @param[in]				nIsTrading				是否交易标记
	 * @param[in]				nTradingDate			交易日期
	 * @param[in]				dUpLimit				上限金额
	 * @param[in]				pszClassID				分类ID
	 * @return					返回insert into/replace语句返回的affect number数量
	 */
	int							Replace_Commodity( short nTypeID, short nExchangeID, const char* pszCode, const char* pszName
												, int nLotSize, int nContractMulti, double dPriceTick, double dPreClose, double dPreSettle
												, double dUpperPrice, double dLowerPrice, double dPrice, double dOpenPrice, double dSettlePrice
												, double dClosePrice, double dBid1Price, double dAsk1Price, double dHighPrice, double dLowPrice
												, double dFluctuationPercent, unsigned __int64 nVolume, unsigned __int64 nTradingVolume, double dAmount
												, short nIsTrading, unsigned int nTradingDate, double dUpLimit, const char* pszClassID );

	/**
	 * @brief					更新一条商品快照信息
	 * @param[in]				nExchangeID				市场代码, '0'：未知 '1'：SSE（上海证劵交易所） '2'：SZSE（深圳证劵交易所） '3'：cffEX（中国金融期货交易） '4'：dcE （大连商品期货交易所） '5'：ZcE（郑州商品期货交易所） '6'：SHfE （上海期货交易所）
	 * @param[in]				pszCode					商品代码
	 * @param[in]				dPreClosePx				昨收价格
	 * @param[in]				dPreSettlePx			昨结价格
	 * @param[in]				dUpperPx				涨停价格
	 * @param[in]				dLowerPx				跌停价格
	 * @param[in]				dLastPx					最新价格
	 * @param[in]				dSettlePx				结算价格
	 * @param[in]				dOpenPx					开盘价格
	 * @param[in]				dClosePx				收盘价格
	 * @param[in]				dBid1Px					买一价格
	 * @param[in]				dAsk1Px					卖一价格
	 * @param[in]				dHighPx					最高价格
	 * @param[in]				dLowPx					最低价格
	 * @param[in]				dAmount					成交金额
	 * @param[in]				nVolume					成交总量
	 * @param[in]				nTradingVolume			现量
	 * @param[in]				dFluctuationPercent		涨跌幅度(用收盘价计算)
	 * @param[in]				nIsTrading				是否交易标记
	 */
	int							Update_Commodity( short nExchangeID, const char* pszCode, double dPreClosePx, double dPreSettlePx, double dUpperPx, double dLowerPx, double dLastPx, double dSettlePx
												, double dOpenPx, double dClosePx, double dBid1Px, double dAsk1Px, double dHighPx, double dLowPx, double dAmount, unsigned __int64 nVolume, unsigned int nTradingVolume
												, double dFluctuationPercent, short nIsTrading );

protected:
	/**
	 * @brief					执行sql语句
	 * @param[in]				pszSqlCmd				sql语句
	 * @return					==0						执行成功
								!=0						失败
	 */
	__inline int				ExecuteSql( const char* pszSqlCmd );

protected:
	MYSQL						m_oMySqlHandle;			///< mysql访问句柄
	MYSQL*						m_pMysqlConnection;		///< connection handle
};


#endif







