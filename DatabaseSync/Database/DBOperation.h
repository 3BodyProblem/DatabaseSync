#ifndef __DATABASE_DBOPERATION_H__
#define __DATABASE_DBOPERATION_H__


#pragma warning(disable:4786)
#include <windows.h>
#include <winsock.h>
#include <string>
#include <fstream>
#include "../Infrastructure/DateTime.h"
#include <mysql.h>


/**
 * @class			QuotationDatabase
 * @brief			�������ݿ������
 * @author			barry
 */
class QuotationDatabase
{
private:
	QuotationDatabase();

public:
	~QuotationDatabase();
	static QuotationDatabase&	GetDbObj();

	/**
	 * @brief					��ʼ�����ݿ����
	 * @return					==0						��ʼ���ɹ�
								!=0						ʧ��
	 */
	int							Initialize();

	/**
	 * @brief					�ͷ���Դ
	 */
	void						Release();

	/**
	 * @brief					�������ݿ�����
	 * @return					==0						�ɹ�
								!=0						ʧ��
	 */
	int							EstablishConnection();

public:
	/**
	 * @brief					��������
	 * @return					==0						�ɹ�
								!=0						ʧ��
	 */
	int							StartTransaction();

	/**
	 * @brief					�ύ����
	 * @return					==0						�ɹ�
								!=0						ʧ��
	 */
	int							Commit();

	/**
	 * @brief					��������
	 * @return					==0						�ɹ�
								!=0						ʧ��
	 */
	int							RollBack();

	/**
	 * @brief					����һ����Ʒ��¼
	 * @param[in]				nType					��Ʒ����
	 * @param[in]				nExchangeID				�г�����, '0'��δ֪ '1'��SSE���Ϻ�֤���������� '2'��SZSE������֤���������� '3'��cffEX���й������ڻ����ף� '4'��dcE ��������Ʒ�ڻ��������� '5'��ZcE��֣����Ʒ�ڻ��������� '6'��SHfE ���Ϻ��ڻ���������
	 * @param[in]				pszCode					��Ʒ����
	 * @param[in]				pszName					��Ʒ����
	 * @param[in]				nLotSize				��С�����������ֱ��ʣ�
	 * @param[in]				nContractMulti			��Լ����
	 * @param[in]				dPriceTick				��С�۸�䶯��λ
	 * @param[in]				dPreClose				���ռ�
	 * @param[in]				dPreSettle				����
	 * @param[in]				dUpperPrice				��ͣ�۸�
	 * @param[in]				dLowerPrice				��ͣ�۸�
	 * @param[in]				dPrice					��ǰ�۸�
	 * @param[in]				dOpenPrice				���̼۸�
	 * @param[in]				dSettlePrice			���۸�
	 * @param[in]				dClosePrice				���ռ۸�
	 * @param[in]				dBid1Price				��һ��
	 * @param[in]				dAsk1Price				��һ��
	 * @param[in]				dHighPrice				��߼�
	 * @param[in]				dLowPrice				��ͼ�
	 * @param[in]				dFluctuationPercent		�ǵ�����(�����̼ۼ���)
	 * @param[in]				nVolume					�ɽ���
	 * @param[in]				nTradingVolume			����
	 * @param[in]				dAmount					�ɽ����
	 * @param[in]				nIsTrading				�Ƿ��ױ��
	 * @param[in]				nTradingDate			��������
	 * @param[in]				dUpLimit				���޽��
	 * @param[in]				pszClassID				����ID
	 * @return					����insert into/replace��䷵�ص�affect number����
	 */
	int							Replace_Commodity( short nTypeID, short nExchangeID, const char* pszCode, const char* pszName
												, int nLotSize, int nContractMulti, double dPriceTick, double dPreClose, double dPreSettle
												, double dUpperPrice, double dLowerPrice, double dPrice, double dOpenPrice, double dSettlePrice
												, double dClosePrice, double dBid1Price, double dAsk1Price, double dHighPrice, double dLowPrice
												, double dFluctuationPercent, unsigned __int64 nVolume, unsigned __int64 nTradingVolume, double dAmount
												, short nIsTrading, unsigned int nTradingDate, double dUpLimit, const char* pszClassID );

	/**
	 * @brief					����һ����Ʒ������Ϣ
	 * @param[in]				nExchangeID				�г�����, '0'��δ֪ '1'��SSE���Ϻ�֤���������� '2'��SZSE������֤���������� '3'��cffEX���й������ڻ����ף� '4'��dcE ��������Ʒ�ڻ��������� '5'��ZcE��֣����Ʒ�ڻ��������� '6'��SHfE ���Ϻ��ڻ���������
	 * @param[in]				pszCode					��Ʒ����
	 * @param[in]				dPreClosePx				���ռ۸�
	 * @param[in]				dPreSettlePx			���۸�
	 * @param[in]				dUpperPx				��ͣ�۸�
	 * @param[in]				dLowerPx				��ͣ�۸�
	 * @param[in]				dLastPx					���¼۸�
	 * @param[in]				dSettlePx				����۸�
	 * @param[in]				dOpenPx					���̼۸�
	 * @param[in]				dClosePx				���̼۸�
	 * @param[in]				dBid1Px					��һ�۸�
	 * @param[in]				dAsk1Px					��һ�۸�
	 * @param[in]				dHighPx					��߼۸�
	 * @param[in]				dLowPx					��ͼ۸�
	 * @param[in]				dAmount					�ɽ����
	 * @param[in]				nVolume					�ɽ�����
	 * @param[in]				nTradingVolume			����
	 * @param[in]				dFluctuationPercent		�ǵ�����(�����̼ۼ���)
	 * @param[in]				nIsTrading				�Ƿ��ױ��
	 */
	int							Update_Commodity( short nExchangeID, const char* pszCode, double dPreClosePx, double dPreSettlePx, double dUpperPx, double dLowerPx, double dLastPx, double dSettlePx
												, double dOpenPx, double dClosePx, double dBid1Px, double dAsk1Px, double dHighPx, double dLowPx, double dAmount, unsigned __int64 nVolume, unsigned int nTradingVolume
												, double dFluctuationPercent, short nIsTrading );

protected:
	/**
	 * @brief					ִ��sql���
	 * @param[in]				pszSqlCmd				sql���
	 * @return					==0						ִ�гɹ�
								!=0						ʧ��
	 */
	__inline int				ExecuteSql( const char* pszSqlCmd );

protected:
	MYSQL						m_oMySqlHandle;			///< mysql���ʾ��
	MYSQL*						m_pMysqlConnection;		///< connection handle
	unsigned int				m_nTransRefCount;		///< �����������ü���
};


#endif







