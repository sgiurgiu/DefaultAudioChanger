/******************************************************************************
* This file is part of DefaultAudioChanger.
* Copyright (c) 2011 Sergiu Giurgiu 
*
* DefaultAudioChanger is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DefaultAudioChanger is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with DefaultAudioChanger.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "DevicesManager.h"

#define STARTUP_KEY _T("DefaultAudioChanger")

CMainDlg::CMainDlg()
{
    ZeroMemory(&notifyIconData, sizeof notifyIconData);
    m_hDialogBrush = CreateSolidBrush(RGB(255, 255, 255));
}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
    return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
    return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    
    ZeroMemory(&notifyIconData, sizeof notifyIconData);
    // center the dialog on the screen
    CenterWindow();

    listView.Attach(GetDlgItem(IDC_DEVICES_LIST));
    listView.SetExtendedListViewStyle(listView.GetExtendedListViewStyle() | LVS_EX_CHECKBOXES |
        LVS_EX_AUTOSIZECOLUMNS | LVS_EX_DOUBLEBUFFER);

    popupMenu.LoadMenu(IDR_POPUP_MENU);
    CMenuHandle menu = popupMenu.GetSubMenu(0);
    menu.SetMenuDefaultItem(ID_POPUPMENU_OPTIONS, FALSE);

    // set icons
    if(!LoadDevicesIcons())
    {
        return FALSE;
    }
    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    UIAddChildWindowContainer(m_hWnd);
    
    LoadInitialDeviceList();

    LoadHotkey();

    LoadStartupSetting();

//	this->SetWindowLong();
    return bHandled=FALSE;
}

void CMainDlg::LoadStartupSetting()
{
    startupCheckBox.Attach(GetDlgItem(IDC_WINSTARTUP_CHECK));
    HKEY currentUser;
    LONG result=::RegOpenCurrentUser(KEY_ALL_ACCESS,&currentUser);
    if(result!=ERROR_SUCCESS)
    {		
        return ;//oh well....
    }
    HKEY winRunKey;
    result=::RegOpenKeyEx(currentUser,_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,&winRunKey);
    if(result!=ERROR_SUCCESS)
    {
        return;//oh well....
    }
    DWORD keyType;
    result=::RegGetValue(winRunKey,NULL,STARTUP_KEY,RRF_RT_REG_SZ,&keyType,NULL,NULL);
    startupCheckBox.SetCheck(result==ERROR_SUCCESS);
    RegCloseKey (winRunKey);
    RegCloseKey (currentUser);
}

void CMainDlg::LoadHotkey()
{
    hotKeyCheckBox.Attach(GetDlgItem(IDC_HOTKEY_CHECK));
    hotKeyCtrl.Attach(GetDlgItem(IDC_REGHOTKEY));

    hotKeyId=GlobalAddAtom(L"DefaultAudioChange");
    DWORD hotKey=0;
    DWORD hotKeysize=sizeof(DWORD);
    HRESULT regOpResult=::RegQueryValueEx(appSettingsKey,L"HotKey",NULL,NULL,(LPBYTE)&hotKey,&hotKeysize);
    if(regOpResult==ERROR_SUCCESS)
    {
        ::SendMessage(hotKeyCtrl.m_hWnd, HKM_SETHOTKEY, hotKey, 0L);		
        hotKeyCheckBox.SetCheck(TRUE);
        hotKeyCtrl.EnableWindow(TRUE);
        BYTE wVirtualKeyCode = LOBYTE(LOWORD(hotKey));
        BYTE wModifiers = HIBYTE(LOWORD(hotKey));
        if(!RegisterHotKey(m_hWnd,hotKeyId,hkf2modf(wModifiers),wVirtualKeyCode))
        {
            //nothing really should happen here			
        }
    }
}

BOOL CMainDlg::LoadDevicesIcons()
{
    AudioDevice defaultDevice;	
    if (devicesManager->GetDefaultDevice(defaultDevice))
    {
        SetIcon(defaultDevice.largeIcon->GetIcon(), TRUE);
        SetIcon(defaultDevice.smallIcon->GetIcon(), FALSE);
        return TRUE;
    }
    else
    {
        ::MessageBox(NULL,L"Cannot determine the default device",L"Error",MB_OK|MB_ICONERROR);
        return FALSE;
    }
}

void CMainDlg::LoadInitialDeviceList()
{

    listView.DeleteAllItems();
    /*if (listView.GetHeader(). .GetItemCount() <= 0) {
        listView.AddColumn(L"Device", 0, 0);
    }*/
    listView.AddColumn(L"Device", 0, 0);
    auto& audioDevices=devicesManager->GetAudioDevices();
    CImageList imgList;
    imgList.Create(16,16,ILC_COLOR32|ILC_ORIGINALSIZE|ILC_MASK,(int)audioDevices.size(),1);
    for(auto& device : audioDevices)
    {
        imgList.AddIcon(device.largeIcon->GetIcon());
    }

    listView.SetImageList(imgList,LVSIL_SMALL);

    DWORD lpcValues=0,lpcMaxValueNameLen=0;
    LONG regOpResult=::RegQueryInfoKey(deviceSettingsKey,NULL,NULL,NULL,NULL,NULL,NULL,&lpcValues,&lpcMaxValueNameLen,NULL,NULL,NULL);	
    
    int count = 0;
    for (auto& device : audioDevices)
    {
        listView.AddItem(count,0,device.deviceName.c_str(),count);
        listView.SetItemData(count,(DWORD_PTR)&device);
        listView.SetColumnWidth(count,LVSCW_AUTOSIZE);        
        DWORD keyType;
        regOpResult=::RegQueryValueEx(deviceSettingsKey,device.deviceId.c_str(),NULL,&keyType,NULL,NULL);
        if(regOpResult==ERROR_SUCCESS)
        {
            listView.SetCheckState(count,TRUE);
        }
        /*size_t menuItemLen=device.deviceName.length()+10;
        std::wstring menuName(L"&");
        menuName += std::to_wstring(count + 1);
        menuName += L" ";
        menuName += device.deviceName;
        menu.AppendMenu(MF_ENABLED|MF_STRING,WM_USER+count+1,menuName.c_str());*/
        count++;
    }
    
}

