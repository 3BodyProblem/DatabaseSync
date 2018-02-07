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


typedef std::set<unsigned char>				SET_TYPE;				///< ��Ʒ���ͼ���
typedef std::map<unsigned char,SET_TYPE>	MAP_MK2TYPESET;			///< ���г���Ӧ����Ч���Ͱ�������
typedef std::map<std::string,std::pair<std::string,std::string>>	MAP_CODE4SHORT;			///< ���뵽��Ƶ�ӳ���


/**
 * @class						ShortSpell
 * @brief						���ӳ����
 * @author						barry
 */
class ShortSpell
{
private:
	ShortSpell();

public:
	static ShortSpell&			GetObj();

	/**
	 * @brief					�ӱ����ļ�������������
	 * @return					==0							���أ��ɹ�
								!=0							���أ�ʧ��
	 */
	int							LoadFromCSV();

	/**
	 * @brief					���浽�����ļ�
	 * @return					==0							���棬�ɹ�
								!=0							���棬ʧ��
	 */
	int							SaveToCSV();

	/**
	 * @brief					������Ʒ���뷵�ؼ�ƴ
	 * @param[in]				sCode						��Ʒ����(ǰ׺Ϊ�г����ţ����� SH.600000)
	 * @param[in]				sName						��Ʒ����(��δ��ѯ����Ӧ���õ�����£���sName����ʵʱ������Ʒ��ƴ)
	 * @return					������Ʒ��ƴ
	 */
	std::string					GetShortSpell( std::string sCode, std::string sName );

protected:
	CriticalObject				m_oLock;					///< �ٽ�������
	MAP_CODE4SHORT				m_mapCode2ShortSpell;		///< ��Ʒ���뵽��Ƶ�ӳ��(SH.600000,PFYH)
	unsigned int				m_nLastCount;				///< ���һ��ͳ������
};


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

	/**
	 * @brief					�ж�ĳ�г�ĳ�����Ƿ��ڰ�������
	 * @param[in]				cMkID						�г�ID
	 * @param[in]				cType						��Ʒ����
	 * @return					true						�ڰ�������
								false						����
	 */
	bool						InWhiteTable( unsigned char cMkID, unsigned char cType );

protected:
	/**
	 * @brief					�������г����������ò�ת�浽��ӦĿ¼��
	 * @param[in]				refIniFile					�����ļ�
	 */
	void						ParseAndSaveMkConfig( inifile::IniFile& refIniFile );

protected:
	MAP_MK2TYPESET				m_mapMkID2TypeSet;			///< ���г�����Ʒ���Ͷ�Ӧ�İ���������
	std::vector<std::string>	m_vctMkNameCfg;				///< �г������б�
	std::string					m_sQuoPluginPath;			///< ������·��
	std::string					m_sDBHost;					///< mysql��ַ
	std::string					m_sDBUser;					///< mysql�û�
	std::string					m_sDBPswd;					///< mysql����
	std::string					m_sDBTable;					///< mysqlд���
	unsigned short				m_nDBPort;					///< mysql�˿�
};





#endif








