#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>

class CMMNotificationClient;
struct IMMDeviceEnumerator;
struct IMMDevice;

class Icon {
public:
    Icon() : icon(nullptr)
    {}
    Icon(HICON icon) : icon(icon)
    {}
    Icon(const Icon& other) = delete;
    Icon& operator=(const Icon& other) = delete;
    Icon(Icon&& other) = delete;
    Icon& operator=(Icon&& other) = delete;
    ~Icon()
    {
        if (icon) {
            DestroyIcon(icon);
        }
    }
    void SetIcon(HICON icon) {
        this->icon = icon;
    }
    HICON GetIcon() const {
        return icon;
    }

private:
    HICON icon;
};
struct AudioDevice{
    typedef std::shared_ptr<Icon> IconPtr;
    std::wstring deviceId;
    std::wstring deviceName;
    IconPtr smallIcon;
    IconPtr largeIcon;
    bool missing;
    AudioDevice() :deviceId(L""), deviceName(L""),
        smallIcon(nullptr), largeIcon(nullptr), missing(false)
    {}
};
typedef std::function<void(LPCWSTR deviceId)> device_listener;

class CDevicesManager
{
public:
    CDevicesManager(void);
    ~CDevicesManager(void);
    HRESULT InitializeDeviceEnumerator();
    bool GetDefaultDevice(AudioDevice& audioDevice);
    void ReleaseDeviceEnumerator();
    HRESULT LoadAudioDevices();
    const std::vector<AudioDevice>& GetAudioDevices() const;
    HRESULT SwitchDevices(const std::vector<std::wstring>& ids);
    HRESULT SetDefaultDevice(const std::wstring& id);
    HRESULT ClearAbsentDevices(PHKEY pkey);
    void addDeviceAddedListener(device_listener listener);
    void addDeviceRemovedListener(device_listener listener);
    bool GetDevice(LPCWSTR deviceId, AudioDevice** device);
private:
    UINT ExtractDeviceIcons(LPWSTR iconPath,HICON *iconLarge,HICON *iconSmall);
    void notifyDeviceAddedListeners(LPCWSTR deviceId);
    void notifyDeviceRemovedListeners(LPCWSTR deviceId);
    void addDevice(IMMDevice *device, LPCWSTR pwszID);
    friend class CMMNotificationClient;
private:
    IMMDeviceEnumerator *pEnum;
    CMMNotificationClient* client;
    std::vector<AudioDevice> devices;
    std::vector<device_listener> device_added_listener;
    std::vector<device_listener> device_removed_listener;    
};

