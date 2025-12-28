#pragma once
class CmfctimerDlg : public CDialogEx
{
public:
	CmfctimerDlg(CWnd* pParent = nullptr);
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFC_TIMER_DIALOG };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
protected:
	HICON m_hIcon;
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};