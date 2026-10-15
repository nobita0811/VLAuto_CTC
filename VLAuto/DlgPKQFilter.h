#pragma once

#define TONGPK_KIND 8
const TCHAR TongPKKind[TONGPK_KIND][12] =
{ _T("Bất kỳ"), _T("Binh giáp"), _T("Thiên công"), _T("Dị dung"), _T("Thí luyện"), _T("Thiên ý"), _T("Lễ phẩm"), _T("Hoạt động") };

// CPKQuestFilter dialog

class CPKQuestFilter : public CDialog
{
	DECLARE_DYNAMIC(CPKQuestFilter)

public:
	CPKQuestFilter(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPKQuestFilter();

// Dialog Data
	enum { IDD = IDD_TONGPKFILTER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	int m_nQuestKind;
	int m_nLevel;
	int m_nLetters;
};
