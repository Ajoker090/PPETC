// ARF24_DEMODlg.cpp : 桫��`��
//

#include "stdafx.h"
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <cstring>
#include <afx.h>
#include <afxpriv.h>

#include "ARF24_DEMO.h"
#include "ARF24_DEMODlg.h"
#include "SetTag.h"
#include "SetRouter.h"
#include "math.h"

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/sqlstring.h>
#include <cppconn/prepared_statement.h>

using namespace std;
using namespace sql;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int		g_baud[] = {9600, 19200, 38400, 57600, 115200};
BYTE	g_CMD_TEST_BAUD_RATE[] = {0xFC, 0x00, 0x91, 0x07, 0x97, 0xA7, 0xD2};
BYTE	g_CMD_GET_PANID[] = {0xFC, 0x00, 0x91, 0x03, 0xA3, 0xB3, 0xE6};
BYTE	g_CMD_GET_SHORTADDR[] = {0xFC, 0x00, 0x91, 0x04, 0xC4, 0xD4, 0x29};
BYTE	g_CMD_GET_MACADDR[] = {0xFC, 0x00, 0x91, 0x08, 0xA8, 0xB8, 0xF5};
BYTE	g_CMD_GET_TYPE[] = {0xFC, 0x00, 0x91, 0x0B, 0xCB, 0xEB, 0x4E};
BYTE	g_CMD_SET_TO_COORDINATOR[] = {0xFC, 0x00, 0x91, 0x09, 0xA9, 0xC9, 0x08};
BYTE	g_CMD_SET_TO_ROUTER[] = {0xFC, 0x00, 0x91, 0x0A, 0xBA, 0xDA, 0x2B};
BYTE	g_CMD_GET_CHANNEL[] = {0xFC, 0x00, 0x91, 0x0D, 0x34, 0x2B, 0xF9};
BYTE	g_CMD_TEST_FIRMWARE[] = {0xFC, 0x00, 0x91, 0x0B, 0xCB, 0xEB, 0x4E};

BYTE	g_CMD_TEST_BAUD_RATE_DMATEK[] = {0xFC, 0x01, 0x02, 0x0A, 0x03, 0x04, 0x0A};

CRITICAL_SECTION    g_cs;

//�d�O��@��B�f
DWORD ComReadThread(LPVOID lparam)
{	
	DWORD	actualReadLen=0;	//桕���?ǻ���u�f
	DWORD	willReadLen;	
	
	DWORD dwReadErrors;
	COMSTAT	cmState;
	
	CARF24_DEMODlg *pdlg;
	pdlg = (CARF24_DEMODlg *)lparam;

	pdlg->m_Receive_Data_Len = 0;
		
	// ?�Zǲ������Ì�d�O�����˩z
	ASSERT(pdlg->m_hcom != INVALID_HANDLE_VALUE); 	
	//?�Z�d�O
	PurgeComm(pdlg->m_hcom, PURGE_RXCLEAR | PURGE_TXCLEAR );	
	SetCommMask (pdlg->m_hcom, EV_RXCHAR | EV_CTS | EV_DSR);
	try {
			sql::Driver *driver;
			sql::Connection *con;
			sql::Statement *stmt;
//			sql::ResultSet *res;
			sql::PreparedStatement *pstmt;

			/* Create a connection */
			driver = get_driver_instance();
			con = driver->connect("tcp://127.0.0.1:3306", "root", "lovelahm16");
			/* Connect to the MySQL test database */
			con->setSchema("test");

			stmt = con->createStatement();
			stmt->execute("DROP TABLE IF EXISTS SpaceInfo");
			stmt->execute("CREATE TABLE SpaceInfo(SpaceID INT, TagID char(40), Tsignal char(10), StartTime TIME, EndTime TIME)");
			stmt->execute("DROP TABLE IF EXISTS TagInfo");
			stmt->execute("CREATE TABLE TagInfo(TagID char(40), Tsignal char(10), Recieve_Time TIME)");
			delete stmt;
			/* '?' is the supported placeholder syntax */
			pstmt = con->prepareStatement("INSERT INTO SpaceInfo(SpaceID) VALUES (?)");
			for (int i = 1; i <= 6; i++) {
				pstmt->setInt(1, i);
				pstmt->executeUpdate();
			}
			delete pstmt;
			while(pdlg->m_hcom != NULL && pdlg->m_hcom != INVALID_HANDLE_VALUE){
				ClearCommError(pdlg->m_hcom,&dwReadErrors,&cmState);
				willReadLen = cmState.cbInQue ;
				if (willReadLen <= 0)
				{
					Sleep(10);
					continue;
				}			
				if(willReadLen + pdlg->m_Receive_Data_Len > MAX_RECEIVE_DATA_LEN)
					willReadLen = MAX_RECEIVE_DATA_LEN - pdlg->m_Receive_Data_Len;


				ReadFile(pdlg->m_hcom, pdlg->m_Receive_Data_Char + pdlg->m_Receive_Data_Len, willReadLen, &actualReadLen, 0);
				if (actualReadLen <= 0)
				{
					Sleep(10);
					continue;
				}
				pdlg->m_Receive_Data_Len += actualReadLen;

				EnterCriticalSection(&g_cs); 
				pdlg->ParseData(driver,con,stmt,pstmt);
				LeaveCriticalSection(&g_cs); 
		}		
	}
	catch (sql::SQLException &e) {
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	}
	cout << endl;
	return 0;
}

