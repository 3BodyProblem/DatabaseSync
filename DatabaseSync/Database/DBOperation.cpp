#pragma warning(disable : 4996)
#include "QLMatchCH.h"
#include "DBOperation.h"
#include "../DatabaseSync.h"


QuotationDatabase::QuotationDatabase()
{
	::memset( &m_vctTransRefCount, 0, sizeof(m_vctTransRefCount) );
	::memset( &m_pMysqlConnectionList, 0, sizeof(m_pMysqlConnectionList) );
	::memset( &m_vctMySqlHandle, 0, sizeof(m_vctMySqlHandle) );
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

int QuotationDatabase::Initialize( enum XDFMarket eMkID )
{
	MYSQL*			pRetHandle = ::mysql_init( &(m_vctMySqlHandle[eMkID]) );

	if( NULL == pRetHandle )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 call func. ::mysql_init(), %s", ::mysql_error( &(m_vctMySqlHandle[eMkID]) ) );
		return -1;
	}

	return 0;
}

void QuotationDatabase::Release()
{
	for( int n = 0; n < 64; n++ )
	{
		if( NULL != m_pMysqlConnectionList[n] )
		{
			m_pMysqlConnectionList[n] = NULL;
			::mysql_close( &(m_vctMySqlHandle[n]) );
		}
	}
}

int QuotationDatabase::EstablishConnection( enum XDFMarket eMkID )
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationDatabase::EstablishConnection() : building database connection ......" );

	m_pMysqlConnectionList[eMkID] = ::mysql_real_connect( &(m_vctMySqlHandle[eMkID])
														, Configuration::GetConfig().GetDbHost().c_str()
														, Configuration::GetConfig().GetDbAccount().c_str()
														, Configuration::GetConfig().GetDbPassword().c_str()
														, Configuration::GetConfig().GetDbTableName().c_str()
														, Configuration::GetConfig().GetDbPort()
														, NULL, 0 );

	if( NULL == m_pMysqlConnectionList[eMkID] )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 establish database connection(%s) ......", ::mysql_error( &(m_vctMySqlHandle[eMkID]) ) );
		return -1;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationDatabase::EstablishConnection() : database connection has been established ......" );

	if( 0 != ::mysql_select_db( &(m_vctMySqlHandle[eMkID]), Configuration::GetConfig().GetDbTableName().c_str() ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 select table, (%s) ......", ::mysql_error( &(m_vctMySqlHandle[eMkID]) ) );
		return -2;
	}

	if( 0 < ExecuteSql( eMkID, "set names gbk;" ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Initialize() : failed 2 set mysql client character set 2 gbk, (%s) ......", ::mysql_error( &(m_vctMySqlHandle[eMkID]) ) );
		return -3;
	}

	return 0;
}

int QuotationDatabase::ExecuteSql( enum XDFMarket eMkID, const char* pszSqlCmd )
{
	if( NULL == m_pMysqlConnectionList[eMkID] )
	{
		if( 0 != Initialize( eMkID ) )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ExecuteSql() : failed 2 initialize QuotationDatabase obj." );
			return -1;
		}

		if( 0 != EstablishConnection( eMkID ) )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ExecuteSql() : failed 2 establish connection 4 QuotationDatabase obj." );
			return -2;
		}
	}

	if( NULL != m_pMysqlConnectionList[eMkID] && NULL != pszSqlCmd )
	{
		int	nRet = ::mysql_query( &(m_vctMySqlHandle[eMkID]), pszSqlCmd );

		if( nRet < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::ExecuteSql() : errorcode = %d, [ERROR] %s", nRet, ::mysql_error( &(m_vctMySqlHandle[eMkID]) ) );
		}

		return nRet;
	}

	return -1024;
}

int QuotationDatabase::StartTransaction( enum XDFMarket eMkID )
{
	if( NULL == m_pMysqlConnectionList[eMkID] )
	{
		if( 0 != Initialize( eMkID ) )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::StartTransaction() : failed 2 initialize QuotationDatabase obj." );
			return -1;
		}

		if( 0 != EstablishConnection( eMkID ) )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::StartTransaction() : failed 2 establish connection 4 QuotationDatabase obj." );
			return -2;
		}
	}

	if( NULL != m_pMysqlConnectionList[eMkID] )
	{
		if( m_vctTransRefCount[eMkID] > 0 )
		{
			return 1;
		}

		int	nRet = ::mysql_query( &(m_vctMySqlHandle[eMkID]), "START TRANSACTION;" );

		if( nRet < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::StartTransaction() : errorcode = %d, [ERROR] %s", nRet, ::mysql_error( &(m_vctMySqlHandle[eMkID]) ) );
			return -3;
		}

		m_vctTransRefCount[eMkID]++;

		return nRet;
	}

	return -1024;
}

