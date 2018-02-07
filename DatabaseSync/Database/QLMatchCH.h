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
	static bool MatchPinYin(const wchar_t *name, const wchar_t *key);
	static bool MatchPinYin(const wchar_t name, const wchar_t key);
	static bool ModifyPinYin(char * PyList);

	static char GetSimpPinYin(const wchar_t name);

	static BOOL InitStaticData();
	static int wcsCmpPY(const wchar_t* x, const wchar_t* y);

	static bool GetFirstPinYin(const wchar_t name, wchar_t *outkey);
	static bool GetPosPinYin(const wchar_t name, long *Pos);


protected:
//	bool ChtoPy(const TCHAR Ch);
	static bool MatchPinYin2(const wchar_t name, const wchar_t key);
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
		wchar_t	src;
		wchar_t	dst;
	};

	struct strPYIdx
	{
		wchar_t	src;
		wchar_t	dst;
		long pos;
	};


	static vector<stMohuYin> m_MHYList;
	static bool m_bInit;
	static vector<strPYIdx> m_PYIdxList;

};

#endif // !defined(AFX_QLMATCHCH_H__E1202B7B_E5BA_4308_9FCB_C7687BE9A46F__INCLUDED_)
