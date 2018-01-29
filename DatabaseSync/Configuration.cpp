#include <string>
#include <algorithm>
#include "DatabaseSync.h"
#include "Configuration.h"
#include "Quotation/SvrStatus.h"
#include "Database/QLMatchCH.h"


char*	__BasePath(char *in)
{
	if( !in )
		return NULL;

	int	len = strlen(in);
	for( int i = len-1; i >= 0; i-- )
	{
		if( in[i] == '\\' || in[i] == '/' )
		{
			in[i + 1] = 0;
			break;
		}
	}

	return in;
}


std::string GetModulePath( void* hModule )
{
	char					szPath[MAX_PATH] = { 0 };
#ifndef LINUXCODE
		int	iRet = ::GetModuleFileName( (HMODULE)hModule, szPath, MAX_PATH );
		if( iRet <= 0 )	{
			return "";
		} else {
			return __BasePath( szPath );
		}
#else
		if( !hModule ) {
			int iRet =  readlink( "/proc/self/exe", szPath, MAX_PATH );
			if( iRet <= 0 ) {
				return "";
			} else {
				return __BasePath( szPath );
			}
		} else {
			class MDll	*pModule = (class MDll *)hModule;
			strncpy( szPath, pModule->GetDllSelfPath(), sizeof(szPath) );
			if( strlen(szPath) == 0 ) {
				return "";
			} else {
				return __BasePath(szPath);
			}
		}
#endif
}


HMODULE						g_oModule;


MkCfgWriter::MkCfgWriter( std::string sSubFolder )
 : m_sCfgFolder( sSubFolder )
{
}

bool MkCfgWriter::ConfigurateConnection4Mk( unsigned int nIndication, std::string sIP, unsigned short nPort, std::string sAccountID, std::string sPswd )
{
	inifile::IniFile		oIniFile;
	std::string				sPath = "./";
	char					pszKey[64] = { 0 };
	char					pszPort[64] = { 0 };

	::sprintf( pszPort, "%u", nPort );
	///< ��ĳ���г��������ļ�
	if( "cff_setting" == m_sCfgFolder )	{					///< �н��ڻ�
		sPath += "cff/tran2ndcff.ini";
	}
	else if( "cffopt_setting" == m_sCfgFolder )	{			///< �н���Ȩ
		sPath += "cffopt/tran2ndcffopt.ini";
	}
	else if( "cnf_setting" == m_sCfgFolder )	{			///< ��Ʒ�ڻ�
		sPath += "cnf/tran2ndcnf.ini";
	}
	else if( "cnfopt_setting" == m_sCfgFolder )	{			///< ��Ʒ��Ȩ
		sPath += "cnfopt/tran2ndcnfopt.ini";
	}
	else if( "sh_setting" == m_sCfgFolder )	{				///< �Ϻ��ֻ�
		sPath += "sh/tran2ndsh.ini";
	}
	else if( "shopt_setting" == m_sCfgFolder )	{			///< �Ϻ���Ȩ
		sPath += "shopt/tran2ndshopt.ini";
	}
	else if( "sz_setting" == m_sCfgFolder )	{				///< �����ֻ�
		sPath += "sz/tran2ndsz.ini";
	}
	else if( "szopt_setting" == m_sCfgFolder )	{			///< ������Ȩ
		sPath += "szopt/tran2ndszopt.ini";
	}
	else	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : invalid market subfolder name : %s", m_sCfgFolder.c_str() );
		return false;
	}

	///< �жϲ����Ƿ���Ч
	if( sIP == "" || nPort == 0 || sAccountID == "" || sPswd == "" )	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN
											, "MkCfgWriter::ConfigurateConnection4Mk() : invalid market paramenters [%s:%u], ip:%s port:%u account:%s pswd:%s"
											, m_sCfgFolder.c_str(), nIndication, sIP.c_str(), nPort, sAccountID.c_str(), sPswd.c_str() );
		return true;
	}

	///< �����ļ� & �ж��ļ��Ƿ����
	if( 0 != oIniFile.load( sPath.c_str() ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : invalid Mk configuration path = %s", sPath.c_str() );
		return false;
	}

	///< д��������Ϣ��Ŀ���ļ���
	::sprintf( pszKey, "ServerIP_%u", nIndication );
	if( 0 != oIniFile.setValue( std::string("Communication"), std::string(pszKey), sIP ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save ip [%s]", pszKey );
		return false;
	}

	::sprintf( pszKey, "ServerPort_%u", nIndication );
	if( 0 != oIniFile.setValue( std::string("Communication"), std::string(pszKey), std::string(pszPort) ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save port [%s]", pszKey );
		return false;
	}

	::sprintf( pszKey, "LoginName_%u", nIndication );
	if( 0 != oIniFile.setValue( std::string("Communication"), std::string(pszKey), sAccountID ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save account [%s]", pszKey );
		return false;
	}

	::sprintf( pszKey, "LoginPWD_%u", nIndication );
	if( 0 != oIniFile.setValue( std::string("Communication"), std::string(pszKey), sPswd ) )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save password [%s]", pszKey );
		return false;
	}

	if( 0 != oIniFile.save() )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "MkCfgWriter::ConfigurateConnection4Mk() : cannot save Mk configuration, path = %s", sPath.c_str() );
		return false;
	}

	return true;
}


