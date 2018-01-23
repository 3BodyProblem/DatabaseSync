#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "Quotation.h"
#include "SvrStatus.h"
#include "../Infrastructure/File.h"
#include "../DatabaseSync.h"
#include "../Database/DBOperation.h"


WorkStatus::WorkStatus()
: m_eWorkStatus( ET_SS_UNACTIVE )
{
}

WorkStatus::WorkStatus( const WorkStatus& refStatus )
{
	CriticalLock	section( m_oLock );

	m_eWorkStatus = refStatus.m_eWorkStatus;
}

WorkStatus::operator enum E_SS_Status()
{
	CriticalLock	section( m_oLock );

	return m_eWorkStatus;
}

std::string& WorkStatus::CastStatusStr( enum E_SS_Status eStatus )
{
	static std::string	sUnactive = "未激活";
	static std::string	sDisconnected = "断开状态";
	static std::string	sConnected = "连通状态";
	static std::string	sLogin = "登录成功";
	static std::string	sInitialized = "初始化中";
	static std::string	sWorking = "推送行情中";
	static std::string	sUnknow = "不可识别状态";

	switch( eStatus )
	{
	case ET_SS_UNACTIVE:
		return sUnactive;
	case ET_SS_DISCONNECTED:
		return sDisconnected;
	case ET_SS_CONNECTED:
		return sConnected;
	case ET_SS_LOGIN:
		return sLogin;
	case ET_SS_INITIALIZING:
		return sInitialized;
	case ET_SS_WORKING:
		return sWorking;
	default:
		return sUnknow;
	}
}

WorkStatus&	WorkStatus::operator= ( enum E_SS_Status eWorkStatus )
{
	CriticalLock	section( m_oLock );

	if( m_eWorkStatus != eWorkStatus )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "WorkStatus::operator=() : Exchange Session Status [%s]->[%s]"
											, CastStatusStr(m_eWorkStatus).c_str(), CastStatusStr(eWorkStatus).c_str() );
				
		m_eWorkStatus = eWorkStatus;
	}

	return *this;
}


///< ----------------------------------------------------------------
const unsigned int		MAX_SNAPSHOT_BUFFER_SIZE = sizeof(XDFAPI_StockData5) * 20000;		///< 最大快照缓存长度


Quotation::Quotation()
 : m_pDataBuffer( NULL )
{
}

Quotation::~Quotation()
{
	Release();

	if( NULL != m_pDataBuffer )
	{
		delete [] m_pDataBuffer;
		m_pDataBuffer = NULL;
	}
}

WorkStatus& Quotation::GetWorkStatus()
{
	return m_oWorkStatus;
}

int Quotation::Initialize()
{
	if( GetWorkStatus() == ET_SS_UNACTIVE )
	{
		unsigned int				nSec = 0;
		int							nErrCode = 0;

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : ............ [%s] Quotation Is Activating............" );

		if( (nErrCode = m_oQuoDataCenter.Initialize( this )) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 initialize DataCenter, errorcode = %d", nErrCode );
			Release();
			return -1;
		}

		if( (nErrCode = m_oQuotPlugin.Initialize( this )) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 initialize Quotation Plugin, errorcode = %d", nErrCode );
			Release();
			return -2;
		}

		if( (nErrCode = m_oQuotPlugin.RecoverDataCollector()) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 activate Quotation Plugin, errorcode = %d", nErrCode );
			Release();
			return -3;
		}

		if( NULL == m_pDataBuffer )
		{
			if( NULL == (m_pDataBuffer = new char[MAX_SNAPSHOT_BUFFER_SIZE]) )
			{
				QuoCollector::GetCollector()->OnLog( TLV_WARN, "Quotation::Initialize() : failed 2 allocate memory buffer" );
				return -4;
			}
		}

		m_oWorkStatus = ET_SS_WORKING;				///< 更新Quotation会话的状态
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Initialize() : ............ Quotation Activated!.............." );
	}

	return 0;
}

void Quotation::Halt()
{
	if( m_oWorkStatus != ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Halt() : ............ Halting .............." );

		m_oWorkStatus = ET_SS_UNACTIVE;			///< 更新Quotation会话的状态
//		m_oQuotPlugin.Release();				///< 释放行情源插件
//		m_oQuoDataCenter.Release();				///< 释放行情数据资源

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Halt() : ............ Halted! .............." );
	}
}

int Quotation::Release()
{
	if( m_oWorkStatus != ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroying .............." );

//		m_oWorkStatus = ET_SS_UNACTIVE;			///< 更新Quotation会话的状态
//		m_oQuotPlugin.Release();				///< 释放行情源插件
//		m_oQuoDataCenter.Release();				///< 释放行情数据资源

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::Release() : ............ Destroyed! .............." );
	}

	return 0;
}

std::string& Quotation::QuoteApiVersion()
{
	return m_oQuotPlugin.GetVersion();
}

