#include "Quotation.h"
#include "DataCenter.h"
#include <windows.h>
#include <time.h>
#include <sys/timeb.h>
#include "SvrStatus.h"
#include "../Infrastructure/File.h"
#include "../DatabaseSync.h"
#include "../Database/DBOperation.h"


///< -------------------- Configuration ------------------------------------------------
const int		XDF_SH_COUNT = 16000;					///< 上海Lv1
const int		XDF_SHL2_COUNT = 0;						///< 上海Lv2(QuoteClientApi内部屏蔽)
const int		XDF_SHOPT_COUNT = 500;					///< 上海期权
const int		XDF_SZ_COUNT = 8000;					///< 深证Lv1
const int		XDF_SZL2_COUNT = 0;						///< 深证Lv2(QuoteClientApi内部屏蔽)
const int		XDF_SZOPT_COUNT = 0;					///< 深圳期权
const int		XDF_CF_COUNT = 500;						///< 中金期货
const int		XDF_ZJOPT_COUNT = 500;					///< 中金期权
const int		XDF_CNF_COUNT = 500;					///< 商品期货(上海/郑州/大连)
const int		XDF_CNFOPT_COUNT = 500;					///< 商品期权(上海/郑州/大连)
unsigned int	s_nNumberInSection = 50;				///< 一个市场有可以缓存多少个数据块
///< -----------------------------------------------------------------------------------


CacheAlloc::CacheAlloc()
 : m_nMaxCacheSize( 0 ), m_nAllocateSize( 0 ), m_pDataCache( NULL )
{
	m_nMaxCacheSize = (XDF_SH_COUNT + XDF_SHOPT_COUNT + XDF_SZ_COUNT + XDF_SZOPT_COUNT + XDF_CF_COUNT + XDF_ZJOPT_COUNT + XDF_CNF_COUNT + XDF_CNFOPT_COUNT) * sizeof(T_TICK_LINE) * s_nNumberInSection;
}

CacheAlloc::~CacheAlloc()
{
	CacheAlloc::GetObj().FreeCaches();
}

CacheAlloc& CacheAlloc::GetObj()
{
	static	CacheAlloc	obj;

	return obj;
}

char* CacheAlloc::GetBufferPtr()
{
	CriticalLock	section( m_oLock );

	if( NULL == m_pDataCache )
	{
		m_pDataCache = new char[m_nMaxCacheSize];
		::memset( m_pDataCache, 0, m_nMaxCacheSize );
	}

	return m_pDataCache;
}

unsigned int CacheAlloc::GetMaxBufLength()
{
	return m_nMaxCacheSize;
}

unsigned int CacheAlloc::GetDataLength()
{
	CriticalLock	section( m_oLock );

	return m_nAllocateSize;
}

char* CacheAlloc::GrabCache( enum XDFMarket eMkID, unsigned int& nOutSize )
{
	char*					pData = NULL;
	unsigned int			nBufferSize4Market = 0;
	CriticalLock			section( m_oLock );

	try
	{
		nOutSize = 0;
		m_pDataCache = GetBufferPtr();

		if( NULL == m_pDataCache )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : invalid buffer pointer." );
			return NULL;
		}

		switch( eMkID )
		{
		case XDF_SH:		///< 上海Lv1
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SHOPT:		///< 上海期权
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZ:		///< 深证Lv1
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZOPT:		///< 深圳期权
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CF:		///< 中金期货
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_ZJOPT:		///< 中金期权
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CNF:		///< 商品期货(上海/郑州/大连)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		default:
			{
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : unknow market id" );
				return NULL;
			}
		}

		if( (m_nAllocateSize + nBufferSize4Market) > m_nMaxCacheSize )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : not enough space ( %u > %u )", (m_nAllocateSize + nBufferSize4Market), m_nMaxCacheSize );
			return NULL;
		}

		nOutSize = nBufferSize4Market;
		pData = m_pDataCache + m_nAllocateSize;
		m_nAllocateSize += nBufferSize4Market;
	}
	catch( std::exception& err )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : an exceptin occur : %s", err.what() );
	}
	catch( ... )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CacheAlloc::GrabCache() : unknow exception" );
	}

	return pData;
}

