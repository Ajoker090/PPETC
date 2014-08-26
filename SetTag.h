#pragma once
#include "afxwin.h"

#include "define.h"

// SetTag dialog

class SetTag : public CDialog
{
	DECLARE_DYNAMIC(SetTag)

public:
	SetTag(CWnd* pParent = NULL);   // standard constructor
	virtual ~SetTag();

// Dialog Data
	enum { IDD = IDD_DIALOG_SET_TAG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	SaveInfo	m_saveinfo;
	HANDLE		m_hfile;


	CEdit m_edit_1_ID;
	CEdit m_edit_1_name;
	CEdit m_edit_2_ID;
	CEdit m_edit_2_name;
	CEdit m_edit_3_ID;
	CEdit m_edit_3_name;
	CEdit m_edit_4_ID;
	CEdit m_edit_4_name;
	CEdit m_edit_5_ID;
	CEdit m_edit_5_name;
	CButton m_button_save;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButton1();
};