int Quotation::SaveShLv1()
{
	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SH, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取上海的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1() : cannot fetch market infomation." );
		return -2;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1() : Loading... SHL1 WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取上海市场停牌标识 ----------------------------------------
	XDFAPI_ReqFuncParam		tagSHL1Param = { 0 };
	unsigned int			nFlagBufSize = sizeof(XDFAPI_StopFlag) * pKindHead->WareCount + 1024;
	char*					pszFlagBuf = new char[nFlagBufSize];
	tagSHL1Param.MkID = XDF_SH;
	tagSHL1Param.nBufLen = nFlagBufSize;
	nErrorCode = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 102, &tagSHL1Param, pszFlagBuf );
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszFlagBuf+m);
		char*				pbuf = pszFlagBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			char			pszCode[8] = { 0 };

			if( abs(pMsgHead->MsgType) == 23 )			///< stop flag
			{
				T_LINE_PARAM		tagParam = { 0 };
				XDFAPI_StopFlag*	pData = (XDFAPI_StopFlag*)pbuf;
				std::string			sCode( pData->Code, 6 );

				::memcpy( tagParam.Code, pData->Code, sizeof(pData->Code) );
				tagParam.IsTrading = 1;
				if( pData->SFlag == '*' )
				{
					tagParam.IsTrading = 0;
				}

				if( NULL == m_oQuoDataCenter.BuildSecurity( XDF_SH, sCode, tagParam ) )
				{
					QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1() : cannot build security[%s] attribute", sCode.c_str() );
				}

				pbuf += sizeof(XDFAPI_StopFlag);
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszFlagBuf )
	{
		delete [] pszFlagBuf;
		pszFlagBuf = NULL;
	}

	///< ---------------- 获取上海市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SH, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int				noffset = (sizeof(XDFAPI_NameTableSh) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
		char*			pszCodeBuf = new char[noffset];
		T_LINE_PARAM	tagParam = { 0 };

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SH, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				if( abs(pMsgHead->MsgType) == 5 )
				{
					XDFAPI_NameTableSh*		pData = (XDFAPI_NameTableSh*)pbuf;
					T_LINE_PARAM*			pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( pData->Code, 6 ), tagParam );

					///< 构建商品集合
					if( NULL != pTagParam )
					{
						::memcpy( pTagParam->Name, pData->Name, sizeof(pData->Name) );
						pTagParam->Type = pData->SecKind;
						pTagParam->ExchangeID = 1;
						//pTagParam->ContractMulti = 0;
						pTagParam->LotSize = vctKindInfo[pData->SecKind].LotSize;
						pTagParam->PriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						pTagParam->PriceTick = 0;
						pTagParam->PreClosePx = 0;
						//pTagParam->PreSettlePx = 0;
						//pTagParam->PrePosition = 0;
						pTagParam->UpperPrice = 0;
						pTagParam->LowerPrice = 0;
						pTagParam->FluctuationPercent = 0;		///< 涨跌幅度(用收盘价计算)
						pTagParam->TradingVolume = 0;
						pTagParam->TradingDate = ServerStatus::GetStatusObj().FetchMkDate( XDF_SH );
						::sprintf( pTagParam->ClassID, "%d", (int)(tagParam.Type) );
					}

					ServerStatus::GetStatusObj().AnchorSecurity( XDF_SH, pTagParam->Code, pTagParam->Name );

					nNum++;
					pbuf += sizeof(XDFAPI_NameTableSh);
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1() : Loading... SHL1 Nametable Size = %d", nNum );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShLv1() : cannot fetch nametable" );
		return -3;
	}

	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_StockData5) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SH, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		T_LINE_PARAM		tagParam = { 0 };
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		QuotationDatabase::GetDbObj().StartTransaction();
		for( int i = 0; i < MsgCount; i++ )
		{
			T_LINE_PARAM*	pTagParam = NULL;
			T_TICK_LINE		tagTickLine = { 0 };

			if( abs(pMsgHead->MsgType) == 21 )			///< 指数
			{
				XDFAPI_IndexData*		pData = (XDFAPI_IndexData*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData), tagTickLine );	///< Tick线
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( pData->Code, 6 ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					//pTagParam->ContractMulti = 0;
					pTagParam->PriceTick = 0;
					pTagParam->PreClosePx = 0;
					//pTagParam->PreSettlePx = 0;
					//pTagParam->PrePosition = 0;
					pTagParam->UpperPrice = 0;
					pTagParam->LowerPrice = 0;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}

				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< 快照数据
			{
				XDFAPI_StockData5*		pData = (XDFAPI_StockData5*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5), tagTickLine );	///< Tick线
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( pData->Code, 6 ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					//pTagParam->ContractMulti = 0;
					pTagParam->PriceTick = 0;
					pTagParam->PreClosePx = 0;
					//pTagParam->PreSettlePx = 0;
					//pTagParam->PrePosition = 0;
					pTagParam->UpperPrice = 0;
					pTagParam->LowerPrice = 0;
					pTagParam->FluctuationPercent = 0;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}

				pbuf += sizeof(XDFAPI_StockData5);
				nNum++;
			}

			if( NULL != pTagParam )
			{
				QuotationDatabase::GetDbObj().Replace_Commodity( pTagParam->Type, '1', tagTickLine.Code, pTagParam->Name, pTagParam->LotSize, pTagParam->ContractMulti, pTagParam->PriceTick, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx
					, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.OpenPx, tagTickLine.SettlePx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx
					, pTagParam->FluctuationPercent, tagTickLine.Volume, pTagParam->TradingVolume, tagTickLine.Amount, pTagParam->IsTrading, pTagParam->TradingDate, 0, pTagParam->ClassID );
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		QuotationDatabase::GetDbObj().Commit();						///< 提交mysql事务
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShLv1() : Loading... SHL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveShOpt()
{
	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SHOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取上海期权的基础信息 -----------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShOpt() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShOpt() : Loading... SHOPT WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取上海期权的市场码表数据 ------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SHOPT, NULL, NULL, nCodeCount );				///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int				noffset = (sizeof(XDFAPI_NameTableShOpt) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
		char*			pszCodeBuf = new char[noffset];
		T_LINE_PARAM	tagParam = { 0 };

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SHOPT, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf + m + sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				if( abs(pMsgHead->MsgType) == 2 )
				{
					XDFAPI_NameTableShOpt*	pData = (XDFAPI_NameTableShOpt*)pbuf;
					T_LINE_PARAM*			pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SHOPT, std::string( pData->Code, 8 ), tagParam );

					///< 构建商品集合
					if( NULL != pTagParam )
					{
						::memcpy( pTagParam->Name, pData->Name, sizeof(pData->Name) );
						pTagParam->Type = pData->SecKind;
						pTagParam->ExchangeID = 1;
						pTagParam->PriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						pTagParam->ContractUnit = pData->ContractUnit;
						//pTagParam->ContractMulti = 0;
						pTagParam->LotSize = vctKindInfo[pData->SecKind].LotSize;
						pTagParam->PriceTick = 0;
						pTagParam->PreClosePx = pData->PreClosePx / pTagParam->PriceRate;
						pTagParam->PreSettlePx = pData->PreSettlePx / pTagParam->PriceRate;
						pTagParam->PrePosition = pData->LeavesQty / pTagParam->PriceRate;
						pTagParam->UpperPrice = pData->UpLimit / pTagParam->PriceRate;
						pTagParam->LowerPrice = pData->DownLimit / pTagParam->PriceRate;
						pTagParam->FluctuationPercent = 0;		///< 涨跌幅度(用收盘价计算)
						pTagParam->TradingVolume = 0;
						pTagParam->TradingDate = ServerStatus::GetStatusObj().FetchMkDate( XDF_SH );
						::sprintf( pTagParam->ClassID, "%d", (int)(tagParam.Type) );
						pTagParam->IsTrading = (pData->StatusFlag[1] == '0');
					}

					ServerStatus::GetStatusObj().AnchorSecurity( XDF_SHOPT, pTagParam->Code, pTagParam->Name );

					pbuf += sizeof(XDFAPI_NameTableShOpt);
					nNum++;
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveShOpt() : Loading... SHOPT Nametable Size = %d", nNum );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveShOpt() : cannot fetch nametable" );
		return -3;
	}

	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_ShOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SHOPT, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		T_LINE_PARAM		tagParam = { 0 };
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		QuotationDatabase::GetDbObj().StartTransaction();
		for( int i = 0; i < MsgCount; i++ )
		{
			T_LINE_PARAM*	pTagParam = NULL;
			T_TICK_LINE		tagTickLine = { 0 };

			if( abs(pMsgHead->MsgType) == 15 )			///< 指数
			{
				XDFAPI_ShOptData*		pData = (XDFAPI_ShOptData*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData), tagTickLine );
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SHOPT, std::string( pData->Code, 8 ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					//pTagParam->ContractMulti = 0;
					pTagParam->PreSettlePx = pData->PreSettlePx / pTagParam->PriceRate;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}

				pbuf += sizeof(XDFAPI_ShOptData);
				nNum++;
			}

			if( NULL != pTagParam )
			{
				QuotationDatabase::GetDbObj().Replace_Commodity( pTagParam->Type, '1', tagTickLine.Code, pTagParam->Name, pTagParam->LotSize, pTagParam->ContractMulti, pTagParam->PriceTick, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx
					, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.OpenPx, tagTickLine.SettlePx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx
					, pTagParam->FluctuationPercent, tagTickLine.Volume, pTagParam->TradingVolume, tagTickLine.Amount, pTagParam->IsTrading, pTagParam->TradingDate, 0, pTagParam->ClassID );
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		QuotationDatabase::GetDbObj().Commit();
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSHOPT() : Loading... SHLOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveSzLv1()
{
	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZ, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取深圳的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1() : Loading... SZL1 WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取深圳市场停牌标识 ----------------------------------------
	XDFAPI_ReqFuncParam		tagSZL1Param = { 0 };
	unsigned int			nFlagBufSize = sizeof(XDFAPI_StopFlag) * pKindHead->WareCount + 1024;
	char*					pszFlagBuf = new char[nFlagBufSize];
	tagSZL1Param.MkID = XDF_SZ;
	tagSZL1Param.nBufLen = nFlagBufSize;
	nErrorCode = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 102, &tagSZL1Param, pszFlagBuf );
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszFlagBuf+m);
		char*				pbuf = pszFlagBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			char			pszCode[8] = { 0 };

			if( abs(pMsgHead->MsgType) == 23 )			///< stop flag
			{
				T_LINE_PARAM		tagParam = { 0 };
				XDFAPI_StopFlag*	pData = (XDFAPI_StopFlag*)pbuf;
				std::string			sCode( pData->Code, 6 );

				::memcpy( tagParam.Code, pData->Code, sizeof(pData->Code) );
				tagParam.IsTrading = 1;
				if( pData->SFlag == '*' )
				{
					tagParam.IsTrading = 0;
				}

				if( NULL == m_oQuoDataCenter.BuildSecurity( XDF_SZ, sCode, tagParam ) )
				{
					QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1() : cannot build security[%s] attribute", sCode.c_str() );
				}

				pbuf += sizeof(XDFAPI_StopFlag);
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszFlagBuf )
	{
		delete [] pszFlagBuf;
		pszFlagBuf = NULL;
	}

	///< ---------------- 获取深圳市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZ, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int				noffset = (sizeof(XDFAPI_NameTableSz) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
		char*			pszCodeBuf = new char[noffset];
		T_LINE_PARAM	tagParam = { 0 };

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZ, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				if( abs(pMsgHead->MsgType) == 6 )
				{
					XDFAPI_NameTableSz*		pData = (XDFAPI_NameTableSz*)pbuf;
					T_LINE_PARAM*			pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( pData->Code, 6 ), tagParam );

					///< 构建商品集合
					if( NULL != pTagParam )
					{
						::memcpy( pTagParam->Name, pData->Name, sizeof(pData->Name) );
						pTagParam->Type = pData->SecKind;
						pTagParam->ExchangeID = 1;
						//pTagParam->ContractMulti = 0;
						pTagParam->LotSize = vctKindInfo[pData->SecKind].LotSize;
						pTagParam->PriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						pTagParam->PriceTick = 0;
						pTagParam->PreClosePx = 0;
						//pTagParam->PreSettlePx = 0;
						//pTagParam->PrePosition = 0;
						pTagParam->UpperPrice = 0;
						pTagParam->LowerPrice = 0;
						pTagParam->FluctuationPercent = 0;		///< 涨跌幅度(用收盘价计算)
						pTagParam->TradingVolume = 0;
						pTagParam->TradingDate = ServerStatus::GetStatusObj().FetchMkDate( XDF_SZ );
						::sprintf( pTagParam->ClassID, "%d", (int)(tagParam.Type) );
					}

					ServerStatus::GetStatusObj().AnchorSecurity( XDF_SZ, pTagParam->Code, pTagParam->Name );

					pbuf += sizeof(XDFAPI_NameTableSz);
					nNum++;
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1() : Loading... SZL1 Nametable Size = %d", nNum );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzLv1() : cannot fetch nametable" );
		return -3;
	}

	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_StockData5) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SZ, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		T_LINE_PARAM		tagParam = { 0 };
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		QuotationDatabase::GetDbObj().StartTransaction();
		for( int i = 0; i < MsgCount; i++ )
		{
			T_LINE_PARAM*	pTagParam = NULL;
			T_TICK_LINE		tagTickLine = { 0 };

			if( abs(pMsgHead->MsgType) == 21 )			///< 指数
			{
				XDFAPI_IndexData*		pData = (XDFAPI_IndexData*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData), tagTickLine );		///< Tick线
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( pData->Code, 6 ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					//pTagParam->ContractMulti = 0;
					pTagParam->PriceTick = 0;
					pTagParam->PreClosePx = 0;
					//pTagParam->PreSettlePx = 0;
					//pTagParam->PrePosition = 0;
					pTagParam->UpperPrice = 0;
					pTagParam->LowerPrice = 0;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}


				pbuf += sizeof(XDFAPI_IndexData);
				nNum++;
			}
			else if( abs(pMsgHead->MsgType) == 22 )		///< 快照数据
			{
				XDFAPI_StockData5*		pData = (XDFAPI_StockData5*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5), tagTickLine );	///< Tick线
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( pData->Code, 6 ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					//pTagParam->ContractMulti = 0;
					pTagParam->PriceTick = 0;
					pTagParam->PreClosePx = 0;
					//pTagParam->PreSettlePx = 0;
					//pTagParam->PrePosition = 0;
					pTagParam->UpperPrice = 0;
					pTagParam->LowerPrice = 0;
					pTagParam->FluctuationPercent = 0;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}


				pbuf += sizeof(XDFAPI_StockData5);
				nNum++;
			}

			if( NULL != pTagParam )
			{
				QuotationDatabase::GetDbObj().Replace_Commodity( pTagParam->Type, '2', tagTickLine.Code, pTagParam->Name, pTagParam->LotSize, pTagParam->ContractMulti, pTagParam->PriceTick, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx
					, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.OpenPx, tagTickLine.SettlePx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx
					, pTagParam->FluctuationPercent, tagTickLine.Volume, pTagParam->TradingVolume, tagTickLine.Amount, pTagParam->IsTrading, pTagParam->TradingDate, 0, pTagParam->ClassID );
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		QuotationDatabase::GetDbObj().Commit();
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzLv1() : Loading... SZL1 Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveSzOpt()
{
	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_SZOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取深圳期权的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzOpt() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveSzOpt() : Loading... SZOPT WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取深圳期权市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZOPT, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int				noffset = (sizeof(XDFAPI_NameTableSzOpt) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
		char*			pszCodeBuf = new char[noffset];
		T_LINE_PARAM	tagParam = { 0 };

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_SZOPT, pszCodeBuf, noffset, nCodeCount );		///< 获取码表
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				if( abs(pMsgHead->MsgType) == 9 )
				{
					XDFAPI_NameTableSzOpt*	pData = (XDFAPI_NameTableSzOpt*)pbuf;
					T_LINE_PARAM*			pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SZOPT, std::string( pData->Code, 8 ), tagParam );

					///< 构建商品集合
					if( NULL != pTagParam )
					{
						::memcpy( pTagParam->Name, pData->Name, sizeof(pData->Name) );
						pTagParam->Type = pData->SecKind;
						pTagParam->ExchangeID = 1;
						//pTagParam->ContractMulti = 0;
						pTagParam->LotSize = vctKindInfo[pData->SecKind].LotSize;
						pTagParam->PriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						pTagParam->PriceTick = 0;
						pTagParam->ContractUnit = pData->ContractUnit;
						pTagParam->PreClosePx = pData->PreClosePx / pTagParam->PriceRate;
						pTagParam->PreSettlePx = pData->PreSettlePx / pTagParam->PriceRate;
						pTagParam->PrePosition = pData->LeavesQty / pTagParam->PriceRate;
						pTagParam->UpperPrice = pData->UpLimit / pTagParam->PriceRate;
						pTagParam->LowerPrice = pData->DownLimit / pTagParam->PriceRate;
						pTagParam->FluctuationPercent = 0;		///< 涨跌幅度(用收盘价计算)
						pTagParam->TradingVolume = 0;
						pTagParam->TradingDate = ServerStatus::GetStatusObj().FetchMkDate( XDF_SH );
						::sprintf( pTagParam->ClassID, "%d", (int)(tagParam.Type) );
					}

					ServerStatus::GetStatusObj().AnchorSecurity( XDF_SZOPT, pTagParam->Code, pTagParam->Name );

					pbuf += sizeof(XDFAPI_NameTableSzOpt);
					nNum++;
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSZOPT() : Loading... SZOPT Nametable Size = %d", nNum );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveSzOpt() : cannot fetch nametable" );
		return -3;
	}

	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_SzOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SZOPT, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		T_LINE_PARAM		tagParam = { 0 };
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		QuotationDatabase::GetDbObj().StartTransaction();
		for( int i = 0; i < MsgCount; i++ )
		{
			T_LINE_PARAM*	pTagParam = NULL;
			T_TICK_LINE		tagTickLine = { 0 };

			if( abs(pMsgHead->MsgType) == 35 )			///< 指数
			{
				XDFAPI_SzOptData*		pData = (XDFAPI_SzOptData*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData), tagTickLine );
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SZOPT, std::string( pData->Code, 8 ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					//pTagParam->ContractMulti = 0;
					pTagParam->PreClosePx = tagTickLine.PreClosePx;
					pTagParam->PreSettlePx = tagTickLine.PreSettlePx;
					//pTagParam->PrePosition = 0;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}

				pbuf += sizeof(XDFAPI_SzOptData);
				nNum++;
			}

			if( NULL != pTagParam )
			{
				QuotationDatabase::GetDbObj().Replace_Commodity( pTagParam->Type, '2', tagTickLine.Code, pTagParam->Name, pTagParam->LotSize, pTagParam->ContractMulti, pTagParam->PriceTick, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx
					, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.OpenPx, tagTickLine.SettlePx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx
					, pTagParam->FluctuationPercent, tagTickLine.Volume, pTagParam->TradingVolume, tagTickLine.Amount, pTagParam->IsTrading, pTagParam->TradingDate, 0, pTagParam->ClassID );
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		QuotationDatabase::GetDbObj().Commit();
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::ReloadSZOPT() : Loading... SZOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveCFF()
{
	int						nNum = 0;
	std::ofstream			oDumper;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CF, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取中金期货的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCFF() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFF() : Loading... CFF WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取中金期货市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CF, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int				noffset = (sizeof(XDFAPI_NameTableZjqh) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
		char*			pszCodeBuf = new char[noffset];
		T_LINE_PARAM	tagParam = { 0 };

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CF, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				if( abs(pMsgHead->MsgType) == 4 )
				{
					XDFAPI_NameTableZjqh*	pData = (XDFAPI_NameTableZjqh*)pbuf;
					T_LINE_PARAM*			pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CF, std::string( pData->Code, 6 ), tagParam );

					///< 构建商品集合
					if( NULL != pTagParam )
					{
						::memcpy( pTagParam->Name, pData->Name, sizeof(pData->Name) );
						pTagParam->Type = pData->SecKind;
						pTagParam->ExchangeID = 3;
						pTagParam->ContractMulti = pData->ContractMult;
						pTagParam->LotSize = vctKindInfo[pData->SecKind].LotSize;
						pTagParam->PriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						pTagParam->PriceTick = 0;
						//pTagParam->PreClosePx = 0;
						//pTagParam->PreSettlePx = 0;
						//pTagParam->PrePosition = 0;
						pTagParam->UpperPrice = 0;
						pTagParam->LowerPrice = 0;
						pTagParam->FluctuationPercent = 0;		///< 涨跌幅度(用收盘价计算)
						pTagParam->TradingVolume = 0;
						pTagParam->TradingDate = ServerStatus::GetStatusObj().FetchMkDate( XDF_CF );
						::sprintf( pTagParam->ClassID, "%d", (int)(tagParam.Type) );
					}

					ServerStatus::GetStatusObj().AnchorSecurity( XDF_CF, pTagParam->Code, pTagParam->Name );

					pbuf += sizeof(XDFAPI_NameTableZjqh);
					nNum++;
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFF() : Loading... CFF Nametable Size = %d", nNum );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCFF() : cannot fetch nametable" );
		return -3;
	}

	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CffexData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CF, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		T_LINE_PARAM		tagParam = { 0 };
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		QuotationDatabase::GetDbObj().StartTransaction();
		for( int i = 0; i < MsgCount; i++ )
		{
			T_LINE_PARAM*	pTagParam = NULL;
			T_TICK_LINE		tagTickLine = { 0 };

			if( abs(pMsgHead->MsgType) == 20 )			///< 指数
			{
				XDFAPI_CffexData*		pData = (XDFAPI_CffexData*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData), tagTickLine );
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CF, std::string( pData->Code, 6 ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					//pTagParam->ContractMulti = 0;
					pTagParam->PreClosePx = tagTickLine.PreClosePx;
					pTagParam->PreSettlePx = tagTickLine.PreSettlePx;
					pTagParam->PrePosition = pData->PreOpenInterest;
					pTagParam->UpperPrice = tagTickLine.UpperPx;
					pTagParam->LowerPrice = tagTickLine.LowerPx;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}

				pbuf += sizeof(XDFAPI_CffexData);
				nNum++;
			}

			if( NULL != pTagParam )
			{
				QuotationDatabase::GetDbObj().Replace_Commodity( pTagParam->Type, '3', tagTickLine.Code, pTagParam->Name, pTagParam->LotSize, pTagParam->ContractMulti, pTagParam->PriceTick, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx
					, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.OpenPx, tagTickLine.SettlePx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx
					, pTagParam->FluctuationPercent, tagTickLine.Volume, pTagParam->TradingVolume, tagTickLine.Amount, pTagParam->IsTrading, pTagParam->TradingDate, 0, pTagParam->ClassID );
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		QuotationDatabase::GetDbObj().Commit();
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFF() : Loading... CFF Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveCFFOPT()
{
	return -1;

	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_ZJOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取中金期权的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCFFOPT() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFFOPT() : Loading... CFFOPT WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取中金期权市场码表数据 ----------------------------------------
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_ZJOPT, NULL, NULL, nCodeCount );				///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int		noffset = (sizeof(XDFAPI_NameTableZjOpt) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;///< 根据商品数量，分配获取快照表需要的缓存
		char*	pszCodeBuf = new char[noffset];

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_ZJOPT, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				if( abs(pMsgHead->MsgType) == 3 )
				{
					std::ofstream			oDumper;
					T_LINE_PARAM			tagParam = { 0 };
					char					pszLine[1024] = { 0 };
					XDFAPI_NameTableZjOpt*	pData = (XDFAPI_NameTableZjOpt*)pbuf;

					///< 行情数据集合
//					m_oQuoDataCenter.BuildSecurity( XDF_ZJOPT, std::string( pData->Code ), tagParam );
/*
					tagStaticLine.Type = 0;
					tagStaticLine.eMarketID = XDF_ZJOPT;
					tagStaticLine.Date = DateTime::Now().DateToLong();
					::memcpy( tagStaticLine.Code, pData->Code, sizeof(pData->Code) );
					//::memcpy( tagStaticLine.Name, pData->Name, sizeof(pData->Name) );
					tagStaticLine.LotSize = vctKindInfo[pData->SecKind].LotSize;
					tagStaticLine.ContractMult = pData->ContractMult;
					tagStaticLine.ContractUnit = pData->ContractUnit;
					tagStaticLine.StartDate = pData->StartDate;
					tagStaticLine.EndDate = pData->EndDate;
					tagStaticLine.XqDate = pData->XqDate;
					tagStaticLine.DeliveryDate = pData->DeliveryDate;
					tagStaticLine.ExpireDate = pData->ExpireDate;
					::memcpy( tagStaticLine.UnderlyingCode, pData->ObjectCode, sizeof(pData->ObjectCode) );*/
					//::memcpy( tagStaticLine.UnderlyingName, pData->UnderlyingName, sizeof(pData->UnderlyingName) );
					//tagStaticLine.OptionType = pData->OptionType;		///< 期权类型：'E'-欧式 'A'-美式
					//tagStaticLine.CallOrPut = pData->CallOrPut;			///< 认沽认购：'C'认购 'P'认沽
					//tagStaticLine.ExercisePx = pData->XqPrice / tagParam.PriceRate;
//					ServerStatus::GetStatusObj().AnchorSecurity( XDF_ZJOPT, tagStaticLine.Code, tagStaticLine.Name );

					pbuf += sizeof(XDFAPI_NameTableZjOpt);
					nNum++;
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFFOPT() : Loading... CFFOPT Nametable Size = %d", nNum );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCFFOPT() : cannot fetch nametable" );
		return -3;
	}

	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_ZjOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_ZJOPT, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		for( int i = 0; i < MsgCount; i++ )
		{
			T_TICK_LINE		tagTickLine = { 0 };

			if( abs(pMsgHead->MsgType) == 18 )			///< 指数
			{
//				m_oQuoDataCenter.UpdateTickLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData), tagTickLine );

				pbuf += sizeof(XDFAPI_ZjOptData);
				nNum++;
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCFFOPT() : Loading... CFFOPT Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveCNF()
{
	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CNF, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取商品期货的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCNF() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNF() : Loading... CNF WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取商品期货市场码表数据 ----------------------------------------
	char	cMkID = XDF_CNF;
	XDFAPI_MarketStatusInfo	tagStatus = { 0 };
	cMkID = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &cMkID, &tagStatus );
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNF, NULL, NULL, nCodeCount );				///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int				noffset = (sizeof(XDFAPI_NameTableCnf) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
		char*			pszCodeBuf = new char[noffset];
		T_LINE_PARAM	tagParam = { 0 };

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNF, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				if( abs(pMsgHead->MsgType) == 7 )
				{
					XDFAPI_NameTableCnf*	pData = (XDFAPI_NameTableCnf*)pbuf;
					T_LINE_PARAM*			pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CNF, std::string( pData->Code, 6 ), tagParam );

					///< 构建商品集合
					if( NULL != pTagParam )
					{
						::memcpy( pTagParam->Code, pData->Code, sizeof(pData->Code) );
						::memcpy( pTagParam->Name, pData->Name, sizeof(pData->Name) );
						pTagParam->Type = pData->SecKind;
						pTagParam->ExchangeID = 1;
						//pTagParam->ContractMulti = 0;
						pTagParam->LotSize = vctKindInfo[pData->SecKind].LotSize;
						pTagParam->PriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						pTagParam->PriceTick = 0;
						pTagParam->PreClosePx = 0;
						//pTagParam->PreSettlePx = 0;
						//pTagParam->PrePosition = 0;
						pTagParam->UpperPrice = 0;
						pTagParam->LowerPrice = 0;
						pTagParam->FluctuationPercent = 0;		///< 涨跌幅度(用收盘价计算)
						pTagParam->TradingVolume = 0;
						pTagParam->TradingDate = ServerStatus::GetStatusObj().FetchMkDate( XDF_SH );
						::sprintf( pTagParam->ClassID, "%d", (int)(tagParam.Type) );
					}

					ServerStatus::GetStatusObj().AnchorSecurity( XDF_CNF, pTagParam->Code, pTagParam->Name );

					pbuf += sizeof(XDFAPI_NameTableCnf);
					nNum++;
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNF() : Loading... CNF Nametable Size = %d", nNum );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCNF() : cannot fetch nametable" );
		return -3;
	}

	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CNFutureData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];
	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CNF, pszCodeBuf, noffset );			///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		T_LINE_PARAM		tagParam = { 0 };
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		QuotationDatabase::GetDbObj().StartTransaction();
		for( int i = 0; i < MsgCount; i++ )
		{
			T_LINE_PARAM*	pTagParam = NULL;
			T_TICK_LINE		tagTickLine = { 0 };

			if( abs(pMsgHead->MsgType) == 26 )			///< 指数
			{
				XDFAPI_CNFutureData*		pData = (XDFAPI_CNFutureData*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData), tagTickLine, tagStatus.MarketDate );
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CNF, std::string( pData->Code, 6 ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					pTagParam->ContractMulti = 0;
					pTagParam->ContractUnit = 0;
					pTagParam->PreClosePx = tagTickLine.PreClosePx;
					pTagParam->PreSettlePx = tagTickLine.PreSettlePx;
					pTagParam->PrePosition = pTagParam->PrePosition;
					pTagParam->UpperPrice = tagTickLine.UpperPx;
					pTagParam->LowerPrice = tagTickLine.LowerPx;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}

				pbuf += sizeof(XDFAPI_CNFutureData);
				nNum++;
			}

			if( NULL != pTagParam )
			{
				QuotationDatabase::GetDbObj().Replace_Commodity( pTagParam->Type, '4', tagTickLine.Code, pTagParam->Name, pTagParam->LotSize, pTagParam->ContractMulti, pTagParam->PriceTick, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx
					, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.OpenPx, tagTickLine.SettlePx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx
					, pTagParam->FluctuationPercent, tagTickLine.Volume, pTagParam->TradingVolume, tagTickLine.Amount, pTagParam->IsTrading, pTagParam->TradingDate, 0, pTagParam->ClassID );
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		QuotationDatabase::GetDbObj().Commit();
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNF() : Loading... CNF Snaptable Size = %d", nNum );

	return 0;
}