int QuotationDatabase::Commit( enum XDFMarket eMkID )
{
	if( NULL != m_pMysqlConnectionList[eMkID] )
	{
		if( 0 == m_vctTransRefCount[eMkID] )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Commit() : [ERROR] transaction is not available. refcount = %d", m_vctTransRefCount[eMkID] );
			return -1;
		}

		int	nRet = ::mysql_query( &(m_vctMySqlHandle[eMkID]), "COMMIT;" );
		///<int	nRet = ::mysql_commit( &(m_vctMySqlHandle[eMkID]) );

		m_vctTransRefCount[eMkID] = 0;
		if( nRet < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Commit() : errorcode = %d, [ERROR] %s", nRet, ::mysql_error( &(m_vctMySqlHandle[eMkID]) ) );
			return -2;
		}

		return nRet;
	}

	return -1024;
}

int QuotationDatabase::RollBack( enum XDFMarket eMkID )
{
	if( NULL != m_pMysqlConnectionList[eMkID] )
	{
		int	nRet = ::mysql_query( &(m_vctMySqlHandle[eMkID]), "ROLLBACK;" );
		///<int	nRet = ::mysql_rollback( &(m_vctMySqlHandle[eMkID]) );

		m_vctTransRefCount[eMkID] = 0;
		if( nRet < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::RollBack() : errorcode = %d, [ERROR] %s", nRet, ::mysql_error( &(m_vctMySqlHandle[eMkID]) ) );
		}

		return nRet;
	}

	return -1024;
}

int QuotationDatabase::Replace_Commodity( enum XDFMarket eMkID, short nTypeID, short nExchangeID, const char* pszCode, const char* pszName
										, int nLotSize, int nContractMulti, double dPriceTick, double dPreClose, double dPreSettle
										, double dUpperPrice, double dLowerPrice, double dPrice, double dOpenPrice, double dSettlePrice
										, double dClosePrice, double dBid1Price, double dAsk1Price, double dHighPrice, double dLowPrice
										, double dFluctuationPercent, unsigned __int64 nVolume, unsigned __int64 nTradingVolume, double dAmount
										, short nIsTrading, unsigned int nTradingDate, double dUpLimit, const char* pszClassID )
{
	char		pszSqlCmd[1024*2] = { 0 };
	char		pszTradingDate[64] = { 0 };
	char		pszDumpCode[32] = { 0 };

	///< '0'：未知 '1'：SSE（上海证劵交易所） '2'：SZSE（深圳证劵交易所） '3'：cffEX（中国金融期货交易） '4'：dcE （大连商品期货交易所） '5'：ZcE（郑州商品期货交易所） '6'：SHfE （上海期货交易所）
	switch( nExchangeID )
	{
	case 1:
		::strcpy( pszDumpCode, "SH." );
		::strcpy( pszDumpCode + 3, pszCode );
		break;
	case 2:
		::strcpy( pszDumpCode, "SZ." );
		::strcpy( pszDumpCode + 3, pszCode );
		break;
	case 3:
		::strcpy( pszDumpCode, "CFF." );
		::strcpy( pszDumpCode + 4, pszCode );
		break;
	case 4:
		::strcpy( pszDumpCode, "DCE." );
		::strcpy( pszDumpCode + 4, pszCode );
		break;
	case 5:
		::strcpy( pszDumpCode, "ZCE." );
		::strcpy( pszDumpCode + 4, pszCode );
		break;
	case 6:
		::strcpy( pszDumpCode, "SHFE." );
		::strcpy( pszDumpCode + 5, pszCode );
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Replace_Commodity() : invalid exchange id = %d", nExchangeID );
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
	return ExecuteSql( eMkID, pszSqlCmd );
}

int QuotationDatabase::Update_Commodity( enum XDFMarket eMkID, short nExchangeID, const char* pszCode, double dPreClosePx, double dPreSettlePx, double dUpperPx, double dLowerPx, double dLastPx, double dSettlePx
										, double dOpenPx, double dClosePx, double dBid1Px, double dAsk1Px, double dHighPx, double dLowPx, double dAmount, unsigned __int64 nVolume, unsigned int nTradingVolume
										, double dFluctuationPercent, short nIsTrading )
{
	char		pszSqlCmd[1024*2] = { 0 };

	if( 0 >= ::sprintf( pszSqlCmd
					, "UPDATE commodity SET preClose=%f,preSettle=%f,upperPrice=%f,lowerPrice=%f,price=%f,openPrice=%f,settlementPrice=%f,closePrice=%f,bid1=%f,ask1=%f,highPrice=%f,lowPrice=%f,uplowPercent=%.4f,volume=%I64d,nowVolumn=%d,amount=%f,isTrading=%d,updatetime=now() WHERE exchange=%d AND code=\'%s\';"
					, dPreClosePx, dPreSettlePx, dUpperPx, dLowerPx, dLastPx, dOpenPx, dSettlePx, dClosePx, dBid1Px, dAsk1Px, dHighPx, dLowPx, dFluctuationPercent, nVolume, nTradingVolume, dAmount, nIsTrading
					, nExchangeID, pszCode ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationDatabase::Update_Commodity() : ::sprintf() invalid return value." );
		return -1024*2;
	}

	return ExecuteSql( eMkID, pszSqlCmd );
}




