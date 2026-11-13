#pragma once

#include "DlgMovePoints.h"		// MOVEPOINTS_LISTS, CDlgMovePoints::GetMapName/GetTrainMapID

// CDlgMove dialog - tab Di chuyen (co luon giao dien 2 danh sach toa do)
// Danh sach 0: toa do luyen cong (m_Nodes); Danh sach 1: di den noi luyen cong (m_GoNodes).
// Quy uoc: so dong listbox == so node (chi so dong == chi so node, khong co khoang trong).

class CDlgMove : public CDialog
{
	DECLARE_DYNAMIC(CDlgMove)

public:
	CDlgMove(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMove();
	void			UpdateNearByPlayers(BOOL bSave);
	void			UpdatePointsList();			// dung lai ca 2 listbox + o ban do tu m_Nodes/m_GoNodes

// Dialog Data
	enum { IDD = IDD_MOVE_DIALOG };

protected:
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void	OnOK() {}
	virtual void	OnCancel() {}
	afx_msg void	OnButtonsClicked(UINT nID);
	afx_msg void	OnListSelChange(UINT nID);
	afx_msg void	OnEnKillfocusLureRange();

	void			RefreshList(int nList);		// dung lai 1 listbox tu m_Nodes (giu dong dang chon)
	void			ShowSelNode();				// dua dong dang chon (m_nEditList) vao o Sua X/Y
	void			UpdateMapLabel();			// o ban do chung cua 2 danh sach
	void			SendReadPosCmd(int nCmd, int nKind);	// Gui lenh doc toa do xuong VLHook (reply ve cua so chinh)
	void			SaveList(int nList);		// Ghi 1 danh sach ra file .mvp
	void			LoadList(int nList);		// Doc 1 danh sach tu file .mvp
	BOOL			EditSelNode();				// Sua dong dang chon theo X, Y nhap tay
	POINT*			GetNodes(int nList);
	int&			GetNodesMapID(int nList);

	DECLARE_MESSAGE_MAP()
public:
	CTcvnEdit		m_NameEdit;	// Dan ten copy tu game: giu byte TCVN3 goc
	CToolTipCtrl	m_ToolTip;
	CListBox		m_PointList[MOVEPOINTS_LISTS];	// 2 danh sach toa do
	CBitmap			m_bmpIcon[4];					// icon nut: Mo file / Ghi file / Doc TD / Ket thuc
	int				m_nEditList;					// danh sach dang lien ket voi o Sua (-1 = chua chon dong nao)
};
