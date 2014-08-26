// SetRouter.cpp : implementation file
//

#include "stdafx.h"
#include "ARF24_DEMO.h"
#include "SetRouter.h"


// CSetRouter dialog

IMPLEMENT_DYNAMIC(CSetRouter, CDialog)

CSetRouter::CSetRouter(CWnd* pParent /*=NULL*/)
	: CDialog(CSetRouter::IDD, pParent)
{

}

CSetRouter::~CSetRouter()
{
}

void CSetRouter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_edit_1_ID);
	DDX_Control(pDX, IDC_EDIT3, m_edit_2_ID);
	DDX_Control(pDX, IDC_EDIT5, m_edit_3_ID);
	DDX_Control(pDX, IDC_EDIT7, m_edit_4_ID);
	DDX_Control(pDX, IDC_EDIT2, m_edit_1_name);
	DDX_Control(pDX, IDC_EDIT4, m_edit_2_name);
	DDX_Control(pDX, IDC_EDIT6, m_edit_3_name);
	DDX_Control(pDX, IDC_EDIT8, m_edit_4_name);
	DDX_Control(pDX, IDC_BUTTON1, m_button_save);
}


BEGIN_MESSAGE_MAP(CSetRouter, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &CSetRouter::OnBnClickedButton1)
END_MESSAGE_MAP()


// CSetRouter message handlers

VOID CSetRouter::GetModulePath(LPTSTR path, LPCTSTR module)
{
	TCHAR* s;
	HANDLE Handle = NULL;
	if(module)
		Handle = GetModuleHandle(module);
	GetModuleFileName((HMODULE)Handle, path, MAX_PATH);
	s = _tcsrchr(path, '\\');
	if(s) s[1] = 0;
}