void CARF24_DEMODlg::ParseData(sql::Driver *driver, sql::Connection *con, sql::Statement *stmt, sql::PreparedStatement *pstmt)
{
	int start;
	bool isbreak = false;
	CString str, str_log, str_tagid, str_signal;
	
		while(!isbreak){
			switch(m_lastcmd){
				case CMD_NULL:{
					/*
					ROUTER�ͳ�ȥ�����ݸ�ʽ��                                            
					0xFF 0xFF���̶�ͷ��2Byte                                                    2
					Type���������ͣ�1Byte     = AR_DATA_TYPE_TAG_SIGNALSTRENGTH                 1
					Len�����ݳ��ȣ�1Byte      ��8+8+1+1+1+1+1 = 21                              1
					DATA0---DATA7��Router��MAC��ַ���ݣ�8 Byte                                  8
					DATA0---DATA7��Tag��MAC��ַ���ݣ�8 Byte                                     8
					DATA0���ź�ǿ�ȣ�1 Byte                                                     1
					DATA0���¶�������1 Byte                                                     1
					DATA0���¶�С����1 Byte                                                     1
					DATA0��ʪ��������1 Byte                                                     1
					DATA0��ʪ��С����1 Byte                                                     1
					CheckSum��У��ͣ���ͷ��ʼ�����ݽ�β���������ݵ�У��͡�1Byte               1
					�ܳ��ȣ�26                                                                   26
					*/
					if(m_Receive_Data_Len < 26)
						return;			//���յ����ݳ��Ȳ������ȴ��������
					//���ҿ�ʼ��־�ַ�
					start = FindChar(m_Receive_Data_Char, m_Receive_Data_Len, 0xFF, 0xFF);
					if(start < 0)
					{
						//û�п�ʼ��־�ַ�������
						m_Receive_Data_Len = 0;
						return;
					}
					if(start + 26 > m_Receive_Data_Len)
					{
						//�÷��û�н����꣬�Ƚ������ڴ���
						return;
					}
					if(m_Receive_Data_Char[start+2] != 0x01 || m_Receive_Data_Char[start+3] != 21)
					{
						//��ͷ���ԣ�����
						m_Receive_Data_Len = m_Receive_Data_Len - start - 4;
						memcpy(m_Receive_Data_Char, m_Receive_Data_Char+start+4, m_Receive_Data_Len);
						break;
					}
					//�ж�У���
					BYTE checksum = 0;
					for(int i=0; i<25; i++)
					{
						checksum += m_Receive_Data_Char[start+i];
					}
					if(checksum != m_Receive_Data_Char[start+25])
					{
						//У��Ͳ���ȷ��������Щ����
						m_Receive_Data_Len = m_Receive_Data_Len - start - 26;
						memcpy(m_Receive_Data_Char, m_Receive_Data_Char+start+26, m_Receive_Data_Len);
						break;
					}
					//��tag��Router��ȡ���� from save file
					memcpy(m_current_router, m_Receive_Data_Char+start+4, ID_LEN);
					memcpy(m_current_tag, m_Receive_Data_Char+start+4+8, ID_LEN);
					m_current_single = (char)m_Receive_Data_Char[start+20];
					m_current_T_INT = m_Receive_Data_Char[start+21];
					m_current_T_DEC = m_Receive_Data_Char[start+22];
					m_current_H_INT = m_Receive_Data_Char[start+23];
					m_current_H_DEC = m_Receive_Data_Char[start+24];

					//Ѱ�ҵ�ǰ���Tag�Ƿ��Ѿ������
					int index = -1;
					for(int i=0; i<MAX_RECEIVEINFO_COUNT; i++)
					{
						if(m_receiveinfo[i].ReceivData && memcmp(m_receiveinfo[i].TagId, m_current_tag, ID_LEN) == 0)
						{
							index = i;
							break;
						}
					}
					if(index == -1)
					{
						//���tagû���յ���
						//���ҵ�һ���յĵط������Ÿ�tag����Ϣ
						for(int i=0; i<MAX_RECEIVEINFO_COUNT; i++)
						{
							if(!m_receiveinfo[i].ReceivData)
							{
								index = i;
									IdToString(str_tagid, (char *)m_current_tag);
									char Ctagid[40] = {0};
									wcstombs(Ctagid, str_tagid, str_tagid.GetLength());
									const char * Ptagid = Ctagid;
									
									pstmt = con->prepareStatement("UPDATE SpaceInfo SET StartTime = now()WHERE IFNULL(StartTime,1) = 1 AND SpaceID = ?");
									//pstmt ->setString(1,Ptagid);
									pstmt ->setInt(1,index+1);
									pstmt ->executeUpdate();
									delete pstmt;
									
									/*IdToString(str_tagid, (char *)m_current_tag);
									char Ctagid[40] = {0};
									wcstombs(Ctagid, str_tagid, str_tagid.GetLength());
									const char * Ptagid = Ctagid;*/
									
									str_signal.Format(TEXT("%03d"), m_current_single);
									char Csignal[10] = {0};
									wcstombs(Csignal, str_signal, str_signal.GetLength());
									const char * Psignal = Csignal;
									
									pstmt = con->prepareStatement("UPDATE SpaceInfo SET EndTime = now() ,TagID = ?, Tsignal = ? where SpaceID = ? ");
									pstmt ->setString(1, Ptagid);
									pstmt ->setString(2, Psignal);
									pstmt ->setInt(3,index+1);
								//	pstmt ->setString(4,Ptagid);
									pstmt->executeUpdate();
									delete pstmt;
									
									pstmt = con->prepareStatement("INSERT INTO TagInfo(TagID, Tsignal, Recieve_Time) VALUES (?,?, now())");
									pstmt ->setString(1, Ptagid);
									pstmt ->setString(2, Psignal);
								//	pstmt ->setString(3, now());
									pstmt ->executeUpdate();
									delete pstmt;
								break;
							}
						}					
						memcpy(m_receiveinfo[index].TagId, m_current_tag, ID_LEN); //char_type
						m_receiveinfo[index].ReceivData = true;
						m_receiveinfo[index].FirstReceiveTime = GetTickCount(); 
						memcpy(m_receiveinfo[index].RouterId, m_current_router, ID_LEN);//char_type
						m_receiveinfo[index].Single = m_current_single;
						m_receiveinfo[index].T_INT = m_current_T_INT;  //Temperature
						m_receiveinfo[index].T_DEC = m_current_T_DEC;
						m_receiveinfo[index].H_INT = m_current_H_INT;  //Humidity;
						m_receiveinfo[index].H_DEC = m_current_H_DEC;
				  
					}
					else
					{
						//���Tag�Ѿ��յ������ж��ǲ��Ǹ�����Router ��ID
						if(m_current_single > m_receiveinfo[index].Single)
						{
							m_receiveinfo[index].Single = m_current_single;
							memcpy(m_receiveinfo[index].RouterId, m_current_router, ID_LEN);
						}
					}
					//���ҵ�ǰ���յ��źŵ�Router�ǲ���4��ָ��Router�е�һ��
					for(int i=0; i<MAX_ROUTER_COUNT; i++)
					{
						if(memcmp(m_saveinfo.router[i].ID, m_current_router, ID_LEN) == 0)
						{
							m_receiveinfo[index].SingleToRouter[i] = m_current_single;
						}
					}
					//�ҠZ�U�����ýg?ǻ���D�f��
					m_Receive_Data_Len = m_Receive_Data_Len - start - 26;
					memcpy(m_Receive_Data_Char, m_Receive_Data_Char+start+26, m_Receive_Data_Len);
					//���P?��ǻ�f�ސ�����Log�`����
					CTime t = CTime::GetCurrentTime();
					str.Format(TEXT("   %02d:%02d:%02d  "), t.GetHour(), t.GetMinute(), t.GetSecond());
					str_log = str;
			
					IdToString(str, (char *)m_current_router);
					str_log += str + TEXT("  ");

					IdToString(str, (char *)m_current_tag);
					str_log += str + TEXT("  ");

					str.Format(TEXT("%03d"), m_current_single);
					str_log += str + TEXT("\r\n");

					DWORD write = str_log.GetLength();
					char s[128];
					wcstombs(s, str_log, str_log.GetLength());
					WriteFile(m_hfile_log, s, write, &write, NULL);
					
				}
				break;	
			case CMD_TEST_BAUD:
				{
					if(m_Receive_Data_Len < 5)
						return;			//Ն��ǻ�f����H�����Û��Ն���U��
					if(m_Receive_Data_Char[0] == 0x01 && m_Receive_Data_Char[1] == 0x02 &&
						m_Receive_Data_Char[2] == 0x03 && m_Receive_Data_Char[3] == 0x04 &&
						m_Receive_Data_Char[4] == 0x05)
					{
						m_test_baudrate_ok = true;
					}
					m_lastcmd = CMD_NULL;
					//��ýҠZ��ǻ�f��
					m_Receive_Data_Len = m_Receive_Data_Len - 5;
					memcpy(m_Receive_Data_Char, m_Receive_Data_Char+5, m_Receive_Data_Len);
				}
				break;	
			case CMD_TEST_BAUD_DMATEK:
				{
					if(m_Receive_Data_Len < 3)
						return;			//Ն��ǻ�f����H�����Û��Ն���U��
					if(m_Receive_Data_Char[0] == 0x03 && m_Receive_Data_Char[1] == 0x04 &&
						m_Receive_Data_Char[2] == 0x0A)
					{
						m_test_baudrate_ok = true;
					}
					m_lastcmd = CMD_NULL;
					//��ýҠZ��ǻ�f��
					m_Receive_Data_Len = m_Receive_Data_Len - 3;
					memcpy(m_Receive_Data_Char, m_Receive_Data_Char+3, m_Receive_Data_Len);
				}
				break;	
			}
		}						
}