LRESULT CMainDlg::OnSpecificDeviceSelected(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
    WORD index=wID-WM_USER-1;
    auto& devices=devicesManager->GetAudioDevices();
    if(index<0 || index>=devices.size())
    {
        bHandled=FALSE;
        return 0;
    }
    auto& device=devices.at(index);
    devicesManager->SetDefaultDevice(device.deviceId.c_str());
    UpdateApplicationIcon();
    bHandled=TRUE;
    return 0;
}

HBRUSH CMainDlg::OnCtlColorDlg(CDCHandle dc, CWindow wnd)
{
    return m_hDialogBrush;
}

HBRUSH CMainDlg::OnCtlColorStatic(CDCHandle dc, CStatic wndStatic)
{
    return m_hDialogBrush;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if(notifyIconData.cbSize)
    {
        Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
        ZeroMemory(&notifyIconData, sizeof notifyIconData);
    }


    DeleteObject(m_hDialogBrush);

    // unregister message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);
    pLoop->RemoveIdleHandler(this);


    return 0;
}
void CMainDlg::SetDevicesManager(CDevicesManager* devicesManager)
{
    this->devicesManager=devicesManager;
    devicesManager->addDeviceAddedListener([this](LPCWSTR deviceId){
		LoadInitialDeviceList();
    });
    devicesManager->addDeviceRemovedListener([&](LPCWSTR deviceId){
		ATLTRACE2(L"Device removed %S\n", deviceId);
		LoadInitialDeviceList();
    });
}