ShortSpell::ShortSpell()
{
	m_mapCode2ShortSpell.clear();
}

ShortSpell& ShortSpell::GetObj()
{
	static ShortSpell		obj;

	return obj;
}

int ShortSpell::LoadFromCSV()
{
	std::ifstream			objCSV;
	unsigned int			nCodeCount = 0;

	objCSV.open( "shortspell.csv" );
	if( !objCSV.is_open() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "ShortSpell::LoadFromCSV() : failed 2 load data from shortspell.csv" );
		return -1;
	}

	while( true )
	{
		char			pszLine[1024] = { 0 };

		if( !objCSV.getline( pszLine, sizeof(pszLine) ) ) {
			break;
		}

		int				nLen = ::strlen( pszLine );

		for( int n = 0; n < nLen; n++ )
		{
			if( ',' == pszLine[n] )
			{
				std::string		sKey( pszLine, n );
				std::string		sValue( pszLine + n + 1, nLen - n );

				m_mapCode2ShortSpell[sKey] = sValue;
			}
		}

		nCodeCount++;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "ShortSpell::LoadFromCSV() : load code number = %u", nCodeCount );

	return 0;
}

std::string ShortSpell::GetShortSpell( std::string sCode, std::string sName )
{
	char						pszSpell[218] = { 0 };
	MAP_CODE4SHORT::iterator	it = m_mapCode2ShortSpell.find( sCode );

	if( it != m_mapCode2ShortSpell.end() )
	{
		return it->second;
	}

/*	if( false == CQLMatchCH::GetFirstPinYin( (const TCHAR*)sName.c_str(), (TCHAR*)pszSpell ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "ShortSpell::GetShortSpell() : failed 2 convert name 2 spell" );
		return "";
	}
	else*/
	{
		return pszSpell;
	}
}


Configuration::Configuration()
 : m_nDBPort( 5873 )
{
	///< ���ø��г��Ĵ�����������Ŀ¼����
	m_vctMkNameCfg.push_back( "cff_setting" );
	m_vctMkNameCfg.push_back( "cffopt_setting" );
	m_vctMkNameCfg.push_back( "cnf_setting" );
	m_vctMkNameCfg.push_back( "cnfopt_setting" );
	m_vctMkNameCfg.push_back( "sh_setting" );
	m_vctMkNameCfg.push_back( "shopt_setting" );
	m_vctMkNameCfg.push_back( "sz_setting" );
	m_vctMkNameCfg.push_back( "szopt_setting" );

	m_mapMkID2TypeSet[XDF_SH].insert( 0 );		///< ָ��
	m_mapMkID2TypeSet[XDF_SH].insert( 1 );		///< A��
	m_mapMkID2TypeSet[XDF_SH].insert( 7 );		///< ETF
	m_mapMkID2TypeSet[XDF_SZ].insert( 0 );		///< ָ��
	m_mapMkID2TypeSet[XDF_SZ].insert( 1 );		///< A��
	m_mapMkID2TypeSet[XDF_SZ].insert( 7 );		///< ETF
	m_mapMkID2TypeSet[XDF_SZ].insert( 8 );		///< ��С��
	m_mapMkID2TypeSet[XDF_SZ].insert( 9 );		///< ��ҵ��
}