void CacheAlloc::FreeCaches()
{
	if( NULL != m_pDataCache )
	{
		delete [] m_pDataCache;
		m_pDataCache = NULL;
	}
}


std::string	JoinPath( std::string sPath, std::string sFileName )
{
	unsigned int		nSepPos = sPath.length() - 1;

	if( sPath[nSepPos] == '/' || sPath[nSepPos] == '\\' ) {
		return sPath + sFileName;
	} else {
		return sPath + "/" + sFileName;
	}
}

std::string	GenFilePathByWeek( std::string sFolderPath, std::string sFileName, unsigned int nMkDate )
{
	char				pszTmp[32] = { 0 };
	DateTime			oDate( nMkDate/10000, nMkDate%10000/100, nMkDate%100 );
	std::string&		sNewPath = JoinPath( sFolderPath, sFileName );

	::itoa( oDate.GetDayOfWeek(), pszTmp, 10 );
	sNewPath += ".";sNewPath += pszTmp;
	return sNewPath;
}


static unsigned int			s_nCNFTradingDate = 0;
static unsigned int			s_nCNFOPTTradingDate = 0;


QuotationData::QuotationData()
 : m_pQuotation( NULL )
{
}

QuotationData::~QuotationData()
{
}

int QuotationData::Initialize( void* pQuotation )
{
	int					nErrorCode = 0;

	Release();
	m_pQuotation = pQuotation;

	nErrorCode = m_arrayTickLine.Instance( CacheAlloc::GetObj().GetBufferPtr(), CacheAlloc::GetObj().GetMaxBufLength()/sizeof(T_TICK_LINE) );
	if( 0 > nErrorCode )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 initialize tick lines manager obj. errorcode = %d", nErrorCode );
		return -1;
	}

	if( false == m_oThdTickDump.IsAlive() )
	{
		if( 0 != m_oThdTickDump.Create( "ThreadDumpTickLine()", ThreadDumpTickLine, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create tick line thread(1)" );
			return -2;
		}
	}

	if( false == m_oThdIdle.IsAlive() )
	{
		if( 0 != m_oThdIdle.Create( "ThreadOnIdle()", ThreadOnIdle, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create onidle line thread" );
			return -3;
		}
	}

	if( 0 != QuotationDatabase::GetDbObj().Initialize() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 initialize QuotationDatabase obj." );
		return -4;
	}

	if( 0 != QuotationDatabase::GetDbObj().EstablishConnection() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 establish connection 4 QuotationDatabase obj." );
		return -5;
	}

	return 0;
}

void QuotationData::Release()
{
	m_mapModuleStatus.clear();
	m_mapSHL1.clear();
	m_mapSHOPT.clear();
	m_mapSZL1.clear();
	m_mapSZOPT.clear();
	m_mapCFF.clear();
	m_mapCFFOPT.clear();
	m_mapCNF.clear();
	m_mapCNFOPT.clear();
}

short QuotationData::GetModuleStatus( enum XDFMarket eMarket )
{
	CriticalLock				section( m_oLock );
	TMAP_MKID2STATUS::iterator	it = m_mapModuleStatus.find( eMarket );

	if( m_mapModuleStatus.end() == it )
	{
		return -1;
	}

	return it->second;
}

void QuotationData::UpdateModuleStatus( enum XDFMarket eMarket, int nStatus )
{
	CriticalLock	section( m_oLock );

	m_mapModuleStatus[eMarket] = nStatus;
}

void* QuotationData::ThreadOnIdle( void* pSelf )
{
	ServerStatus&		refStatus = ServerStatus::GetStatusObj();
	QuotationData&		refData = *(QuotationData*)pSelf;
	Quotation&			refQuotation = *((Quotation*)refData.m_pQuotation);
	T_TICKLINE_CACHE&	refTickBuf = refData.m_arrayTickLine;
	MAP_MK_CLOSECFG&	refCloseCfg = Configuration::GetConfig().GetMkCloseCfg();

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			SimpleThread::Sleep( 1000 * 2 );

			///< 检查是否需要落日线
			for( MAP_MK_CLOSECFG::iterator it = refCloseCfg.begin(); it != refCloseCfg.end(); it++ )
			{
				for( TB_MK_CLOSECFG::iterator itCfg = it->second.begin(); itCfg != it->second.end(); itCfg++ )
				{
					if( DateTime::Now().TimeToLong() >= itCfg->CloseTime )	{
						ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)(it->first), "收盘后" );
					}
				}
			}

			refQuotation.UpdateMarketsTime();										///< 更新各市场的日期和时间
			refStatus.UpdateTickBufOccupancyRate( refTickBuf.GetPercent() );		///< TICK缓存占用率
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadOnIdle() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadOnIdle() : unknow exception" );
		}
	}

	return NULL;
}

