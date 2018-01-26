// QLMatchCH.cpp: implementation of the CQLMatchCH class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "QLMatchCH.h"
#include "QWin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma  warning(disable:4996)

#define PINYINNAME _T("PinYin")
#define PINYINEXT _T("py")
#define MOHUYINNAME _T("MoHuYin")

#define QL_MAX_PATH 512
char  CQLMatchCH::m_Data[MAX_DATA_SIZE];
bool  CQLMatchCH::m_bInit = false;
CQLMatchCH::stHead CQLMatchCH::m_stHead = {0, 0, 0, 0};
vector<CQLMatchCH::stMohuYin> CQLMatchCH::m_MHYList;
vector<CQLMatchCH::strPYIdx> CQLMatchCH::m_PYIdxList;

USHORT CQLMatchCH::m_SortBuf[65536];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQLMatchCH::CQLMatchCH()
{
}

CQLMatchCH::~CQLMatchCH()
{
}

bool CQLMatchCH::MatchPinYin2(const TCHAR name, const TCHAR key)
{
	if(!InitStaticData())
	{
		return false;
	}

	bool ret = false;

	int pos = (int)name - (int)m_stHead.usStart;

	if(m_stHead.filednum > 0 && pos >= 0 && pos < MAX_DATA_SIZE / m_stHead.filednum)
	{
		int i = 0;

		for(i=0; i<m_stHead.filednum; i++)
		{
			if(m_Data[pos * m_stHead.filednum + i] == 0)
			{
				break;
			}
			else if(m_Data[pos * m_stHead.filednum + i] == key)
			{
				ret = true;
				break;
			}
		}
	}

	return ret;
}

bool CQLMatchCH::MatchPinYin(const TCHAR name, const TCHAR key)
{
	if(!InitStaticData())
	{
		return false;
	}

	bool ret = MatchPinYin2(name, key);
	if (!ret)
	{
		int mhcount = (int)m_MHYList.size();
		int i = 0;
		for(i = 0; i < mhcount; i++)
		{
			if( m_MHYList[i].src == key)
			{
				ret = MatchPinYin2(name, m_MHYList[i].dst);
				if (ret)
					break;
			}
		}
	}
	return ret;
}

bool CQLMatchCH::MatchPinYin(const TCHAR *name, const TCHAR *key)
{
	if(!InitStaticData())
	{
		return false;
	}

	bool bRlt = false;

	if (!name || !key)
		return false;

	TCHAR szName[64] = {0};
	memset(szName, 0, sizeof(szName));
	_tcscpy(szName, name);

	int nNameLen = _tcslen(name);
	int nKeyLen = _tcslen(key);

	int i = 0, j = 0;
	for (i = 0; i < nNameLen; i++)
	{
		if (szName[i] == _T(' '))
		{
			j = i;
			for (j = i; j < nNameLen; j++)
			{
				szName[j] = szName[j+1];
			}

			i--;
		}
	}

	nNameLen = _tcslen(szName);

	if (nNameLen < nKeyLen)
	{
		return false;
	}

	if (nNameLen <= 0 || nKeyLen <= 0)
		return false;

	bRlt = true;
	int nCount = 0;

	for (nCount = 0; nCount < nKeyLen; nCount ++)
	{
		if(!MatchPinYin(szName[nCount], key[nCount]))
		{
			bRlt = false;
			break;
		}
	}

	return bRlt;
}

int CQLMatchCH::wcsCmpPY(const wchar_t* x, const wchar_t* y)
{
	if(!InitStaticData())
	{
		return 0;
	}

	int ret = 0;

	wchar_t* p = (wchar_t*)x;
	wchar_t* q = (wchar_t*)y;

	for(; *p!=0 && *q!=0; p++, q++)
	{
		if(m_SortBuf[*p] > m_SortBuf[*q])
		{
			ret = 1;
			break;
		}
		else if(m_SortBuf[*p] < m_SortBuf[*q])
		{
			ret = -1;
			break;
		}
	}
	if(ret == 0)
	{
		if(*p != 0)
		{
			ret = 1;
		}
		else if(*q != 0)
		{
			ret = -1;
		}
	}

	return ret;
}

