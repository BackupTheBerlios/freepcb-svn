// DlgMyMessageBox.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgMyMessageBox.h"


// CDlgMyMessageBox dialog

IMPLEMENT_DYNAMIC(CDlgMyMessageBox, CDialog)
CDlgMyMessageBox::CDlgMyMessageBox(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMyMessageBox::IDD, pParent)
{
}

CDlgMyMessageBox::~CDlgMyMessageBox()
{
}

void CDlgMyMessageBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_MYMESSAGE, m_message);
	DDX_Control(pDX, IDC_CHECK1, m_check_dont_show);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_message.SetWindowText( *m_mess );
	}
	else
	{
		bDontShowBoxState = m_check_dont_show.GetCheck();
	}
}

BEGIN_MESSAGE_MAP(CDlgMyMessageBox, CDialog)
END_MESSAGE_MAP()


void CDlgMyMessageBox::Initialize( CString * mess )
{
	m_mess = mess;
}

// CDlgMyMessageBox message handlers