BOOL CSetRouter::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	
	WCHAR szPath[MAX_PATH] = {0};
	GetModulePath(szPath, NULL);
	CString filepath;
	filepath = szPath;
	filepath = filepath + SAVE_FILE_NAME;
	m_hfile = CreateFile(filepath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);   
  	if(m_hfile == INVALID_HANDLE_VALUE)   
	{   
		//文件不存在，创建文件
		m_hfile = CreateFile(filepath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,NULL);   
  		if(m_hfile == INVALID_HANDLE_VALUE)   
		{   
			//创建文件失败
			MessageBox(TEXT("Failed to create file !"));
			return FALSE;  
		}
		memset(&m_saveinfo, 0, sizeof(m_saveinfo));
		DWORD write;
		WriteFile(m_hfile, &m_saveinfo, sizeof(m_saveinfo), &write, NULL);
	} 
	else
	{
		//文件存在，读取数据
		DWORD Read;
		ReadFile(m_hfile, &m_saveinfo, sizeof(m_saveinfo), &Read, NULL);		
	}
	//把数据显示到文本框中
	CString str_text, str;

	str_text = TEXT("");
	for(int i=0; i<ID_LEN; i++)
	{
		str.Format(TEXT("%02X"), (BYTE)m_saveinfo.router[0].ID[i]);
		str_text += str;
	}
	m_edit_1_ID.SetWindowTextW(str_text);

	str_text = TEXT("");
	for(int i=0; i<ID_LEN; i++)
	{
		str.Format(TEXT("%02X"), (BYTE)m_saveinfo.router[1].ID[i]);
		str_text += str;
	}
	m_edit_2_ID.SetWindowTextW(str_text);

	str_text = TEXT("");
	for(int i=0; i<ID_LEN; i++)
	{
		str.Format(TEXT("%02X"), (BYTE)m_saveinfo.router[2].ID[i]);
		str_text += str;
	}
	m_edit_3_ID.SetWindowTextW(str_text);

	str_text = TEXT("");
	for(int i=0; i<ID_LEN; i++)
	{
		str.Format(TEXT("%02X"), (BYTE)m_saveinfo.router[3].ID[i]);
		str_text += str;
	}
	m_edit_4_ID.SetWindowTextW(str_text);

	

	//Name
	WCHAR name[NAME_LEN+1] = {0};
	mbstowcs(name, m_saveinfo.router[0].Name, NAME_LEN);
	m_edit_1_name.SetWindowTextW(name);

	mbstowcs(name, m_saveinfo.router[1].Name, NAME_LEN);
	m_edit_2_name.SetWindowTextW(name);

	mbstowcs(name, m_saveinfo.router[2].Name, NAME_LEN);
	m_edit_3_name.SetWindowTextW(name);

	mbstowcs(name, m_saveinfo.router[3].Name, NAME_LEN);
	m_edit_4_name.SetWindowTextW(name);

	

	m_button_save.EnableWindow(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSetRouter::IsIDOK(CString str)
{
	if(str.GetLength() != 16)
		return FALSE;
	for(int i=0; i<16; i++)
	{
		if((str.GetAt(i) >= '0' && str.GetAt(i) <= '9') || (str.GetAt(i) >= 'A' && str.GetAt(i) <= 'F'))
			;
		else
			return FALSE;
	}
	return TRUE;
}

BOOL CSetRouter::StrToID(CString str, char *ID)
{
	int len = str.GetLength();
	for(int i=0; i<len/2; i++)
	{
		if(str.GetAt(2*i) >= '0' && str.GetAt(2*i) <= '9')
			ID[i] = (str.GetAt(2*i) - '0')<<4;
		else if(str.GetAt(2*i) >= 'A' && str.GetAt(2*i) <= 'F')
			ID[i] = (str.GetAt(2*i) - 'A' + 10)<<4;
		else
			return FALSE;

		if(str.GetAt(2*i+1) >= '0' && str.GetAt(2*i+1) <= '9')
			ID[i] += (str.GetAt(2*i+1) - '0');
		else if(str.GetAt(2*i+1) >= 'A' && str.GetAt(2*i+1) <= 'F')
			ID[i] += (str.GetAt(2*i+1) - 'A' + 10);
		else
			return FALSE;
	}
	return TRUE;
}

void CSetRouter::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	CString str;

	m_edit_1_ID.GetWindowTextW(str);
	if(!IsIDOK(str))
	{
		MessageBox(TEXT("TAG ID錯誤，請輸入16位0-F的十六進制數據！"));
		return;
	}
	StrToID(str, m_saveinfo.router[0].ID);

	m_edit_2_ID.GetWindowTextW(str);
	if(!IsIDOK(str))
	{
		MessageBox(TEXT("TAG ID錯誤，請輸入16位0-F的十六進制數據！"));
		return;
	}
	StrToID(str, m_saveinfo.router[1].ID);

	m_edit_3_ID.GetWindowTextW(str);
	if(!IsIDOK(str))
	{
		MessageBox(TEXT("TAG ID錯誤，請輸入16位0-F的十六進制數據！"));
		return;
	}
	StrToID(str, m_saveinfo.router[2].ID);

	m_edit_4_ID.GetWindowTextW(str);
	if(!IsIDOK(str))
	{
		MessageBox(TEXT("TAG ID錯誤，請輸入16位0-F的十六進制數據！"));
		return;
	}
	StrToID(str, m_saveinfo.router[3].ID);

	
	//name

	m_edit_1_name.GetWindowTextW(str);
	wcstombs(m_saveinfo.router[0].Name, str, NAME_LEN);

	m_edit_2_name.GetWindowTextW(str);
	wcstombs(m_saveinfo.router[1].Name, str, NAME_LEN);

	m_edit_3_name.GetWindowTextW(str);
	wcstombs(m_saveinfo.router[2].Name, str, NAME_LEN);

	m_edit_4_name.GetWindowTextW(str);
	wcstombs(m_saveinfo.router[3].Name, str, NAME_LEN);

	

	DWORD write;
	::SetFilePointer(m_hfile, 0, 0, FILE_BEGIN);
	WriteFile(m_hfile, &m_saveinfo, sizeof(m_saveinfo), &write, NULL);
	OnCancel();
}
