// QLMatchCH.h: interface for the CQLMatchCH class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_QLMATCHCH_H__E1202B7B_E5BA_4308_9FCB_C7687BE9A46F__INCLUDED_)
#define AFX_QLMATCHCH_H__E1202B7B_E5BA_4308_9FCB_C7687BE9A46F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include <tchar.h>
#include <windows.h>

using namespace std;

class CQLMatchCH  
{
public:
	CQLMatchCH();
	virtual ~CQLMatchCH();

public:
	static bool MatchPinYin(const TCHAR *name, const TCHAR *key);
	static bool MatchPinYin(const TCHAR name, const TCHAR key);
	static bool ModifyPinYin(char * PyList);

	static BOOL InitStaticData();
	static int wcsCmpPY(const wchar_t* x, const wchar_t* y);

	static bool GetFirstPinYin(const TCHAR name, TCHAR *outkey);
	static bool GetPosPinYin(const TCHAR name, long *Pos);


protected:
//	bool ChtoPy(const TCHAR Ch);
	static bool MatchPinYin2(const TCHAR name, const TCHAR key);
	static bool ReadSortFile();
	static bool ReadPYIdxFile();

protected:
	//HANDLE m_hFile;
	//HANDLE m_hFile2;
	//char m_PyLst[8];
	//int m_nPyCount;
	struct stHead
	{
		char fieldlen;
		char filednum;
		unsigned short usStart;
		unsigned short usEnd;
	};
	static stHead m_stHead;
	//char * m_pRead;
	//DWORD m_dwSize;
	#define MAX_DATA_SIZE (200*1024)
	static char  m_Data[MAX_DATA_SIZE];
	static USHORT m_SortBuf[65536];

	struct stMohuYin
	{
		TCHAR	src;
		TCHAR	dst;
	};

	struct strPYIdx
	{
		TCHAR	src;
		TCHAR	dst;
		long pos;
	};


	static vector<stMohuYin> m_MHYList;
	static bool m_bInit;
	static vector<strPYIdx> m_PYIdxList;

};

#endif // !defined(AFX_QLMATCHCH_H__E1202B7B_E5BA_4308_9FCB_C7687BE9A46F__INCLUDED_)
