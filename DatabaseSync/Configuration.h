#ifndef __DATA_CONFIGURATION_H__
#define __DATA_CONFIGURATION_H__
#pragma warning(disable: 4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "Infrastructure/Lock.h"
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


/**
 * @class						T_CLOSE_CFG
 * @brief						收盘配置信息
 * @author						barry
 */
typedef struct
{
	unsigned int				LastDate;							///< 最后落盘日期
	unsigned int				CloseTime;							///< 收盘时间
} CLOSE_CFG;


typedef std::vector<CLOSE_CFG>			TB_MK_CLOSECFG;				///< 市场收盘配置表
typedef std::map<short,TB_MK_CLOSECFG>	MAP_MK_CLOSECFG;			///< 各市场收盘配置


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

protected:
	/**
	 * @brief					解析各市场的行情配置并转存到对应目录中
	 * @param[in]				refIniFile					配置文件
	 */
	void						ParseAndSaveMkConfig( inifile::IniFile& refIniFile );

protected:
	std::vector<std::string>	m_vctMkNameCfg;				///< 市场名称列表

private:
	std::string					m_sQuoPluginPath;			///< 行情插件路径
	std::string					m_sDBHost;					///< mysql地址
	std::string					m_sDBUser;					///< mysql用户
	std::string					m_sDBPswd;					///< mysql密码
	std::string					m_sDBTable;					///< mysql写入表
	unsigned short				m_nDBPort;					///< mysql端口
};





#endif








