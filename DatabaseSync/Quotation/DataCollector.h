#ifndef __DATA_COLLECTOR_H__
#define	__DATA_COLLECTOR_H__


#include <string>
#include "../QuoteClientApi.h"
#include "../Infrastructure/Dll.h"
#include "../Infrastructure/Lock.h"
#include "../../../DataNode/DataNode/Interface.h"


/**
 * @class				CollectorStatus
 * @brief				当前行情会话的状态
 * @detail				服务框架需要通过这个判断（组合初始化策略实例）来判断是否需要重新初始化等动作
 * @author				barry
 */
class CollectorStatus
{
public:
	CollectorStatus();

	CollectorStatus( const CollectorStatus& obj );

public:
	enum E_SS_Status		Get() const;

	bool					Set( enum E_SS_Status eNewStatus );

private:
	mutable CriticalObject	m_oCSLock;			///< 临界区
	enum E_SS_Status		m_eStatus;			///< 当前行情逻辑状态，用于判断当前该做什么操作了
	int						m_nMarketID;		///< 数据采集器对应的市场ID
};


/**
 * @class					DataCollector
 * @brief					数据采集模块控制注册接口
 * @note					采集模块只提供三种形式的回调通知( I_DataHandle: 初始化映像数据， 实时行情数据， 初始化完成标识 ) + 重新初始化方法函数
 * @date					2017/5/3
 * @author					barry
 */
class DataCollector
{
public:
	DataCollector();

	/**
	 * @brief				数据采集模块初始化
	 * @param[in]			pIQuotationSpi				行情回调接口
	 * @return				==0							成功
							!=0							错误
	 */
	int						Initialize( QuoteClientSpi* pIQuotationSpi );

	/**
	 * @breif				数据采集模块释放退出
	 */
	void					Release();

public:///< 数据采集模块事件定义
	/**
 	 * @brief				初始化/重新初始化回调
	 * @note				同步函数，即函数返回后，即初始化操作已经做完，可以判断执行结果是否为“成功”
	 * @return				==0							成功
							!=0							错误
	 */
	int						RecoverDataCollector();

	/**
	 * @brief				暂停数据采集器
	 */
	void					HaltDataCollector();

	/**
	 * @biref				取得当前数据采集模块状态
	 * @param[out]			pszStatusDesc				返回出状态描述串
	 * @param[in,out]		nStrLen						输入描述串缓存长度，输出描述串有效内容长度
	 * @return				E_SS_Status状态值
	 */
	enum E_SS_Status		InquireDataCollectorStatus( char* pszStatusDesc, unsigned int& nStrLen );

	/**
	 * @brief				操作符转义
	 */
	QuoteClientApi*			operator->();

	/**
	 * @brief				获取查询接口
	 */
	QuotePrimeApi*			GetPrimeApi();

	/**
	 * @brief				获取行情插件版本号
	 */
	std::string&			GetVersion();

private:
	Dll						m_oDllPlugin;					///< 插件加载类
	CollectorStatus			m_oCollectorStatus;				///< 数据采集模块的状态
	QuoteClientApi*			m_pQuoteClientApi;				///< 导出行情源控制类
	QuotePrimeApi*			m_pQuotePrimeApi;				///< 行情数据查询接口
	std::string				m_sPluginVersion;				///< 版本号
};







#endif









