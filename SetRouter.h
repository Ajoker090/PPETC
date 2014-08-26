#pragma once

#include "define.h"
#include "afxwin.h"

// CSetRouter dialog

class CSetRouter : public CDialog
{
	DECLARE_DYNAMIC(CSetRouter)

public:
	CSetRouter(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSetRouter();

// Dialog Data
	enum { IDD = IDD_DIALOG_SET_ROUTER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	SaveInfo	m_saveinfo;
	HANDLE		m_hfile;

	VOID GetModulePath(LPTSTR path, LPCTSTR module);
	BOOL IsIDOK(CString str);
	BOOL StrToID(CString str, char *ID);

	CEdit m_edit_1_ID;
	CEdit m_edit_2_ID;
	CEdit m_edit_3_ID;
	CEdit m_edit_4_ID;
	CEdit m_edit_1_name;
	CEdit m_edit_2_name;
	CEdit m_edit_3_name;
	CEdit m_edit_4_name;
	CButton m_button_save;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButton1();
};
