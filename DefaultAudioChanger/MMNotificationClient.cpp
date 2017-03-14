#include "stdafx.h"
#include "MMNotificationClient.h"
#include "DevicesManager.h"
#include <Functiondiscoverykeys_devpkey.h>


CMMNotificationClient::CMMNotificationClient(IMMDeviceEnumerator *_pEnum, CDevicesManager* manager) :_cRef(1), 
    _pEnum(_pEnum), manager(manager)
{

}

CMMNotificationClient::~CMMNotificationClient()
{
    //SAFE_RELEASE(_pEnum);
}

ULONG STDMETHODCALLTYPE CMMNotificationClient::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG STDMETHODCALLTYPE CMMNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::QueryInterface(
    REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid)
    {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid)
    {
        AddRef();
        *ppvInterface = (IMMNotificationClient*)this;
    }
    else
    {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDefaultDeviceChanged(
    EDataFlow flow, ERole role,
    LPCWSTR pwstrDeviceId)
{
    char  *pszFlow = "?????";
    char  *pszRole = "?????";

   // _PrintDeviceName(pwstrDeviceId);

    switch (flow)
    {
    case eRender:
        pszFlow = "eRender";
        break;
    case eCapture:
        pszFlow = "eCapture";
        break;
    }

    switch (role)
    {
    case eConsole:
        pszRole = "eConsole";
        break;
    case eMultimedia:
        pszRole = "eMultimedia";
        break;
    case eCommunications:
        pszRole = "eCommunications";
        break;
    }

    ATLTRACE2("  -->New default device: flow = %s, role = %s\n",
        pszFlow, pszRole);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
   // _PrintDeviceName(pwstrDeviceId);
	ATLTRACE2("  -->Added device\n");
   // manager->notifyDeviceAddedListeners(pwstrDeviceId);
    
    return S_OK;
};

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
   // _PrintDeviceName(pwstrDeviceId);
	ATLTRACE2("  -->Removed device\n");
  //  manager->notifyDeviceRemovedListeners(pwstrDeviceId);
    
    return S_OK;
}
HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceStateChanged(
    LPCWSTR pwstrDeviceId,
    DWORD dwNewState)
{
    char  *pszState = "?????";

   // _PrintDeviceName(pwstrDeviceId);

    switch (dwNewState)
    {
    case DEVICE_STATE_ACTIVE:
        pszState = "ACTIVE";
		manager->notifyDeviceAddedListeners(pwstrDeviceId);
        break;
    case DEVICE_STATE_DISABLED:
        pszState = "DISABLED";
		manager->notifyDeviceRemovedListeners(pwstrDeviceId);
        break;
    case DEVICE_STATE_NOTPRESENT:
        pszState = "NOTPRESENT";
		manager->notifyDeviceRemovedListeners(pwstrDeviceId);
        break;
    case DEVICE_STATE_UNPLUGGED:
        pszState = "UNPLUGGED";
		manager->notifyDeviceRemovedListeners(pwstrDeviceId);
        break;
    }

    ATLTRACE2("  -->New device state is DEVICE_STATE_%s (0x%8.8x)\n",
        pszState, dwNewState);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnPropertyValueChanged(
    LPCWSTR pwstrDeviceId,
    const PROPERTYKEY key)
{
   // _PrintDeviceName(pwstrDeviceId);

    ATLTRACE2("  -->Changed device property "
        "{%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}#%d\n",
        key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
        key.fmtid.Data4[0], key.fmtid.Data4[1],
        key.fmtid.Data4[2], key.fmtid.Data4[3],
        key.fmtid.Data4[4], key.fmtid.Data4[5],
        key.fmtid.Data4[6], key.fmtid.Data4[7],
        key.pid);
    return S_OK;
}

/*
HRESULT CMMNotificationClient::_PrintDeviceName(LPCWSTR pwstrId)
{
    HRESULT hr = S_OK;
    IMMDevice *pDevice = NULL;
    IPropertyStore *pProps = NULL;
    PROPVARIANT varString;

    CoInitialize(NULL);
    PropVariantInit(&varString);

    if (_pEnum == NULL)
    {
        // Get enumerator for audio endpoint devices.
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
            NULL, CLSCTX_INPROC_SERVER,
            __uuidof(IMMDeviceEnumerator),
            (void**)&_pEnum);
    }
    if (hr == S_OK)
    {
        hr = _pEnum->GetDevice(pwstrId, &pDevice);
    }
    if (hr == S_OK)
    {
        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    }
    if (hr == S_OK)
    {
        // Get the endpoint device's friendly-name property.
        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varString);
    }
    ATLTRACE2("----------------------\nDevice name: \"%S\"\n"
        "  Endpoint ID string: \"%S\"\n",
        (hr == S_OK) ? varString.pwszVal : L"null device",
        (pwstrId != NULL) ? pwstrId : L"null ID");

    PropVariantClear(&varString);

    SAFE_RELEASE(pProps);
    SAFE_RELEASE(pDevice);
    CoUninitialize();
    return hr;
}
*/