int CARF24_DEMODlg::FindChar(BYTE *str, int strlen, BYTE c1, BYTE c2)
{
	for(int i=0; i<strlen-1; i++)
	{
		if(str[i] == c1 && str[i+1] == c2)
			return i;
	}
	return -1;
}

// �[�����[��t���������ɵȳXǻ CAboutDlg ���s�z

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// ���s�z�f��
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �E��

// 桫�
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CARF24_DEMODlg ���s�z




CARF24_DEMODlg::CARF24_DEMODlg(CWnd* pParent /*=NULL*/)
	: CDialog(CARF24_DEMODlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CARF24_DEMODlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_CONNCET, m_button_connect);
	DDX_Control(pDX, IDC_PICTURE_CONNECT, m_picture_connect);
	DDX_Control(pDX, IDC_PICTURE_DISCONNECT, m_picture_disconnect);
	DDX_Control(pDX, IDC_BUTTON_SET_TAG, m_button_set_tag);
	DDX_Control(pDX, IDC_BUTTON_SET_ROUTER, m_button_set_router);
	DDX_Control(pDX, IDC_LIST_INFO, m_list_info);
	DDX_Control(pDX, IDC_STATIC_COM, m_static_com);
	DDX_Control(pDX, IDC_STATIC_BAUD, m_static_baud);
	DDX_Control(pDX, IDC_COMBO_COM, m_combo_com);
	DDX_Control(pDX, IDC_COMBO_BAUD, m_combo_baud);
	DDX_Control(pDX, IDC_BUTTON_AUTO_CONNECT, m_button_autoconnect);
	DDX_Control(pDX, IDC_BUTTON_CLEAN, m_button_clean);
}

BEGIN_MESSAGE_MAP(CARF24_DEMODlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SET_TAG, &CARF24_DEMODlg::OnBnClickedButtonSetTag)
	ON_BN_CLICKED(IDC_BUTTON_SET_ROUTER, &CARF24_DEMODlg::OnBnClickedButtonSetRouter)
	ON_BN_CLICKED(IDC_BUTTON_CONNCET, &CARF24_DEMODlg::OnBnClickedButtonConncet)
	ON_BN_CLICKED(IDC_BUTTON_AUTO_CONNECT, &CARF24_DEMODlg::OnBnClickedButtonAutoConnect)
	ON_BN_CLICKED(IDC_BUTTON_CLEAN, &CARF24_DEMODlg::OnBnClickedButtonClean)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CARF24_DEMODlg �m���ҠZ��t

