#ifndef __DATA_CONFIGURATION_H__
#define __DATA_CONFIGURATION_H__
#pragma warning(disable: 4786)
#include <set>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "Infrastructure/Lock.h"
#include "Infrastructure/File.h"
#include "Infrastructure/IniFile.h"


extern	HMODULE					g_oModule;							///< 当前dll模块变量
extern	std::string				GetModulePath( void* hModule );		///< 获取当前模块路径


/**
 * @class						MkCfgWriter
 * @brief						市场配置写入类
 * @author						barry
 */
class MkCfgWriter
{
public:
	MkCfgWriter( std::string sSubFolder );

	/**
	 * @brief					更新新连接配置到指定的市场行情配置文件中
	 * @param[in]				nIndication							配置项指向标识号
	 * @param[in]				sIP									行情地址
	 * @param[in]				nPort								行情端口
	 * @param[in]				sAccountID							用户帐号
	 * @param[in]				sPswd								用户密码
	 */
	virtual bool				ConfigurateConnection4Mk( unsigned int nIndication, std::string sIP, unsigned short nPort, std::string sAccountID, std::string sPswd );

protected:
	std::string					m_sCfgFolder;						///< 市场驱动所在目录路径
};


typedef std::set<unsigned char>				SET_TYPE;				///< 商品类型集合
typedef std::map<unsigned char,SET_TYPE>	MAP_MK2TYPESET;			///< 各市场对应的有效类型白名单集
typedef std::map<std::string,std::pair<std::string,std::string>>	MAP_CODE4SHORT;			///< 代码到简称的映射表


/**
 * @class						ShortSpell
 * @brief						简称映射类
 * @author						barry
 */
class ShortSpell
{
private:
	ShortSpell();

public:
	static ShortSpell&			GetObj();

	/**
	 * @brief					从本地文件加载配置数据
	 * @return					==0							加载，成功
								!=0							加载，失败
	 */
	int							LoadFromCSV();

	/**
	 * @brief					保存到本地文件
	 * @return					==0							保存，成功
								!=0							保存，失败
	 */
	int							SaveToCSV();

	/**
	 * @brief					根据商品代码返回简拼
	 * @param[in]				sCode						商品代码(前缀为市场代号，比如 SH.600000)
	 * @param[in]				sName						商品名称(以未查询到对应配置的情况下，拿sName进行实时计算商品简拼)
	 * @return					返回商品简拼
	 */
	std::string					GetShortSpell( std::string sCode, std::string sName );

protected:
	CriticalObject				m_oLock;					///< 临界区对象
	MAP_CODE4SHORT				m_mapCode2ShortSpell;		///< 商品代码到简称的映射(SH.600000,PFYH)
	unsigned int				m_nLastCount;				///< 最后一次统计总数
};


/**
 * @class						Configuration
 * @brief						配置信息
 * @date						2017/5/15
 * @author						barry
 */
class Configuration
{
protected:
	Configuration();

public:
	/**
	 * @brief					获取配置对象的单键引用对象
	 */
	static Configuration&		GetConfig();

	/**
	 * @brief					加载配置项
	 * @return					==0					成功
								!=					出错
	 */
    int							Initialize();

public:
	/**
	 * @brief					获取数据库名
	 */
	const std::string&			GetDbHost();

	/**
	 * @brief					获取数据库帐号
	 */
	const std::string&			GetDbAccount();

	/**
	 * @brief					获取数据库密码
	 */
	const std::string&			GetDbPassword();

	/**
	 * @brief					获取数据表名
	 */
	const std::string&			GetDbTableName();

	/**
	 * @brief					获取数据库端口
	 */
	unsigned short				GetDbPort();

	/**
	 * @brief					获取插件加载路径(含文件名)
	 */
	const std::string&			GetDataCollectorPluginPath() const;

	/**
	 * @brief					判断某市场某类型是否在白名单里
	 * @param[in]				cMkID						市场ID
	 * @param[in]				cType						商品类型
	 * @return					true						在白名单里
								false						不在
	 */
	bool						InWhiteTable( unsigned char cMkID, unsigned char cType );

protected:
	/**
	 * @brief					解析各市场的行情配置并转存到对应目录中
	 * @param[in]				refIniFile					配置文件
	 */
	void						ParseAndSaveMkConfig( inifile::IniFile& refIniFile );

protected:
	MAP_MK2TYPESET				m_mapMkID2TypeSet;			///< 各市场的商品类型对应的白名单集合
	std::vector<std::string>	m_vctMkNameCfg;				///< 市场名称列表
	std::string					m_sQuoPluginPath;			///< 行情插件路径
	std::string					m_sDBHost;					///< mysql地址
	std::string					m_sDBUser;					///< mysql用户
	std::string					m_sDBPswd;					///< mysql密码
	std::string					m_sDBTable;					///< mysql写入表
	unsigned short				m_nDBPort;					///< mysql端口
};





#endif








