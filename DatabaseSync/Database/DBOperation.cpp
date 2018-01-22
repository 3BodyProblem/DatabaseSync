#pragma warning(disable : 4996)
#include "DBOperation.h"
#include "../DatabaseSync.h"


QuotationDatabase::QuotationDatabase()
 : m_pMysqlConnection( NULL )
{
}

QuotationDatabase::~QuotationDatabase()
{
	Release();
}

QuotationDatabase& QuotationDatabase::GetDbObj()
{
	static QuotationDatabase	obj;

	return obj;
}

int QuotationDatabase::Initialize()
{
	MYSQL*	pRetHandle = ::mysql_init( &m_oMySqlHandle );

	if( NULL == pRetHandle )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 call func. ::mysql_init(), %s", ::mysql_error( &m_oMySqlHandle ) );
		return -1;
	}

	return 0;
}

void QuotationDatabase::Release()
{
	if( NULL != m_pMysqlConnection )
	{
		m_pMysqlConnection = NULL;
		::mysql_close( &m_oMySqlHandle );
	}
}

int QuotationDatabase::EstablishConnection()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationDatabase::EstablishConnection() : building database connection ......" );

	m_pMysqlConnection = ::mysql_real_connect( &m_oMySqlHandle, "192.168.3.171", "potc", "potc", "potc", 5873, NULL, 0 );

	if( NULL == m_pMysqlConnection )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 establish database connection(%s) ......", ::mysql_error( &m_oMySqlHandle ) );
		return -1;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationDatabase::EstablishConnection() : database connection has been established ......" );

	if( 0 != ::mysql_select_db( &m_oMySqlHandle, "potc" ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 select table, (%s) ......", ::mysql_error( &m_oMySqlHandle ) );
		return -2;
	}

	if( 0 < ExecuteSql( "set names gbk;" ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 set mysql client character set 2 gbk, (%s) ......", ::mysql_error( &m_oMySqlHandle ) );
		return -3;
	}

	return 0;
}

int QuotationDatabase::ExecuteSql( const char* pszSqlCmd )
{
	if( NULL != m_pMysqlConnection && NULL != pszSqlCmd )
	{
		int	nRet = ::mysql_query( &m_oMySqlHandle, pszSqlCmd );

		if( nRet < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::ExecuteSql() : errorcode = %d, [ERROR] %s", nRet, ::mysql_error( &m_oMySqlHandle ) );
		}

		return nRet;
	}

	return -1024;
}

int QuotationDatabase::Replace_Commodity( short nTypeID, short nExchangeID, const char* pszCode, const char* pszName
										, int nLotSize, int nContractMulti, double dPriceTick, double dPreClose, double dPreSettle
										, double dUpperPrice, double dLowerPrice, double dPrice, double dOpenPrice, double dSettlePrice
										, double dClosePrice, double dBid1Price, double dAsk1Price, double dHighPrice, double dLowPrice
										, double dFluctuationPercent, unsigned __int64 nVolume, unsigned __int64 nTradingVolume, double dAmount
										, short nIsTrading, unsigned int nTradingDate, double dUpLimit, const char* pszClassID )
{
	char		pszSqlCmd[1024*2] = { 0 };
	char		pszTradingDate[64] = { 0 };

	::sprintf( pszTradingDate, "%d-%d-%d", nTradingDate/10000, nTradingDate%10000/100, nTradingDate%100 );

	if( 0 >= ::sprintf( pszSqlCmd
					, "REPLACE INTO commodity (typeId,exchange,code,name,lotSize,contractMulti,priceTick,preClose,preSettle,upperPrice,lowerPrice,price,openPrice,settlementPrice,closePrice,bid1,ask1,highPrice,lowPrice,uplowPercent,volume,nowVolumn,amount,isTrading,tradingDate,upLimit,classId,addTime,updatetime) VALUES (%d,%d,\'%s\',\'%s\',%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%I64d,%I64d,%f,%d,\'%s\',0,\'%s\',now(),now());"
					, nTypeID, nExchangeID, pszCode, pszName, nLotSize, nContractMulti, dPriceTick, dPreClose, dPreSettle, dUpperPrice, dLowerPrice, dPrice, dOpenPrice, dSettlePrice, dClosePrice, dBid1Price, dAsk1Price, dHighPrice, dLowPrice, dFluctuationPercent, nVolume, nTradingVolume, dAmount, nIsTrading
					, pszTradingDate, pszClassID ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Replace_Commodity() : ::sprintf() invalid return value." );
		return -1024*2;
	}

	return ExecuteSql( pszSqlCmd );
}

int QuotationDatabase::Update_Commodity( short nExchangeID, const char* pszCode, double dPreClosePx, double dPreSettlePx, double dUpperPx, double dLowerPx, double dLastPx, double dSettlePx
										, double dOpenPx, double dClosePx, double dBid1Px, double dAsk1Px, double dHighPx, double dLowPx, double dAmount, unsigned __int64 nVolume, unsigned int nTradingVolume
										, double dFluctuationPercent, short nIsTrading )
{
	char		pszSqlCmd[1024*2] = { 0 };

	if( 0 >= ::sprintf( pszSqlCmd
					, "UPDATE commodity SET preClose=%f,preSettle=%f,upperPrice=%f,lowerPrice=%f,price=%f,openPrice=%f,settlementPrice=%f,closePrice=%f,bid1=%f,ask1=%f,highPrice=%f,lowPrice=%f,uplowPercent=%f,volume=%I64d,nowVolumn=%d,amount=%f,isTrading=%d,addTime=now(),updatetime=now() WHERE exchange=%d AND code=\'%s\';"
					, dPreClosePx, dPreSettlePx, dUpperPx, dLowerPx, dLastPx, dOpenPx, dSettlePx, dClosePx, dBid1Px, dAsk1Px, dHighPx, dLowPx, dFluctuationPercent, nVolume, nTradingVolume, dAmount, nIsTrading
					, nExchangeID, pszCode ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Update_Commodity() : ::sprintf() invalid return value." );
		return -1024*2;
	}

	//::printf( "%s\n", pszSqlCmd );
	return ExecuteSql( pszSqlCmd );
}




