#pragma warning(disable : 4996)
#include "QLMatchCH.h"
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

	if( FALSE == CQLMatchCH::InitStaticData() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 initialize QLMatchCH library" );
		return -2;
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

	m_pMysqlConnection = ::mysql_real_connect( &m_oMySqlHandle
											, Configuration::GetConfig().GetDbHost().c_str()
											, Configuration::GetConfig().GetDbAccount().c_str()
											, Configuration::GetConfig().GetDbPassword().c_str()
											, Configuration::GetConfig().GetDbTableName().c_str()
											, Configuration::GetConfig().GetDbPort()
											, NULL, 0 );

	if( NULL == m_pMysqlConnection )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 establish database connection(%s) ......", ::mysql_error( &m_oMySqlHandle ) );
		return -1;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationDatabase::EstablishConnection() : database connection has been established ......" );

	if( 0 != ::mysql_select_db( &m_oMySqlHandle, Configuration::GetConfig().GetDbTableName().c_str() ) )
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

int QuotationDatabase::StartTransaction()
{
	if( NULL != m_pMysqlConnection )
	{
		int	nRet = ::mysql_query( &m_oMySqlHandle, "BEGIN TRANSACTION;" );

		if( nRet < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::StartTransaction() : errorcode = %d, [ERROR] %s", nRet, ::mysql_error( &m_oMySqlHandle ) );
		}

		return nRet;
	}

	return -1024;
}

int QuotationDatabase::Commit()
{
	if( NULL != m_pMysqlConnection )
	{
		int	nRet = ::mysql_commit( &m_oMySqlHandle );
		//int	nRet = ::mysql_query( &m_oMySqlHandle, "COMMIT;" );

		if( nRet < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Commit() : errorcode = %d, [ERROR] %s", nRet, ::mysql_error( &m_oMySqlHandle ) );
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
	char		pszDumpCode[32] = { 0 };
	char		cMkID = nExchangeID;
	///< '0'：未知 '1'：SSE（上海证劵交易所） '2'：SZSE（深圳证劵交易所） '3'：cffEX（中国金融期货交易） '4'：dcE （大连商品期货交易所） '5'：ZcE（郑州商品期货交易所） '6'：SHfE （上海期货交易所）
	switch( cMkID )
	{
	case '1':
		::strcpy( pszDumpCode, "SH." );
		::strcpy( pszDumpCode + 3, pszCode );
		break;
	case '2':
		::strcpy( pszDumpCode, "SZ." );
		::strcpy( pszDumpCode + 3, pszCode );
		break;
	case '3':
		::strcpy( pszDumpCode, "CFF." );
		::strcpy( pszDumpCode + 4, pszCode );
		break;
	case '4':
		::strcpy( pszDumpCode, "DCE." );
		::strcpy( pszDumpCode + 4, pszCode );
		break;
	case '5':
		::strcpy( pszDumpCode, "ZCE." );
		::strcpy( pszDumpCode + 4, pszCode );
		break;
	case '6':
		::strcpy( pszDumpCode, "SHFE." );
		::strcpy( pszDumpCode + 5, pszCode );
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Replace_Commodity() : invalid exchange id = %c", cMkID );
		return -1024;
	}

	::sprintf( pszTradingDate, "%d-%d-%d", nTradingDate/10000, nTradingDate%10000/100, nTradingDate%100 );

	if( 0 >= ::sprintf( pszSqlCmd
					, "REPLACE INTO commodity (typeId,exchange,code,name,lotSize,contractMulti,priceTick,preClose,preSettle,upperPrice,lowerPrice,price,openPrice,settlementPrice,closePrice,bid1,ask1,highPrice,lowPrice,uplowPercent,volume,nowVolumn,amount,isTrading,tradingDate,upLimit,classId,addTime,updatetime,hzpy) VALUES (%d,%d,\'%s\',\'%s\',%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%I64d,%I64d,%f,%d,\'%s\',0,\'%s\',now(),now(),\'%s\');"
					, nTypeID, nExchangeID, pszCode, pszName, nLotSize, nContractMulti, dPriceTick, dPreClose, dPreSettle, dUpperPrice, dLowerPrice, dPrice, dOpenPrice, dSettlePrice, dClosePrice, dBid1Price, dAsk1Price, dHighPrice, dLowPrice, dFluctuationPercent, nVolume, nTradingVolume, dAmount, nIsTrading
					, pszTradingDate, pszClassID, ShortSpell::GetObj().GetShortSpell( pszDumpCode, pszName ).c_str() ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Replace_Commodity() : ::sprintf() invalid return value." );
		return -1024*2;
	}

	//::printf( "%s\n", pszSqlCmd );
	return ExecuteSql( pszSqlCmd );
}

int QuotationDatabase::Update_Commodity( short nExchangeID, const char* pszCode, double dPreClosePx, double dPreSettlePx, double dUpperPx, double dLowerPx, double dLastPx, double dSettlePx
										, double dOpenPx, double dClosePx, double dBid1Px, double dAsk1Px, double dHighPx, double dLowPx, double dAmount, unsigned __int64 nVolume, unsigned int nTradingVolume
										, double dFluctuationPercent, short nIsTrading )
{
	char		pszSqlCmd[1024*2] = { 0 };

	if( 0 >= ::sprintf( pszSqlCmd
					, "UPDATE commodity SET preClose=%f,preSettle=%f,upperPrice=%f,lowerPrice=%f,price=%f,openPrice=%f,settlementPrice=%f,closePrice=%f,bid1=%f,ask1=%f,highPrice=%f,lowPrice=%f,uplowPercent=%f,volume=%I64d,nowVolumn=%d,amount=%f,isTrading=%d,updatetime=now() WHERE exchange=%d AND code=\'%s\';"
					, dPreClosePx, dPreSettlePx, dUpperPx, dLowerPx, dLastPx, dOpenPx, dSettlePx, dClosePx, dBid1Px, dAsk1Px, dHighPx, dLowPx, dFluctuationPercent, nVolume, nTradingVolume, dAmount, nIsTrading
					, nExchangeID, pszCode ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Update_Commodity() : ::sprintf() invalid return value." );
		return -1024*2;
	}

	//::printf( "%s\n", pszSqlCmd );
	return ExecuteSql( pszSqlCmd );
}




