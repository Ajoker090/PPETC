// ARF24_DEMO.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CARF24_DEMOApp:
// �йش����ʵ�֣������ ARF24_DEMO.cpp
//

class CARF24_DEMOApp : public CWinApp
{
public:
	CARF24_DEMOApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CARF24_DEMOApp theApp;