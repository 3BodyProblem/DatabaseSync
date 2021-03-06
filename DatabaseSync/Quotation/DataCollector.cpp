#include "DataCollector.h"
#include "../Configuration.h"
#include "../DatabaseSync.h"


/**
 * @brief					QuoteClientApi.DLL导出函数，用以创建数据源管理对象
 */
typedef QuoteClientApi*		(__stdcall *T_Func_CreateQuoteApi)( const char *pszDebugPath );

/**
 * @brief					获取查询接口
 */
typedef QuotePrimeApi *		(__stdcall *T_Func_CreatePrimeApi)();

/**
 * @brief					获取版本号
 */
typedef	const char *		(__stdcall *T_GetDllVersion)( int &nMajorVersion, int &nMinorVersion );
int	nMajorVersion, nMinorVersion;

CollectorStatus::CollectorStatus()
: m_eStatus( ET_SS_UNACTIVE ), m_nMarketID( -1 )
{
}

CollectorStatus::CollectorStatus( const CollectorStatus& obj )
{
	m_eStatus = obj.m_eStatus;
	m_nMarketID = obj.m_nMarketID;
}

enum E_SS_Status CollectorStatus::Get() const
{
	CriticalLock			lock( m_oCSLock );

	return m_eStatus;
}

bool CollectorStatus::Set( enum E_SS_Status eNewStatus )
{
	CriticalLock			lock( m_oCSLock );

	m_eStatus = eNewStatus;

	return true;
}


DataCollector::DataCollector()
 : m_pQuoteClientApi( NULL ), m_pQuotePrimeApi( NULL )
{
}

int DataCollector::Initialize( QuoteClientSpi* pIQuotationSpi )
{
	Release();
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "DataCollector::Initialize() : initializing data collector plugin ......" );

	int						nErrorCode = 0;
	T_Func_CreateQuoteApi	pFuncCreateApi = NULL;
	T_Func_CreatePrimeApi	pFuncCreatePrimeApi = NULL;
	T_GetDllVersion			pFuncGetDllVersion = NULL;
	std::string				sModulePath = GetModulePath(NULL) + Configuration::GetConfig().GetDataCollectorPluginPath();

	///< 加载行情源QuoteClientApi.DLL
	if( 0 != (nErrorCode = m_oDllPlugin.LoadDll( sModulePath )) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::Initialize() : failed 2 load data collector module, errorcode=%d", nErrorCode );
		return nErrorCode;
	}

	///< 提取导出函数
	pFuncCreateApi = (T_Func_CreateQuoteApi)m_oDllPlugin.GetDllFunction( "CreateQuoteApi" );
	if( NULL == pFuncCreateApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::Initialize() : invalid CreateQuoteApi() return (pointer is NULL)" );
		return -2;
	}

	pFuncCreatePrimeApi = (T_Func_CreatePrimeApi)m_oDllPlugin.GetDllFunction( "CreatePrimeApi" );
	if( NULL == pFuncCreatePrimeApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::Initialize() : invalid CreatePrimeApi() return (pointer is NULL)" );
		return -3;
	}

	pFuncGetDllVersion = (T_GetDllVersion)m_oDllPlugin.GetDllFunction( "GetDllVersion" );
	if( NULL == pFuncGetDllVersion )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::Initialize() : invalid GetDllVersion() return (pointer is NULL)" );
		return -4;
	}

	///< 创建控制类对象
	m_pQuoteClientApi = pFuncCreateApi( "" );
	m_pQuotePrimeApi = pFuncCreatePrimeApi();
	m_sPluginVersion = pFuncGetDllVersion( nMajorVersion, nMinorVersion );
	if( NULL == m_pQuoteClientApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::Initialize() : invalid QuoteClientApi* pointer (equal 2 NULL)" );
		return -5;
	}

	if( NULL == m_pQuotePrimeApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::Initialize() : invalid QuotePrimeApi* pointer (equal 2 NULL)" );
		return -6;
	}

	///< 初始化数据源模块
	if( m_pQuoteClientApi->Init() <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::Initialize() : failed 2 initialize m_pQuoteClientApi->Init()n" );
		return -7;
	}

	///< 注册行情和事件回调接口
	m_pQuoteClientApi->RegisterSpi( pIQuotationSpi );
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "DataCollector::Initialize() : data collector plugin is initialized, plugin version [%s] ......", m_sPluginVersion.c_str() );

	return 0;
}

void DataCollector::Release()
{
	if( NULL != m_pQuoteClientApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "DataCollector::Release() : releasing plugin ......" );
		m_pQuoteClientApi->EndWork();
		m_pQuoteClientApi->Release();
		m_pQuoteClientApi = NULL;
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "DataCollector::Release() : plugin is released ......" );
	}

	m_oDllPlugin.CloseDll();
}

std::string& DataCollector::GetVersion()
{
	return m_sPluginVersion;
}

void DataCollector::HaltDataCollector()
{
	if( NULL != m_pQuoteClientApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "DataCollector::HaltDataCollector() : [NOTICE] data collector is Halting ......" );

		m_pQuoteClientApi->EndWork();

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "DataCollector::HaltDataCollector() : [NOTICE] data collector Halted ......" );
	}
}

int DataCollector::RecoverDataCollector()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "DataCollector::RecoverDataCollector() : recovering data collector ......" );

	if( NULL == m_pQuoteClientApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::RecoverDataCollector() : invalid fuction pointer(NULL)" );
		return -1;
	}

	int			nErrorCode = 0;

	if( (nErrorCode=m_pQuoteClientApi->BeginWork()) <=0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "DataCollector::RecoverDataCollector() : failed 2 call m_pQuoteClientApi->BeginWork()" );
		return -2;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "DataCollector::RecoverDataCollector() : data collector recovered ......" );

	return nErrorCode;
}

QuoteClientApi* DataCollector::operator->()
{
	return m_pQuoteClientApi;
}

QuotePrimeApi* DataCollector::GetPrimeApi()
{
	return m_pQuotePrimeApi;
}

enum E_SS_Status DataCollector::InquireDataCollectorStatus( char* pszStatusDesc, unsigned int& nStrLen )
{
/*	if( NULL == m_pFuncGetStatus )
	{
		return ET_SS_UNACTIVE;
	}
*/
//	m_oCollectorStatus.Set( (enum E_SS_Status)m_pFuncGetStatus( pszStatusDesc, nStrLen ) );

	return m_oCollectorStatus.Get();
}














