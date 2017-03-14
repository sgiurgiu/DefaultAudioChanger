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
#include <strsafe.h>

CAppModule _Module;
CDevicesManager devicesManager;
HKEY deviceSettingsKey;
HKEY appSettingsKey;
CMainDlg dlgMain;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL)
{
    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);

    
    dlgMain.SetDevicesManager(&devicesManager);
    dlgMain.SetDeviceSettingsKey(deviceSettingsKey);
    dlgMain.SetAppSettingsKey(appSettingsKey);

    if(dlgMain.Create(NULL) == NULL)
    {
        ATLTRACE(_T("Main dialog creation failed!\n"));
        return 0;
    }
    DWORD subKeyCount;
    DWORD maxSubKeyLength;
    DWORD lpcValues,lpcMaxValueNameLen;
    LONG result=::RegQueryInfoKey(deviceSettingsKey,NULL,NULL,NULL,&subKeyCount,&maxSubKeyLength,NULL,&lpcValues,&lpcMaxValueNameLen,NULL,NULL,NULL);	

    if(dlgMain.ShowTrayIcon())
    {
        dlgMain.ShowWindow(lpcValues<=0?SW_SHOWDEFAULT:SW_HIDE);
    }
    else
    {
        dlgMain.ShowWindow(SW_SHOWDEFAULT);
    }
    

    int nRet = theLoop.Run();

    _Module.RemoveMessageLoop();
    return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
    HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);

    ATLASSERT(SUCCEEDED(hRes));
    
    if(FAILED(devicesManager.InitializeDeviceEnumerator()))
    {
        return -1;
    }
    if(FAILED(devicesManager.LoadAudioDevices()))
    {
        return -1;
    }
    HKEY currentUser;
    LONG result=::RegOpenCurrentUser(KEY_ALL_ACCESS,&currentUser);
    if(result!=ERROR_SUCCESS)
    {
        ::MessageBox(NULL,L"Cannot open current's user registry key",L"Error",MB_OK|MB_ICONERROR);
        return -1;
    }
    
    result=::RegCreateKeyEx(currentUser,L"Software\\Zergiu.com\\DAC\\Devices",0,
        NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&deviceSettingsKey,NULL);
    if(result!=ERROR_SUCCESS)
    {
        ::MessageBox(NULL,L"Cannot open current's user registry key",L"Error",MB_OK|MB_ICONERROR);
        return -1;
    }
    
    result=::RegCreateKeyEx(currentUser,L"Software\\Zergiu.com\\DAC\\Settings",0,
        NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&appSettingsKey,NULL);
    if(result!=ERROR_SUCCESS)
    {
        ::MessageBox(NULL,L"Cannot open current's user registry key",L"Error",MB_OK|MB_ICONERROR);
        return -1;
    }

    //this removes the devices that were present in the system and selected for switch and are
    //no longer available (because the user may have switched audio cards for example, added new ones
    //or removed some)
    /*if(FAILED(devicesManager.ClearAbsentDevices(&deviceSettingsKey)))
    {
        ::MessageBox(NULL,L"Cannot clear old devices from registry",L"Error",MB_OK|MB_ICONERROR);
        return -1;
    }*/

    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(NULL, 0, 0, 0L);

    AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

    hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));

    int nRet = Run(lpstrCmdLine);

    _Module.Term();
    devicesManager.ReleaseDeviceEnumerator();
    ::CoUninitialize();

    ::RegCloseKey(deviceSettingsKey);
    ::RegCloseKey(currentUser);
    
    return nRet;
}
