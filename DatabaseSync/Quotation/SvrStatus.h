#ifndef __SERVER_STATUS_H__
#define __SERVER_STATUS_H__


#pragma warning(disable:4786)
#include <stdexcept>
#include "../QuoteCltDef.h"


/**
 * @class					T_SECURITY_STATUS
 * @brief					��Ʒ״̬��������
 */
typedef struct
{
	unsigned int			MkDate;					///< �г�����
	unsigned int			MkTime;					///< �г�����ʱ��
	char					MkStatusDesc[32];		///< �г�״̬������
	char					Code[32];				///< ��Ʒ����
	char					Name[64];				///< ��Ʒ����
	double					LastPx;					///< ���¼۸�
	double					Amount;					///< �ɽ����
	unsigned __int64		Volume;					///< �ɽ���
} T_SECURITY_STATUS;


/**
 * @class					ServerStatus
 * @brief					����״̬ά����
 * @author					barry
 */
class ServerStatus
{
private:
	ServerStatus();

public:
	static ServerStatus&	GetStatusObj();

public:///< ��Ʒ״̬���·���
	/**
	 * @brief				ê��һ����Ʒ���ṩ���������)
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			pszCode					��Ʒ����
	 * @param[in]			pszName					��Ʒ����
	 */
	void					AnchorSecurity( enum XDFMarket eMkID, const char* pszCode, const char* pszName );

	/**
	 * @brief				����һ����Ʒ����״̬
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			pszCode					��Ʒ����
	 * @param[in]			dLastPx					���¼۸�
	 * @param[in]			dAmount					�ɽ����
	 * @param[in]			nVolume					�ɽ���
	 */
	void					UpdateSecurity( enum XDFMarket eMkID, const char* pszCode, double dLastPx, double dAmount, unsigned __int64 nVolume );

	/**
	 * @brief				��ȡ��Ʒ״̬����
	 * @param[in]			eMkID					�г�ID
	 * @return				������Ʒ״̬����
	 */
	T_SECURITY_STATUS&		FetchSecurity( enum XDFMarket eMkID );

public:///< �г�ʱ����·���
	/**
	 * @brief				�����г�ʱ��
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			nMkDate					�г�����
	 * @param[in]			nMkTime					�г�ʱ��
	 */
	void					UpdateMkTime( enum XDFMarket eMkID, unsigned nMkDate, unsigned int nMkTime );

	/**
	 * @brief				��ȡĳ�г���ʱ��
	 * @param[in]			eMkID					�г�ID
	 * @return				�����г�ʱ��
	 */
	unsigned int			FetchMkTime( enum XDFMarket eMkID );

	/**
	 * @brief				��ȡĳ�г�������
	 * @param[in]			eMkID					�г�ID
	 * @return				�����г�����
	 */
	unsigned int			FetchMkDate( enum XDFMarket eMkID );

public:///< ���г���tick����ռ���ʴ�ȡ����
	/**
	 * @brief				����tick����ռ����
	 * @param[in]			nRate					tick����ռ����
	 */
	void					UpdateTickBufOccupancyRate( int nRate );

	/**
	 * @brief				��ȡtick����ռ����
	 */
	int						FetchTickBufOccupancyRate();

public:///< ���г���״̬
	/**
	 * @brief				��ȡ���г�����ռ����
	 * @param[in]			eMkID					�г�ID
	 * @param[in]			pszDesc					�г�״̬������
	 */
	void					UpdateMkStatus( enum XDFMarket eMkID, const char* pszDesc );

	/**
	 * @brief				��ȡ�г�����״̬������
	 * @param[in]			eMkID					�г�ID
	 */
	const char*				GetMkStatus( enum XDFMarket eMkID );

public:///< ���̶�ʧ����ͳ��
	/**
	 * @brief				����tick����ʧ�ܼ���
	 */
	void					AddTickLostRef();

	/**
	 * @brief				��ȡtick����ʧ�ܼ���
	 */
	unsigned int			GetTickLostRef();

protected:
	T_SECURITY_STATUS		m_vctLastSecurity[256];			///< ���г��ĵ�һ����Ʒ��״̬���±�
	unsigned int			m_nTickLostCount;				///< Tick����ʧ��ͳ��
	unsigned int			m_nTickBufOccupancyRate;		///< Tick�����ռ����
};



#endif