BOOL CARF24_DEMODlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ε������...���ɵȳX�ߘ��ƞ��f�ɵȸ��z

	// IDM_ABOUTBOX ���|����f�w�ع�[�ʩz
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// �M��ɭ���s�zǻ�Z���z�g���[��t�����O������s�z�C���z��ε�`��
	//  ���kɭ���F
	SetIcon(m_hIcon, TRUE);			// �M�����Z��
	SetIcon(m_hIcon, FALSE);		// �M�����Z��

	// TODO: ��ɭ�ߘǉT�Xǻ���a�w�y�u

	MoveWindow(0, 0, APP_WIDTH, APP_HEIGHT);

	m_button_connect.MoveWindow(BUTTON_CONNECT_LEFT, BUTTON_CONNECT_TOP, BUTTON_CONNECT_WIDTH, BUTTON_CONNECT_HEIGHT);
	m_picture_connect.MoveWindow(PICTURE_CONNECT_LEFT, PICTURE_CONNECT_TOP, PICTURE_CONNECT_WIDTH, PICTURE_CONNECT_HEIGHT);
	m_picture_disconnect.MoveWindow(PICTURE_DISCONNECT_LEFT, PICTURE_DISCONNECT_TOP, PICTURE_DISCONNECT_WIDTH, PICTURE_DISCONNECT_HEIGHT);
	m_button_set_tag.MoveWindow(BUTTON_SETTAG_LEFT, BUTTON_SETTAG_TOP, BUTTON_SETTAG_WIDTH, BUTTON_SETTAG_HEIGHT);
	m_button_set_router.MoveWindow(BUTTON_SETROUTER_LEFT, BUTTON_SETROUTER_TOP, BUTTON_SETROUTER_WIDTH, BUTTON_SETROUTER_HEIGHT);
	m_static_baud.MoveWindow(STATIC_BAUD_LEFT, STATIC_BAUD_TOP, STATIC_BAUD_WIDTH, STATIC_BAUD_HEIGHT);
	m_static_com.MoveWindow(STATIC_COM_LEFT, STATIC_COM_TOP, STATIC_COM_WIDTH, STATIC_COM_HEIGHT);
	m_combo_com.MoveWindow(COMBO_COM_LEFT, COMBO_COM_TOP, COMBO_COM_WIDTH, COMBO_COM_HEIGHT);
	m_combo_baud.MoveWindow(COMBO_BAUD_LEFT, COMBO_BAUD_TOP, COMBO_BAUD_WIDTH, COMBO_BAUD_HEIGHT);
	m_button_autoconnect.MoveWindow(BUTTON_AUTOCONNECT_LEFT, BUTTON_AUTOCONNECT_TOP, BUTTON_AUTOCONNECT_WIDTH, BUTTON_AUTOCONNECT_HEIGHT);
	m_button_clean.MoveWindow(BUTTON_CLEAN_LEFT, BUTTON_CLEAN_TOP, BUTTON_CLEAN_WIDTH, BUTTON_CLEAN_HEIGHT);


	DWORD dwStyle = m_list_info.GetExtendedStyle();
	m_list_info.SetExtendedStyle(dwStyle | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
	m_list_info.InsertColumn(1,TEXT("TAG ID"), LVCFMT_CENTER, 120);
	m_list_info.InsertColumn(2,TEXT("TAG Name"), LVCFMT_CENTER, 100);
	m_list_info.InsertColumn(3,TEXT("Router ID"), LVCFMT_CENTER, 120);
	m_list_info.InsertColumn(4,TEXT("Router Name"), LVCFMT_CENTER, 100);	
	m_list_info.InsertColumn(5,TEXT("Signal"), LVCFMT_CENTER, 60);
	m_list_info.InsertColumn(6,TEXT("Temperature"), LVCFMT_CENTER, 100);
	m_list_info.InsertColumn(7,TEXT("Humidity"), LVCFMT_CENTER, 100);
	m_list_info.InsertColumn(8,TEXT("Time"), LVCFMT_CENTER, 120);

	m_combo_com.SetCurSel(0);
	m_combo_baud.SetCurSel(0);
	
	m_list_info.MoveWindow(LIST_INFO_LEFT, LIST_INFO_TOP, LIST_INFO_WIDTH, LIST_INFO_HEIGHT);

	InitializeCriticalSection(&g_cs); 
	m_bconnect = false;
	m_lastcmd = CMD_NULL;

	m_hdc = ::GetDC(this->GetSafeHwnd());
	m_mdc = CreateCompatibleDC(m_hdc);
	m_router_dc = CreateCompatibleDC(m_hdc);
	m_mbmp = CreateCompatibleBitmap(m_hdc, DRAW_WIDTH, DRAW_HEIGHT);
	m_routerbmp = CreateCompatibleBitmap(m_hdc, BMP_ROUTER_WIDTH, BMP_ROUTER_HEIGHT);
	HDC ndc = CreateCompatibleDC(m_hdc);
	CBitmap bmp;
	bmp.LoadBitmapW(IDB_BITMAP_ROUTER);
	SelectObject(ndc, bmp);
	SelectObject(m_router_dc, m_routerbmp);
	SelectObject(m_mdc, m_mbmp);
	BitBlt(m_router_dc, 0, 0, BMP_ROUTER_WIDTH, BMP_ROUTER_HEIGHT, ndc, 0, 0, SRCCOPY);

	m_Brush_white = ::CreateSolidBrush(RGB(255, 255, 255));
	m_Brush_Blue = ::CreateSolidBrush(RGB(0, 0, 255));

	GetTagRouterInfo();
	
	srand(GetTickCount());

	m_current_listitem_count = 0;

	return TRUE;  // �؜�ε���ǒM�����S���۴�u��϶ TRUE
}

void CARF24_DEMODlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ?���N���s�z�ߘ��I���wټ �یu�z���B�wǻ�y�u
//  ���͘�Z���z�����p�[�`��/�y�Zҫ��ǻ MFC ���[��t��
//  ��ε���z���`�ۂU��z

void CARF24_DEMODlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �[���ǻ�M��f�B�`

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// �p�Z�����W�F�邈��и��
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ��Z��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();

		Draw();
	}
}

//�g�[�cބ���I���w���O�C���f���[ɭ�B�f?�������}���z
//
HCURSOR CARF24_DEMODlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CARF24_DEMODlg::OnBnClickedButtonSetTag()
{
	// TODO: Add your control notification handler code here
	SetTag cst;
	cst.DoModal();
}

void CARF24_DEMODlg::OnBnClickedButtonSetRouter()
{
	// TODO: Add your control notification handler code here
	CSetRouter sr;
	sr.DoModal();
}

VOID CARF24_DEMODlg::GetModulePath(LPTSTR path, LPCTSTR module)
{
	TCHAR* s;
	HANDLE Handle = NULL;
	if(module)
		Handle = GetModuleHandle(module);
	GetModuleFileName((HMODULE)Handle, path, MAX_PATH);
	s = _tcsrchr(path, '\\');
	if(s) s[1] = 0;
}

BOOL CARF24_DEMODlg::GetTagRouterInfo()
{
	WCHAR szPath[MAX_PATH] = {0};
	GetModulePath(szPath, NULL);
	CString filepath;
	filepath = szPath;
	filepath = filepath + SAVE_FILE_NAME;
	m_hfile = CreateFile(filepath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);   
  	if(m_hfile == INVALID_HANDLE_VALUE)   
	{   
		//�`�����Պ��쳱�`��
		m_hfile = CreateFile(filepath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,NULL);   
  		if(m_hfile == INVALID_HANDLE_VALUE)   
		{   
			//쳱�`�����?
			MessageBox(TEXT("Failed to create file !"));
			return FALSE;  
		}
		memset(&m_saveinfo, 0, sizeof(m_saveinfo));
		DWORD write;
		WriteFile(m_hfile, &m_saveinfo, sizeof(m_saveinfo), &write, NULL);
	} 
	else
	{
		//�`���Պ����?�f��
		DWORD Read;
		ReadFile(m_hfile, &m_saveinfo, sizeof(m_saveinfo), &Read, NULL);		
	}
	CloseHandle(m_hfile);
	
	
	memset(m_receiveinfo, 0, sizeof(m_receiveinfo));
	m_current_receive_count = 0;
	
	return TRUE;


}

void CARF24_DEMODlg::OpenLogFile()
{
	WCHAR szPath[MAX_PATH] = {0};
	GetModulePath(szPath, NULL);
	CString filepath;
	filepath = szPath;
	filepath = filepath + LOG_FILE_NAME;
	m_hfile_log = CreateFile(filepath,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);   
  	if(m_hfile_log == INVALID_HANDLE_VALUE)   
	{   		
		//쳱�`�����?
		MessageBox(TEXT("Failed to create Log file !"));
		return ;  		
	} 
}

bool CARF24_DEMODlg::OpenCom(int com)
{
	CString str;
	str.Format(TEXT("COM%d"), com);
	return OpenCom(str);
}