int Quotation::SaveCNFOPT()
{
	int						nNum = 0;
	int						nCodeCount = 0;
	int						nKindCount = 0;
	XDFAPI_MarketKindInfo	vctKindInfo[32] = { 0 };
	char					tempbuf[8192] = { 0 };
	int						nErrorCode = m_oQuotPlugin->GetMarketInfo( XDF_CNFOPT, tempbuf, sizeof(tempbuf) );

	///< -------------- 获取商品期权的基础信息 --------------------------------------------
	if( nErrorCode <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCNFOPT() : cannot fetch market infomation." );
		return -1;
	}

	XDFAPI_MarketKindHead* pKindHead = (XDFAPI_MarketKindHead*)(tempbuf+ sizeof(XDFAPI_MsgHead));
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNFOPT() : Loading... CNFOPT WareCount = %d", pKindHead->WareCount );

	int m = sizeof(XDFAPI_MsgHead)+sizeof(XDFAPI_MarketKindHead);
	for( int i = 0; m < nErrorCode; )
	{
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(tempbuf + m);
		char*				pbuf = tempbuf + m +sizeof(XDFAPI_UniMsgHead);
		int					nMsgCount = pMsgHead->MsgCount;

		while( nMsgCount-- > 0 )
		{
			XDFAPI_MarketKindInfo* pInfo = (XDFAPI_MarketKindInfo*)pbuf;
			::memcpy( vctKindInfo + nKindCount++, pInfo, sizeof(XDFAPI_MarketKindInfo) );
			pbuf += sizeof(XDFAPI_MarketKindInfo);
		}

		m += sizeof(XDFAPI_MsgHead) + pMsgHead->MsgLen;
	}

	///< ---------------- 获取商品期权市场码表数据 ----------------------------------------
	char	cMkID = XDF_CNFOPT;
	XDFAPI_MarketStatusInfo	tagStatus = { 0 };
	cMkID = m_oQuotPlugin.GetPrimeApi()->ReqFuncData( 101, &cMkID, &tagStatus );
	nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNFOPT, NULL, NULL, nCodeCount );					///< 先获取一下商品数量
	if( nErrorCode > 0 && nCodeCount > 0 )
	{
		int				noffset = (sizeof(XDFAPI_NameTableCnfOpt) + sizeof(XDFAPI_UniMsgHead))*nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
		char*			pszCodeBuf = new char[noffset];
		T_LINE_PARAM	tagParam = { 0 };

		nErrorCode = m_oQuotPlugin->GetCodeTable( XDF_CNFOPT, pszCodeBuf, noffset, nCodeCount );	///< 获取码表
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
			char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				if( abs(pMsgHead->MsgType) == 11 )
				{
					XDFAPI_NameTableCnfOpt*	pData = (XDFAPI_NameTableCnfOpt*)pbuf;
					T_LINE_PARAM*			pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CNFOPT, std::string( pData->Code ), tagParam );

					///< 构建商品集合
					if( NULL != pTagParam )
					{
						::memcpy( pTagParam->Code, pData->Code, sizeof(pTagParam->Code) );
						::memcpy( pTagParam->Name, pData->Name, sizeof(pData->Name) );
						pTagParam->Type = pData->SecKind;
						pTagParam->ExchangeID = 1;
						pTagParam->ContractMulti = pData->ContractMult;
						pTagParam->LotSize = vctKindInfo[pData->SecKind].LotSize;
						pTagParam->PriceRate = ::pow(10.0, int(vctKindInfo[pData->SecKind].PriceRate) );
						pTagParam->PriceTick = 0;
						pTagParam->PreClosePx = 0;
						//pTagParam->PreSettlePx = 0;
						//pTagParam->PrePosition = 0;
						pTagParam->UpperPrice = 0;
						pTagParam->LowerPrice = 0;
						pTagParam->FluctuationPercent = 0;		///< 涨跌幅度(用收盘价计算)
						pTagParam->TradingVolume = 0;
						pTagParam->TradingDate = ServerStatus::GetStatusObj().FetchMkDate( XDF_CNFOPT );
						::sprintf( pTagParam->ClassID, "%d", (int)(tagParam.Type) );
					}

					ServerStatus::GetStatusObj().AnchorSecurity( XDF_CNFOPT, pTagParam->Code, pTagParam->Name );

					pbuf += sizeof(XDFAPI_NameTableCnfOpt);
					nNum++;
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}

		if( NULL != pszCodeBuf )
		{
			delete []pszCodeBuf;
			pszCodeBuf = NULL;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNFOPT() : Loading... CNFOPT Nametable Size = %d", nNum );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Quotation::SaveCNFOPT() : cannot fetch nametable" );
		return -3;
	}

	nNum = 0;
	///< ---------------- 获取快照表数据 -------------------------------------------
	int		noffset = (sizeof(XDFAPI_CNFutOptData) + sizeof(XDFAPI_UniMsgHead)) * nCodeCount;	///< 根据商品数量，分配获取快照表需要的缓存
	char*	pszCodeBuf = new char[noffset];

	nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CNFOPT, pszCodeBuf, noffset );		///< 获取快照
	for( int m = 0; m < nErrorCode; )
	{
		T_LINE_PARAM		tagParam = { 0 };
		XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(pszCodeBuf+m);
		char*				pbuf = pszCodeBuf+m +sizeof(XDFAPI_UniMsgHead);
		int					MsgCount = pMsgHead->MsgCount;

		QuotationDatabase::GetDbObj().StartTransaction();
		for( int i = 0; i < MsgCount; i++ )
		{
			T_LINE_PARAM*	pTagParam = NULL;
			T_TICK_LINE		tagTickLine = { 0 };

			if( abs(pMsgHead->MsgType) == 34 )			///< 指数
			{
				XDFAPI_CNFutOptData*	pData = (XDFAPI_CNFutOptData*)pbuf;

				m_oQuoDataCenter.UpdateTickLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData), tagTickLine, tagStatus.MarketDate );
				pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CNFOPT, std::string( pData->Code ), tagParam );

				///< 构建商品集合
				if( NULL != pTagParam )
				{
					//pTagParam->ContractMulti = 0;
					pTagParam->PreClosePx = tagTickLine.PreClosePx;
					pTagParam->PreSettlePx = tagTickLine.PreSettlePx;
					pTagParam->PrePosition = pData->PreOpenInterest;
					pTagParam->UpperPrice = tagTickLine.UpperPx;
					pTagParam->LowerPrice = tagTickLine.LowerPx;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
				}

				pbuf += sizeof(XDFAPI_CNFutOptData);
				nNum++;
			}

			if( NULL != pTagParam )
			{
				QuotationDatabase::GetDbObj().Replace_Commodity( pTagParam->Type, '4', tagTickLine.Code, pTagParam->Name, pTagParam->LotSize, pTagParam->ContractMulti, pTagParam->PriceTick, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx
					, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.OpenPx, tagTickLine.SettlePx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx
					, pTagParam->FluctuationPercent, tagTickLine.Volume, pTagParam->TradingVolume, tagTickLine.Amount, pTagParam->IsTrading, pTagParam->TradingDate, 0, pTagParam->ClassID );
			}
		}

		m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		QuotationDatabase::GetDbObj().Commit();
	}

	if( NULL != pszCodeBuf )
	{
		delete []pszCodeBuf;
		pszCodeBuf = NULL;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::SaveCNFOPT() : Loading... CNFOPT Snaptable Size = %d", nNum );

	return 0;
}