BOOL CMainDlg::ShowTrayIcon()
{
    AudioDevice defaultDevice;
    if (!devicesManager->GetDefaultDevice(defaultDevice)) {
        return false;
    }
    if(!notifyIconData.cbSize)
    {
        notifyIconData.cbSize = sizeof(notifyIconData);
        notifyIconData.hWnd = m_hWnd;
        notifyIconData.uID = 1;
        notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
        notifyIconData.dwInfoFlags = NIIF_USER| NIIF_LARGE_ICON;
        notifyIconData.uCallbackMessage = WM_SYSTEMTRAYICON;
        notifyIconData.uTimeout = 1000;
        //apparently the icon that appears usually on the desktop is the large one
        //the small one seems to be somewhat different (maybe it shows up under different circumstances)
        notifyIconData.hIcon = defaultDevice.largeIcon->GetIcon();
        notifyIconData.hBalloonIcon = defaultDevice.largeIcon->GetIcon();
        
        _tcscpy_s(notifyIconData.szTip, defaultDevice.deviceName.c_str());
        _tcscpy_s(notifyIconData.szInfo, defaultDevice.deviceName.c_str());
        _tcscpy_s(notifyIconData.szInfoTitle ,L"Current Audio Device");
        return Shell_NotifyIcon(NIM_ADD, &notifyIconData);
    }
    else
    {
        //apparently the icon that appears usually on the desktop is the large one
        //the small one seems to be somewhat different (maybe it shows up under different circumstances)
        notifyIconData.hIcon = defaultDevice.largeIcon->GetIcon();
        notifyIconData.hBalloonIcon = defaultDevice.largeIcon->GetIcon();
        _tcscpy_s(notifyIconData.szTip, defaultDevice.deviceName.c_str());
        _tcscpy_s(notifyIconData.szInfo, defaultDevice.deviceName.c_str());

        return Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
    }
    
}

LRESULT CMainDlg::OnSysCommand(UINT nCommand, CPoint point)
{
    switch(nCommand)
    {
    case SC_MINIMIZE:
        ShowWindow(SW_HIDE);
        break;        
    default:
        SetMsgHandled(FALSE);
    }
    return 0;
}

LRESULT CMainDlg::OnSystemTrayIcon(UINT, WPARAM wParam, LPARAM lParam)
{
    ATLASSERT(wParam == 1);
    switch(lParam)
    {
    case WM_LBUTTONDBLCLK:
        SendMessage(WM_COMMAND, ID_POPUPMENU_OPTIONS);
        break;
    case WM_RBUTTONUP:
        {
            SetForegroundWindow(m_hWnd);
            CMenuHandle menu = popupMenu.GetSubMenu(0);		
            CPoint Position;
            ATLVERIFY(GetCursorPos(&Position));
            menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN, Position.x, Position.y, m_hWnd);
        }
        break;
    }
    return 0;
}

LRESULT CMainDlg::OnScRestore(UINT, INT, HWND)
{
    ShowWindow(SW_SHOW);
    BringWindowToTop();
    return 0;
}
LRESULT CMainDlg::OnScClose(UINT, INT, HWND)
{
    PostMessage(WM_COMMAND, IDCLOSE);
    return 0;
}
LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAboutDlg dlg;
    dlg.DoModal();
    return 0;
}

LRESULT CMainDlg::OnBtnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // TODO: Add validation code 
    CloseDialog(wID);
    return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
    UnregisterHotKey(m_hWnd,hotKeyId);
    GlobalDeleteAtom(hotKeyId);
    DestroyWindow();	
    ::PostQuitMessage(nVal);
}


LRESULT CMainDlg::OnBnClickedHideButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ShowWindow(SW_HIDE);
    return 0;
}


