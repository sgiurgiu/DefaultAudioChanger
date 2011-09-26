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
CMainDlg dlgMain;

#define PIPE_NAME L"\\\\.\\pipe\\DefaultAudioChangerPipe"

int Run(LPTSTR /*lpstrCmdLine*/ = NULL)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	
	dlgMain.SetDevicesManager(&devicesManager);
	dlgMain.SetDeviceSettingsKey(deviceSettingsKey);

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

void SendServerSwitchMessage()
{
	int msg=1;
	HANDLE pipe=CreateFile(PIPE_NAME,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);

	if(pipe==INVALID_HANDLE_VALUE)
	{
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError(); 

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		// Display the error message and exit the process

		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
			(lstrlen((LPCTSTR)lpMsgBuf)+40) * sizeof(TCHAR)); 
		StringCchPrintf((LPTSTR)lpDisplayBuf, 
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			TEXT("failed with error %d: %s"), 
			 dw, lpMsgBuf); 
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
		return;
	}

// The pipe connected; change to message-read mode. 
   DWORD dwMode = PIPE_READMODE_MESSAGE; 
   BOOL fSuccess = SetNamedPipeHandleState( 
      pipe,    // pipe handle 
      &dwMode,  // new pipe mode 
      NULL,     // don't set maximum bytes 
      NULL);    // don't set maximum time
   DWORD bWritten;
   BOOL result=WriteFile(pipe,&msg,sizeof(int),&bWritten,NULL);

	if(!result)
	{
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError(); 

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		// Display the error message and exit the process

		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
			(lstrlen((LPCTSTR)lpMsgBuf)+40) * sizeof(TCHAR)); 
		StringCchPrintf((LPTSTR)lpDisplayBuf, 
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			TEXT("failed with error %d: %s"), 
			 dw, lpMsgBuf); 
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
    
	}

	CloseHandle(pipe);
}
DWORD WINAPI CreateListeningThread(LPVOID data)
{
	for(;;)
	{
		HANDLE pipe=::CreateNamedPipe(PIPE_NAME,
			PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
			1,10,10,0,NULL);

		if(pipe==INVALID_HANDLE_VALUE)
		{		
			return 0;
		}
		if(ConnectNamedPipe(pipe, NULL))
		{
			int msg=0;
			DWORD bRead=0;		
			if(ReadFile(pipe,&msg,sizeof(msg),&bRead,NULL))
			{

				DWORD lpcValues,lpcMaxValueNameLen;
				LONG result=::RegQueryInfoKey(deviceSettingsKey,NULL,NULL,NULL,NULL,NULL,NULL,&lpcValues,&lpcMaxValueNameLen,NULL,NULL,NULL);	
				if(lpcValues<=0) continue;
				std::vector<LPWSTR> ids;
				for(DWORD i=0;i<lpcValues;i++)
				{
					DWORD len=lpcMaxValueNameLen+1;
					WCHAR *valName=new WCHAR[len];
					::RegEnumValue(deviceSettingsKey,i,valName,&len,NULL,REG_NONE,NULL,NULL);
					ids.push_back(valName);
				}
				
				devicesManager.SwitchDevices(&ids);
				dlgMain.UpdateApplicationIcon();

				for(auto it=ids.begin();it!=ids.end();++it)
				{
					delete[] (*it);
				}
				ids.clear();
			}
		}
		FlushFileBuffers(pipe); 
		DisconnectNamedPipe(pipe);
		::CloseHandle(pipe);
	}


	return 0;
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

	HANDLE pipe=::CreateNamedPipe(PIPE_NAME,
		PIPE_ACCESS_DUPLEX|FILE_FLAG_FIRST_PIPE_INSTANCE,
		PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
		1,10,10,0,NULL);
	HANDLE thread=NULL;
	if(pipe==INVALID_HANDLE_VALUE)
	{
		//the process already started, we are the client here, so we must connect to this pipe and send the message
		SendServerSwitchMessage();
		return 0;
	}
	else
	{	
		CloseHandle(pipe);
		thread=::CreateThread(NULL,0,CreateListeningThread,NULL,0,NULL);		
	}

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
	::CloseHandle(thread);
	
	return nRet;
}