bool __stdcall	Quotation::XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus )
{
	char	pszDesc[128] = { 0 };

	if( XDF_CF == cMarket && nStatus >= 2 )										///< 此处为特别处理，中金期货不会有“可服务”状态的通知(BUG)
	{
		nStatus = XRS_Normal;
	}

	switch( nStatus )
	{
	case 0:
		{
			::strcpy( pszDesc, "不可用" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 1:
		{
			::strcpy( pszDesc, "未知状态" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 2:
		{
			::strcpy( pszDesc, "初始化" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, pszDesc );
		}
		break;
	case 5:
		{
			::strcpy( pszDesc, "可服务" );
			ServerStatus::GetStatusObj().UpdateMkStatus( (enum XDFMarket)cMarket, "行情中" );
			m_vctMkSvrStatus[cMarket] = ET_SS_INITIALIZING;						///< 设置“初始中”状态标识，以重新加载码表快照到数据库
		}
		break;
	default:
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::XDF_OnRspStatusChanged() : Market(%d), Status=非法状态值", (int)cMarket, pszDesc );
		return false;
	}

	///< 更新模块状态
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Quotation::XDF_OnRspStatusChanged() : Market(%d), Status=%s", (int)cMarket, pszDesc );
	m_oQuoDataCenter.UpdateModuleStatus( (enum XDFMarket)cMarket, nStatus );	///< 更新模块工作状态

	return true;
}

void Quotation::SyncNametable2Database()
{
	for( int nMkID = 0; nMkID < 64; nMkID++ )
	{
		int					nErrorCode = 0;
		enum E_SS_Status	eModuleStatus = m_vctMkSvrStatus[nMkID];

		if( ET_SS_INITIALIZING == eModuleStatus )
		{
			switch( (enum XDFMarket)nMkID )
			{
			case XDF_SH:		///< 上证L1
				nErrorCode = SaveShLv1();
				break;
			case XDF_SHL2:		///< 上证L2(QuoteClientApi内部屏蔽)
				break;
			case XDF_SHOPT:		///< 上证期权
				nErrorCode = SaveShOpt();
				break;
			case XDF_SZ:		///< 深证L1
				nErrorCode = SaveSzLv1();
				break;
			case XDF_SZOPT:		///< 深圳期权
				nErrorCode = SaveSzOpt();
				break;
			case XDF_SZL2:		///< 深圳L2(QuoteClientApi内部屏蔽)
				break;
			case XDF_CF:		///< 中金期货
				nErrorCode = SaveCFF();
				break;
			case XDF_ZJOPT:		///< 中金期权
				nErrorCode = SaveCFFOPT();
				break;
			case XDF_CNF:		///< 商品期货(上海/郑州/大连)
				nErrorCode = SaveCNF();
				break;
			case XDF_CNFOPT:	///< 商品期货和商品期权(上海/郑州/大连)
				nErrorCode = SaveCNFOPT();
				break;
			default:
				continue;
			}

			if( 0 == nErrorCode )
			{
				m_vctMkSvrStatus[nMkID] = ET_SS_WORKING;	///< 设置“可服务”状态标识
			}
		}
	}
}

void Quotation::SyncSnapshot2Database()
{
	int				nErrorCode = 0;

	if( NULL == m_pDataBuffer )	{
		return;
	}

	///< ---------------- Shanghai Lv1 Snapshot -------------------------------------------
	if( ET_SS_WORKING == m_vctMkSvrStatus[XDF_SH] )
	{
		nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SH, m_pDataBuffer, MAX_SNAPSHOT_BUFFER_SIZE );		///< 获取快照
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().StartTransaction();
		}
		for( int m = 0; m < nErrorCode; )
		{
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(m_pDataBuffer+m);
			char*				pbuf = m_pDataBuffer+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;
			T_LINE_PARAM		tagParam = { 0 };

			for( int i = 0; i < MsgCount; i++ )
			{
				T_LINE_PARAM*	pTagParam = NULL;
				T_TICK_LINE		tagTickLine = { 0 };

				if( abs(pMsgHead->MsgType) == 21 )			///< 指数
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData), tagTickLine );			///< Tick线
					pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SH, std::string( tagTickLine.Code ), tagParam );

					pbuf += sizeof(XDFAPI_IndexData);
				}
				else if( abs(pMsgHead->MsgType) == 22 )		///< 快照数据
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5), tagTickLine );		///< Tick线

					pbuf += sizeof(XDFAPI_StockData5);
				}

				if( NULL != pTagParam )
				{
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					QuotationDatabase::GetDbObj().Update_Commodity( '1', tagTickLine.Code, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.SettlePx
						, tagTickLine.OpenPx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx, tagTickLine.Amount, tagTickLine.Volume, pTagParam->TradingVolume
						, pTagParam->FluctuationPercent, pTagParam->IsTrading );
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().Commit();
		}
	}

	///< ---------------- Shanghai Option Snapshot -------------------------------------------
	if( ET_SS_WORKING == m_vctMkSvrStatus[XDF_SHOPT] )
	{
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().StartTransaction();
		}
		nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SHOPT, m_pDataBuffer, MAX_SNAPSHOT_BUFFER_SIZE );		///< 获取快照
		for( int m = 0; m < nErrorCode; )
		{
			T_LINE_PARAM		tagParam = { 0 };
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(m_pDataBuffer+m);
			char*				pbuf = m_pDataBuffer+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				T_LINE_PARAM*	pTagParam = NULL;
				T_TICK_LINE		tagTickLine = { 0 };

				if( abs(pMsgHead->MsgType) == 15 )			///< 指数
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData), tagTickLine );
					pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SHOPT, std::string( tagTickLine.Code ), tagParam );

					pbuf += sizeof(XDFAPI_ShOptData);
				}

				if( NULL != pTagParam )
				{
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					QuotationDatabase::GetDbObj().Update_Commodity( '1', tagTickLine.Code, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.SettlePx
						, tagTickLine.OpenPx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx, tagTickLine.Amount, tagTickLine.Volume, pTagParam->TradingVolume
						, pTagParam->FluctuationPercent, pTagParam->IsTrading );
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().Commit();
		}
	}

	///< ---------------- Shenzheng Lv1 Snapshot -------------------------------------------
	if( ET_SS_WORKING == m_vctMkSvrStatus[XDF_SZ] )
	{
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().StartTransaction();
		}
		nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SZ, m_pDataBuffer, MAX_SNAPSHOT_BUFFER_SIZE );	///< 获取快照
		for( int m = 0; m < nErrorCode; )
		{
			T_LINE_PARAM		tagParam = { 0 };
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(m_pDataBuffer+m);
			char*				pbuf = m_pDataBuffer+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				T_LINE_PARAM*	pTagParam = NULL;
				T_TICK_LINE		tagTickLine = { 0 };

				if( abs(pMsgHead->MsgType) == 21 )			///< 指数
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData), tagTickLine );		///< Tick线
					pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( tagTickLine.Code ), tagParam );

					pbuf += sizeof(XDFAPI_IndexData);
				}
				else if( abs(pMsgHead->MsgType) == 22 )		///< 快照数据
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5), tagTickLine );	///< Tick线
					pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SZ, std::string( tagTickLine.Code ), tagParam );

					pbuf += sizeof(XDFAPI_StockData5);
				}

				if( NULL != pTagParam )
				{
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;				///< 涨跌幅度(用收盘价计算)
					QuotationDatabase::GetDbObj().Update_Commodity( '2', tagTickLine.Code, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.SettlePx
						, tagTickLine.OpenPx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx, tagTickLine.Amount, tagTickLine.Volume, pTagParam->TradingVolume
						, pTagParam->FluctuationPercent, pTagParam->IsTrading );
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().Commit();
		}
	}

	///< ---------------- Shenzheng Option Snapshot -------------------------------------------
	if( ET_SS_WORKING == m_vctMkSvrStatus[XDF_SZOPT] )
	{
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().StartTransaction();
		}
		nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_SZOPT, m_pDataBuffer, MAX_SNAPSHOT_BUFFER_SIZE );	///< 获取快照
		for( int m = 0; m < nErrorCode; )
		{
			T_LINE_PARAM		tagParam = { 0 };
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(m_pDataBuffer+m);
			char*				pbuf = m_pDataBuffer+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				T_LINE_PARAM*	pTagParam = NULL;
				T_TICK_LINE		tagTickLine = { 0 };

				if( abs(pMsgHead->MsgType) == 35 )			///< 指数
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData), tagTickLine );
					pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_SZOPT, std::string( tagTickLine.Code ), tagParam );

					pbuf += sizeof(XDFAPI_SzOptData);
				}

				if( NULL != pTagParam )
				{
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;					///< 涨跌幅度(用收盘价计算)
					QuotationDatabase::GetDbObj().Update_Commodity( '2', tagTickLine.Code, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.SettlePx
						, tagTickLine.OpenPx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx, tagTickLine.Amount, tagTickLine.Volume, pTagParam->TradingVolume
						, pTagParam->FluctuationPercent, pTagParam->IsTrading );
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().Commit();
		}
	}

	///< ---------------- CFF Future Snapshot -------------------------------------------
	if( ET_SS_WORKING == m_vctMkSvrStatus[XDF_CF] )
	{
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().StartTransaction();
		}
		nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CF, m_pDataBuffer, MAX_SNAPSHOT_BUFFER_SIZE );	///< 获取快照
		for( int m = 0; m < nErrorCode; )
		{
			T_LINE_PARAM		tagParam = { 0 };
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(m_pDataBuffer+m);
			char*				pbuf = m_pDataBuffer+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				T_LINE_PARAM*	pTagParam = NULL;
				T_TICK_LINE		tagTickLine = { 0 };

				if( abs(pMsgHead->MsgType) == 20 )			///< 指数
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData), tagTickLine );
					pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CF, std::string( tagTickLine.Code ), tagParam );

					pbuf += sizeof(XDFAPI_CffexData);
				}

				if( NULL != pTagParam )
				{
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;					///< 涨跌幅度(用收盘价计算)
					QuotationDatabase::GetDbObj().Update_Commodity( '3', tagTickLine.Code, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.SettlePx
						, tagTickLine.OpenPx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx, tagTickLine.Amount, tagTickLine.Volume, pTagParam->TradingVolume
						, pTagParam->FluctuationPercent, pTagParam->IsTrading );
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().Commit();
		}
	}

	///< ---------------- CNF Future Snapshot -------------------------------------------
	if( ET_SS_WORKING == m_vctMkSvrStatus[XDF_CNF] )
	{
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().StartTransaction();
		}
		nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CNF, m_pDataBuffer, MAX_SNAPSHOT_BUFFER_SIZE );		///< 获取快照
		for( int m = 0; m < nErrorCode; )
		{
			T_LINE_PARAM		tagParam = { 0 };
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(m_pDataBuffer+m);
			char*				pbuf = m_pDataBuffer+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				T_LINE_PARAM*	pTagParam = NULL;
				T_TICK_LINE		tagTickLine = { 0 };

				if( abs(pMsgHead->MsgType) == 26 )			///< 指数
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData), tagTickLine, ServerStatus::GetStatusObj().FetchMkDate( XDF_CNF ) );
					pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CNF, std::string( tagTickLine.Code ), tagParam );

					pbuf += sizeof(XDFAPI_CNFutureData);
				}

				if( NULL != pTagParam )
				{
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;						///< 涨跌幅度(用收盘价计算)
					QuotationDatabase::GetDbObj().Update_Commodity( '4', tagTickLine.Code, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.SettlePx
						, tagTickLine.OpenPx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx, tagTickLine.Amount, tagTickLine.Volume, pTagParam->TradingVolume
						, pTagParam->FluctuationPercent, pTagParam->IsTrading );
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().Commit();
		}
	}

	///< ---------------- CNF Option Snapshot -------------------------------------------
	if( ET_SS_WORKING == m_vctMkSvrStatus[XDF_CNFOPT] )
	{
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().StartTransaction();
		}
		nErrorCode = m_oQuotPlugin->GetLastMarketDataAll( XDF_CNFOPT, m_pDataBuffer, MAX_SNAPSHOT_BUFFER_SIZE );	///< 获取快照
		for( int m = 0; m < nErrorCode; )
		{
			T_LINE_PARAM		tagParam = { 0 };
			XDFAPI_UniMsgHead*	pMsgHead = (XDFAPI_UniMsgHead*)(m_pDataBuffer+m);
			char*				pbuf = m_pDataBuffer+m +sizeof(XDFAPI_UniMsgHead);
			int					MsgCount = pMsgHead->MsgCount;

			for( int i = 0; i < MsgCount; i++ )
			{
				T_LINE_PARAM*	pTagParam = NULL;
				T_TICK_LINE		tagTickLine = { 0 };

				if( abs(pMsgHead->MsgType) == 34 )			///< 指数
				{
					m_oQuoDataCenter.UpdateTickLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData), tagTickLine, ServerStatus::GetStatusObj().FetchMkDate( XDF_CNFOPT ) );
					pTagParam = m_oQuoDataCenter.BuildSecurity( XDF_CNFOPT, std::string( tagTickLine.Code ), tagParam );

					pbuf += sizeof(XDFAPI_CNFutOptData);
				}

				if( NULL != pTagParam )
				{
					pTagParam->TradingVolume = tagTickLine.Volume - pTagParam->Volume;
					pTagParam->Volume = tagTickLine.Volume;
					pTagParam->FluctuationPercent = tagTickLine.NowPx/tagTickLine.PreClosePx;					///< 涨跌幅度(用收盘价计算)
					QuotationDatabase::GetDbObj().Update_Commodity( '4', tagTickLine.Code, tagTickLine.PreClosePx, tagTickLine.PreSettlePx, tagTickLine.UpperPx, tagTickLine.LowerPx, tagTickLine.NowPx, tagTickLine.SettlePx
						, tagTickLine.OpenPx, tagTickLine.ClosePx, tagTickLine.BidPx1, tagTickLine.AskPx1, tagTickLine.HighPx, tagTickLine.LowPx, tagTickLine.Amount, tagTickLine.Volume, pTagParam->TradingVolume
						, pTagParam->FluctuationPercent, pTagParam->IsTrading );
				}
			}

			m += (sizeof(XDFAPI_UniMsgHead) + pMsgHead->MsgLen - sizeof(pMsgHead->MsgCount));
		}
		if( nErrorCode > 0 ) {
			QuotationDatabase::GetDbObj().Commit();
		}
	}

}

