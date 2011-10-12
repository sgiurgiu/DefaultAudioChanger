#pragma once

#include <vector>

typedef struct _AudioDevice{
	LPWSTR deviceId;
	LPWSTR deviceName;
	HICON smallIcon;
	HICON largeIcon;
}AUDIODEVICE,*PAUDIODEVICE;

class CDevicesManager
{
public:
	CDevicesManager(void);
	~CDevicesManager(void);
	HRESULT InitializeDeviceEnumerator();
	const PAUDIODEVICE GetDefaultDevice();
	void ReleaseDeviceEnumerator();
	HRESULT LoadAudioDevices();
	const std::vector<AUDIODEVICE>* GetAudioDevices() const;
	HRESULT SwitchDevices(std::vector<LPWSTR> *ids);
	HRESULT SetDefaultDevice(LPWSTR id);
	HRESULT ClearAbsentDevices(PHKEY pkey);
private:
	UINT ExtractDeviceIcons(LPWSTR iconPath,HICON *iconLarge,HICON *iconSmall);
private:
	IMMDeviceEnumerator *pEnum;
	std::vector<AUDIODEVICE> devices;
	
};