__inline bool	PrepareTickFile( T_TICK_LINE* pTickLine, std::string& sCode, std::ofstream& oDumper )
{
	char				pszFilePath[512] = { 0 };

	switch( pTickLine->eMarketID )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SSE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::sprintf( pszFilePath, "SZSE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_CF:		///< 中金期货
	case XDF_ZJOPT:		///< 中金期权
		::sprintf( pszFilePath, "CFFEX/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			switch( pTickLine->Type )
			{
			case 1:
			case 4:
				::sprintf( pszFilePath, "CZCE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
				break;
			case 2:
			case 5:
				::sprintf( pszFilePath, "DCE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
				break;
			case 3:
			case 6:
				::sprintf( pszFilePath, "SHFE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
				break;
			case 7:
				::sprintf( pszFilePath, "INE/TICK/%s/%u/", pTickLine->Code, pTickLine->Date );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareTickFile() : invalid market subid (Type=%d)", pTickLine->Type );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareTickFile() : invalid market id (%s)", pTickLine->eMarketID );
		return false;
	}

	if( oDumper.is_open() )	oDumper.close();
	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "TICK%s_%u.csv", pTickLine->Code, pTickLine->Date );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::app );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PrepareTickFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	sCode = pTickLine->Code;
	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "date,time,preclosepx,presettlepx,openpx,highpx,lowpx,closepx,nowpx,settlepx,upperpx,lowerpx,amount,volume,openinterest,numtrades,bidpx1,bidvol1,bidpx2,bidvol2,askpx1,askvol1,askpx2,askvol2,voip,tradingphasecode,prename\n";
		oDumper << sTitle;
	}

	return true;
}

typedef char	STR_TICK_LINE[512];
void* QuotationData::ThreadDumpTickLine( void* pSelf )
{
	unsigned __int64			nDumpNumber = 0;
	QuotationData&				refData = *(QuotationData*)pSelf;
	Quotation&					refQuotation = *((Quotation*)refData.m_pQuotation);

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			char*					pBufPtr = CacheAlloc::GetObj().GetBufferPtr();
			unsigned int			nMaxDataNum = refData.m_arrayTickLine.GetRecordCount();
			std::string				sCode;
			std::ofstream			oDumper;
			STR_TICK_LINE			pszLine = { 0 };

			SimpleThread::Sleep( 1000 * 1 );
			if( NULL == pBufPtr || 0 == nMaxDataNum ) {
				continue;
			}

			if( nDumpNumber > 6000 ) {
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuotationData::ThreadDumpTickLine() : (%I64d) dumped... ( Count=%u )", nDumpNumber, nMaxDataNum );
				nDumpNumber = 0;
			}

			while( true )
			{
				T_TICK_LINE		tagTickData = { 0 };

				if( refData.m_arrayTickLine.GetData( &tagTickData ) <= 0 )
				{
					break;
				}

				if( sCode != tagTickData.Code )
				{
					if( false == PrepareTickFile( &tagTickData, sCode, oDumper ) )
					{
						continue;
					}
				}

				if( !oDumper.is_open() )
				{
					QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : invalid file handle" );
					SimpleThread::Sleep( 1000 * 10 );
					continue;
				}

				int		nLen = ::sprintf( pszLine, "%u,%u,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%I64d,%I64d,%I64d,%f,%I64d,%f,%I64d,%f,%I64d,%f,%I64d,%f,%s,%s\n"
					, tagTickData.Date, tagTickData.Time, tagTickData.PreClosePx, tagTickData.PreSettlePx
					, tagTickData.OpenPx, tagTickData.HighPx, tagTickData.LowPx, tagTickData.ClosePx, tagTickData.NowPx, tagTickData.SettlePx
					, tagTickData.UpperPx, tagTickData.LowerPx, tagTickData.Amount, tagTickData.Volume, tagTickData.OpenInterest, tagTickData.NumTrades
					, tagTickData.BidPx1, tagTickData.BidVol1, tagTickData.BidPx2, tagTickData.BidVol2, tagTickData.AskPx1, tagTickData.AskVol1, tagTickData.AskPx2, tagTickData.AskVol2
					, tagTickData.Voip, tagTickData.TradingPhaseCode, tagTickData.PreName );
				oDumper.write( pszLine, nLen );
				nDumpNumber++;
			}

		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadDumpTickLine() : unknow exception" );
		}
	}

	return NULL;
}

