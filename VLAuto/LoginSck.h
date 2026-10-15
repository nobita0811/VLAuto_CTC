#pragma once

#define MAX_DATA_SIZE 8000

class CLoginDialog;

enum ProtocolCode
{
    ptCodeFirst = 1977,
    ptCodeLogin,
    ptCodeCheckNew,
};

class AccountBegin
{
public:
	char Reserve1[16];
	int ServerKey;
	int ClientKey;
	char Reserve2[16];
	char Version[8];
};

class VLAProtocol
{
public:
	int nPtCode;
	int nPtPara;
	int nDataSize;
	BYTE PtData[MAX_DATA_SIZE];

public:
	VLAProtocol(void);
};

class CLoginSck :
	public CSocket
{
public:
	CLoginDialog *m_pLoginDlg;

	CLoginSck(void);
	~CLoginSck(void);
	BOOL Create(CLoginDialog *pMainDlg);
	virtual void OnReceive(int nErrorCode);

private:
	int m_nServerKey;
	int m_nClientKey;

	BOOL ReceiveData(void *lpBuf, int nSize);
	void OnReceiveProcess();
	void DecodeEncodeData(size_t uSize, unsigned char *pbyBuf, unsigned uKey);
};