bool CQLMatchCH::ReadPYIdxFile()
{
	TCHAR dllpath[MAX_PATH], drv[_MAX_DRIVE], dir[_MAX_DIR], fn[_MAX_FNAME], ext[_MAX_EXT];
	TCHAR sortpath[MAX_PATH];

	GetModuleFileName(theApp.m_hInstance, dllpath, MAX_PATH);

	_tsplitpath(dllpath, drv, dir, fn, ext);

	_tmakepath(sortpath, drv, dir, _T("pyidx"), _T("ini"));


	m_PYIdxList.clear();

	TCHAR tcReturnString[256] = {0};
	long size;
	size = GetPrivateProfileSection(_T("pyidx"),tcReturnString,256,sortpath);
	int nPos = 0;
	while (nPos < size)
	{
		if (tcReturnString[nPos] == _T('='))
		{
			strPYIdx buf;
			memset(&buf,0,sizeof(strPYIdx));
			buf.src = _totupper(tcReturnString[nPos-1]);
			buf.dst = _totupper(tcReturnString[nPos+1]);
			if(buf.src != 0 && buf.dst != 0)
			{
				m_PYIdxList.push_back(buf);
			}
		}
		nPos ++;
	}

	//获取位置信息
	unsigned int i;
	long pos = -1;
	for(i=0;i<m_PYIdxList.size();i++)
	{
		if(GetPosPinYin(m_PYIdxList[i].dst,&pos))
		{
			m_PYIdxList[i].pos = pos;
		}
		else
		{
			m_PYIdxList[i].pos = -1;
		}
	}
	return true;
}


bool CQLMatchCH::ReadSortFile()
{
	TCHAR dllpath[MAX_PATH], drv[_MAX_DRIVE], dir[_MAX_DIR], fn[_MAX_FNAME], ext[_MAX_EXT];
	TCHAR sortpath[MAX_PATH];

	GetModuleFileName(theApp.m_hInstance, dllpath, MAX_PATH);

	_tsplitpath(dllpath, drv, dir, fn, ext);

	_tmakepath(sortpath, drv, dir, _T("hanzi"), _T("sort"));

	FILE* pf = _tfopen(sortpath, _T("rb"));

	if(NULL != pf)
	{
		USHORT count = fread(m_SortBuf, 2, 65536, pf);

		fclose(pf);

		return true;
	}

	return false;
}