int QuotationData::UpdatePreName( enum XDFMarket eMarket, std::string& sCode, char* pszPreName, unsigned int nSize )
{
	switch( eMarket )
	{
	case XDF_SZ:	///< 深证L1
		{
			T_MAP_QUO::iterator it = m_mapSZL1.find( sCode );
			if( it == m_mapSZL1.end() )
			{
				T_LINE_PARAM&		refData = it->second;
				::memcpy( refData.PreName, pszPreName, 4 );
			}
		}
		break;
	}
	return 0;
}

T_LINE_PARAM* QuotationData::BuildSecurity( enum XDFMarket eMarket, std::string& sCode, T_LINE_PARAM& refParam )
{
	T_MAP_QUO::iterator it = NULL;
	unsigned int		nBufSize = 0;
	char*				pDataPtr = NULL;
	int					nErrorCode = 0;

	switch( eMarket )
	{
	case XDF_SH:	///< 上证L1
		{
			it = m_mapSHL1.find( sCode );
			if( it == m_mapSHL1.end() )
			{
				m_mapSHL1[sCode] = refParam;
				T_LINE_PARAM&	refData = m_mapSHL1[sCode];
				return &refData;
			}
		}
		break;
	case XDF_SHOPT:	///< 上期
		{
			it = m_mapSHOPT.find( sCode );
			if( it == m_mapSHOPT.end() )
			{
				m_mapSHOPT[sCode] = refParam;
				T_LINE_PARAM&	refData = m_mapSHOPT[sCode];
				return &refData;
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			it = m_mapSZL1.find( sCode );
			if( it == m_mapSZL1.end() )
			{
				m_mapSZL1[sCode] = refParam;
				T_LINE_PARAM&	refData = m_mapSZL1[sCode];
				return &refData;
			}
		}
		break;
	case XDF_SZOPT:	///< 深期
		{
			it = m_mapSZOPT.find( sCode );
			if( it == m_mapSZOPT.end() )
			{
				m_mapSZOPT[sCode] = refParam;
				T_LINE_PARAM&	refData = m_mapSZOPT[sCode];
				return &refData;
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			it = m_mapCFF.find( sCode );
			if( it == m_mapCFF.end() )
			{
				m_mapCFF[sCode] = refParam;
				T_LINE_PARAM&	refData = m_mapCFF[sCode];
				return &refData;
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			it = m_mapCFFOPT.find( sCode );
			if( it == m_mapCFFOPT.end() )
			{
				m_mapCFFOPT[sCode] = refParam;
				T_LINE_PARAM&	refData = m_mapCFFOPT[sCode];
				return &refData;
			}
		}
		break;
	case XDF_CNF:	///< 商品期货(上海/郑州/大连)
		{
			it = m_mapCNF.find( sCode );
			if( it == m_mapCNF.end() )
			{
				m_mapCNF[sCode] = refParam;
				T_LINE_PARAM&	refData = m_mapCNF[sCode];
				return &refData;
			}
		}
		break;
	case XDF_CNFOPT:///< 商品期权(上海/郑州/大连)
		{
			it = m_mapCNFOPT.find( sCode );
			if( it == m_mapCNFOPT.end() )
			{
				m_mapCNFOPT[sCode] = refParam;
				T_LINE_PARAM&	refData = m_mapCNFOPT[sCode];
				return &refData;
			}
		}
		break;
	default:
		nErrorCode = -1024;
		break;
	}

	if( nErrorCode < 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::BuildSecurity() : an error occur while building security table, marketid=%d, errorcode=%d", (int)eMarket, nErrorCode );
	}

	return &(it->second);
}

bool PreDayFile( std::ofstream& oDumper, enum XDFMarket eMarket, std::string sCode, char cType, unsigned int nTradeDate )
{
	char		pszFilePath[512] = { 0 };

	switch( eMarket )
	{
	case XDF_SH:		///< 上海Lv1
	case XDF_SHOPT:		///< 上海期权
	case XDF_SHL2:		///< 上海Lv2(QuoteClientApi内部屏蔽)
		::strcpy( pszFilePath, "SSE/DAY/" );
		break;
	case XDF_SZ:		///< 深证Lv1
	case XDF_SZOPT:		///< 深圳期权
	case XDF_SZL2:		///< 深证Lv2(QuoteClientApi内部屏蔽)
		::strcpy( pszFilePath, "SZSE/DAY/" );
		break;
	case XDF_CF:		///< 中金期货
	case XDF_ZJOPT:		///< 中金期权
		::strcpy( pszFilePath, "CFFEX/DAY/" );
		break;
	case XDF_CNF:		///< 商品期货(上海/郑州/大连)
	case XDF_CNFOPT:	///< 商品期权(上海/郑州/大连)
		{
			switch( cType )
			{
			case 1:
			case 4:
				::strcpy( pszFilePath, "CZCE/DAY/" );
				break;
			case 2:
			case 5:
				::strcpy( pszFilePath, "DCE/DAY/" );
				break;
			case 3:
			case 6:
				::strcpy( pszFilePath, "SHFE/DAY/" );
				break;
			case 7:
				::strcpy( pszFilePath, "INE/DAY/" );
				break;
			default:
				QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PreDayFile() : invalid market subid (Type=%d)", (int)cType );
				return false;
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PreDayFile() : invalid market id (%s)", eMarket );
		return false;
	}

	if( oDumper.is_open() )	oDumper.close();
	std::string				sFilePath = JoinPath( Configuration::GetConfig().GetDumpFolder(), pszFilePath );
	File::CreateDirectoryTree( sFilePath );
	char	pszFileName[128] = { 0 };
	::sprintf( pszFileName, "DAY%s.csv", sCode.c_str() );
	sFilePath += pszFileName;
	oDumper.open( sFilePath.c_str() , std::ios::out|std::ios::app );

	if( !oDumper.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::PreDayFile() : cannot open file (%s)", sFilePath.c_str() );
		return false;
	}

	oDumper.seekp( 0, std::ios::end );
	if( 0 == oDumper.tellp() )
	{
		std::string		sTitle = "date,openpx,highpx,lowpx,closepx,settlepx,amount,volume,openinterest,numtrades,voip\n";
		oDumper << sTitle;
	}

	return true;
}

int QuotationData::UpdateTickLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, unsigned int nTradeDate )
{
	int				nErrorCode = 0;
	T_TICK_LINE		refTickLine = { 0 };
	unsigned int	nMachineDate = DateTime::Now().DateToLong();
	unsigned int	nMachineTime = DateTime::Now().TimeToLong() * 1000;

	refTickLine.eMarketID = eMarket;
	refTickLine.Date = nMachineDate;
	refTickLine.Time = nMachineTime;
	switch( eMarket )
	{
	case XDF_SH:	///< 上证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*		pStock = (XDFAPI_StockData5*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.PriceRate;
						//refTickLine.PreSettlePx = 0;
						refTickLine.OpenPx = pStock->Open / refParam.PriceRate;
						refTickLine.HighPx = pStock->High / refParam.PriceRate;
						refTickLine.LowPx = pStock->Low / refParam.PriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						//refTickLine.SettlePx = 0;
						refTickLine.UpperPx = pStock->HighLimit;
						refTickLine.LowerPx = pStock->LowLimit;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
						refTickLine.NumTrades = pStock->Records;
						refTickLine.BidPx1 = pStock->Buy[0].Price;
						refTickLine.BidVol1 = pStock->Buy[0].Volume;
						refTickLine.BidPx2 = pStock->Buy[1].Price;
						refTickLine.BidVol2 = pStock->Buy[1].Volume;
						refTickLine.AskPx1 = pStock->Sell[0].Price;
						refTickLine.AskVol1 = pStock->Sell[0].Volume;
						refTickLine.AskPx2 = pStock->Sell[1].Price;
						refTickLine.AskVol2 = pStock->Sell[1].Volume;
						refTickLine.Voip = pStock->Voip / refParam.PriceRate;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
				}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*		pStock = (XDFAPI_IndexData*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSHL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSHL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.PriceRate;
						//refTickLine.PreSettlePx = 0;
						refTickLine.OpenPx = pStock->Open / refParam.PriceRate;
						refTickLine.HighPx = pStock->High / refParam.PriceRate;
						refTickLine.LowPx = pStock->Low / refParam.PriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						//refTickLine.SettlePx = 0;
						//refTickLine.UpperPx = 0;
						//refTickLine.LowerPx = 0;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
						//refTickLine.NumTrades = pStock->Records;
						//refTickLine.Voip = pStock->Voip / refParam.PriceRate;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
					}
				}
				break;
			}
		}
		break;
	case XDF_SHOPT:	///< 上期
		{
			XDFAPI_ShOptData*		pStock = (XDFAPI_ShOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapSHOPT.find( std::string(pStock->Code, 8 ) );

			if( it != m_mapSHOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				::strncpy( refTickLine.Code, pStock->Code, 8 );
				refTickLine.PreClosePx = refParam.PreClosePx / refParam.PriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePx / refParam.PriceRate;
				refTickLine.OpenPx = pStock->OpenPx / refParam.PriceRate;
				refTickLine.HighPx = pStock->HighPx / refParam.PriceRate;
				refTickLine.LowPx = pStock->LowPx / refParam.PriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.PriceRate;
				refTickLine.UpperPx = refParam.UpperPrice / refParam.PriceRate;
				refTickLine.LowerPx = refParam.LowerPrice / refParam.PriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->Position;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.PriceRate;
				::memcpy( refTickLine.TradingPhaseCode, pStock->TradingPhaseCode, sizeof(pStock->TradingPhaseCode) );
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
			}
		}
		break;
	case XDF_SZ:	///< 深证L1
		{
			switch( nSnapSize )
			{
			case sizeof(XDFAPI_StockData5):
				{
					XDFAPI_StockData5*		pStock = (XDFAPI_StockData5*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.PriceRate;
						//refTickLine.PreSettlePx = 0;
						refTickLine.OpenPx = pStock->Open / refParam.PriceRate;
						refTickLine.HighPx = pStock->High / refParam.PriceRate;
						refTickLine.LowPx = pStock->Low / refParam.PriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						//refTickLine.SettlePx = 0;
						refTickLine.UpperPx = pStock->HighLimit;
						refTickLine.LowerPx = pStock->LowLimit;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
						refTickLine.NumTrades = pStock->Records;
						refTickLine.BidPx1 = pStock->Buy[0].Price;
						refTickLine.BidVol1 = pStock->Buy[0].Volume;
						refTickLine.BidPx2 = pStock->Buy[1].Price;
						refTickLine.BidVol2 = pStock->Buy[1].Volume;
						refTickLine.AskPx1 = pStock->Sell[0].Price;
						refTickLine.AskVol1 = pStock->Sell[0].Volume;
						refTickLine.AskPx2 = pStock->Sell[1].Price;
						refTickLine.AskVol2 = pStock->Sell[1].Volume;
						refTickLine.Voip = pStock->Voip / refParam.PriceRate;
						::memcpy( refTickLine.PreName, refParam.PreName, 4 );
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
					}
				}
				break;
			case sizeof(XDFAPI_IndexData):
				{
					XDFAPI_IndexData*		pStock = (XDFAPI_IndexData*)pSnapData;
					T_MAP_QUO::iterator		it = m_mapSZL1.find( std::string(pStock->Code, 6 ) );

					if( it != m_mapSZL1.end() )
					{
						T_LINE_PARAM&		refParam = it->second;
						///< ------------ Tick Lines -------------------------
						::strncpy( refTickLine.Code, pStock->Code, 6 );
						refTickLine.PreClosePx = pStock->PreClose / refParam.PriceRate;
						//refTickLine.PreSettlePx = 0;
						refTickLine.OpenPx = pStock->Open / refParam.PriceRate;
						refTickLine.HighPx = pStock->High / refParam.PriceRate;
						refTickLine.LowPx = pStock->Low / refParam.PriceRate;
						refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
						refTickLine.NowPx = refTickLine.ClosePx;
						//refTickLine.SettlePx = 0;
						//refTickLine.UpperPx = 0;
						//refTickLine.LowerPx = 0;
						refTickLine.Amount = pStock->Amount;
						refTickLine.Volume = pStock->Volume;
						//refTickLine.OpenInterest = 0;
						//refTickLine.NumTrades = pStock->Records;
						//refTickLine.Voip = pStock->Voip / refParam.PriceRate;
						nErrorCode = m_arrayTickLine.PutData( &refTickLine );
					}
				}
				break;
			}
		}
		break;
	case XDF_SZOPT:	///< 深期
		{
			XDFAPI_SzOptData*		pStock = (XDFAPI_SzOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapSZOPT.find( std::string(pStock->Code,8) );

			if( it != m_mapSZOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				::strncpy( refTickLine.Code, pStock->Code, 8 );
				refTickLine.PreClosePx = refParam.PreClosePx / refParam.PriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePx / refParam.PriceRate;
				refTickLine.OpenPx = pStock->OpenPx / refParam.PriceRate;
				refTickLine.HighPx = pStock->HighPx / refParam.PriceRate;
				refTickLine.LowPx = pStock->LowPx / refParam.PriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.PriceRate;
				refTickLine.UpperPx = refParam.UpperPrice / refParam.PriceRate;
				refTickLine.LowerPx = refParam.LowerPrice / refParam.PriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->Position;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.PriceRate;
				::memcpy( refTickLine.TradingPhaseCode, pStock->TradingPhaseCode, sizeof(pStock->TradingPhaseCode) );
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
			}
		}
		break;
	case XDF_CF:	///< 中金期货
		{
			XDFAPI_CffexData*		pStock = (XDFAPI_CffexData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapCFF.find( std::string(pStock->Code,6) );

			if( it != m_mapCFF.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				::strncpy( refTickLine.Code, pStock->Code, 6 );
				refTickLine.Time = pStock->DataTimeStamp;
				refTickLine.PreClosePx = pStock->PreClose / refParam.PriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePrice / refParam.PriceRate;
				refTickLine.OpenPx = pStock->Open / refParam.PriceRate;
				refTickLine.HighPx = pStock->High / refParam.PriceRate;
				refTickLine.LowPx = pStock->Low / refParam.PriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.PriceRate;
				refTickLine.UpperPx = pStock->UpperPrice / refParam.PriceRate;
				refTickLine.LowerPx = pStock->LowerPrice / refParam.PriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->OpenInterest;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.PriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
			}
		}
		break;
	case XDF_ZJOPT:	///< 中金期权
		{
			XDFAPI_ZjOptData*		pStock = (XDFAPI_ZjOptData*)pSnapData;
			T_MAP_QUO::iterator		it = m_mapCFFOPT.find( std::string(pStock->Code) );

			if( it != m_mapCFFOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				::strcpy( refTickLine.Code, pStock->Code );
				refTickLine.Time = pStock->DataTimeStamp;
				refTickLine.PreClosePx = pStock->PreClose / refParam.PriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePrice / refParam.PriceRate;
				refTickLine.OpenPx = pStock->Open / refParam.PriceRate;
				refTickLine.HighPx = pStock->High / refParam.PriceRate;
				refTickLine.LowPx = pStock->Low / refParam.PriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.PriceRate;
				refTickLine.UpperPx = pStock->UpperPrice / refParam.PriceRate;
				refTickLine.LowerPx = pStock->LowerPrice / refParam.PriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->OpenInterest;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.PriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
			}
		}
		break;
	case XDF_CNF:	///< 商品期货(上海/郑州/大连)
		{
			XDFAPI_CNFutureData*		pStock = (XDFAPI_CNFutureData*)pSnapData;
			T_MAP_QUO::iterator			it = m_mapCNF.find( std::string(pStock->Code,6) );

			if( nTradeDate > 0 )
			{
				 s_nCNFTradingDate = nTradeDate;
			}

			if( it != m_mapCNF.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				refTickLine.Type = refParam.Type;
				::strncpy( refTickLine.Code, pStock->Code, 6 );
				refTickLine.Date = s_nCNFTradingDate;
				refTickLine.Time = pStock->DataTimeStamp;
				refTickLine.PreClosePx = pStock->PreClose / refParam.PriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePrice / refParam.PriceRate;
				refTickLine.OpenPx = pStock->Open / refParam.PriceRate;
				refTickLine.HighPx = pStock->High / refParam.PriceRate;
				refTickLine.LowPx = pStock->Low / refParam.PriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.PriceRate;
				refTickLine.UpperPx = pStock->UpperPrice / refParam.PriceRate;
				refTickLine.LowerPx = pStock->LowerPrice / refParam.PriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->OpenInterest;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.PriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
			}
		}
		break;
	case XDF_CNFOPT:///< 商品期权(上海/郑州/大连)
		{
			XDFAPI_CNFutOptData*		pStock = (XDFAPI_CNFutOptData*)pSnapData;
			T_MAP_QUO::iterator			it = m_mapCNFOPT.find( std::string(pStock->Code) );

			if( nTradeDate > 0 )
			{
				 s_nCNFOPTTradingDate = nTradeDate;
			}

			if( it != m_mapCNFOPT.end() )
			{
				T_LINE_PARAM&		refParam = it->second;
				///< ------------ Tick Lines -------------------------
				refTickLine.Type = refParam.Type;
				::strcpy( refTickLine.Code, pStock->Code );
				refTickLine.Date = s_nCNFOPTTradingDate;
				refTickLine.Time = pStock->DataTimeStamp;
				refTickLine.PreClosePx = pStock->PreClose / refParam.PriceRate;
				refTickLine.PreSettlePx = pStock->PreSettlePrice / refParam.PriceRate;
				refTickLine.OpenPx = pStock->Open / refParam.PriceRate;
				refTickLine.HighPx = pStock->High / refParam.PriceRate;
				refTickLine.LowPx = pStock->Low / refParam.PriceRate;
				refTickLine.ClosePx = pStock->Now / refParam.PriceRate;
				refTickLine.NowPx = refTickLine.ClosePx;
				refTickLine.SettlePx = pStock->SettlePrice / refParam.PriceRate;
				refTickLine.UpperPx = pStock->UpperPrice / refParam.PriceRate;
				refTickLine.LowerPx = pStock->LowerPrice / refParam.PriceRate;
				refTickLine.Amount = pStock->Amount;
				refTickLine.Volume = pStock->Volume;
				refTickLine.OpenInterest = pStock->OpenInterest;
				//refTickLine.NumTrades = 0;
				refTickLine.BidPx1 = pStock->Buy[0].Price;
				refTickLine.BidVol1 = pStock->Buy[0].Volume;
				refTickLine.BidPx2 = pStock->Buy[1].Price;
				refTickLine.BidVol2 = pStock->Buy[1].Volume;
				refTickLine.AskPx1 = pStock->Sell[0].Price;
				refTickLine.AskVol1 = pStock->Sell[0].Volume;
				refTickLine.AskPx2 = pStock->Sell[1].Price;
				refTickLine.AskVol2 = pStock->Sell[1].Volume;
				//refTickLine.Voip = pStock->Voip / refParam.PriceRate;
				nErrorCode = m_arrayTickLine.PutData( &refTickLine );
			}
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::UpdateTickLine() : invalid market id(%d)", (int)eMarket );
		break;
	}

	if( nErrorCode < 0 )
	{
		ServerStatus::GetStatusObj().AddTickLostRef();
		return -1;
	}

	ServerStatus::GetStatusObj().UpdateSecurity( (enum XDFMarket)(refTickLine.eMarketID), refTickLine.Code, refTickLine.NowPx, refTickLine.Amount, refTickLine.Volume );	///< 更新各市场的商品状态

	return 0;
}




