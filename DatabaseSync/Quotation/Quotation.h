#ifndef __CTP_QUOTATION_H__
#define __CTP_QUOTATION_H__


#pragma warning(disable:4786)
#include <stdexcept>
#include "DataCenter.h"
#include "DataCollector.h"


/**
 * @class			WorkStatus
 * @brief			����״̬����
 * @author			barry
 */
class WorkStatus
{
public:
	/**
	 * @brief					Ӧ״ֵ̬ӳ���״̬�ַ���
	 */
	static	std::string&		CastStatusStr( enum E_SS_Status eStatus );

public:
	/**
	 * @brief					����
	 * @param					eMkID			�г����
	 */
	WorkStatus();
	WorkStatus( const WorkStatus& refStatus );

	/**
	 * @brief					��ֵ����
								ÿ��ֵ�仯������¼��־
	 */
	WorkStatus&					operator= ( enum E_SS_Status eWorkStatus );

	/**
	 * @brief					����ת����
	 */
	operator					enum E_SS_Status();

private:
	CriticalObject				m_oLock;				///< �ٽ�������
	enum E_SS_Status			m_eWorkStatus;			///< ���鹤��״̬
};


/**
 * @class			Quotation
 * @brief			�Ự�������
 * @detail			��װ�������Ʒ�ڻ���Ȩ���г��ĳ�ʼ����������Ƶȷ���ķ���
 * @author			barry
 */
class Quotation : public QuoteClientSpi
{
public:
	Quotation();
	~Quotation();

	/**
	 * @brief					��ʼ��ctp����ӿ�
	 * @return					>=0			�ɹ�
								<0			����
	 * @note					������������������У�ֻ������ʱ��ʵ�ĵ���һ��
	 */
	int							Initialize();

	/**
	 * @brief					�ͷ�ctp����ӿ�
	 */
	int							Release();

	/**
	 * @brief					ֹͣ����
	 */
	void						Halt();

	/**
	 * @brief					��ȡ�������汾��
	 */
	std::string&				QuoteApiVersion();

	/**
	 * @brief					��ȡ�Ự״̬��Ϣ
	 */
	WorkStatus&					GetWorkStatus();

public:///< ����ӿڵĻص�����
	virtual bool __stdcall		XDF_OnRspStatusChanged( unsigned char cMarket, int nStatus );
	virtual void __stdcall		XDF_OnRspRecvData( XDFAPI_PkgHead * pHead, const char * pszBuf, int nBytes );
	virtual void __stdcall		XDF_OnRspOutLog( unsigned char nLogType, unsigned char nLogLevel,const char * pLogBuf );
	virtual int	__stdcall		XDF_OnRspNotify( unsigned int nNotifyNo, void* wParam, void* lParam );

public:///< ������º���
	/**
	 * @brief					���»�ȡ�����г������ں�ʱ��
	 */
	void						SyncMarketsTime();

	/**
	 * @brief					�����/���յ���Ϣ��ʼ�������ݿ�
	 */
	void						SyncNametable2Database();

	/**
	 * @brief					�����������ݵ����ݿ�
	 */
	void						SyncSnapshot2Database();

protected:///< �����г���������
	/**
	 * @brief					�����Ϻ�Lv1�Ļ�����Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveShLv1();

	/**
	 * @brief					�����Ϻ�Option�Ļ�����Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveShOpt();

	/**
	 * @brief					��������Lv1�Ļ�����Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveSzLv1();

	/**
	 * @brief					����������Ȩ�Ļ�����Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveSzOpt();

	/**
	 * @brief					�����н��ڻ��Ļ�����Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveCFF();

	/**
	 * @brief					�����н���Ȩ�Ļ�����Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveCFFOPT();

	/**
	 * @brief					������Ʒ�ڻ��Ļ�����Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveCNF();

	/**
	 * @brief					������Ʒ��Ȩ�Ļ�����Ϣ
	 * @return					==0			�ɹ�
								!=0			ʧ��
	 */
	int							SaveCNFOPT();

private:
	WorkStatus					m_vctMkSvrStatus[64];	///< ���г�����״̬
	DataCollector				m_oQuotPlugin;			///< ������
	QuotationData				m_oQuoDataCenter;		///< �������ݼ���
	WorkStatus					m_oWorkStatus;			///< ����״̬
	char*						m_pDataBuffer;			///< ���ݻ���ָ��
};




#endif