void Quotation::SyncMarketsTime()
{
	QuotePrimeApi*			pQueryApi = m_oQuotPlugin.GetPrimeApi();
	enum XDFMarket			vctMkID[] = { XDF_SH, XDF_SHOPT, XDF_SZ, XDF_SZOPT, XDF_CF, XDF_ZJOPT, XDF_CNF, XDF_CNFOPT };

	if( NULL != pQueryApi )
	{
		unsigned int	nLoopCount = sizeof(vctMkID)/sizeof(enum XDFMarket);

		for( int n = 0; n < nLoopCount; n++ )
		{
			int						nErrCode = 0;
			char					nMkID = vctMkID[n];
			XDFAPI_MarketStatusInfo	tagStatus = { 0 };

			nErrCode = pQueryApi->ReqFuncData( 101, &nMkID, &tagStatus );
			if( 1 == nErrCode )
			{
				ServerStatus::GetStatusObj().UpdateMkTime( vctMkID[n], tagStatus.MarketDate, tagStatus.MarketTime );
			}
		}
	}
}

void __stdcall	Quotation::XDF_OnRspRecvData( XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes )
{
	int						nLen = nBytes;
	char*					pbuf = (char *)pszBuf;
	XDFAPI_UniMsgHead*		pMsgHead = NULL;
	unsigned char			nMarketID = pHead->MarketID;

	if( m_vctMkSvrStatus[nMarketID] != ET_SS_WORKING )				///< 如果市场未就绪，就不接收回调
	{
		return;
	}

	switch( nMarketID )
	{
		case XDF_SH:												///< 处理上海Lv1的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 21:										///< 指数快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_IndexData), tagTickLine );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_SH, pbuf, sizeof(XDFAPI_StockData5), tagTickLine );

								pbuf += sizeof(XDFAPI_StockData5);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 23:///< 停牌标识
						{
							//XDFAPI_StopFlag* pData = (XDFAPI_StopFlag*)pbuf;
							//::printf( "停牌标识=%c\n", pData->SFlag );
							pbuf += sizeof(XDFAPI_StopFlag);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_SHOPT:												///< 处理上海期权的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 15:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_SHOPT, pbuf, sizeof(XDFAPI_ShOptData), tagTickLine );

								pbuf += sizeof(XDFAPI_ShOptData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_ShOptMarketStatus);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_SZ:												///< 处理深圳Lv1的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 21:										///< 指数快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_IndexData), tagTickLine );

								pbuf += sizeof(XDFAPI_IndexData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 22:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_SZ, pbuf, sizeof(XDFAPI_StockData5), tagTickLine );

								pbuf += sizeof(XDFAPI_StockData5);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 24:///< XDFAPI_PreNameChg					///< 前缀变更包
						{
							while( nMsgCount-- > 0 )
							{
								XDFAPI_PreNameChg*	pPreNameChg = (XDFAPI_PreNameChg*)pbuf;

								m_oQuoDataCenter.UpdatePreName( XDF_SZ, std::string( pPreNameChg->Code, 6 ), (char*)(pPreNameChg->PreName + 0), sizeof(pPreNameChg->PreName) );

								pbuf += sizeof(XDFAPI_PreNameChg);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					case 23:///< 停牌标识
						{
							//XDFAPI_StopFlag* pData = (XDFAPI_StopFlag*)pbuf;
							//::printf( "停牌标识=%c\n", pData->SFlag );
							pbuf += sizeof(XDFAPI_StopFlag);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_SZOPT:												///< 处理深圳期权的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 35:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_SZOPT, pbuf, sizeof(XDFAPI_SzOptData), tagTickLine );

								pbuf += sizeof(XDFAPI_SzOptData);
							}
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_CF:												///< 处理中金期货的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 20:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_CF, pbuf, sizeof(XDFAPI_CffexData), tagTickLine );

								pbuf += sizeof(XDFAPI_CffexData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_ZJOPT:												///< 处理中金期权的行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 18:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_ZJOPT, pbuf, sizeof(XDFAPI_ZjOptData), tagTickLine );

								pbuf += sizeof(XDFAPI_ZjOptData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_CNF:												///< 处理商品期货行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 26:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_CNF, pbuf, sizeof(XDFAPI_CNFutureData), tagTickLine );

								pbuf += sizeof(XDFAPI_CNFutureData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
		case XDF_CNFOPT:											///< 处理商品期权行情推送
		{
			while( nLen > 0 )
			{
				pMsgHead = (XDFAPI_UniMsgHead*)pbuf;

				if( pMsgHead->MsgType < 0 )							///< 复合数据包
				{
					int				nMsgCount = pMsgHead->MsgCount;
					pbuf += sizeof(XDFAPI_UniMsgHead);
					nLen -= sizeof(XDFAPI_UniMsgHead);

					switch ( abs(pMsgHead->MsgType) )
					{
					case 34:										///< 商品快照
						{
							while( nMsgCount-- > 0 )
							{
								T_TICK_LINE		tagTickLine = { 0 };
								m_oQuoDataCenter.UpdateTickLine( XDF_CNFOPT, pbuf, sizeof(XDFAPI_CNFutOptData), tagTickLine );

								pbuf += sizeof(XDFAPI_CNFutOptData);
							}
						}
						break;
					}

					nLen -= pMsgHead->MsgLen;
				}
				else												///< 单一数据包
				{
					pbuf += sizeof(XDFAPI_MsgHead);
					nLen -= sizeof(XDFAPI_MsgHead);

					switch ( pMsgHead->MsgType )
					{
					case 1:///< 市场状态信息
						{
							pbuf += sizeof(XDFAPI_MarketStatusInfo);
							nLen -= pMsgHead->MsgLen;
						}
						break;
					}
				}
			}
		}
		break;
	}
}

void __stdcall	Quotation::XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel, const char * pLogBuf )
{
	QuoCollector::GetCollector()->OnLog( TLV_DETAIL, "Quotation::XDF_OnRspOutLog() : %s", pLogBuf );
}

int	__stdcall	Quotation::XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam )
{
	return 0;
}