LRESULT CMainDlg::OnBnClickedSwitchButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{	
    std::vector<std::wstring> ids;
    int count=listView.GetItemCount();
    
    for(int i=0;i<count;i++)
    {
        if(listView.GetCheckState(i))
        {
            AudioDevice* device=(AudioDevice*)listView.GetItemData(i);
            std::wstring id=device->deviceId;
            ids.push_back(id);
        }
    }
    	
    HRESULT hr=devicesManager->SwitchDevices(ids);
    if(SUCCEEDED(hr))
    {
        UpdateApplicationIcon();
    }
    return 0;
}
LRESULT CMainDlg::OnBnClickedReloadButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    devicesManager->LoadAudioDevices();
    LoadInitialDeviceList();
    UpdateApplicationIcon();
    return 0;
}
void CMainDlg::UpdateApplicationIcon()
{
    AudioDevice defaultDevice;
    if (devicesManager->GetDefaultDevice(defaultDevice))
    {
        SetIcon(defaultDevice.largeIcon->GetIcon(), TRUE);
        SetIcon(defaultDevice.smallIcon->GetIcon(), FALSE);
        notifyIconData.hIcon = defaultDevice.largeIcon->GetIcon();
        ShowTrayIcon();
    }
}

void CMainDlg::SetDeviceSettingsKey(HKEY key)
{
    deviceSettingsKey=key;
}

void CMainDlg::SetAppSettingsKey(HKEY key)
{
    appSettingsKey=key;
}

LRESULT CMainDlg::OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
    LPNMLISTVIEW pNMListView=(LPNMLISTVIEW)pnmh;
    if (pNMListView->uOldState == 0 && pNMListView->uNewState == 0)
    {
        return 0;    // No change
    }
    BOOL bPrevState = (BOOL)(((pNMListView->uOldState & 
                LVIS_STATEIMAGEMASK)>>12)-1);  
    if (bPrevState < 0)    // On startup there's no previous state 
    {
        bPrevState = 0; // so assign as false (unchecked)
    }
    // New check box state
    BOOL bChecked = 
            (BOOL)(((pNMListView->uNewState & LVIS_STATEIMAGEMASK)>>12)-1);   
    if (bChecked < 0) // On non-checkbox notifications assume false
    {
        bChecked = 0; 
    }
    if (bPrevState == bChecked) // No change in check box
    {
        return 0;
    }

    AudioDevice* device=(AudioDevice*)listView.GetItemData(pNMListView->iItem);

    if(bChecked)
    {
        LONG result = ::RegSetValueEx(deviceSettingsKey, device->deviceId.c_str(), NULL, REG_SZ, 
            (const BYTE*)device->deviceName.c_str(), (DWORD) (device->deviceName.length() * sizeof(TCHAR)) );
        if(result!=ERROR_SUCCESS)
        {
            ::MessageBox(m_hWnd,L"Cannot save key in the registry",L"Error",MB_OK|MB_ICONERROR);
        }
    }
    else
    {
        LONG result=::RegDeleteKeyValue(deviceSettingsKey,NULL,device->deviceId.c_str());
        if(result!=ERROR_SUCCESS)
        {
            ::MessageBox(m_hWnd,L"Cannot save key in the registry",L"Error",MB_OK|MB_ICONERROR);
        }
    }

    bHandled=true;
    return 0;
}

LRESULT CMainDlg::OnBnClickedHotkeyCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    BOOL checked=hotKeyCheckBox.GetCheck();
    hotKeyCtrl.EnableWindow(checked);
    if(!checked)
    {
        LONG result=::RegDeleteKeyValue(appSettingsKey,NULL,L"HotKey");
        UnregisterHotKey(m_hWnd,hotKeyId);
        hotKeyCtrl.SetHotKey(0,0);
    }
    else
    {
        WORD hotKey;
        DWORD hotKeysize=sizeof(WORD);
        LONG result=::RegQueryValueEx(appSettingsKey,L"HotKey",NULL,NULL,(LPBYTE)&hotKey,&hotKeysize);
        if(result==ERROR_SUCCESS)
        {
            ::SendMessage(hotKeyCtrl.m_hWnd, HKM_SETHOTKEY, hotKey, 0L);		
        }
    }
    bHandled=true;
    return 0;
}