int	CvtStr2MkID( std::string sStr )
{
	if( sStr == "cff_setting" )	{
		return XDF_CF;
	} else if( sStr == "cffopt_setting" )	{
		return XDF_ZJOPT;
	} else if( sStr == "cnf_setting" )	{
		return XDF_CNF;
	} else if( sStr == "cnfopt_setting" )	{
		return XDF_CNFOPT;
	} else if( sStr == "sh_setting" )	{
		return XDF_SH;
	} else if( sStr == "shopt_setting" )	{
		return XDF_SHOPT;
	} else if( sStr == "sz_setting" )	{
		return XDF_SZ;
	} else if( sStr == "sz_setting" )	{
		return XDF_SZOPT;
	} else {
		return -1;				///< ����ʶ����г����ñ�ʶ�ַ���
	}
}

Configuration& Configuration::GetConfig()
{
	static Configuration	obj;

	return obj;
}

int Configuration::Initialize()
{
	std::string			sPath;
	inifile::IniFile	oIniFile;
	int					nErrCode = 0;
    char				pszTmp[1024] = { 0 };

    ::GetModuleFileName( g_oModule, pszTmp, sizeof(pszTmp) );
    sPath = pszTmp;
    sPath = sPath.substr( 0, sPath.find(".dll") ) + ".ini";
	if( 0 != (nErrCode=oIniFile.load( sPath )) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Configuration::Initialize() : configuration file isn\'t exist. [%s], errorcode=%d", sPath.c_str(), nErrCode );
		return -1;
	}

	///< ���������ص�ַ
	m_sQuoPluginPath = oIniFile.getStringValue( std::string("SRV"), std::string("QuoPlugin"), nErrCode );
	if( 0 != nErrCode )	{
		m_sQuoPluginPath = "./QuoteClientApi.dll";
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : default quotation plugin path: %s", m_sQuoPluginPath.c_str() );
	}

	///< Mysql������Ϣ
	m_sDBHost = oIniFile.getStringValue( std::string("MYSQL"), std::string("DBHost"), nErrCode );
	if( 0 != nErrCode )	{
		m_sDBHost = "127.0.0.1";
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : default mysql host: %s", m_sDBHost.c_str() );
	}
	m_sDBUser = oIniFile.getStringValue( std::string("MYSQL"), std::string("DBUser"), nErrCode );
	if( 0 != nErrCode )	{
		m_sDBUser = "default";
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : default mysql login account: %s", m_sDBUser.c_str() );
	}
	m_sDBPswd = oIniFile.getStringValue( std::string("MYSQL"), std::string("DBPswd"), nErrCode );
	if( 0 != nErrCode )	{
		m_sDBPswd = "default";
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : default mysql login password: %s", m_sDBPswd.c_str() );
	}
	m_sDBTable = oIniFile.getStringValue( std::string("MYSQL"), std::string("DBTable"), nErrCode );
	if( 0 != nErrCode )	{
		m_sDBTable = "defaulttable";
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : default mysql table: %s", m_sDBTable.c_str() );
	}
	m_nDBPort = oIniFile.getIntValue( std::string("MYSQL"), std::string("DBPort"), nErrCode );
	if( 0 != nErrCode )	{
		m_nDBPort = 31256;
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : default mysql port: %d", m_nDBPort );
	}

	///< ÿ���г��Ļ����У����Ի����tick���ݵ�����
	int	nNum4OneMarket = oIniFile.getIntValue( std::string("SRV"), std::string("ItemCountInBuffer"), nErrCode );
	if( nNum4OneMarket > 0 )	{
		s_nNumberInSection = nNum4OneMarket;
	}
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "Configuration::Initialize() : Setting Item Number ( = %d) In One Market Buffer ...", s_nNumberInSection );

	///< ת����г������õ���Ӧ���ļ� && ��������Ҫ����صĸ��г��Ĵ���
	ParseAndSaveMkConfig( oIniFile );

	return 0;
}

