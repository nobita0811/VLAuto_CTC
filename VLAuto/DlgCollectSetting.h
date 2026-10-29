#pragma once

#define MAX_LIST_ITEMS_UI	64		// = MAX_BLACK_ITEMS / MAX_NOSAVE_ITEMS (VLHook/VLClasses.h)
#define LIST_ITEM_LEN		31		// Moi dong toi da 31 ky tu TCVN3

// Danh sach ten vat pham luu trong Maps\*.txt (TCVN3), hien thi Unicode
enum { nl_black, nl_ruong, nl_count };	// BlackItems.txt (khong nhat), RuongItem.txt (khong cat vao ruong)

struct ITEM_NAME_LIST
{
	LPCTSTR			szFile;			// Ten file trong thu muc Maps
	int				nReloadCmd;		// DEBUG_CMD de VLHook nap lai file
	UINT			nListID, nComboID, nAddID, nDelID;
	CListCtrl		List;
	CComboBox		Combo;
	CStringArray	arrHeader;		// Dong chu thich (#) dau file, giu nguyen khi ghi
	DEBUG_PARAM		ReloadParam;	// VLHook doc bang ReadProcessMemory sau PostMessage
};

// CDlgCollectSetting dialog

class CDlgCollectSetting : public CDialog
{
	DECLARE_DYNAMIC(CDlgCollectSetting)

public:
	CDlgCollectSetting(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgCollectSetting();
	void			UpdateMagicOptions(BOOL bSave);
	void			OnBnClickedAddButton();
	void			OnBnClickedDelButton();
	void			OnBnClickedSaveButton();
	void			OnBnClickedLoadButton();
	void			LoadNameList(ITEM_NAME_LIST& nl);
	BOOL			SaveNameList(ITEM_NAME_LIST& nl);
	void			ReloadNameListInGame(ITEM_NAME_LIST& nl);
	void			RequestInventoryNames();
	void			OnBnClickedNameAdd(ITEM_NAME_LIST& nl);
	void			OnBnClickedNameDel(ITEM_NAME_LIST& nl);

// Dialog Data
	enum { IDD = IDD_COLLECT_SETTING_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK();
	virtual void	OnCancel();

	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnCbnSelchangeItemattribCombo();
	afx_msg void	OnLbnSelchangeItemattribList();
	afx_msg void	OnCbnDropdownNameCombo();
	afx_msg BOOL	OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);

	DECLARE_MESSAGE_MAP()
public:
	CComboBox		m_ItemAttribCombo;
	CListBox		m_ItemAttribList;
	CToolTipCtrl	m_ToolTip;
	ITEM_NAME_LIST	m_NameList[nl_count];
	DEBUG_PARAM		m_InvParam;		// VLHook doc bang ReadProcessMemory sau PostMessage
};
