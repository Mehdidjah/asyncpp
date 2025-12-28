#include "pch.h"
#include "framework.h"
#include "mfc_timer.h"
#include "mfc_timerDlg.h"
#include "afxdialogex.h"
#include <sstream>
#include <thread>
#include "extensions/windows/promise_windows.hpp"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
protected:
    DECLARE_MESSAGE_MAP()
};
CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()
CmfctimerDlg::CmfctimerDlg(CWnd* pParent )
    : CDialogEx(IDD_MFC_TIMER_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}
void CmfctimerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}
BEGIN_MESSAGE_MAP(CmfctimerDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()
BOOL CmfctimerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    auto updateText = [=](int sourceLine) {
#ifdef UNICODE
        std::wstringstream os;
#else
        std::stringstream os;
#endif
        os << "thread id   = " << std::this_thread::get_id() << "\n"
           << "source line = " << sourceLine;
        CWnd *label = GetDlgItem(IDC_STATIC);
        label->SetWindowText(os.str().c_str());
    };
    updateText(__LINE__);
    using namespace promise;
    doWhile([=](DeferLoop &loop) {
        promise::delay(1000).then([=]() {
            updateText(__LINE__);
            return promise::delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return promise::delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return promise::yield();
        }).then([=]() {
            updateText(__LINE__);
            return promise::delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return promise::delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return promise::delay(1000);
        }).then([=]() {
            updateText(__LINE__);
            return promise::delay(1000);
        }).then(loop);
    });
    return TRUE;
}
void CmfctimerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}
void CmfctimerDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}
HCURSOR CmfctimerDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}