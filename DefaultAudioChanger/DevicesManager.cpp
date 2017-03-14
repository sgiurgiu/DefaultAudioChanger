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

#include "StdAfx.h"
#include "DevicesManager.h"
#include <Mmdeviceapi.h>
#include "PolicyConfig.h"
#include <Propidl.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <algorithm>
#include <strsafe.h>
#include "MMNotificationClient.h"

#define RETURN_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }

#define SAFE_RELEASE(punk)  \
              if ((punk) != nullptr)  \
                { (punk)->Release(); (punk) = nullptr; }

CDevicesManager::CDevicesManager(void) :pEnum(nullptr), client(nullptr)
{

}


CDevicesManager::~CDevicesManager(void)
{
    ReleaseDeviceEnumerator();
    delete client;
}

void CDevicesManager::ReleaseDeviceEnumerator()
{
    if (pEnum) {
        pEnum->UnregisterEndpointNotificationCallback(client);
    }	
    SAFE_RELEASE(pEnum)
}

HRESULT CDevicesManager::InitializeDeviceEnumerator()
{
    HRESULT hr = S_OK;
    if(!pEnum)
    {
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
            CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
        client = new CMMNotificationClient(pEnum, this);
        pEnum->RegisterEndpointNotificationCallback(client);
    }
    return hr;
}
HRESULT CDevicesManager::SwitchDevices(const std::vector<std::wstring>& ids)
{
    if(ids.empty()) return E_FAIL;
    AudioDevice defaultDevice;	
    if (!GetDefaultDevice(defaultDevice)) return E_FAIL;
        
    auto foundDefault = std::find_if(ids.cbegin(), ids.end(), [&defaultDevice](const auto& id) {
        return defaultDevice.deviceId == id;
    });
	auto newDefault = foundDefault;
    if (newDefault  != ids.end()) {
        ++newDefault;
    }
    if (newDefault == ids.end()) {
        newDefault = ids.begin();
    }
    
    return SetDefaultDevice(*newDefault);
}


HRESULT CDevicesManager::SetDefaultDevice(const std::wstring& id)
{
    IPolicyConfigVista *pPolicyConfig;
    ERole reserved = eConsole;

    HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), 
        NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
    if (SUCCEEDED(hr))
    {		
        hr = pPolicyConfig->SetDefaultEndpoint(id.c_str(), eConsole);
        hr = pPolicyConfig->SetDefaultEndpoint(id.c_str(), eMultimedia);
        hr = pPolicyConfig->SetDefaultEndpoint(id.c_str(), eCommunications);
        pPolicyConfig->Release();
    }

    return hr;

}

HRESULT CDevicesManager::LoadAudioDevices()
{
    devices.clear();
    if(!pEnum)
    {
        return E_FAIL;
    }
    IMMDeviceCollection *devicesCollection=NULL;
    IMMDevice *device=NULL;    
    LPWSTR pwszID = NULL;
    HRESULT hr=pEnum->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,&devicesCollection);
    RETURN_ON_ERROR(hr)
    UINT count;
    hr=devicesCollection->GetCount(&count);
    RETURN_ON_ERROR(hr)

    for(UINT i=0;i<count;i++)
    {
        
        hr=devicesCollection->Item(i,&device);
        RETURN_ON_ERROR(hr)
        
        hr=device->GetId(&pwszID);
        RETURN_ON_ERROR(hr)
        addDevice(device, pwszID);

        CoTaskMemFree(pwszID);
        pwszID=NULL;       
        SAFE_RELEASE(device)
    }

Exit:
    CoTaskMemFree(pwszID);
    SAFE_RELEASE(devicesCollection)
    SAFE_RELEASE(device)
    return hr;
}

void CDevicesManager::addDevice(IMMDevice *device, LPCWSTR pwszID)
{
    IPropertyStore *pProps = NULL;
    AudioDevice audioDeviceStruct;
    HRESULT hr = device->OpenPropertyStore(STGM_READ, &pProps);
	RETURN_ON_ERROR(hr)
    PROPVARIANT varName;
    PropVariantInit(&varName);
    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
    RETURN_ON_ERROR(hr)
    PROPVARIANT varIconPath;
    PropVariantInit(&varIconPath);
    hr = pProps->GetValue(PKEY_DeviceClass_IconPath, &varIconPath);
    RETURN_ON_ERROR(hr)
    
    audioDeviceStruct.deviceId.assign(pwszID);
    audioDeviceStruct.deviceName.assign(varName.pwszVal);
    HICON largeIcon, smallIcon;
    if (ExtractDeviceIcons(varIconPath.pwszVal, &largeIcon, &smallIcon) > 1) {
        audioDeviceStruct.largeIcon = std::make_shared<Icon>(largeIcon);
        audioDeviceStruct.smallIcon = std::make_shared<Icon>(smallIcon);
    }

    devices.push_back(audioDeviceStruct);
 Exit:
    PropVariantClear(&varIconPath);
    PropVariantClear(&varName);
    SAFE_RELEASE(pProps)
}