BOOL CQLMatchCH::InitStaticData()
{
	BOOL ret = TRUE;

	while(!m_bInit)
	{
		m_bInit = true;
		TCHAR respath[QL_MAX_PATH], respath2[QL_MAX_PATH], dllpath[QL_MAX_PATH], drv[_MAX_DRIVE], dir[QL_MAX_PATH], fn[_MAX_FNAME], ext[_MAX_EXT];
		::GetModuleFileName(theApp.m_hInstance, dllpath, QL_MAX_PATH-1);
		_tsplitpath(dllpath, drv, dir, fn, ext);

		//读取拼音映射表
		_tmakepath(respath, drv, dir, PINYINNAME, PINYINEXT);
		memset(&m_stHead, 0, sizeof(stHead));

		HANDLE hfPY = CreateFile(respath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

		if (hfPY != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(hfPY, 0L, NULL, SEEK_SET);

			int size = GetFileSize(hfPY, NULL);

			if(size > sizeof(stHead) && size-sizeof(stHead) <= MAX_DATA_SIZE)
			{
				unsigned long rlt=0;
				ReadFile(hfPY,&m_stHead, sizeof(stHead),&rlt,NULL);

				if((m_stHead.usEnd - m_stHead.usStart) * m_stHead.filednum <= size - sizeof(stHead))
				{
					ReadFile(hfPY, m_Data, (m_stHead.usEnd - m_stHead.usStart) * m_stHead.filednum, &rlt, NULL);
				}
			}

			CloseHandle(hfPY);
		}
		else
		{
			ret = FALSE;
			break;
		}

		//读取多音字表
		_tmakepath(respath, drv, dir, PINYINNAME, _T("dyz"));
		HANDLE hfDYZ = CreateFile(respath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

		if (hfDYZ != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(hfDYZ, 0L, NULL, SEEK_SET);

			int size = GetFileSize(hfDYZ, NULL);

			if(size > 2)
			{
				unsigned long rlt=0;
				
				//判断是否为unicode文件
				BYTE unicodeflg[2];
				ReadFile(hfDYZ, unicodeflg, 2, &rlt, NULL);
				wchar_t* douyinzi = NULL;
				int count = 0;
				if(unicodeflg[0] == 0xff && unicodeflg[1] == 0xfe)
				{//是unicode
					count = (size - 2) / 2;
					douyinzi = new wchar_t[count + 1];
					ReadFile(hfDYZ, douyinzi, count*2, &rlt, NULL);
					douyinzi[count] = 0;
				}
				else
				{//是gbk
					/*SetFilePointer(hfDYZ, 0L, NULL, SEEK_SET);
					count = size / 2;
					douyinzi = new wchar_t[count + 1];
					char* douyinziA = new char[size + 1];
					ReadFile(hfDYZ, douyinziA, size, &rlt, NULL);
					C2T(douyinziA, douyinzi, count + 1, CP_GBK);
					delete []douyinziA;*/
				}

				int i;
				for(i=0; i<count; i++)
				{
					if(douyinzi[i] == '\r' || douyinzi[i] == '\n')
					{
						continue;
					}

					WORD pos = douyinzi[i] - m_stHead.usStart;
					for(i++; i<count; i++)
					{
						if(douyinzi[i] == '\r' || douyinzi[i] == '\n')
						{
							break;
						}
						
						bool bfind = false;
						int ii = 0;
						for(ii=0; ii<m_stHead.filednum; ii++)
						{
							if(m_Data[pos * m_stHead.filednum + ii] == 0)
							{
								break;
							}
							else if(m_Data[pos * m_stHead.filednum + ii] == douyinzi[i])
							{
								bfind = true;
								break;
							}
						}

						if(!bfind)
						{
							m_Data[pos * m_stHead.filednum + ii] = douyinzi[i];
						}
					}
				}

				delete []douyinzi;
			}

			CloseHandle(hfDYZ);
		}

		//读取模糊音映射表
		_tmakepath(respath2, drv, dir, MOHUYINNAME, _T("ini"));
		TCHAR szMHY[256] = {0};
		INT size = GetPrivateProfileSection(_T("MOHUYIN"), szMHY, MAX_PATH, respath2);
		int nPos = 0;
		while (nPos < size)
		{
			if (szMHY[nPos] == _T('='))
			{
				stMohuYin buf;
				buf.src = _totupper(szMHY[nPos-1]);
				buf.dst = _totupper(szMHY[nPos+1]);
				if(buf.src != 0 && buf.dst != 0)
				{
					m_MHYList.push_back(buf);
				}
			}
			nPos ++;
		}
		/*HANDLE hfMH = CreateFile(respath2,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

		if (hfMH != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(hfMH, 0L, NULL, SEEK_SET);

			int size = GetFileSize(hfMH, NULL);

			if(size > 0)
			{
				unsigned long rlt=0;
				char *pData = new char[size + 1];
				pData[size] = 0;

				if (ReadFile(hfMH, pData, size ,&rlt, NULL))
				{
					int nPos = 0;
					int nCount = 0;

					while (nPos < size)
					{
						if (pData[nPos] == '=')
						{
							stMohuYin buf;
							buf.src = pData[nPos-1];
							buf.dst = pData[nPos+1];

							if(buf.src != 0 && buf.dst != 0)
							{
								m_MHYList.push_back(buf);
							}
						}
						nPos ++;
					}
				}

				delete []pData;
			}

			CloseHandle(hfMH);
		}
		else
		{
			ret = FALSE;
			break;
		}*/

		ret = ReadSortFile();
		
		ReadPYIdxFile();
		break;
	}
	
	return ret;
}


bool CQLMatchCH::GetFirstPinYin(const TCHAR name, TCHAR *outkey)
{
	if(!InitStaticData())
	{
		return false;
	}

	long pos = -1;
	bool rlt;
	rlt = GetPosPinYin(name,&pos);
	if(!rlt)
	{
		return false;
	}
	int i;
	int size = m_PYIdxList.size();
	if(size<=0)
		return false;
	if(pos<m_PYIdxList[0].pos)
		return false;
	if(pos>m_PYIdxList[size-1].pos)
	{
		*outkey = m_PYIdxList[size-1].src;
		return true;
	}
	for(i=0;i<size;i++)
	{
		if(i<size-1)
		{
			if((pos>=m_PYIdxList[i].pos)&&(pos<m_PYIdxList[i+1].pos))
			{
				*outkey = m_PYIdxList[i].src;
				return true;
			}
		}
	}
	return false;
}

bool CQLMatchCH::GetPosPinYin(const TCHAR name, long *Pos)
{
	if(!InitStaticData())
	{
		return false;
	}

	*Pos = 0;
	int ipos = (int)name;
	if(ipos>=65536)
		return false;

	*Pos = m_SortBuf[ipos];
	return true;
}
