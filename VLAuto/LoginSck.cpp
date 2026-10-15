#include "stdafx.h"
#include "../GameDef.h"
#include "VLAuto.h"
#include "LoginSck.h"
#include "DlgLogin.h"

//Class VLAProtocol
VLAProtocol::VLAProtocol(void)
{
	memset(this, 0, sizeof(VLAProtocol));
}

//Class CLoginSck
CLoginSck::CLoginSck(void)
{
	m_pLoginDlg = NULL;
	m_nServerKey = 0;
	m_nClientKey = 0;
}

CLoginSck::~CLoginSck(void)
{
	Close();
}

BOOL CLoginSck::Create(CLoginDialog *pLoginDlg)
{
	m_pLoginDlg = pLoginDlg;
	return CSocket::Create();
}

void CLoginSck::OnReceive(int nErrorCode)
{
	CSocket::OnReceive(nErrorCode);

	if (nErrorCode)
	{
		CString szErrorMsg;
		szErrorMsg.Format(_T("Winsock OnReceive Error - Error Code = %d"), nErrorCode);
		AfxMessageBox(szErrorMsg);
	}
	else
		OnReceiveProcess();
}

#define ACC_BEGIN_SIZE 48
#define VLAHEADER_SIZE 12

BOOL CLoginSck::ReceiveData(void *lpBuf, int nSize)
{
	BYTE *lpReadBuf = (BYTE*)lpBuf;
	int nByte2Read = nSize;
	int nBytesRet = 0;

	while (nByte2Read > 0)
	{
		nBytesRet = Receive(lpReadBuf, nByte2Read);

		if (nBytesRet == SOCKET_ERROR || nBytesRet == 0)
		{
			DWORD dwError = GetLastError();
			CString ErrorMsg;
			ErrorMsg.Format(_T("Lỗi truyền dữ liệu. Size=%d, Code=%d."), nSize, dwError);
			m_pLoginDlg->OnErrorMessage(ErrorMsg, TRUE);
			return FALSE;
		}

		nByte2Read -= nBytesRet;
		lpReadBuf += nBytesRet;
	}

	return TRUE;
}

void CLoginSck::OnReceiveProcess()
{
	int nDataSize;

	if (m_nServerKey == 0 && m_nClientKey == 0)
	{
		AccountBegin AccBegin;
		if (!ReceiveData(&AccBegin, ACC_BEGIN_SIZE))
			return;

		m_nServerKey = ~AccBegin.ServerKey;
		m_nClientKey = ~AccBegin.ClientKey;
		if (strcmp(CLIENT_VERSION, AccBegin.Version))
		{
			m_pLoginDlg->OnErrorMessage(IDS_INCORRECT_VERSION);
		}
		else
		{	//Send Login Information
			char szLoginInfo[160] = "";
			char szLoginEmail[64] = "";
			char szHardDiskID[32] = "";
			unsigned int uNewCode;
			rand_s(&uNewCode);
			m_nServerKey ^= (uNewCode + HASH_CODE);
			m_pLoginDlg->GetEmailAddress(szLoginEmail);
			GetHardDriveComputerID(szHardDiskID);
			sprintf(szLoginInfo, "%sHardDiskID:%sSevrHashKey:%08XClientSN:%d", szLoginEmail, szHardDiskID, uNewCode, CLIENT_SERIAL);
			memset(szLoginEmail, 0, sizeof(szLoginEmail));
			nDataSize = (int)strlen(szLoginInfo) + VLAHEADER_SIZE;
			VLAProtocol VLALogin;
			VLALogin.nPtCode = ptCodeLogin;
			VLALogin.nPtPara = LOGIN_PARAM;
			VLALogin.nDataSize = nDataSize - VLAHEADER_SIZE;
			strcpy((char*)&VLALogin.PtData, szLoginInfo);
			DecodeEncodeData(nDataSize, (unsigned char*)&VLALogin, m_nClientKey);
			m_nClientKey = (m_nClientKey * 31) + 134775813L;
			Send(&VLALogin, nDataSize);
		}
	}
	else
	{
		VLAProtocol VLAData;

		if (!ReceiveData(&VLAData, VLAHEADER_SIZE))
			return;

		DecodeEncodeData(VLAHEADER_SIZE, (unsigned char*)&VLAData, m_nServerKey);

		if (VLAData.nDataSize > MAX_DATA_SIZE)
		{
			CString ErrorMsg;
			ErrorMsg.Format(_T("Lỗi truyền dữ liệu. Size=%d, Code=-10."), VLAData.nDataSize);
			m_pLoginDlg->OnErrorMessage(ErrorMsg, TRUE);
			return;
		}

		if (!ReceiveData(&VLAData.PtData, VLAData.nDataSize))
			return;

		DecodeEncodeData(VLAData.nDataSize, (unsigned char*)&VLAData.PtData, m_nServerKey);
		m_nServerKey = (m_nServerKey * 31) + 134775813L;

		switch (VLAData.nPtCode)
		{
		case ptCodeLogin:
			if (VLAData.nPtPara == 0) //LoginOK
			{
				m_pLoginDlg->ProcessLogin(&VLAData.PtData, VLAData.nDataSize);
			}
			else if (VLAData.nPtPara == 1) //LoginFailed
			{
				m_pLoginDlg->OnErrorMessage(IDS_INCORRECT_EMAIL);
			}
			break;
		}
	}
}

void CLoginSck::DecodeEncodeData(size_t uSize, unsigned char *pbyBuf, unsigned uKey)
{
    unsigned *puBuf = (unsigned *)pbyBuf;
    unsigned uRemainSize = uSize % 4;
    uSize /= 4;

    while (uSize-- > 0)
        *puBuf++ ^= uKey;

    pbyBuf = (unsigned char *)puBuf;
    
    while (uRemainSize-- > 0)
    {
        *pbyBuf++ ^= (unsigned char)(uKey & 0xff);
        uKey >>= 8;
    }
}