UINT CDevicesManager::ExtractDeviceIcons(LPWSTR iconPath,HICON *iconLarge,HICON *iconSmall)
{
    TCHAR *filePath;
    int iconIndex;
    WCHAR* token=NULL;
    filePath=wcstok_s(iconPath,L",",&token);
    TCHAR *iconIndexStr=wcstok_s(NULL,L",",&token);
    iconIndex=_wtoi(iconIndexStr);
    TCHAR filePathExp[MAX_PATH];
    ExpandEnvironmentStrings(filePath,filePathExp,MAX_PATH);
    //HMODULE module=LoadLibraryEx(filePathExp,NULL,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    return ExtractIconEx(filePathExp,iconIndex,iconLarge,iconSmall,1);
}
const std::vector<AudioDevice>& CDevicesManager::GetAudioDevices() const
{
    return devices;
}

bool CDevicesManager::GetDefaultDevice(AudioDevice& audioDevice)
{
    if(!pEnum)
    {
        return false;
    }
    bool found = false;
    LPWSTR pwszID = NULL;
    IMMDevice *immDevice;
    HRESULT hr = pEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &immDevice);
    RETURN_ON_ERROR(hr);
    hr = immDevice->GetId(&pwszID);
    RETURN_ON_ERROR(hr);
    {
        auto it = std::find_if(devices.begin(), devices.end(), [=](const AudioDevice& device){
            return device.deviceId == pwszID;
        });
        if (it != devices.end()) {
            audioDevice = *it;
            found = true;
        }
    }
Exit:	
    CoTaskMemFree(pwszID);
    SAFE_RELEASE(immDevice);

    return found;
}
bool CDevicesManager::GetDevice(LPCWSTR deviceId, AudioDevice** device)
{
    for (auto& d : devices) {
        if (d.deviceId == deviceId) {
            *device = &d;
            return true;
        }
    }
    return false;
}
HRESULT CDevicesManager::ClearAbsentDevices(PHKEY pkey)
{
    HRESULT hr=S_OK;	
    DWORD index=0;	
    DWORD lpcValues,lpcMaxValueNameLen;
    HKEY key;
    key=*pkey;
    LONG result=::RegQueryInfoKey(key,NULL,NULL,NULL,NULL,NULL,NULL,&lpcValues,&lpcMaxValueNameLen,NULL,NULL,NULL);	
    if(lpcValues<=0) return hr;

    std::vector<std::wstring> ids;
    for(DWORD i=0;i<lpcValues;i++)
    {
        DWORD len=lpcMaxValueNameLen+1;
        WCHAR *valName=new WCHAR[len];
        ::RegEnumValue(key,i,valName,&len,NULL,REG_NONE,NULL,NULL);
        ids.push_back(valName);
        delete[] valName;
    }

    for(DWORD i=0;i<lpcValues;i++)
    {
        auto id=ids[i];
        bool found=false;
        for(auto it=devices.cbegin();it!=devices.cend();++it)
        {
            auto devKey=(*it).deviceId;
            if(devKey == id)
            {
                found=true;
                break;
            }
        }

        if(!found)
        {
            hr=::RegDeleteValue(key,id.c_str());			
        }
    }


    return S_OK;
}
void CDevicesManager::addDeviceAddedListener(device_listener listener)
{
    device_added_listener.push_back(listener);
}
void CDevicesManager::addDeviceRemovedListener(device_listener listener)
{
    device_removed_listener.push_back(listener);
}
void CDevicesManager::notifyDeviceAddedListeners(LPCWSTR deviceId)
{
	LoadAudioDevices();
	for (auto& listener : device_added_listener) {
		listener(deviceId);
	}

}
void CDevicesManager::notifyDeviceRemovedListeners(LPCWSTR deviceId)
{
	LoadAudioDevices();
    for (auto& listener : device_removed_listener) {
        listener(deviceId);
    }
	
}