bool CARF24_DEMODlg::OpenCom(CString str_com)
{	
	str_com = TEXT("\\\\.\\") + str_com;
	m_hcom = CreateFile(str_com, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	///���d�O
	if(m_hcom != INVALID_HANDLE_VALUE && m_hcom != NULL)
	{		
		DCB  dcb;    
		dcb.DCBlength = sizeof(DCB); 
		// �K?�d�O�y�f
		GetCommState(m_hcom, &dcb);

		dcb.BaudRate = CBR_115200;					// �M���薃޷ 
		dcb.fBinary = TRUE;						// �M�닋�v��ҫ�d��ɭ�����|�M��TRUE
		dcb.fParity = TRUE;						// �E��?�vƀ�� 
		dcb.ByteSize = 8;						// �f�ޏm,ع�[:4-8 
		dcb.Parity = NOPARITY;					// ƀ��ҫ�d
		dcb.StopBits = ONESTOPBIT;				// �j�Ώm 0,1,2 = 1, 1.5, 2

		dcb.fOutxCtsFlow = FALSE;				// No CTS output flow control 
		dcb.fOutxDsrFlow = FALSE;				// No DSR output flow control 
		dcb.fDtrControl = DTR_CONTROL_ENABLE; 
		// DTR flow control type 
		dcb.fDsrSensitivity = FALSE;			// DSR sensitivity 
		dcb.fTXContinueOnXoff = TRUE;			// XOFF continues Tx 
		dcb.fOutX = FALSE;					// No XON/XOFF out flow control 
		dcb.fInX = FALSE;						// No XON/XOFF in flow control 
		dcb.fErrorChar = FALSE;				// Disable error replacement 
		dcb.fNull = FALSE;					// Disable null stripping 
		dcb.fRtsControl = RTS_CONTROL_ENABLE; 
		// RTS flow control 
		dcb.fAbortOnError = FALSE;			// �g�d�O�������d�����鸙�Γd�O��ދ


		if (!SetCommState(m_hcom, &dcb))
		{
			///L"���d�O���?;			
			return false;
		}
		////����]�C�t
		COMMTIMEOUTS  cto;
		GetCommTimeouts(m_hcom, &cto);
		cto.ReadIntervalTimeout = MAXDWORD;  
		cto.ReadTotalTimeoutMultiplier = 10;  
		cto.ReadTotalTimeoutConstant = 10;    
		cto.WriteTotalTimeoutMultiplier = 50;  
		cto.WriteTotalTimeoutConstant = 100;    
		if (!SetCommTimeouts(m_hcom, &cto))
		{
			///L"����M���]�C�y�f";		
			return false;
		}	

		//������O����ǻ����?
		SetCommMask (m_hcom, EV_RXCHAR);
		
		//���M��ǲ��?
	//	SetupComm(m_hcom,8192,8192);

		//���a�wǲ��?��ǻ�|��
		PurgeComm(m_hcom,PURGE_TXCLEAR|PURGE_RXCLEAR);
	}
	else
	{		
		return false;
	}
	return true;
}


bool CARF24_DEMODlg::SetBaudRate(int baudrate)
{
	if(m_hcom != INVALID_HANDLE_VALUE && m_hcom != NULL)
	{		
		DCB  dcb;    
		dcb.DCBlength = sizeof(DCB); 
		// �K?�d�O�y�f
		GetCommState(m_hcom, &dcb);

		dcb.BaudRate = baudrate;

		if (!SetCommState(m_hcom, &dcb))
		{
			///L"���d�O���?;			
			return false;
		}
		return true;
	}
	return false;
}


void CARF24_DEMODlg::OnBnClickedButtonConncet()
{
	// TODO: Add your control notification handler code here
	if(m_bconnect)
	{
		m_button_connect.SetWindowTextW(TEXT("Connect Module"));
		m_button_autoconnect.SetWindowTextW(TEXT("Auto Connect"));
		m_picture_connect.ShowWindow(SW_HIDE);
		m_picture_disconnect.ShowWindow(SW_SHOW);
		KillTimer(1);
		CloseHandle(m_hcom);
		m_bconnect = false;
		CloseHandle(m_hfile_log);
	}
	else
	{
		GetTagRouterInfo();
		CString str_com;
		m_combo_com.GetWindowTextW(str_com);
		if(!OpenCom(str_com))
		{
			MessageBox(TEXT("Open ") + str_com + TEXT(" Fail !"));
			return;
		}
		int baud;
		baud = m_combo_baud.GetCurSel();
		SetBaudRate(g_baud[baud]);

		OpenLogFile();

		HANDLE m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ComReadThread, this, 0, NULL);
		CloseHandle(m_hThread);

		m_picture_connect.ShowWindow(SW_SHOW);
		m_picture_disconnect.ShowWindow(SW_HIDE);

		m_button_connect.SetWindowTextW(TEXT("DisConnect"));
		m_button_autoconnect.SetWindowTextW(TEXT("DisConnect"));

		SetTimer(1, 1000, NULL);
		OnBnClickedButtonClean();

		m_bconnect = true;
	}
}

void CARF24_DEMODlg::OnBnClickedButtonAutoConnect()
{
	// TODO: Add your control notification handler code here
	if(m_bconnect)
	{
		m_button_connect.SetWindowTextW(TEXT("Connect Module"));
		m_button_autoconnect.SetWindowTextW(TEXT("Auto Connect"));
		m_picture_connect.ShowWindow(SW_HIDE);
		m_picture_disconnect.ShowWindow(SW_SHOW);
		KillTimer(1);
		CloseHandle(m_hcom);
		m_bconnect = false;	
		CloseHandle(m_hfile_log);
	}
	else
	{
		GetTagRouterInfo();

		for(int m_com_index = 1; m_com_index<100; m_com_index++)
		{
			if(!OpenCom(m_com_index))
			{
				continue;
			}	
			HANDLE m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ComReadThread, this, 0, NULL);
			CloseHandle(m_hThread);
			for(int i=0; i<5; i++)
			{
				if(!SetBaudRate(g_baud[i]))
				{
					CString str;
					str.Format(TEXT("%d"), g_baud[i]);
					str = TEXT("Set Baud Rate:")+str+TEXT(" Fail!");
					MessageBox(str, NULL, MB_OK|MB_ICONHAND );
					return;
				}
				m_test_baudrate_ok = false;
				m_Receive_Data_Len = 0;
				m_lastcmd = CMD_TEST_BAUD;
				DWORD write;
				WriteFile(m_hcom, g_CMD_TEST_BAUD_RATE, sizeof(g_CMD_TEST_BAUD_RATE), &write, NULL);
				Sleep(100);
				if(m_test_baudrate_ok)
				{
					OpenLogFile();

					CString str;
					str.Format(TEXT("COM%d"), m_com_index);				
					m_combo_com.SetWindowTextW(str);

					m_combo_baud.SetCurSel(i);
					
					m_picture_connect.ShowWindow(SW_SHOW);
					m_picture_disconnect.ShowWindow(SW_HIDE);

					m_button_connect.SetWindowTextW(TEXT("DisConnect"));
					m_button_autoconnect.SetWindowTextW(TEXT("DisConnect"));

					SetTimer(1, 1000, NULL);
					OnBnClickedButtonClean();
					
					m_bconnect = true;
					return;
				}
				else
				{
					m_test_baudrate_ok = false;
					m_Receive_Data_Len = 0;
					m_lastcmd = CMD_TEST_BAUD_DMATEK;					
					WriteFile(m_hcom, g_CMD_TEST_BAUD_RATE_DMATEK, sizeof(g_CMD_TEST_BAUD_RATE_DMATEK), &write, NULL);
					Sleep(100);
					if(m_test_baudrate_ok)
					{
						OpenLogFile();

						CString str;
						str.Format(TEXT("COM%d"), m_com_index);				
						m_combo_com.SetWindowTextW(str);

						m_combo_baud.SetCurSel(i);
						
						m_picture_connect.ShowWindow(SW_SHOW);
						m_picture_disconnect.ShowWindow(SW_HIDE);

						m_button_connect.SetWindowTextW(TEXT("DisConnect"));
						m_button_autoconnect.SetWindowTextW(TEXT("DisConnect"));

						SetTimer(1, 1000, NULL);
						OnBnClickedButtonClean();
						
						m_bconnect = true;
						return;
					}
				}
			}
			CloseHandle(m_hcom);
			Sleep(10);
			
		}
		MessageBox(TEXT("Connect Zigbee Module Fail!"), NULL, MB_OK|MB_ICONEXCLAMATION );
		CloseHandle(m_hcom);
	}
}

