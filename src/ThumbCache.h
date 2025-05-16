#pragma once
#include "NTMU.h"
#include <initguid.h>
#include <emptyvc.h>

DEFINE_GUID(CLSID_EmptyVolumeCache, 0x889900C3, 0x59F3, 0x4C2F, 0xAE,0x21, 0xA4,0x09,0xEA,0x01,0xE6,0x05);

class CEmptyVolumeCacheCallBack : public IEmptyVolumeCacheCallBack
{
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override
	{
		if (riid == __uuidof(IUnknown) || riid == __uuidof(IEmptyVolumeCacheCallBack))
		{
			*ppvObject = static_cast<IEmptyVolumeCacheCallBack *>(this);
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef() override { return 2; /* On stack */ }
	STDMETHODIMP_(ULONG) Release() override { return 1; /* On stack */ }
	STDMETHODIMP ScanProgress(DWORDLONG dwlSpaceUsed, DWORD dwFlags, LPCWSTR pcwszStatus)
	{
		return S_OK;
	}
	STDMETHODIMP PurgeProgress(DWORDLONG dwlSpaceFreed, DWORDLONG dwlSpaceToFree, DWORD dwFlags, LPCWSTR pcwszStatus)
	{
		return S_OK;
	}
};