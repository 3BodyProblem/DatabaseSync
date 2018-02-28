//-----------------------------------------------------------------------------------------------------------------------------
//                `-.   \    .-'
//        ,-"`````""-\__ |  /							�ļ����ƣ�MQWinCAStruct
//         '-.._    _.-'` '-o,							�ļ�������QWinCA���ݽṹ
//             _>--:{{<   ) |)							�ļ���д��Lumy
//         .-''      '-.__.-o`							�������ڣ�2017.04.03
//        '-._____..-/`  |  \							�������ڣ�2017.04.03
//                ,-'   /    `-.
//-----------------------------------------------------------------------------------------------------------------------------
#ifndef __QWinCA_MQWinCAStruct_HPP__
#define __QWinCA_MQWinCAStruct_HPP__
//-----------------------------------------------------------------------------------------------------------------------------
#pragma pack(1)
//.............................................................................................................................
typedef struct											//QWIN��Ʒ��Ϣ��¼
{
	unsigned int				uiProductID;			//��Ʒ���
	char						szProductName[32];		//��Ʒ����
	char						szRightInfo[300];		//Ȩ���������Զ���','�ָ���
	unsigned int				uiStartDate;			//��ʼ���ڡ�YYYYMMDD��ʽ��
	unsigned int				uiEndDate;				//�������ڡ�YYYYMMDD��ʽ��
	unsigned int				uiLimitCount;			//������������������ն�������
	char						szMarket[64];			//֧���г����Զ���','�ָ���
														//1:��������ֻ���������Ʊ������ծȯ�ȣ�
														//2:���������Ȩ������ETF��Ȩ��
														//3:�н�����ָ�ڻ�
														//4:�н�����ָ��Ȩ
														//5:������Ʒ��������Ʒ�ڻ�
														//6:������Ʒ��������Ʒ��Ȩ
	char						szReserved[100];		//����
} tagQWinCA_ProductRecord;
//.............................................................................................................................
typedef struct											//QWIN��Ʒ��Ϣ 
{
	tagQWinCA_ProductRecord		sProductRecord[15];		//��Ʒ��Ϣ
	unsigned int				uiProductRecordCount;	//��Ʒ����
	char						szCheckCode[16];		//��Ʒ��ϢУ��
} tagQWinCA_ProductInfo;
//.............................................................................................................................
typedef struct											//QWIN֤�� 
{
	unsigned int				uiVersion;				//֤��汾
	unsigned int				uiCustomID;				//�ͻ����
	char						szCustomName[64];		//�ͻ�����
	unsigned int				uiCreateDate;			//֤�鴴�����ڡ�YYYYMMDD��ʽ��
	unsigned int				uiCreateTime;			//֤�鴴��ʱ�䡾HHMMSS��ʽ��
	unsigned int				uiDeadDate;				//֤����Ч���ڡ�YYYYMMDD��ʽ��
	unsigned int				uiMachineCode;			//������
	tagQWinCA_ProductInfo		sProductInfo;			//��Ʒ��Ϣ
	char						szReserved[388];		//����
	char						szCheckCode[16];		//֤��У��
} tagQWinCA_Certificate;
//.............................................................................................................................
typedef struct											//QWINӲ������Ϣ
{
	unsigned int				uiVersion;				//��Ϣ�汾
	unsigned int				uiCustomID;				//�ͻ����
	char						szCustomName[64];		//�ͻ�����
	unsigned int				uiCreateDate;			//֤�鴴�����ڡ�YYYYMMDD��ʽ��
	unsigned int				uiCreateTime;			//֤�鴴��ʱ�䡾HHMMSS��ʽ��
	char						szPassword[16];			//�ͻ���Կ
	char						szReserved[16];			//����
	char						szCheckCode[16];		//֤��У��
} tagQWinCA_HardLockInfo;
//.............................................................................................................................
#pragma pack()
//-----------------------------------------------------------------------------------------------------------------------------
class MQWinCA_Interface									//QWinCA�ṩ�ӿ�
{
public:
	//��ȡ����������
	virtual unsigned int GetMachineCode(void) = 0;
	//��ȡ��ǰQWinCA֤����Ϣ
	virtual bool GetQWinCA(const char * szQWinPath,tagQWinCA_Certificate * lpOut,char * szErrorString,unsigned int uiErrorSize) = 0;
	//��ȡӲ������Ϣ
	virtual bool ReadHardLockInfo(tagQWinCA_HardLockInfo * lpOut,char * szErrorString,unsigned int uiErrorSize) = 0;
};
//.............................................................................................................................
//����һ��QWINCA�ӿڶ���
extern MQWinCA_Interface * QWinCA_CreateObject(void);
//�ͷ�һ��QWinCA�ӿڶ���
extern void QWinCA_DeleteObject(MQWinCA_Interface * lpObject);
//-----------------------------------------------------------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------------------------------------------------------