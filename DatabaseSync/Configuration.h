#ifndef __DATA_CONFIGURATION_H__
#define __DATA_CONFIGURATION_H__
#pragma warning(disable: 4786)
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "Infrastructure/Lock.h"
#include "Infrastructure/IniFile.h"


extern	HMODULE					g_oModule;							///< ��ǰdllģ�����
extern	std::string				GetModulePath( void* hModule );		///< ��ȡ��ǰģ��·��


/**
 * @class						MkCfgWriter
 * @brief						�г�����д����
 * @author						barry
 */
class MkCfgWriter
{
public:
	MkCfgWriter( std::string sSubFolder );

	/**
	 * @brief					�������������õ�ָ�����г����������ļ���
	 * @param[in]				nIndication							������ָ���ʶ��
	 * @param[in]				sIP									�����ַ
	 * @param[in]				nPort								����˿�
	 * @param[in]				sAccountID							�û��ʺ�
	 * @param[in]				sPswd								�û�����
	 */
	virtual bool				ConfigurateConnection4Mk( unsigned int nIndication, std::string sIP, unsigned short nPort, std::string sAccountID, std::string sPswd );

protected:
	std::string					m_sCfgFolder;						///< �г���������Ŀ¼·��
};


/**
 * @class						T_CLOSE_CFG
 * @brief						����������Ϣ
 * @author						barry
 */
typedef struct
{
	unsigned int				LastDate;							///< �����������
	unsigned int				CloseTime;							///< ����ʱ��
} CLOSE_CFG;


typedef std::vector<CLOSE_CFG>			TB_MK_CLOSECFG;				///< �г��������ñ�
typedef std::map<short,TB_MK_CLOSECFG>	MAP_MK_CLOSECFG;			///< ���г���������


/**
 * @class						Configuration
 * @brief						������Ϣ
 * @date						2017/5/15
 * @author						barry
 */
class Configuration
{
protected:
	Configuration();

public:
	/**
	 * @brief					��ȡ���ö���ĵ������ö���
	 */
	static Configuration&		GetConfig();

	/**
	 * @brief					����������
	 * @return					==0					�ɹ�
								!=					����
	 */
    int							Initialize();

public:
	/**
	 * @brief					��ȡ���ݿ���
	 */
	const std::string&			GetDbHost();

	/**
	 * @brief					��ȡ���ݿ��ʺ�
	 */
	const std::string&			GetDbAccount();

	/**
	 * @brief					��ȡ���ݿ�����
	 */
	const std::string&			GetDbPassword();

	/**
	 * @brief					��ȡ���ݱ���
	 */
	const std::string&			GetDbTableName();

	/**
	 * @brief					��ȡ���ݿ�˿�
	 */
	unsigned short				GetDbPort();

	/**
	 * @brief					��ȡ�������·��(���ļ���)
	 */
	const std::string&			GetDataCollectorPluginPath() const;

protected:
	/**
	 * @brief					�������г����������ò�ת�浽��ӦĿ¼��
	 * @param[in]				refIniFile					�����ļ�
	 */
	void						ParseAndSaveMkConfig( inifile::IniFile& refIniFile );

protected:
	std::vector<std::string>	m_vctMkNameCfg;				///< �г������б�

private:
	std::string					m_sQuoPluginPath;			///< ������·��
	std::string					m_sDBHost;					///< mysql��ַ
	std::string					m_sDBUser;					///< mysql�û�
	std::string					m_sDBPswd;					///< mysql����
	std::string					m_sDBTable;					///< mysqlд���
	unsigned short				m_nDBPort;					///< mysql�˿�
};





#endif