void CARF24_DEMODlg::OnBnClickedButtonClean()
{
	// TODO: Add your control notification handler code here
	while(m_list_info.GetItemCount())
		m_list_info.DeleteItem(0);

	m_current_listitem_count = 0;
	Draw();
}

void CARF24_DEMODlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	EnterCriticalSection(&g_cs); 
	if(nIDEvent == 1)
	{
		int index_list;
		bool updatadraw = false;
		for(int i=0; i<MAX_RECEIVEINFO_COUNT; i++)
		{
			if(!m_receiveinfo[i].ReceivData)
				continue;
			if(GetTickCount() - m_receiveinfo[i].FirstReceiveTime < 1000)
				continue;

			index_list = FindTagIdInList(m_receiveinfo[i].TagId);
			if(index_list < 0)
			{
				//�ߘǂk			
				memcpy(m_listshowinfo[m_current_listitem_count].TagId, m_receiveinfo[i].TagId, ID_LEN);
				m_listshowinfo[m_current_listitem_count].index = m_current_listitem_count;
				index_list = m_current_listitem_count++;
				
				//�ߘ�tag ID
				CString str;
				IdToString(str, m_receiveinfo[i].TagId);
				m_list_info.InsertItem(index_list, str);
				
				//�ߘ�tag Name
				int index = FindTagIdInSave(m_receiveinfo[i].TagId);
				if(index != -1)
				{
					str = m_saveinfo.tag[index].Name;
					m_list_info.SetItemText(index_list, 1, str);
				}
			}
			else
			{
				
			}
			//�d�ۂk�|��
			//����Router ID
			CString str;
			IdToString(str, m_receiveinfo[i].RouterId);
			m_list_info.SetItemText(index_list, 2, str);

			//����Router Name
			//�ߘ�tag Name
			int index = FindRouterIdInSave(m_receiveinfo[i].RouterId);
			if(index != -1)
			{
				str = m_saveinfo.router[index].Name;
				m_list_info.SetItemText(index_list, 3, str);
			}
			else
			{
				m_list_info.SetItemText(index_list, 3, TEXT(""));
			}
			
			//�����|��
			str.Format(TEXT("%d"), m_receiveinfo[i].Single);
			m_list_info.SetItemText(index_list, 4, str);

			//���ېy�H
			str.Format(TEXT("%d.%d"), m_receiveinfo[i].T_INT, m_receiveinfo[i].T_DEC);
			m_list_info.SetItemText(index_list, 5, str);

			//���ۈ^�H
			str.Format(TEXT("%d.%d"), m_receiveinfo[i].H_INT, m_receiveinfo[i].H_DEC);
			m_list_info.SetItemText(index_list, 6, str);

			CTime t = CTime::GetCurrentTime();
			str.Format(TEXT("%02d:%02d:%02d"), t.GetHour(), t.GetMinute(), t.GetSecond());
			m_list_info.SetItemText(index_list, 7, str);
			
			memcpy(&m_listshowinfo[index_list].reciveinfo, &m_receiveinfo[i], sizeof(ReceiveInfo));
			
			//����mǻ�|�������Ɛ`��
			CString str_log;
			str.Format(TEXT("*  %02d:%02d:%02d  "), t.GetHour(), t.GetMinute(), t.GetSecond());
			str_log = str;

			IdToString(str, (char *)m_receiveinfo[i].RouterId);
			str_log += str + TEXT("  ");

			IdToString(str, (char *)m_receiveinfo[i].TagId);
			str_log += str + TEXT("  ");

			str.Format(TEXT("%03d"), m_receiveinfo[i].Single);
			str_log += str + TEXT("\r\n");

			DWORD write = str_log.GetLength();
			char s[128];
			wcstombs(s, str_log, str_log.GetLength());
			WriteFile(m_hfile_log, s, write, &write, NULL);

			//�ҠZ�U��?�������m��
			memset(&m_receiveinfo[i], 0, sizeof(ReceiveInfo));			
			updatadraw = true;
		}
		if(updatadraw)
			Draw();
	}
	LeaveCriticalSection(&g_cs); 	

	CDialog::OnTimer(nIDEvent);
}

int CARF24_DEMODlg::FindTagIdInList(char *id)
{
	for(int i=0; i<m_current_listitem_count; i++)
	{
		if(memcmp(m_listshowinfo[i].TagId, id, ID_LEN) == 0)
			return i;
	}
	return -1;
}

int CARF24_DEMODlg::FindTagIdInSave(char *id)
{
	for(int i=0; i<MAX_TAG_COUNT; i++)
	{
		if(memcmp(m_saveinfo.tag[i].ID, id, ID_LEN) == 0)
			return i;
	}
	return -1;
}

int CARF24_DEMODlg::FindRouterIdInSave(char *id)
{
	for(int i=0; i<MAX_ROUTER_COUNT; i++)
	{
		if(memcmp(m_saveinfo.router[i].ID, id, ID_LEN) == 0)
			return i;
	}
	return -1;
}

