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
const int		XDF_SH_COUNT = 16000;					///< �Ϻ�Lv1
const int		XDF_SHL2_COUNT = 0;						///< �Ϻ�Lv2(QuoteClientApi�ڲ�����)
const int		XDF_SHOPT_COUNT = 500;					///< �Ϻ���Ȩ
const int		XDF_SZ_COUNT = 8000;					///< ��֤Lv1
const int		XDF_SZL2_COUNT = 0;						///< ��֤Lv2(QuoteClientApi�ڲ�����)
const int		XDF_SZOPT_COUNT = 0;					///< ������Ȩ
const int		XDF_CF_COUNT = 500;						///< �н��ڻ�
const int		XDF_ZJOPT_COUNT = 500;					///< �н���Ȩ
const int		XDF_CNF_COUNT = 500;					///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
const int		XDF_CNFOPT_COUNT = 500;					///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
unsigned int	s_nNumberInSection = 50;				///< һ���г��п��Ի�����ٸ����ݿ�
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
		case XDF_SH:		///< �Ϻ�Lv1
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SHL2:		///< �Ϻ�Lv2(QuoteClientApi�ڲ�����)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SHOPT:		///< �Ϻ���Ȩ
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZ:		///< ��֤Lv1
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZL2:		///< ��֤Lv2(QuoteClientApi�ڲ�����)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_SZOPT:		///< ������Ȩ
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CF:		///< �н��ڻ�
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_ZJOPT:		///< �н���Ȩ
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CNF:		///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
			nBufferSize4Market = sizeof(T_TICK_LINE) * s_nNumberInSection;
			break;
		case XDF_CNFOPT:	///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
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

	if( false == m_oThdSnapSync.IsAlive() )
	{
		if( 0 != m_oThdSnapSync.Create( "ThreadSyncSnapshot()", ThreadSyncSnapshot, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create snapshot thread" );
			return -1;
		}
	}

	if( false == m_oThdNametableSync.IsAlive() )
	{
		if( 0 != m_oThdSnapSync.Create( "ThreadSyncNametable()", ThreadSyncNametable, this ) ) {
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 create nametable thread" );
			return -2;
		}
	}

	if( 0 != QuotationDatabase::GetDbObj().Initialize() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 initialize QuotationDatabase obj." );
		return -3;
	}

	if( 0 != QuotationDatabase::GetDbObj().EstablishConnection() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::Initialize() : failed 2 establish connection 4 QuotationDatabase obj." );
		return -4;
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

void* QuotationData::ThreadSyncSnapshot( void* pSelf )
{
	QuotationData&				refData = *(QuotationData*)pSelf;
	Quotation&					refQuotation = *((Quotation*)refData.m_pQuotation);

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			SimpleThread::Sleep( 1000 );
			refQuotation.SyncMarketsTime();						///< ���¸��г������ں�ʱ��
			refQuotation.SyncSnapshot2Database();				///< ���¸��г��������ݵ����ݿ�

/*
			char*					pBufPtr = CacheAlloc::GetObj().GetBufferPtr();
			unsigned int			nMaxDataNum = refData.m_arrayTickLine.GetRecordCount();
			std::string				sCode;
			std::ofstream			oDumper;
			STR_TICK_LINE			pszLine = { 0 };

			SimpleThread::Sleep( 1000 * 1 );
			if( NULL == pBufPtr || 0 == nMaxDataNum ) {
				continue;
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
			}
*/
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadSyncSnapshot() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadSyncSnapshot() : unknow exception" );
		}
	}

	return NULL;
}

void* QuotationData::ThreadSyncNametable( void* pSelf )
{
	QuotationData&				refData = *(QuotationData*)pSelf;
	Quotation&					refQuotation = *((Quotation*)refData.m_pQuotation);

	while( false == SimpleThread::GetGlobalStopFlag() )
	{
		try
		{
			SimpleThread::Sleep( 1000 );
			refQuotation.SyncNametable2Database();				///< �����г�������յȳ�ʼ�����ݵ����ݿ�
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadSyncNametable() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuotationData::ThreadSyncNametable() : unknow exception" );
		}
	}

	return NULL;
}

int QuotationData::UpdatePreName( enum XDFMarket eMarket, std::string& sCode, char* pszPreName, unsigned int nSize )
{
	switch( eMarket )
	{
	case XDF_SZ:	///< ��֤L1
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
	case XDF_SH:	///< ��֤L1
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
	case XDF_SHOPT:	///< ����
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
	case XDF_SZ:	///< ��֤L1
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
	case XDF_SZOPT:	///< ����
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
	case XDF_CF:	///< �н��ڻ�
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
	case XDF_ZJOPT:	///< �н���Ȩ
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
	case XDF_CNF:	///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
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
	case XDF_CNFOPT:///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
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

int QuotationData::UpdateTickLine( enum XDFMarket eMarket, char* pSnapData, unsigned int nSnapSize, T_TICK_LINE& refTickLine, unsigned int nTradeDate )
{
	int				nErrorCode = -1;
	unsigned int	nMachineDate = DateTime::Now().DateToLong();
	unsigned int	nMachineTime = DateTime::Now().TimeToLong() * 1000;

	refTickLine.eMarketID = eMarket;
	refTickLine.Date = nMachineDate;
	refTickLine.Time = nMachineTime;
	switch( eMarket )
	{
	case XDF_SH:	///< ��֤L1
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
					}
				}
				break;
			}
		}
		break;
	case XDF_SHOPT:	///< ����
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
			}
		}
		break;
	case XDF_SZ:	///< ��֤L1
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
					}
				}
				break;
			}
		}
		break;
	case XDF_SZOPT:	///< ����
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
			}
		}
		break;
	case XDF_CF:	///< �н��ڻ�
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
			}
		}
		break;
	case XDF_ZJOPT:	///< �н���Ȩ
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
			}
		}
		break;
	case XDF_CNF:	///< ��Ʒ�ڻ�(�Ϻ�/֣��/����)
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
			}
		}
		break;
	case XDF_CNFOPT:///< ��Ʒ��Ȩ(�Ϻ�/֣��/����)
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

	ServerStatus::GetStatusObj().UpdateSecurity( (enum XDFMarket)(refTickLine.eMarketID), refTickLine.Code, refTickLine.NowPx, refTickLine.Amount, refTickLine.Volume );	///< ���¸��г�����Ʒ״̬

	return 0;
}




