// ARF24_DEMODlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "define.h"
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/sqlstring.h>
#include <cppconn/prepared_statement.h>

// CARF24_DEMODlg 对话框
class CARF24_DEMODlg : public CDialog
{
// 构造
public:
	CARF24_DEMODlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_ARF24_DEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	SaveInfo	m_saveinfo;
	HANDLE		m_hfile;
	HANDLE		m_hcom;
	HANDLE		m_hfile_log;

	int		m_Receive_Data_Len;
	BYTE	m_Receive_Data_Char[MAX_RECEIVE_DATA_LEN];

	bool	m_test_baudrate_ok;

	int		m_lastcmd;

	ReceiveInfo	m_receiveinfo[MAX_RECEIVEINFO_COUNT];
	int		m_current_receive_count;

	ListShowInfo	m_listshowinfo[MAX_RECEIVEINFO_COUNT];
	int		m_current_listitem_count;

	BYTE	m_current_tag[ID_LEN];
	BYTE	m_current_router[ID_LEN];
	char	m_current_single;
	char	m_current_T_INT;
	char	m_current_T_DEC;
	char	m_current_H_INT;
	char	m_current_H_DEC;

	bool	m_bconnect;

	HDC	m_hdc;
	HDC	m_mdc;
	HDC m_router_dc;

	HBITMAP m_mbmp;
	HBITMAP m_routerbmp;

	HBRUSH m_Brush_white;
	HBRUSH m_Brush_Blue;

	BOOL GetTagRouterInfo();
	bool OpenCom(int com);
	bool OpenCom(CString str);
	bool SetBaudRate(int baudrate);
	void ParseData(sql::Driver *driver, sql::Connection *con, sql::Statement *stmt, sql::PreparedStatement *pstmt);
	VOID GetModulePath(LPTSTR path, LPCTSTR module);
	int FindChar(BYTE *str, int strlen, BYTE c1, BYTE c2);
	int FindTagIdInList(char *id);
//	void IdToChar(const char *p ,char *byte)
	void IdToString(CString &str, char *id);
	int FindTagIdInSave(char *id);
	int FindRouterIdInSave(char *id);
	void Draw();
	void OpenLogFile();

	CButton m_button_connect;
	CStatic m_picture_connect;
	CStatic m_picture_disconnect;
	CButton m_button_set_tag;
	CButton m_button_set_router;
	CListCtrl m_list_info;
	afx_msg void OnBnClickedButtonSetTag();
	afx_msg void OnBnClickedButtonSetRouter();
	CStatic m_static_com;
	CStatic m_static_baud;
	CComboBox m_combo_com;
	CComboBox m_combo_baud;
	CButton m_button_autoconnect;
	CButton m_button_clean;
	afx_msg void OnBnClickedButtonConncet();
	afx_msg void OnBnClickedButtonAutoConnect();
	afx_msg void OnBnClickedButtonClean();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