BYTE CMainDlg::hkf2modf(BYTE hkf)
{
    BYTE modf = 0;
    if (hkf & HOTKEYF_ALT) modf |= MOD_ALT;
    if (hkf & HOTKEYF_SHIFT) modf |= MOD_SHIFT;
    if (hkf & HOTKEYF_CONTROL) modf |= MOD_CONTROL;
    if (hkf & HOTKEYF_EXT) modf |= MOD_WIN;
    return modf;
}
BYTE CMainDlg::modf2hkf(BYTE modf)
{
    BYTE hkf=0;
    if(modf & MOD_ALT) hkf|=HOTKEYF_ALT;
    if(modf & MOD_SHIFT) hkf|=HOTKEYF_SHIFT;
    if(modf & MOD_CONTROL) hkf|=HOTKEYF_CONTROL;
    if(modf & MOD_WIN) hkf|=HOTKEYF_EXT;
    return hkf;
}

LRESULT CMainDlg::OnHotKeyChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    WORD hotKey;
    DWORD hotKeysize=sizeof(WORD);
    WORD wVirtualKeyCode,wModifiers ;
    hotKeyCtrl.GetHotKey(wVirtualKeyCode, wModifiers);
    hotKey=MAKEWORD(wVirtualKeyCode, wModifiers);
    LONG result=::RegSetValueEx(appSettingsKey,L"HotKey",NULL,REG_BINARY,(BYTE*)&hotKey,sizeof(hotKeysize));
    UnregisterHotKey(m_hWnd,hotKeyId);
    ATLTRACE2(L"wVirtualKeyCode:= %d, wModifiers=%d\n",wVirtualKeyCode,wModifiers);
    if(!RegisterHotKey(m_hWnd,hotKeyId,hkf2modf((BYTE)wModifiers),wVirtualKeyCode))
    {
        ::MessageBox(m_hWnd,L"Cannot register this hotkey",L"Error",MB_OK|MB_ICONERROR);
    }

    return 0;
}

LRESULT CMainDlg::OnHotKey(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    if(wParam==hotKeyId)
    {
        return OnBnClickedSwitchButton(0,0,0,bHandled);	
    }

    return 0;
}

LRESULT CMainDlg::OnBnClickedWinstartupCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    HKEY currentUser;
    LONG result=::RegOpenCurrentUser(KEY_ALL_ACCESS,&currentUser);
    if(result!=ERROR_SUCCESS)
    {	
        ::MessageBox(m_hWnd,L"Cannot open the registry",L"Error",MB_OK|MB_ICONERROR);
        return 0;//oh well....
    }
    HKEY winRunKey;
    result=::RegOpenKeyEx(currentUser,_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,&winRunKey);
    if(result!=ERROR_SUCCESS)
    {
        ::MessageBox(m_hWnd,L"Cannot open the registry",L"Error",MB_OK|MB_ICONERROR);
        RegCloseKey (currentUser);
        return 0;//oh well....
    }
    if(startupCheckBox.GetCheck())
    {
        TCHAR szFileName[MAX_PATH];
        DWORD filenameLength=GetModuleFileName( NULL, szFileName, MAX_PATH );
        if(filenameLength>0)
        {
            szFileName[filenameLength*sizeof(TCHAR)]=NULL;
        }

        result=::RegSetValueEx(winRunKey,STARTUP_KEY,NULL,REG_SZ,(BYTE*)&szFileName,filenameLength*sizeof(TCHAR));
        if(result!=ERROR_SUCCESS)
        {
            ::MessageBox(m_hWnd,L"Cannot save the preference in the registry",L"Error",MB_OK|MB_ICONERROR);
        }
    }
    else
    {
        result=::RegDeleteKeyValue(winRunKey,NULL,STARTUP_KEY);
        if(result!=ERROR_SUCCESS)
        {
            ::MessageBox(m_hWnd,L"Cannot save the preference in the registry",L"Error",MB_OK|MB_ICONERROR);
        }
    }

    RegCloseKey (winRunKey);
    RegCloseKey (currentUser);
    return 0;
}