void Configuration::ParseAndSaveMkConfig( inifile::IniFile& refIniFile )
{
	for( std::vector<std::string>::iterator it = m_vctMkNameCfg.begin(); it != m_vctMkNameCfg.end(); it++ )
	{
		int					nErrCode = 0;
		std::string&		sMkCfgName = *it;
		char				pszKey[54] = { 0 };
		std::string			sServerIP_0 = refIniFile.getStringValue( sMkCfgName, std::string("ServerIP_0"), nErrCode );
		int					nServerPort_0 = refIniFile.getIntValue( sMkCfgName, std::string("ServerPort_0"), nErrCode );
		std::string			sLoginName_0 = refIniFile.getStringValue( sMkCfgName, std::string("LoginName_0"), nErrCode );
		std::string			sLoginPWD_0 = refIniFile.getStringValue( sMkCfgName, std::string("LoginPWD_0"), nErrCode );
		std::string			sServerIP_1 = refIniFile.getStringValue( sMkCfgName, std::string("ServerIP_1"), nErrCode );
		int					nServerPort_1 = refIniFile.getIntValue( sMkCfgName, std::string("ServerPort_1"), nErrCode );
		std::string			sLoginName_1 = refIniFile.getStringValue( sMkCfgName, std::string("LoginName_1"), nErrCode );
		std::string			sLoginPWD_1 = refIniFile.getStringValue( sMkCfgName, std::string("LoginPWD_1"), nErrCode );
		std::string			sShowCode = refIniFile.getStringValue( sMkCfgName, std::string("ShowCode"), nErrCode );
		MkCfgWriter			objCfgWriter( sMkCfgName );
		short				nMarketID = CvtStr2MkID( sMkCfgName );

		objCfgWriter.ConfigurateConnection4Mk( 0, sServerIP_0, nServerPort_0, sLoginName_0, sLoginPWD_0 );
		objCfgWriter.ConfigurateConnection4Mk( 1, sServerIP_1, nServerPort_1, sLoginName_1, sLoginPWD_1 );
		if( nMarketID >= 0 && sShowCode.length() > 2 )	{
			ServerStatus::GetStatusObj().AnchorSecurity( (enum XDFMarket)nMarketID, sShowCode.c_str(), "" );
		}
	}
}

bool Configuration::InWhiteTable( unsigned char cMkID, unsigned char cType )
{
	MAP_MK2TYPESET::iterator itMap = m_mapMkID2TypeSet.find( cMkID );

	if( itMap == m_mapMkID2TypeSet.end() )
	{
		return false;
	}

	SET_TYPE::iterator itSet = itMap->second.find( cType );

	if( itSet == itMap->second.end() )
	{
		return false;
	}

	return true;
}

const std::string& Configuration::GetDataCollectorPluginPath() const
{
	return m_sQuoPluginPath;
}

const std::string& Configuration::GetDbHost()
{
	return m_sDBHost;
}

const std::string& Configuration::GetDbAccount()
{
	return m_sDBUser;
}

const std::string& Configuration::GetDbPassword()
{
	return m_sDBPswd;
}

const std::string& Configuration::GetDbTableName()
{
	return m_sDBTable;
}

unsigned short Configuration::GetDbPort()
{
	return m_nDBPort;
}








