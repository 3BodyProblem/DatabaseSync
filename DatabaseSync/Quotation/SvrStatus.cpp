#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "SvrStatus.h"
#include "../DatabaseSync.h"


ServerStatus::ServerStatus()
 : m_nTickLostCount( 0 ), m_nTickBufOccupancyRate( 0 )
{
	::memset( m_vctLastSecurity, 0, sizeof(m_vctLastSecurity) );
}

ServerStatus& ServerStatus::GetStatusObj()
{
	static	ServerStatus	obj;

	return obj;
}

void ServerStatus::AddTickLostRef()
{
	m_nTickLostCount++;
}

unsigned int ServerStatus::GetTickLostRef()
{
	return m_nTickLostCount;
}

void ServerStatus::AnchorSecurity( enum XDFMarket eMkID, const char* pszCode, const char* pszName )
{
	unsigned int			nPos = (unsigned int)eMkID;

	if( NULL == pszCode || NULL == pszName || nPos >= 256 )
	{
		return;
	}

	T_SECURITY_STATUS&		refSecurity = m_vctLastSecurity[nPos];
	unsigned int			nCodeLen = ::strlen( refSecurity.Code );
	unsigned int			nNewCodeLen = ::strlen( pszCode );

	///< 如果代码已经存在 && 新旧代码相等, 则不再重复锚定
	if( nCodeLen > 0 && ::strncmp( refSecurity.Code, pszCode, max(nCodeLen, nNewCodeLen) ) != 0 )
	{
		return;
	}

	::strcpy( refSecurity.Code, pszCode );
	::strcpy( refSecurity.Name, pszName );
}

void ServerStatus::UpdateSecurity( enum XDFMarket eMkID, const char* pszCode, double dLastPx, double dAmount, unsigned __int64 nVolume )
{
	unsigned int			nPos = (unsigned int)eMkID;

	if( NULL == pszCode || nPos >= 256 )
	{
		return;
	}

	int						nCodeLen = ::strlen( pszCode );
	T_SECURITY_STATUS&		refSecurity = m_vctLastSecurity[nPos];

	if( ::memcmp( refSecurity.Code, pszCode, nCodeLen ) == 0 )
	{
		refSecurity.LastPx = dLastPx;
		refSecurity.Amount = dAmount;
		refSecurity.Volume = nVolume;
	}
}

T_SECURITY_STATUS& ServerStatus::FetchSecurity( enum XDFMarket eMkID )
{
	unsigned int			nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return m_vctLastSecurity[255];
	}

	return m_vctLastSecurity[nPos];
}

void ServerStatus::UpdateMkTime( enum XDFMarket eMkID, unsigned nMkDate, unsigned int nMkTime )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return;
	}

	m_vctLastSecurity[nPos].MkDate = nMkDate;
	m_vctLastSecurity[nPos].MkTime = nMkTime;
}

unsigned int ServerStatus::FetchMkDate( enum XDFMarket eMkID )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return 0;
	}

	return m_vctLastSecurity[nPos].MkDate;
}

unsigned int ServerStatus::FetchMkTime( enum XDFMarket eMkID )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return 0;
	}

	return m_vctLastSecurity[nPos].MkTime;
}

void ServerStatus::UpdateTickBufOccupancyRate( int nRate )
{
	m_nTickBufOccupancyRate = nRate;
}

int ServerStatus::FetchTickBufOccupancyRate()
{
	return m_nTickBufOccupancyRate;
}

void ServerStatus::UpdateMkStatus( enum XDFMarket eMkID, const char* pszDesc )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return;
	}

	::strcpy( m_vctLastSecurity[nPos].MkStatusDesc, pszDesc );
}

const char* ServerStatus::GetMkStatus( enum XDFMarket eMkID )
{
	unsigned int	nPos = (unsigned int)eMkID;

	if( nPos >= 256 )
	{
		return m_vctLastSecurity[255].MkStatusDesc;
	}

	return m_vctLastSecurity[nPos].MkStatusDesc;
}