/*void CARF24_DEMODlg::IdToChar(const char *p ,char *byte){
	CString Cstr;
	char c[40] = {0};
	IdToString(Cstr, byte);
	wcstombs(c, Cstr, Cstr.GetLength());
	p = c;
}*/
void CARF24_DEMODlg::IdToString(CString &str, char *id)
{
	CString s;
	str = TEXT("");
	for(int i=0; i<8; i++)
	{
		s.Format(TEXT("%02X"), (BYTE)id[i]);
		str += s;
	}
}

void CARF24_DEMODlg::Draw()
{
	//�����d�z
	int x, y;

	RECT r;
	r.left = 0;
	r.top = 0;
	r.right = DRAW_WIDTH;
	r.bottom = DRAW_HEIGHT;

	CRect cr;

	::FillRect(m_mdc, &r, m_Brush_white);
	::MoveToEx(m_mdc, 0, 0, NULL);
	::LineTo(m_mdc, DRAW_WIDTH-1, 0);
	::LineTo(m_mdc, DRAW_WIDTH-1, DRAW_HEIGHT-1);
	::LineTo(m_mdc, 0, DRAW_HEIGHT-1);
	::LineTo(m_mdc, 0, 0);

	::MoveToEx(m_mdc, 0, DRAW_HEIGHT/2, NULL);
	::LineTo(m_mdc, DRAW_WIDTH/2*3/4, DRAW_HEIGHT/2);

	::MoveToEx(m_mdc, DRAW_WIDTH, DRAW_HEIGHT/2, NULL);
	::LineTo(m_mdc, DRAW_WIDTH - DRAW_WIDTH/2*3/4, DRAW_HEIGHT/2);

	::MoveToEx(m_mdc, DRAW_WIDTH/2, 0, NULL);
	::LineTo(m_mdc, DRAW_WIDTH/2, DRAW_HEIGHT/2*3/4);

	::MoveToEx(m_mdc, DRAW_WIDTH/2, DRAW_HEIGHT, NULL);
	::LineTo(m_mdc, DRAW_WIDTH/2, DRAW_HEIGHT - DRAW_HEIGHT/2*3/4);
	
	//�dRouter
	BitBlt(m_mdc, 1, 1, BMP_ROUTER_WIDTH, BMP_ROUTER_HEIGHT, m_router_dc, 0, 0, SRCCOPY);
	BitBlt(m_mdc, DRAW_WIDTH-1-BMP_ROUTER_WIDTH, 1, BMP_ROUTER_WIDTH, BMP_ROUTER_HEIGHT, m_router_dc, 0, 0, SRCCOPY);
	BitBlt(m_mdc, DRAW_WIDTH-1-BMP_ROUTER_WIDTH, DRAW_HEIGHT-1-BMP_ROUTER_HEIGHT, BMP_ROUTER_WIDTH, BMP_ROUTER_HEIGHT, m_router_dc, 0, 0, SRCCOPY);
	BitBlt(m_mdc, 1, DRAW_HEIGHT-1-BMP_ROUTER_HEIGHT, BMP_ROUTER_WIDTH, BMP_ROUTER_HEIGHT, m_router_dc, 0, 0, SRCCOPY);

	//��Routerދ�fName
	CString str;	
	SetTextColor(m_mdc, RGB(255, 0, 0));
	SetBkMode(m_mdc, TRANSPARENT);

	str = m_saveinfo.router[0].Name;
	if(str.IsEmpty())
		IdToString(str, m_saveinfo.router[0].ID);
	cr.SetRect(1, 1, BMP_ROUTER_WIDTH+1, 1+BMP_ROUTER_HEIGHT*3/4);
	::DrawText(m_mdc, str, -1, &cr, DT_CENTER|DT_BOTTOM|DT_SINGLELINE);

	str = m_saveinfo.router[1].Name;
	if(str.IsEmpty())
		IdToString(str, m_saveinfo.router[1].ID);
	cr.SetRect(DRAW_WIDTH-1-BMP_ROUTER_WIDTH, 1, DRAW_WIDTH-1, 1+BMP_ROUTER_HEIGHT*3/4);
	::DrawText(m_mdc, str, -1, &cr, DT_CENTER|DT_BOTTOM|DT_SINGLELINE);

	str = m_saveinfo.router[2].Name;
	if(str.IsEmpty())
		IdToString(str, m_saveinfo.router[2].ID);
	cr.SetRect(DRAW_WIDTH-1-BMP_ROUTER_WIDTH, DRAW_HEIGHT-1-BMP_ROUTER_HEIGHT, DRAW_WIDTH-1, DRAW_HEIGHT-1-BMP_ROUTER_HEIGHT/4);
	::DrawText(m_mdc, str, -1, &cr, DT_CENTER|DT_BOTTOM|DT_SINGLELINE);

	str = m_saveinfo.router[3].Name;
	if(str.IsEmpty())
		IdToString(str, m_saveinfo.router[3].ID);
	cr.SetRect(1, DRAW_HEIGHT-1-BMP_ROUTER_HEIGHT, BMP_ROUTER_WIDTH, DRAW_HEIGHT-1-BMP_ROUTER_HEIGHT/4);
	::DrawText(m_mdc, str, -1, &cr, DT_CENTER|DT_BOTTOM|DT_SINGLELINE);

	//�dTag��
	int router_index, tag_index;
	for(int i=0; i<m_current_listitem_count; i++)
	{
		if(GetTickCount() - m_listshowinfo[i].reciveinfo.FirstReceiveTime > 10*1000)
			continue;	//��C������d��f��ǻ�����d
		
		//���˽g?���Tag��ީ݆ǻRouter�ی�������ǻ��Ͱ���ۊ���d�Z���������d�Z
		router_index = FindRouterIdInSave(m_listshowinfo[i].reciveinfo.RouterId);
		if(router_index == -1)
		{
			//�������d�Z
			continue;
		}
		else
		{
			SelectObject(m_mdc, m_Brush_Blue);
			//����d�Z
			//����Tag��ީ݆�U����Router
			switch(router_index)
			{
			case 0:		//�H�f��
				{
					if(m_listshowinfo[i].LastDrawSingle == m_listshowinfo[i].reciveinfo.Single)
					{
						//�|��?�H�񰹨����d���Z���m��ۊp�[�f��ǻ���N��
						x = m_listshowinfo[i].LastDrawPoint.x;
						y = m_listshowinfo[i].LastDrawPoint.y;
					}
					else
					{					
						//��z���᫔���N��
						x = rand()%(m_listshowinfo[i].reciveinfo.Single);
						y = (int)sqrt((double)(m_listshowinfo[i].reciveinfo.Single * m_listshowinfo[i].reciveinfo.Single - x*x));
						
						x += BMP_ROUTER_WIDTH;
						y += BMP_ROUTER_HEIGHT;
					}
					Ellipse(m_mdc, x-DRAW_RADIUS, y-DRAW_RADIUS, x+DRAW_RADIUS, y+DRAW_RADIUS);
					cr.SetRect(x+DRAW_RADIUS, y-DRAW_RADIUS, x+DRAW_RADIUS+DRAW_TEXT_WIDTH, y-DRAW_RADIUS+DRAW_TEXT_HEIGHT);
					//���b͘�U?�������ǻ�U?��
					tag_index = FindTagIdInSave(m_listshowinfo[i].reciveinfo.TagId);
					if(tag_index == -1)
					{
						IdToString(str, m_listshowinfo[i].reciveinfo.TagId); 
					}
					else
					{
						str = m_saveinfo.tag[tag_index].Name;
						if(str.IsEmpty())
							IdToString(str, m_saveinfo.tag[tag_index].ID);
					}					
					::DrawText(m_mdc, str, -1, &cr, DT_VCENTER|DT_SINGLELINE);
				}
				break;
			case 1:		//�z�f��
				{
					if(m_listshowinfo[i].LastDrawSingle == m_listshowinfo[i].reciveinfo.Single)
					{
						//�|��?�H�񰹨����d���Z���m��ۊp�[�f��ǻ���N��
						x = m_listshowinfo[i].LastDrawPoint.x;
						y = m_listshowinfo[i].LastDrawPoint.y;
					}
					else
					{					
						//��z���᫔���N��
						x = rand()%(m_listshowinfo[i].reciveinfo.Single);
						y = (int)sqrt((double)(m_listshowinfo[i].reciveinfo.Single * m_listshowinfo[i].reciveinfo.Single - x*x));
						
						x = DRAW_WIDTH-BMP_ROUTER_WIDTH-x;					
						y += BMP_ROUTER_HEIGHT;
					}

					Ellipse(m_mdc, x-DRAW_RADIUS, y-DRAW_RADIUS, x+DRAW_RADIUS, y+DRAW_RADIUS);
					cr.SetRect(x-DRAW_RADIUS-DRAW_TEXT_WIDTH, y-DRAW_RADIUS, x-DRAW_RADIUS, y-DRAW_RADIUS+DRAW_TEXT_HEIGHT);
					//���b͘�U?�������ǻ�U?��
					tag_index = FindTagIdInSave(m_listshowinfo[i].reciveinfo.TagId);
					if(tag_index == -1)
					{
						IdToString(str, m_listshowinfo[i].reciveinfo.TagId); 
					}
					else
					{
						str = m_saveinfo.tag[tag_index].Name;
						if(str.IsEmpty())
							IdToString(str, m_saveinfo.tag[tag_index].ID);
					}					
					::DrawText(m_mdc, str, -1, &cr, DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
				}
				break;
			case 2:		//�z�B��
				{
					if(m_listshowinfo[i].LastDrawSingle == m_listshowinfo[i].reciveinfo.Single)
					{
						//�|��?�H�񰹨����d���Z���m��ۊp�[�f��ǻ���N��
						x = m_listshowinfo[i].LastDrawPoint.x;
						y = m_listshowinfo[i].LastDrawPoint.y;
					}
					else
					{					
						//��z���᫔���N��
						x = rand()%(m_listshowinfo[i].reciveinfo.Single);
						y = (int)sqrt((double)(m_listshowinfo[i].reciveinfo.Single * m_listshowinfo[i].reciveinfo.Single - x*x));
						
						x = DRAW_WIDTH-BMP_ROUTER_WIDTH-x;					
						y = DRAW_HEIGHT-BMP_ROUTER_HEIGHT-y;
					}

					Ellipse(m_mdc, x-DRAW_RADIUS, y-DRAW_RADIUS, x+DRAW_RADIUS, y+DRAW_RADIUS);
					cr.SetRect(x-DRAW_RADIUS-DRAW_TEXT_WIDTH, y-DRAW_RADIUS, x-DRAW_RADIUS, y-DRAW_RADIUS+DRAW_TEXT_HEIGHT);
					//���b͘�U?�������ǻ�U?��
					tag_index = FindTagIdInSave(m_listshowinfo[i].reciveinfo.TagId);
					if(tag_index == -1)
					{
						IdToString(str, m_listshowinfo[i].reciveinfo.TagId); 
					}
					else
					{
						str = m_saveinfo.tag[tag_index].Name;
						if(str.IsEmpty())
							IdToString(str, m_saveinfo.tag[tag_index].ID);
					}					
					::DrawText(m_mdc, str, -1, &cr, DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
				}
				break;
			case 3:		//�H�B��
				{
					if(m_listshowinfo[i].LastDrawSingle == m_listshowinfo[i].reciveinfo.Single)
					{
						//�|��?�H�񰹨����d���Z���m��ۊp�[�f��ǻ���N��
						x = m_listshowinfo[i].LastDrawPoint.x;
						y = m_listshowinfo[i].LastDrawPoint.y;
					}
					else
					{					
						//��z���᫔���N��
						x = rand()%(m_listshowinfo[i].reciveinfo.Single);
						y = (int)sqrt((double)(m_listshowinfo[i].reciveinfo.Single * m_listshowinfo[i].reciveinfo.Single - x*x));
						
						x += BMP_ROUTER_WIDTH;				
						y = DRAW_HEIGHT-BMP_ROUTER_HEIGHT-y;
					}

					Ellipse(m_mdc, x-DRAW_RADIUS, y-DRAW_RADIUS, x+DRAW_RADIUS, y+DRAW_RADIUS);
					cr.SetRect(x+DRAW_RADIUS, y-DRAW_RADIUS, x+DRAW_RADIUS+DRAW_TEXT_WIDTH, y-DRAW_RADIUS+DRAW_TEXT_HEIGHT);
					//���b͘�U?�������ǻ�U?��
					tag_index = FindTagIdInSave(m_listshowinfo[i].reciveinfo.TagId);
					if(tag_index == -1)
					{
						IdToString(str, m_listshowinfo[i].reciveinfo.TagId); 
					}
					else
					{
						str = m_saveinfo.tag[tag_index].Name;
						if(str.IsEmpty())
							IdToString(str, m_saveinfo.tag[tag_index].ID);
					}					
					::DrawText(m_mdc, str, -1, &cr, DT_VCENTER|DT_SINGLELINE);
				}
				break;
			}
			m_listshowinfo[i].LastDrawSingle = m_listshowinfo[i].reciveinfo.Single;
			m_listshowinfo[i].LastDrawPoint.x = x;
			m_listshowinfo[i].LastDrawPoint.y = y;
		}
	}


	BitBlt(m_hdc, DRAW_LEFT, DRAW_TOP, DRAW_WIDTH, DRAW_HEIGHT, m_mdc, 0, 0, SRCCOPY);

}
