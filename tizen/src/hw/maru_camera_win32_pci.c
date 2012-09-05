/*
 * Implementation of MARU Virtual Camera device by PCI bus on Windows.
 *
 * Copyright (c) 2011 - 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Contact:
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */


#include "qemu-common.h"
#include "maru_camera_common.h"
#include "tizen/src/debug_ch.h"

#define CINTERFACE
#define COBJMACROS
#include "ocidl.h"
#include "errors.h"      /* for VFW_E_XXXX */
#include "mmsystem.h"    /* for MAKEFOURCC macro */
#include "maru_camera_win32_interface.h"

MULTI_DEBUG_CHANNEL(tizen, camera_win32);

extern int hax_enabled(void);

/*
 * COM Interface implementations
 *
 */

#define SAFE_RELEASE(x) \
    do { \
        if (x) { \
            (x)->lpVtbl->Release(x); \
            x = NULL; \
        } \
    } while(0)

typedef HRESULT (STDAPICALLTYPE *CallbackFn)(ULONG dwSize, BYTE *pBuffer);

/*
 * HWCGrabCallback
 */

typedef struct HWCGrabCallback
{
    IGrabCallback IGrabCallback_iface;
    long m_cRef;
    CallbackFn m_pCallback;
    STDMETHODIMP (*SetCallback)(IGrabCallback *iface, CallbackFn pCallbackFn);
} HWCGrabCallback;

static inline HWCGrabCallback *impl_from_IGrabCallback(IGrabCallback *iface)
{
    return CONTAINING_RECORD(iface, HWCGrabCallback, IGrabCallback_iface);
}

static STDMETHODIMP HWCGrabCallback_QueryInterface(IGrabCallback *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = (IUnknown*)iface;
    } else if (IsEqualIID(riid, &IID_IGrabCallback)) {
        *ppv = (IGrabCallback*)iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IGrabCallback_AddRef(iface);
    return S_OK;
}

static STDMETHODIMP_(ULONG) HWCGrabCallback_AddRef(IGrabCallback *iface)
{
    HWCGrabCallback *This = impl_from_IGrabCallback(iface);

    return InterlockedIncrement(&This->m_cRef);
}

static STDMETHODIMP_(ULONG) HWCGrabCallback_Release(IGrabCallback *iface)
{
    HWCGrabCallback *This = impl_from_IGrabCallback(iface);

    if (InterlockedDecrement(&This->m_cRef) == 0)
    {
        This->m_pCallback = NULL;
        g_free((void*)This);
        return 0;
    }

    return This->m_cRef;
}

static STDMETHODIMP HWCGrabCallback_Grab(IGrabCallback *iface, ULONG dwSize, BYTE *pBuffer)
{
    HWCGrabCallback *This = impl_from_IGrabCallback(iface);

    if (This->m_pCallback) {
        This->m_pCallback(dwSize, pBuffer);
        return S_OK;
    }

    return E_FAIL;
}

static STDMETHODIMP HWCGrabCallback_SetCallback(IGrabCallback *iface, CallbackFn pCallbackFn)
{
    HWCGrabCallback *This = impl_from_IGrabCallback(iface);

    This->m_pCallback = pCallbackFn;
    return S_OK;
}

static IGrabCallbackVtbl HWCGrabCallback_Vtbl =
{
        HWCGrabCallback_QueryInterface,
        HWCGrabCallback_AddRef,
        HWCGrabCallback_Release,
        HWCGrabCallback_Grab
};

static STDMETHODIMP HWCGrabCallback_Construct(IGrabCallback **ppv)
{
    HWCGrabCallback *This = (HWCGrabCallback *)g_malloc0(sizeof(HWCGrabCallback));

    if (!This) {
        ERR("failed to HWCGrabCallback_Construct, E_OUTOFMEMORY\n");
        return E_OUTOFMEMORY;
    }

    This->IGrabCallback_iface.lpVtbl = &HWCGrabCallback_Vtbl;
    This->m_cRef = 1;
    This->m_pCallback = NULL;
    This->SetCallback = HWCGrabCallback_SetCallback;
    *ppv = &This->IGrabCallback_iface;
    return S_OK;
}

/*
 * HWCPin
 */

typedef struct HWCInPin
{
    IPin IPin_iface;
    IMemInputPin IMemInputPin_iface;
    IBaseFilter *m_pCFilter;
    IPin *m_pConnectedPin;
    IGrabCallback *m_pCallback;
    BOOL m_bReadOnly;
    long m_cRef;
    STDMETHODIMP (*SetGrabCallbackIF)(IPin *iface, IGrabCallback *pCaptureCB);
} HWCInPin;

static inline HWCInPin *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, HWCInPin, IPin_iface);
}

static inline HWCInPin *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, HWCInPin, IMemInputPin_iface);
}

static STDMETHODIMP HWCPin_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    HWCInPin *This = impl_from_IPin(iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin)) {
        *ppv = &This->IPin_iface;
        IPin_AddRef((IPin*)*ppv);
    } else if (IsEqualIID(riid, &IID_IMemInputPin)) {
        *ppv = &This->IMemInputPin_iface;
        IPin_AddRef((IMemInputPin*)*ppv);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

static STDMETHODIMP_(ULONG) HWCPin_AddRef(IPin *iface)
{
    HWCInPin *This = impl_from_IPin(iface);

    return InterlockedIncrement(&This->m_cRef);
}

static STDMETHODIMP_(ULONG) HWCPin_Release(IPin *iface)
{
    HWCInPin *This = impl_from_IPin(iface);

    if (InterlockedDecrement(&This->m_cRef) == 0)
    {
        if (This->m_pCallback) {
            SAFE_RELEASE(This->m_pCallback);
        }
        if (This->m_pConnectedPin) {
            SAFE_RELEASE(This->m_pConnectedPin);
        }
        g_free((void*)This);
        return 0;
    }
    return This->m_cRef;
}

static STDMETHODIMP HWCPin_Connect(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    if (!pmt)
        return S_OK;
    return S_FALSE;
}

static STDMETHODIMP HWCPin_ReceiveConnection(IPin *iface, IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
    PIN_DIRECTION pd;
    FILTER_STATE fs;
    HWCInPin *This = impl_from_IPin(iface);

    if (pConnector == NULL || pmt == NULL)
        return E_POINTER;

    if (This->m_pConnectedPin) {
        return VFW_E_ALREADY_CONNECTED;
    }
    IBaseFilter_GetState(This->m_pCFilter, 0, &fs);
    if (fs != State_Stopped) {
        return VFW_E_NOT_STOPPED;
    }
    IPin_QueryDirection(pConnector, &pd);
    if (pd == PINDIR_INPUT) {
        return VFW_E_INVALID_DIRECTION;
    }

    This->m_pConnectedPin = pConnector;
    IPin_AddRef(This->m_pConnectedPin);
    return S_OK;
}

static STDMETHODIMP HWCPin_Disconnect(IPin *iface)
{
    HWCInPin *This = impl_from_IPin(iface);

    HRESULT hr;
    if (This->m_pConnectedPin == NULL) {
        hr = S_FALSE;
    } else {
        SAFE_RELEASE(This->m_pConnectedPin);
        hr = S_OK;
    }
    return hr;
}

static STDMETHODIMP HWCPin_ConnectedTo(IPin *iface, IPin **pPin)
{
    HWCInPin *This = impl_from_IPin(iface);

    if (pPin == NULL)
        return E_POINTER;

    if (This->m_pConnectedPin == NULL) {
        return VFW_E_NOT_CONNECTED;
    } else {
        *pPin = This->m_pConnectedPin;
        IPin_AddRef(This->m_pConnectedPin);
    }
    return S_OK;
}

static STDMETHODIMP HWCPin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    if (pmt == NULL) {
        return E_POINTER;
    }
    return VFW_E_NOT_CONNECTED;
}

static STDMETHODIMP HWCPin_QueryPinInfo(IPin *iface, PIN_INFO *pInfo)
{
    HWCInPin *This = impl_from_IPin(iface);

    if (pInfo == NULL)
        return E_POINTER;

    pInfo->pFilter = This->m_pCFilter;
    if (This->m_pCFilter) {
        IBaseFilter_AddRef(This->m_pCFilter);
    }
    memcpy((void*)pInfo->achName, (void*)HWCPinName, sizeof(HWCPinName));
    pInfo->dir = PINDIR_INPUT;
    return S_OK;
}

static STDMETHODIMP HWCPin_QueryDirection(IPin *iface, PIN_DIRECTION *pPinDir)
{
    if (pPinDir == NULL)
        return E_POINTER;
    *pPinDir = PINDIR_INPUT;
    return S_OK;
}

static STDMETHODIMP HWCPin_QueryId(IPin *iface, LPWSTR *Id)
{
    PVOID pId;
    if (Id == NULL)
        return E_POINTER;
    pId = CoTaskMemAlloc(sizeof(HWCPinName));
    memcpy((void*)pId, (void*)HWCPinName, sizeof(HWCPinName));
    *Id = (LPWSTR)pId;
    return S_OK;
}

static STDMETHODIMP HWCPin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    if (pmt == NULL)
        return E_POINTER;
    return S_OK;
}

static STDMETHODIMP HWCPin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    if (ppEnum == NULL)
            return E_POINTER;
    return E_NOTIMPL;
}

static STDMETHODIMP HWCPin_QueryInternalConnections(IPin *iface, IPin **ppPin, ULONG *nPin)
{
    return E_NOTIMPL;
}

static STDMETHODIMP HWCPin_EndOfStream(IPin *iface)
{
    return S_OK;
}

static STDMETHODIMP HWCPin_BeginFlush(IPin *iface)
{
    return S_OK;
}

static STDMETHODIMP HWCPin_EndFlush(IPin *iface)
{
    return S_OK;
}

static STDMETHODIMP HWCPin_NewSegment(IPin *iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    return S_OK;
}

static STDMETHODIMP HWCMemInputPin_QueryInterface(IMemInputPin *iface, REFIID riid, void **ppv)
{
    HWCInPin *This = impl_from_IMemInputPin(iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMemInputPin)) {
        *ppv = &This->IMemInputPin_iface;
        IMemInputPin_AddRef((IMemInputPin*)*ppv);
    } else if (IsEqualIID(riid, &IID_IPin)) {
        *ppv = &This->IPin_iface;
        IPin_AddRef((IPin*)*ppv);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}

static STDMETHODIMP_(ULONG) HWCMemInputPin_AddRef(IMemInputPin *iface)
{
    HWCInPin *This = impl_from_IMemInputPin(iface);

    return InterlockedIncrement(&This->m_cRef);
}

static STDMETHODIMP_(ULONG) HWCMemInputPin_Release(IMemInputPin *iface)
{
    HWCInPin *This = impl_from_IMemInputPin(iface);

    if (InterlockedDecrement(&This->m_cRef) == 0)
    {
        if (This->m_pCallback) {
            SAFE_RELEASE(This->m_pCallback);
        }
        if (This->m_pConnectedPin) {
            SAFE_RELEASE(This->m_pConnectedPin);
        }
        g_free((void*)This);
        return 0;
    }
    return This->m_cRef;
}

static STDMETHODIMP HWCMemInputPin_GetAllocator(IMemInputPin *iface, IMemAllocator **ppAllocator)
{
    if (ppAllocator == NULL)
        return E_POINTER;
    return VFW_E_NO_ALLOCATOR;
}

static STDMETHODIMP HWCMemInputPin_NotifyAllocator(IMemInputPin *iface, IMemAllocator *pAllocator, BOOL bReadOnly)
{
    if (pAllocator == NULL)
        return E_POINTER;

    return NOERROR;
}

static STDMETHODIMP HWCMemInputPin_GetAllocatorRequirements(IMemInputPin *iface, ALLOCATOR_PROPERTIES *pProps)
{
    return E_NOTIMPL;
}

static STDMETHODIMP HWCMemInputPin_Receive(IMemInputPin *iface, IMediaSample *pSample)
{
    HWCInPin *This = impl_from_IMemInputPin(iface);

    if (pSample == NULL) {
        ERR("pSample is NULL\n");
        return E_POINTER;
    }
    if (This->m_pCallback != NULL) {
        HRESULT hr;
        BYTE* pBuffer = NULL;
        DWORD dwSize = 0;
        dwSize = IMediaSample_GetSize(pSample);
        hr = IMediaSample_GetPointer(pSample, &pBuffer);
        if (FAILED(hr)) {
            ERR("Receive function : "
                "failed to IMediaSample_GetPointer, 0x%ld\n", hr);
            return hr;
        }
        hr = IGrabCallback_Grab(This->m_pCallback, dwSize, pBuffer);
        if (FAILED(hr)) {
            ERR("Receive function : failed to IGrabCallback_Grab, 0x%ld\n", hr);
            return hr;
        }
    }
    return S_OK;
}

static STDMETHODIMP HWCMemInputPin_ReceiveMultiple(IMemInputPin *iface, IMediaSample **pSamples, long nSamples, long *nSamplesProcessed)
{
    HRESULT hr = S_OK;

    if (pSamples == NULL)
        return E_POINTER;

    *nSamplesProcessed = 0;

    while (nSamples-- > 0)
    {
        hr = IMemInputPin_Receive(iface, pSamples[*nSamplesProcessed]);
        if (hr != S_OK)
            break;
        (*nSamplesProcessed)++;
    }
    return hr;
}

static STDMETHODIMP HWCMemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    return S_FALSE;
}

static STDMETHODIMP HWCPin_SetCallback(IPin *iface, IGrabCallback *pCaptureCB)
{
    HWCInPin *This = impl_from_IPin(iface);

    if (pCaptureCB == NULL) {
        SAFE_RELEASE(This->m_pCallback);
    } else {
        This->m_pCallback = pCaptureCB;
        IGrabCallback_AddRef(This->m_pCallback);
    }

    return S_OK;
}


static IPinVtbl HWCPin_Vtbl =
{
    HWCPin_QueryInterface,
    HWCPin_AddRef,
    HWCPin_Release,
    HWCPin_Connect,
    HWCPin_ReceiveConnection,
    HWCPin_Disconnect,
    HWCPin_ConnectedTo,
    HWCPin_ConnectionMediaType,
    HWCPin_QueryPinInfo,
    HWCPin_QueryDirection,
    HWCPin_QueryId,
    HWCPin_QueryAccept,
    HWCPin_EnumMediaTypes,
    HWCPin_QueryInternalConnections,
    HWCPin_EndOfStream,
    HWCPin_BeginFlush,
    HWCPin_EndFlush,
    HWCPin_NewSegment
};

static IMemInputPinVtbl HWCMemInputPin_Vtbl =
{
    HWCMemInputPin_QueryInterface,
    HWCMemInputPin_AddRef,
    HWCMemInputPin_Release,
    HWCMemInputPin_GetAllocator,
    HWCMemInputPin_NotifyAllocator,
    HWCMemInputPin_GetAllocatorRequirements,
    HWCMemInputPin_Receive,
    HWCMemInputPin_ReceiveMultiple,
    HWCMemInputPin_ReceiveCanBlock
};

static STDMETHODIMP HWCInPin_Construct(IBaseFilter *pFilter, IPin **ppv)
{
    HWCInPin *This = (HWCInPin *)g_malloc0(sizeof(HWCInPin));

    if (!This) {
        return E_OUTOFMEMORY;
    }

    This->IPin_iface.lpVtbl = &HWCPin_Vtbl;
    This->IMemInputPin_iface.lpVtbl = &HWCMemInputPin_Vtbl;
    This->m_bReadOnly = FALSE;
    This->m_pCFilter = pFilter;
    This->m_pConnectedPin = NULL;
    This->m_pCallback = NULL;
    This->m_cRef = 1;
    This->SetGrabCallbackIF = HWCPin_SetCallback;
    *ppv = &This->IPin_iface;

    return S_OK;
}

/*
 * HWCEnumPins
 */

typedef struct HWCEnumPins
{
    IEnumPins IEnumPins_iface;
    IBaseFilter *m_pFilter;
    int m_nPos;
    long m_cRef;
} HWCEnumPins;

static inline HWCEnumPins *impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, HWCEnumPins, IEnumPins_iface);
}

static STDMETHODIMP HWCEnumPins_QueryInterface(IEnumPins *iface, REFIID riid, void **ppv)
{
    if (ppv == NULL)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IEnumPins)) {
        *ppv = iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IEnumPins_AddRef(iface);
    return S_OK;
}

static STDMETHODIMP_(ULONG) HWCEnumPins_AddRef(IEnumPins *iface)
{
    HWCEnumPins *This = impl_from_IEnumPins(iface);

    return InterlockedIncrement(&This->m_cRef);
}

static STDMETHODIMP_(ULONG) HWCEnumPins_Release(IEnumPins *iface)
{
    HWCEnumPins *This = impl_from_IEnumPins(iface);

    if (InterlockedDecrement(&This->m_cRef) == 0) {
        if (This->m_pFilter) {
            SAFE_RELEASE(This->m_pFilter);
        }
        This->m_nPos = 0;
        g_free((void*)This);
        return 0;
    }
    return This->m_cRef;
}

static STDMETHODIMP HWCEnumPins_Next(IEnumPins *iface, ULONG cPins, IPin **ppPins,
                                ULONG *pcFetched)
{
    ULONG fetched;
    HWCEnumPins *This = impl_from_IEnumPins(iface);

    if (ppPins == NULL)
            return E_POINTER;

    if (This->m_nPos < 1 && cPins > 0) {
        IPin *pPin;
        IBaseFilter_FindPin(This->m_pFilter, HWCPinName, &pPin);
        *ppPins = pPin;
        fetched = 1;
        This->m_nPos++;
    } else {
        fetched = 0;
    }

    if (pcFetched != NULL) {
        *pcFetched = fetched;
    }

    return (fetched == cPins) ? S_OK : S_FALSE;
}

static STDMETHODIMP HWCEnumPins_Skip(IEnumPins *iface, ULONG cPins)
{
    HWCEnumPins *This = impl_from_IEnumPins(iface);
    This->m_nPos += cPins;
    return (This->m_nPos >= 1) ? S_FALSE : S_OK;
}

static STDMETHODIMP HWCEnumPins_Reset(IEnumPins *iface)
{
    HWCEnumPins *This = impl_from_IEnumPins(iface);
    This->m_nPos = 0;
    return S_OK;
}

static STDMETHODIMP HWCEnumPins_Construct(IBaseFilter *pFilter, int nPos, IEnumPins **ppv);

static STDMETHODIMP HWCEnumPins_Clone(IEnumPins *iface, IEnumPins **ppEnum)
{
    HWCEnumPins *This = impl_from_IEnumPins(iface);

    if (ppEnum == NULL)
        return E_POINTER;

    HWCEnumPins_Construct(This->m_pFilter, This->m_nPos, ppEnum);
    if (*ppEnum == NULL) {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

static IEnumPinsVtbl HWCEnumPins_Vtbl =
{
    HWCEnumPins_QueryInterface,
    HWCEnumPins_AddRef,
    HWCEnumPins_Release,
    HWCEnumPins_Next,
    HWCEnumPins_Skip,
    HWCEnumPins_Reset,
    HWCEnumPins_Clone
};


static STDMETHODIMP HWCEnumPins_Construct(IBaseFilter *pFilter, int nPos, IEnumPins **ppv)
{
    HWCEnumPins *This = (HWCEnumPins *)g_malloc0(sizeof(HWCEnumPins));

    if (!This) {
        return E_OUTOFMEMORY;
    }

    This->IEnumPins_iface.lpVtbl = &HWCEnumPins_Vtbl;
    This->m_pFilter = pFilter;
    if (This->m_pFilter) {
        IBaseFilter_AddRef(This->m_pFilter);
    }
    This->m_cRef = 1;
    This->m_nPos = nPos;
    *ppv = &This->IEnumPins_iface;

    return S_OK;
}

/*
 * HWCFilter
 */

typedef struct HWCFilter
{
    IBaseFilter IBaseFilter_iface;
    IPin *m_pPin;
    IFilterGraph *m_pFilterGraph;
    FILTER_STATE m_state;
    long m_cRef;
} HWCFilter;

static inline HWCFilter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, HWCFilter, IBaseFilter_iface);
}

static STDMETHODIMP HWCFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = (IUnknown*)iface;
    } else if (IsEqualIID(riid, &IID_IPersist)) {
        *ppv = (IPersist*)iface;
    } else if (IsEqualIID(riid, &IID_IMediaFilter)) {
        *ppv = (IMediaFilter*)iface;
    } else if (IsEqualIID(riid, &IID_IBaseFilter)) {
        *ppv = (IBaseFilter*)iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IBaseFilter_AddRef(iface);
    return S_OK;
}

static STDMETHODIMP_(ULONG) HWCFilter_AddRef(IBaseFilter *iface)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);

    return InterlockedIncrement(&This->m_cRef);
}

static STDMETHODIMP_(ULONG) HWCFilter_Release(IBaseFilter *iface)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);

    if (InterlockedDecrement(&This->m_cRef) == 0) {
        if (This->m_pPin) {
            SAFE_RELEASE(This->m_pPin);
        }
        g_free((void*)This);
        return 0;
    }
    return This->m_cRef;
}

static STDMETHODIMP HWCFilter_GetClassID(IBaseFilter *iface, CLSID *pClsID)
{
    if (pClsID == NULL)
        return E_POINTER;
    return E_NOTIMPL;
}

static STDMETHODIMP HWCFilter_GetState(IBaseFilter *iface, DWORD dwMSecs, FILTER_STATE *State)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);
    *State = This->m_state;
    return S_OK;
}

static STDMETHODIMP HWCFilter_SetSyncSource(IBaseFilter *iface, IReferenceClock *pClock)
{
    return S_OK;
}

static STDMETHODIMP HWCFilter_GetSyncSource(IBaseFilter *iface, IReferenceClock **pClock)
{
    *pClock = NULL;
    return S_OK;
}

static STDMETHODIMP HWCFilter_Stop(IBaseFilter *iface)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);

    IPin_EndFlush(This->m_pPin);
    This->m_state = State_Stopped;
    return S_OK;
}

static STDMETHODIMP HWCFilter_Pause(IBaseFilter *iface)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);
    This->m_state = State_Paused;
    return S_OK;
}

static STDMETHODIMP HWCFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);

    if (This->m_state == State_Stopped){
        HRESULT hr;
        hr = IBaseFilter_Pause(iface);
        if (FAILED(hr)) {
            ERR("HWCFilter_Run : Failed to IBaseFilter_Pause, ret=0xld%\n", hr);
            return hr;
        }
    }

    This->m_state = State_Running;
    return S_OK;
}

static STDMETHODIMP HWCFilter_EnumPins(IBaseFilter *iface, IEnumPins **ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;

    HWCEnumPins_Construct(iface, 0, ppEnum);
    return *ppEnum == NULL ? E_OUTOFMEMORY : S_OK;
}

static STDMETHODIMP HWCFilter_FindPin(IBaseFilter *iface, LPCWSTR Id, IPin **ppPin)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);

    if (ppPin == NULL)
        return E_POINTER;

    if (memcmp((void*)Id, (void*)HWCPinName, sizeof(HWCPinName))) {
        return VFW_E_NOT_FOUND;
    }

    if (!This->m_pPin) {
         HWCInPin_Construct(iface, &This->m_pPin);
    }
    *ppPin = This->m_pPin;

    IPin_AddRef(This->m_pPin);
    return S_OK;
}

static STDMETHODIMP HWCFilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *pInfo)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);

    if (pInfo == NULL)
        return E_POINTER;

    memcpy((void*)pInfo->achName, (void*)HWCFilterName, sizeof(HWCFilterName));
    pInfo->pGraph = This->m_pFilterGraph;
    if (This->m_pFilterGraph) {
        IFilterGraph_AddRef(This->m_pFilterGraph);
    }
    return S_OK;
}

static STDMETHODIMP HWCFilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *pGraph,
                                        LPCWSTR pName)
{
    HWCFilter *This = impl_from_IBaseFilter(iface);

    This->m_pFilterGraph = pGraph;
    return S_OK;
}

static STDMETHODIMP HWCFilter_QueryVendorInfo(IBaseFilter *iface, LPWSTR* pVendorInfo)
{
    return E_NOTIMPL;
}

static IBaseFilterVtbl HWCFilter_Vtbl =
{
    HWCFilter_QueryInterface,
    HWCFilter_AddRef,
    HWCFilter_Release,
    HWCFilter_GetClassID,
    HWCFilter_Stop,
    HWCFilter_Pause,
    HWCFilter_Run,
    HWCFilter_GetState,
    HWCFilter_SetSyncSource,
    HWCFilter_GetSyncSource,
    HWCFilter_EnumPins,
    HWCFilter_FindPin,
    HWCFilter_QueryFilterInfo,
    HWCFilter_JoinFilterGraph,
    HWCFilter_QueryVendorInfo
};

static STDMETHODIMP HWCFilter_Construct(IBaseFilter **ppv)
{
    HWCFilter *This = (HWCFilter *)g_malloc0(sizeof(HWCFilter));

    if (!This) {
        return E_OUTOFMEMORY;
    }

    This->IBaseFilter_iface.lpVtbl = &HWCFilter_Vtbl;
    This->m_pFilterGraph = NULL;
    This->m_state = State_Stopped;
    This->m_cRef = 1;
    HWCInPin_Construct(&This->IBaseFilter_iface, &This->m_pPin);
    *ppv = &This->IBaseFilter_iface;

    return S_OK;
}

/**********************************************************
 *
 * Virtual device implementations
 *
 **********************************************************/


/*
 * Declaration global variables for Win32 COM Interfaces
 */
IGraphBuilder *g_pGB ;
ICaptureGraphBuilder2 *g_pCGB;
IMediaControl *g_pMediaControl;

IPin *g_pOutputPin;
IPin *g_pInputPin;
IBaseFilter *g_pDstFilter;
IBaseFilter *g_pSrcFilter;

IGrabCallback *g_pCallback;

/* V4L2 defines copy from videodev2.h */
#define V4L2_CTRL_FLAG_SLIDER       0x0020

#define V4L2_CTRL_CLASS_USER        0x00980000
#define V4L2_CID_BASE               (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_BRIGHTNESS         (V4L2_CID_BASE+0)
#define V4L2_CID_CONTRAST           (V4L2_CID_BASE+1)
#define V4L2_CID_SATURATION         (V4L2_CID_BASE+2)
#define V4L2_CID_SHARPNESS          (V4L2_CID_BASE+27)

#define V4L2_PIX_FMT_YUYV    MAKEFOURCC('Y', 'U', 'Y', 'V') /* 16  YUV 4:2:2 */
#define V4L2_PIX_FMT_YUV420  MAKEFOURCC('Y', 'U', '1', '2') /* 12  YUV 4:2:0 */
#define V4L2_PIX_FMT_YVU420  MAKEFOURCC('Y', 'V', '1', '2') /* 12  YVU 4:2:0 */

typedef struct tagMaruCamConvertPixfmt {
    uint32_t fmt;   /* fourcc */
    uint32_t bpp;   /* bits per pixel, 0 for compressed formats */
    uint32_t needs_conversion;
} MaruCamConvertPixfmt;

static MaruCamConvertPixfmt supported_dst_pixfmts[] = {
        { V4L2_PIX_FMT_YUYV, 16, 0 },
        { V4L2_PIX_FMT_YUV420, 12, 0 },
        { V4L2_PIX_FMT_YVU420, 12, 0 },
};

typedef struct tagMaruCamConvertFrameInfo {
    uint32_t width;
    uint32_t height;
} MaruCamConvertFrameInfo;

static MaruCamConvertFrameInfo supported_dst_frames[] = {
        { 640, 480 },
        { 352, 288 },
        { 320, 240 },
        { 176, 144 },
        { 160, 120 },
};

#define MARUCAM_CTRL_VALUE_MAX      20
#define MARUCAM_CTRL_VALUE_MIN      1
#define MARUCAM_CTRL_VALUE_MID      10
#define MARUCAM_CTRL_VALUE_STEP     1

struct marucam_qctrl {
    uint32_t id;
    uint32_t hit;
    long min;
    long max;
    long step;
    long init_val;
};

static struct marucam_qctrl qctrl_tbl[] = {
    { V4L2_CID_BRIGHTNESS, 0, },
    { V4L2_CID_CONTRAST, 0, },
    { V4L2_CID_SATURATION,0, },
    { V4L2_CID_SHARPNESS, 0, },
};

static MaruCamState *g_state = NULL;

static uint32_t ready_count = 0;
static uint32_t cur_fmt_idx = 0;
static uint32_t cur_frame_idx = 0;


/*
 * Helper functions - converting image formats, converting values
 */

static uint32_t get_bytesperline(uint32_t pixfmt, uint32_t width)
{
    uint32_t bytesperline;

    switch (pixfmt) {
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
        bytesperline = (width * 12) >> 3;
        break;
    case V4L2_PIX_FMT_YUYV:
    default:
        bytesperline = width * 2;
        break;
    }

    return bytesperline;
}

static uint32_t get_sizeimage(uint32_t pixfmt, uint32_t width, uint32_t height)
{
    return (get_bytesperline(pixfmt, width) * height);
}

void v4lconvert_yuyv_to_yuv420(const unsigned char *src, unsigned char *dest,
        uint32_t width, uint32_t height, uint32_t yvu);


static long value_convert_from_guest(long min, long max, long value)
{
    double rate = 0.0;
    long dist = 0, ret = 0;

    dist = max - min;

    if (dist < MARUCAM_CTRL_VALUE_MAX) {
        rate = (double)MARUCAM_CTRL_VALUE_MAX / (double)dist;
        ret = min + (int32_t)(value / rate);
    } else {
        rate = (double)dist / (double)MARUCAM_CTRL_VALUE_MAX;
        ret = min + (int32_t)(rate * value);
    }
    return ret;
}

static long value_convert_to_guest(long min, long max, long value)
{
    double rate  = 0.0;
    long dist = 0, ret = 0;

    dist = max - min;

    if (dist < MARUCAM_CTRL_VALUE_MAX) {
        rate = (double)MARUCAM_CTRL_VALUE_MAX / (double)dist;
        ret = (int32_t)((double)(value - min) * rate);
    } else {
        rate = (double)dist / (double)MARUCAM_CTRL_VALUE_MAX;
        ret = (int32_t)((double)(value - min) / rate);
    }

    return ret;
}

/*
 * Callback function for grab frames
 */
static STDMETHODIMP marucam_device_callbackfn(ULONG dwSize, BYTE *pBuffer)
{
    void *tmp_buf;
    uint32_t width, height;

    qemu_mutex_lock(&g_state->thread_mutex);
    if (g_state->req_frame == 0) {
        qemu_mutex_unlock(&g_state->thread_mutex);
        return S_OK;
    }
    tmp_buf = g_state->vaddr + g_state->buf_size * (g_state->req_frame - 1);
    qemu_mutex_unlock(&g_state->thread_mutex);

    width = supported_dst_frames[cur_frame_idx].width;
    height = supported_dst_frames[cur_frame_idx].height;

    switch (supported_dst_pixfmts[cur_fmt_idx].fmt) {
    case V4L2_PIX_FMT_YUV420:
        v4lconvert_yuyv_to_yuv420(pBuffer, tmp_buf, width, height, 0);
        break;
    case V4L2_PIX_FMT_YVU420:
        v4lconvert_yuyv_to_yuv420(pBuffer, tmp_buf, width, height, 1);
        break;
    case V4L2_PIX_FMT_YUYV:
        memcpy(tmp_buf, (void*)pBuffer, (size_t)dwSize);
        break;
    }

    qemu_mutex_lock(&g_state->thread_mutex);
    if (ready_count < MARUCAM_SKIPFRAMES) {
        ++ready_count; /* skip a frame cause first some frame are distorted */
        qemu_mutex_unlock(&g_state->thread_mutex);
        return S_OK;
    }
    g_state->req_frame = 0; /* clear request */
    g_state->isr |= 0x01; /* set a flag raising a interrupt. */
    qemu_bh_schedule(g_state->tx_bh);
    qemu_mutex_unlock(&g_state->thread_mutex);
    return S_OK;
}

/*
 * Internal functions for manipulate interfaces
 */

static STDMETHODIMP_(void) CloseInterfaces(void)
{
    if (g_pMediaControl)
        g_pMediaControl->lpVtbl->Stop(g_pMediaControl);

    if (g_pOutputPin)
        g_pOutputPin->lpVtbl->Disconnect(g_pOutputPin);

    SAFE_RELEASE(g_pGB);
    SAFE_RELEASE(g_pCGB);
    SAFE_RELEASE(g_pMediaControl);
    SAFE_RELEASE(g_pOutputPin);
    SAFE_RELEASE(g_pInputPin);
    SAFE_RELEASE(g_pDstFilter);
    SAFE_RELEASE(g_pSrcFilter);
    SAFE_RELEASE(g_pCallback);
}

static STDMETHODIMP_(void) DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt == NULL) {
        return;
    }

    if (pmt->cbFormat != 0) {
        CoTaskMemFree((PVOID)pmt->pbFormat);
        pmt->cbFormat = 0;
        pmt->pbFormat = NULL;
    }
    if (pmt->pUnk != NULL) {
        pmt->pUnk->lpVtbl->Release(pmt->pUnk);
        pmt->pUnk = NULL;
    }

    CoTaskMemFree((PVOID)pmt);
}

static STDMETHODIMP GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    HRESULT hr;
    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;

    if (ppPin == NULL)
    {
        return E_POINTER;
    }

    hr = pFilter->lpVtbl->EnumPins(pFilter, &pEnum);
    if (FAILED(hr))
        return hr;

    while(pEnum->lpVtbl->Next(pEnum, 1, &pPin, 0) == S_OK)
    {
        PIN_DIRECTION PinDirThis;
        hr = pPin->lpVtbl->QueryDirection(pPin, &PinDirThis);
        if (FAILED(hr))
        {
            SAFE_RELEASE(pPin);
            SAFE_RELEASE(pEnum);
            return hr;
        }
        if (PinDir == PinDirThis)
        {
            *ppPin = pPin;
            SAFE_RELEASE(pEnum);
            return S_OK;
        }
        SAFE_RELEASE(pPin);
    }

    SAFE_RELEASE(pEnum);
    return S_FALSE;
}

static STDMETHODIMP GraphBuilder_Init(void)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC, &IID_IGraphBuilder, (void**)&g_pGB);
    if (FAILED(hr)) {
        ERR("Failed to create instance of GraphBuilder, 0x%x\n", hr);
        return hr;
    }

    hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, &IID_ICaptureGraphBuilder2, (void**)&g_pCGB);
    if (FAILED(hr)) {
        ERR("Failed to create instance of CaptureGraphBuilder2, 0x%x\n", hr);
        return hr;
    }

    hr = g_pCGB->lpVtbl->SetFiltergraph(g_pCGB, g_pGB);
    if (FAILED(hr)) {
        ERR("Failed to SetFiltergraph, 0x%x\n", hr);
        return hr;
    }

    hr = g_pGB->lpVtbl->QueryInterface(g_pGB, &IID_IMediaControl, (void **)&g_pMediaControl);
    if (FAILED(hr)) {
        ERR("Failed to QueryInterface for IMediaControl, 0x%x\n", hr);
        return hr;
    }

    hr = HWCGrabCallback_Construct(&g_pCallback);
    if (g_pCallback == NULL)
        hr = E_OUTOFMEMORY;

    hr = ((HWCGrabCallback*)g_pCallback)->SetCallback(g_pCallback, (CallbackFn)marucam_device_callbackfn);

    return hr;
}

static STDMETHODIMP BindSourceFilter(void)
{
    HRESULT hr;
    ICreateDevEnum *pCreateDevEnum = NULL;
    IEnumMoniker *pEnumMK = NULL;
    IMoniker *pMoniKer;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, &IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr)) {
        ERR("Failed to create instance of CreateDevEnum, 0x%x\n", hr);
        return hr;
    }

    hr = pCreateDevEnum->lpVtbl->CreateClassEnumerator(pCreateDevEnum, &CLSID_VideoInputDeviceCategory, &pEnumMK, 0);
    if (FAILED(hr)) {
        ERR("Failed to get VideoInputDeviceCategory, 0x%x\n", hr);
        SAFE_RELEASE(pCreateDevEnum);
        return hr;
    }

    if (!pEnumMK)
    {
        ERR("ClassEnumerator moniker is NULL\n");
        SAFE_RELEASE(pCreateDevEnum);
        return E_FAIL;
    }
    pEnumMK->lpVtbl->Reset(pEnumMK);

    hr = pEnumMK->lpVtbl->Next(pEnumMK, 1, &pMoniKer, NULL);
    if (hr == S_FALSE)
    {
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        IPropertyBag *pBag = NULL;
        hr = pMoniKer->lpVtbl->BindToStorage(pMoniKer, 0, 0, &IID_IPropertyBag, (void **)&pBag);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->lpVtbl->Read(pBag, L"FriendlyName", &var, NULL);
            if (hr == NOERROR)
            {
                hr = pMoniKer->lpVtbl->BindToObject(pMoniKer, NULL, NULL, &IID_IBaseFilter, (void**)&g_pSrcFilter);
                if (FAILED(hr))
                {
                    ERR("Counldn't bind moniker to filter object!!\n");
                }
                else
                {
                    g_pSrcFilter->lpVtbl->AddRef(g_pSrcFilter);
                }
                SysFreeString(var.bstrVal);
            }
            SAFE_RELEASE(pBag);
        }
        SAFE_RELEASE(pMoniKer);
    }

    if (SUCCEEDED(hr))
    {
        hr = g_pGB->lpVtbl->AddFilter(g_pGB, g_pSrcFilter, L"Video Capture");
        if (hr != S_OK && hr != S_FALSE)
        {
            ERR("Counldn't add Video Capture filter to our graph!\n");
            SAFE_RELEASE(g_pSrcFilter);
        }
    }
    SAFE_RELEASE(pEnumMK);
    SAFE_RELEASE(pCreateDevEnum);

    return hr;
}

static STDMETHODIMP BindTargetFilter(void)
{
    HRESULT hr;
    hr = HWCFilter_Construct(&g_pDstFilter);

    if (SUCCEEDED(hr) && g_pDstFilter)
    {
        hr = g_pGB->lpVtbl->AddFilter(g_pGB, g_pDstFilter, L"HWCFilter");
        if (FAILED(hr))
        {
            ERR("Counldn't add HWCFilterr to our graph!\n");
            SAFE_RELEASE(g_pDstFilter);
        }
    }
    return hr;
}

static STDMETHODIMP ConnectFilters(void)
{
    HRESULT hr;

    hr = GetPin(g_pSrcFilter, PINDIR_OUTPUT , &g_pOutputPin);
    if (FAILED(hr)) {
        ERR("Failed to get output pin. 0x%x\n", hr);
        return hr;
    }

    hr = GetPin(g_pDstFilter, PINDIR_INPUT , &g_pInputPin);
    if (FAILED(hr)) {
        ERR("Failed to get input pin. 0x%x\n", hr);
        return hr;
    }

    hr = g_pGB->lpVtbl->Connect(g_pGB, g_pOutputPin, g_pInputPin);
    if (FAILED(hr)) {
        ERR("Failed to connect pins. 0x%x\n", hr);
    }
    return hr;
}

static STDMETHODIMP DisconnectPins(void)
{
    HRESULT hr;

    hr = g_pGB->lpVtbl->Disconnect(g_pGB, g_pOutputPin);
    if (FAILED(hr)) {
        ERR("Failed to disconnect output pin. 0x%x\n", hr);
        return hr;
    }

    hr = g_pGB->lpVtbl->Disconnect(g_pGB, g_pInputPin);
    if (FAILED(hr)) {
        ERR("Failed to disconnect input pin. 0x%x\n", hr);
    }

    return hr;
}

static STDMETHODIMP RemoveFilters(void)
{
    HRESULT hr;

    hr = g_pGB->lpVtbl->RemoveFilter(g_pGB, g_pSrcFilter);
    if (FAILED(hr)) {
        ERR("Failed to remove source filer. 0x%x\n", hr);
        return hr;
    }

    hr = g_pGB->lpVtbl->RemoveFilter(g_pGB, g_pDstFilter);
    if (FAILED(hr)) {
        ERR("Failed to remove destination filer. 0x%x\n", hr);
    }

    return hr;
}

/* default fps is 15 */
#define MARUCAM_DEFAULT_FRAMEINTERVAL    666666

static STDMETHODIMP SetFormat(uint32_t dwWidth, uint32_t dwHeight,
                              uint32_t dwFourcc)
{
    HRESULT hr;
    IAMStreamConfig *pSConfig;
    int iCount = 0, iSize = 0;

    if (dwFourcc == 0) {
        dwFourcc = MAKEFOURCC('Y','U','Y','2');
    }

    hr = g_pCGB->lpVtbl->FindInterface(g_pCGB, &PIN_CATEGORY_CAPTURE, 0,
                                       g_pSrcFilter, &IID_IAMStreamConfig,
                                       (void**)&pSConfig);
    if (FAILED(hr)) {
        ERR("failed to FindInterface method\n");
        return hr;
    }

    hr = pSConfig->lpVtbl->GetNumberOfCapabilities(pSConfig, &iCount, &iSize);
    if (FAILED(hr))
    {
        ERR("failed to GetNumberOfCapabilities method\n");
        SAFE_RELEASE(pSConfig);
        return hr;
    }

    if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
    {
        int iFormat = 0;
        for (iFormat = 0; iFormat < iCount; iFormat++)
        {
            VIDEO_STREAM_CONFIG_CAPS scc;
            AM_MEDIA_TYPE *pmtConfig;

            hr = pSConfig->lpVtbl->GetStreamCaps(pSConfig, iFormat, &pmtConfig,
                                                 (BYTE*)&scc);
            if (hr == S_OK)
            {
                if (IsEqualIID(&pmtConfig->formattype, &FORMAT_VideoInfo))
                {
                    VIDEOINFOHEADER *pvi =
                                         (VIDEOINFOHEADER *)pmtConfig->pbFormat;
                    if ((pvi->bmiHeader.biWidth == (LONG)dwWidth) &&
                        (pvi->bmiHeader.biHeight == (LONG)dwHeight) &&
                        (pvi->bmiHeader.biCompression == (DWORD)dwFourcc))
                    {
                        /* use minimum FPS(maximum frameinterval)
                           with non-VT system  */
                       if (!hax_enabled()) {
                            pvi->AvgTimePerFrame =
                                    (REFERENCE_TIME)scc.MaxFrameInterval;
                        } else {
                            pvi->AvgTimePerFrame =
                                (REFERENCE_TIME)MARUCAM_DEFAULT_FRAMEINTERVAL;
                        }
                        hr = pSConfig->lpVtbl->SetFormat(pSConfig, pmtConfig);
                        DeleteMediaType(pmtConfig);
                        break;
                    }
                }
                DeleteMediaType(pmtConfig);
            }
        }
        if (iFormat >= iCount) {
            ERR("Failed to Set format. "
                "Maybe connected webcam does not support "
                "(%ldx%ld) resolution or YUY2 image format.\n",
                dwWidth, dwHeight);
            hr = E_FAIL;
        }
    }
    SAFE_RELEASE(pSConfig);
    return hr;
}

static STDMETHODIMP QueryVideoProcAmp(long nProperty, long *pMin, long *pMax, long *pStep, long *pDefault)
{
    HRESULT hr;
    long Flags;
    IAMVideoProcAmp *pProcAmp = NULL;

    hr = g_pSrcFilter->lpVtbl->QueryInterface(g_pSrcFilter, &IID_IAMVideoProcAmp, (void**)&pProcAmp);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pProcAmp->lpVtbl->GetRange(pProcAmp, nProperty, pMin, pMax, pStep, pDefault, &Flags);

    SAFE_RELEASE(pProcAmp);
    return hr;
}

static STDMETHODIMP GetVideoProcAmp(long nProperty, long *pValue)
{
    HRESULT hr;
    long Flags;
    IAMVideoProcAmp *pProcAmp = NULL;

    hr = g_pSrcFilter->lpVtbl->QueryInterface(g_pSrcFilter, &IID_IAMVideoProcAmp, (void**)&pProcAmp);
    if (FAILED(hr))
        return hr;

    hr = pProcAmp->lpVtbl->Get(pProcAmp, nProperty, pValue, &Flags);
    if (FAILED(hr)) {
        ERR("Failed to get property for video\n");
    }

    SAFE_RELEASE(pProcAmp);
    return hr;
}

static STDMETHODIMP SetVideoProcAmp(long nProperty, long value)
{
    HRESULT hr;

    IAMVideoProcAmp *pProcAmp = NULL;
    hr = g_pSrcFilter->lpVtbl->QueryInterface(g_pSrcFilter, &IID_IAMVideoProcAmp, (void**)&pProcAmp);
    if (FAILED(hr))
        return hr;

    hr = pProcAmp->lpVtbl->Set(pProcAmp, nProperty, value, VideoProcAmp_Flags_Manual);
    if (FAILED(hr)) {
        ERR("Failed to set property for video\n");
    }
    SAFE_RELEASE(pProcAmp);
    return hr;
}

int marucam_device_check(void)
{
    int ret = 0;
    HRESULT hr = E_FAIL;
    ICreateDevEnum *pCreateDevEnum = NULL;
    IEnumMoniker *pEnumMK = NULL;
    IMoniker *pMoniKer = NULL;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        ERR("[%s] failed to CoInitailizeEx\n", __func__);
        goto error;
    }

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, &IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr)) {
        ERR("[%s] failed to create instance of CLSID_SystemDeviceEnum\n", __func__);
        goto error;
    }

    hr = pCreateDevEnum->lpVtbl->CreateClassEnumerator(pCreateDevEnum, &CLSID_VideoInputDeviceCategory, &pEnumMK, 0);
    if (FAILED(hr))
    {
        ERR("[%s] failed to create class enumerator\n", __func__);
        goto error;
    }

    if (!pEnumMK)
    {
        ERR("[%s] class enumerator is NULL!!\n", __func__);
        goto error;
    }
    pEnumMK->lpVtbl->Reset(pEnumMK);

    hr = pEnumMK->lpVtbl->Next(pEnumMK, 1, &pMoniKer, NULL);
    if (hr == S_FALSE)
    {
        ERR("[%s] enum moniker returns a invalid value.\n", __func__);
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        IPropertyBag *pBag = NULL;
        hr = pMoniKer->lpVtbl->BindToStorage(pMoniKer, 0, 0, &IID_IPropertyBag, (void **)&pBag);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->lpVtbl->Read(pBag, L"FriendlyName", &var, NULL);
            if (hr == NOERROR)
            {
                ret = 1;
                SysFreeString(var.bstrVal);
            }
            SAFE_RELEASE(pBag);
        }
        SAFE_RELEASE(pMoniKer);
    }

error:
    SAFE_RELEASE(pEnumMK);
    SAFE_RELEASE(pCreateDevEnum);
    CoUninitialize();
    return ret;
}

/* MARUCAM_CMD_INIT */
void marucam_device_init(MaruCamState* state)
{
    g_state = state;
}

/* MARUCAM_CMD_OPEN */
void marucam_device_open(MaruCamState* state)
{
    HRESULT hr;
    uint32_t dwHeight, dwWidth;
    MaruCamParam *param = state->param;
    param->top = 0;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        ERR("CoInitailizeEx\n");
        ERR("camera device open failed!!!, [HRESULT : 0x%x]\n", hr);
        param->errCode = EINVAL;
        return;
    }

    hr = GraphBuilder_Init();
    if (FAILED(hr)) {
        ERR("GraphBuilder_Init\n");
        goto error_failed;
    }

    hr = BindSourceFilter();
    if (FAILED(hr)) {
        ERR("BindSourceFilter\n");
        goto error_failed;
    }

    hr = BindTargetFilter();
    if (FAILED(hr)) {
        ERR("BindTargetFilter\n");
        goto error_failed;
    }

    hr = ConnectFilters();
    if (FAILED(hr)) {
        ERR("ConnectFilters\n");
        goto error_failed;
    }

    cur_frame_idx = 0;
    cur_fmt_idx = 0;

    dwHeight = supported_dst_frames[cur_frame_idx].height;
    dwWidth = supported_dst_frames[cur_frame_idx].width;
    hr = SetFormat(dwWidth, dwHeight, 0);
    if (hr != S_OK) {
        ERR("failed to Set default values\n");
        goto error_failed;
    }

    INFO("Open successfully!!!\n");
    return;

error_failed:
    DisconnectPins();
    RemoveFilters();
    CloseInterfaces();
    CoUninitialize();
    param->errCode = EINVAL;
    ERR("camera device open failed!!!, [HRESULT : 0x%x]\n", hr);
}

/* MARUCAM_CMD_CLOSE */
void marucam_device_close(MaruCamState* state)
{
    MaruCamParam *param = state->param;
    param->top = 0;

    DisconnectPins();
    RemoveFilters();
    CloseInterfaces();
    CoUninitialize();
    INFO("Close successfully!!!\n");
}

/* MARUCAM_CMD_START_PREVIEW */
void marucam_device_start_preview(MaruCamState* state)
{
    HRESULT hr;
    uint32_t pixfmt, width, height;
    MaruCamParam *param = state->param;
    param->top = 0;

    ready_count = 0;
    width = supported_dst_frames[cur_frame_idx].width;
    height = supported_dst_frames[cur_frame_idx].height;
    pixfmt = supported_dst_pixfmts[cur_fmt_idx].fmt;
    state->buf_size = get_sizeimage(pixfmt, width, height);

    assert(g_pCallback != NULL);
    hr = ((HWCInPin*)g_pInputPin)->SetGrabCallbackIF(g_pInputPin, g_pCallback);
    if (FAILED(hr)) {
        ERR("Failed to set IGrabCallback interface.\n");
        param->errCode = EINVAL;
        return;
    }

    hr = g_pMediaControl->lpVtbl->Run(g_pMediaControl);
    if (FAILED(hr)) {
        ERR("Failed to run media control. hr=0x%x\n", hr);
        param->errCode = EINVAL;
        return;
    }

    qemu_mutex_lock(&state->thread_mutex);
    state->streamon = 1;
    qemu_mutex_unlock(&state->thread_mutex);

    INFO("Start preview!!!\n");
}

/* MARUCAM_CMD_STOP_PREVIEW */
void marucam_device_stop_preview(MaruCamState* state)
{
    HRESULT hr;
    MaruCamParam *param = state->param;
    param->top = 0;

    hr = g_pMediaControl->lpVtbl->Stop(g_pMediaControl);
    if (FAILED(hr)) {
        ERR("Failed to stop media control.\n");
        param->errCode = EINVAL;
        return;
    }

    hr = ((HWCInPin*)g_pInputPin)->SetGrabCallbackIF(g_pInputPin, NULL);
    if (FAILED(hr)) {
        ERR("Failed to set IGrabCallback interface.\n");
        param->errCode = EINVAL;
        return;
    }

    qemu_mutex_lock(&state->thread_mutex);
    state->streamon = 0;
    qemu_mutex_unlock(&state->thread_mutex);
    state->buf_size = 0;

    INFO("Stop preview!!!\n");
}

/* MARUCAM_CMD_S_PARAM */
void marucam_device_s_param(MaruCamState* state)
{
    MaruCamParam *param = state->param;

    /* We use default FPS of the webcam */
    param->top = 0;
}

/* MARUCAM_CMD_G_PARAM */
void marucam_device_g_param(MaruCamState* state)
{
    MaruCamParam *param = state->param;

    /* We use default FPS of the webcam
     * return a fixed value on guest ini file (1/30).
     */
    param->top = 0;
    param->stack[0] = 0x1000; /* V4L2_CAP_TIMEPERFRAME */
    param->stack[1] = 1; /* numerator */
    param->stack[2] = 30; /* denominator */
}

/* MARUCAM_CMD_S_FMT */
void marucam_device_s_fmt(MaruCamState* state)
{
    uint32_t width, height, pixfmt, pidx, fidx;
    MaruCamParam *param = state->param;

    param->top = 0;
    width = param->stack[0];
    height = param->stack[1];
    pixfmt = param->stack[2];

    for (fidx = 0; fidx < ARRAY_SIZE(supported_dst_frames); fidx++) {
        if ((supported_dst_frames[fidx].width == width) &&
                (supported_dst_frames[fidx].height == height)) {
            break;
        }
    }
    if (fidx == ARRAY_SIZE(supported_dst_frames)) {
        param->errCode = EINVAL;
        return;
    }
    for (pidx = 0; pidx < ARRAY_SIZE(supported_dst_pixfmts); pidx++) {
        if (supported_dst_pixfmts[pidx].fmt == pixfmt) {
            break;
        }
    }
    if (pidx == ARRAY_SIZE(supported_dst_pixfmts)) {
        param->errCode = EINVAL;
        return;
    }

    if ((supported_dst_frames[cur_frame_idx].width != width) &&
            (supported_dst_frames[cur_frame_idx].height != height)) {
        HRESULT hr = SetFormat(width, height, 0);
        if (FAILED(hr)) {
            param->errCode = EINVAL;
            return;
        }
    }

    cur_frame_idx = fidx;
    cur_fmt_idx = pidx;

    pixfmt = supported_dst_pixfmts[cur_fmt_idx].fmt;
    width = supported_dst_frames[cur_frame_idx].width;
    height = supported_dst_frames[cur_frame_idx].height;

    param->stack[0] = width;
    param->stack[1] = height;
    param->stack[2] = 1; /* V4L2_FIELD_NONE */
    param->stack[3] = pixfmt;
    param->stack[4] = get_bytesperline(pixfmt, width);
    param->stack[5] = get_sizeimage(pixfmt, width, height);
    param->stack[6] = 0;
    param->stack[7] = 0;

    TRACE("Set format...\n");
}

/* MARUCAM_CMD_G_FMT */
void marucam_device_g_fmt(MaruCamState* state)
{
    uint32_t width, height, pixfmt;
    MaruCamParam *param = state->param;

    param->top = 0;
    pixfmt = supported_dst_pixfmts[cur_fmt_idx].fmt;
    width = supported_dst_frames[cur_frame_idx].width;
    height = supported_dst_frames[cur_frame_idx].height;

    param->stack[0] = width;
    param->stack[1] = height;
    param->stack[2] = 1; /* V4L2_FIELD_NONE */
    param->stack[3] = pixfmt;
    param->stack[4] = get_bytesperline(pixfmt, width);
    param->stack[5] = get_sizeimage(pixfmt, width, height);
    param->stack[6] = 0;
    param->stack[7] = 0;

    TRACE("Get format...\n");
}

void marucam_device_try_fmt(MaruCamState* state)
{
    uint32_t width, height, pixfmt, i;
    MaruCamParam *param = state->param;

    param->top = 0;
    width = param->stack[0];
    height = param->stack[1];
    pixfmt = param->stack[2];

    for (i = 0; i < ARRAY_SIZE(supported_dst_frames); i++) {
        if ((supported_dst_frames[i].width == width) &&
                (supported_dst_frames[i].height == height)) {
            break;
        }
    }
    if (i == ARRAY_SIZE(supported_dst_frames)) {
        param->errCode = EINVAL;
        return;
    }
    for (i = 0; i < ARRAY_SIZE(supported_dst_pixfmts); i++) {
        if (supported_dst_pixfmts[i].fmt == pixfmt) {
            break;
        }
    }
    if (i == ARRAY_SIZE(supported_dst_pixfmts)) {
        param->errCode = EINVAL;
        return;
    }

    param->stack[0] = width;
    param->stack[1] = height;
    param->stack[2] = 1; /* V4L2_FIELD_NONE */
    param->stack[3] = pixfmt;
    param->stack[4] = get_bytesperline(pixfmt, width);
    param->stack[5] = get_sizeimage(pixfmt, width, height);
    param->stack[6] = 0;
    param->stack[7] = 0;
}

void marucam_device_enum_fmt(MaruCamState* state)
{
    uint32_t index;
    MaruCamParam *param = state->param;

    param->top = 0;
    index = param->stack[0];

    if (index >= ARRAY_SIZE(supported_dst_pixfmts)) {
        param->errCode = EINVAL;
        return;
    }
    param->stack[1] = 0; /* flags = NONE */
    param->stack[2] = supported_dst_pixfmts[index].fmt; /* pixelformat */
    /* set description */
    switch (supported_dst_pixfmts[index].fmt) {
    case V4L2_PIX_FMT_YUYV:
        memcpy(&param->stack[3], "YUYV", 32);
        break;
    case V4L2_PIX_FMT_YUV420:
        memcpy(&param->stack[3], "YU12", 32);
        break;
    case V4L2_PIX_FMT_YVU420:
        memcpy(&param->stack[3], "YV12", 32);
        break;
    }
}

void marucam_device_qctrl(MaruCamState* state)
{
    HRESULT hr;
    uint32_t id, i;
    long property, min, max, step, def_val, set_val;
    char name[32] = {0,};
    MaruCamParam *param = state->param;

    param->top = 0;
    id = param->stack[0];

    switch (id) {
    case V4L2_CID_BRIGHTNESS:
        TRACE("V4L2_CID_BRIGHTNESS\n");
        property = VideoProcAmp_Brightness;
        memcpy((void*)name, (void*)"brightness", 32);
        i = 0;
        break;
    case V4L2_CID_CONTRAST:
        TRACE("V4L2_CID_CONTRAST\n");
        property = VideoProcAmp_Contrast;
        memcpy((void*)name, (void*)"contrast", 32);
        i = 1;
        break;
    case V4L2_CID_SATURATION:
        TRACE("V4L2_CID_SATURATION\n");
        property = VideoProcAmp_Saturation;
        memcpy((void*)name, (void*)"saturation", 32);
        i = 2;
        break;
    case V4L2_CID_SHARPNESS:
        TRACE("V4L2_CID_SHARPNESS\n");
        property = VideoProcAmp_Sharpness;
        memcpy((void*)name, (void*)"sharpness", 32);
        i = 3;
        break;
    default:
        param->errCode = EINVAL;
        return;
    }
    hr = QueryVideoProcAmp(property, &min, &max, &step, &def_val);
    if (FAILED(hr)) {
        param->errCode = EINVAL;
        ERR("failed to query video controls [HRESULT : 0x%x]\n", hr);
        return;
    } else {
        qctrl_tbl[i].hit = 1;
        qctrl_tbl[i].min = min;
        qctrl_tbl[i].max = max;
        qctrl_tbl[i].step = step;
        qctrl_tbl[i].init_val = def_val;

        if ((qctrl_tbl[i].min + qctrl_tbl[i].max) == 0) {
            set_val = 0;
        } else {
            set_val = (qctrl_tbl[i].min + qctrl_tbl[i].max) / 2;
        }
        hr = SetVideoProcAmp(property, set_val);
        if (FAILED(hr)) {
            param->errCode = EINVAL;
            ERR("failed to set video control value, [HRESULT : 0x%x]\n", hr);
            return;
        }
    }

    param->stack[0] = id;
    param->stack[1] = MARUCAM_CTRL_VALUE_MIN;  /* minimum */
    param->stack[2] = MARUCAM_CTRL_VALUE_MAX;  /* maximum */
    param->stack[3] = MARUCAM_CTRL_VALUE_STEP; /*  step   */
    param->stack[4] = MARUCAM_CTRL_VALUE_MID;  /* default_value */
    param->stack[5] = V4L2_CTRL_FLAG_SLIDER;
    /* name field setting */
    memcpy(&param->stack[6], (void*)name, sizeof(name)/sizeof(name[0]));
}

void marucam_device_s_ctrl(MaruCamState* state)
{
    HRESULT hr;
    uint32_t i;
    long property, set_val;
    MaruCamParam *param = state->param;

    param->top = 0;

    switch (param->stack[0]) {
    case V4L2_CID_BRIGHTNESS:
        i = 0;
        property = VideoProcAmp_Brightness;
        break;
    case V4L2_CID_CONTRAST:
        i = 1;
        property = VideoProcAmp_Contrast;
        break;
    case V4L2_CID_SATURATION:
        i = 2;
        property = VideoProcAmp_Saturation;
        break;
    case V4L2_CID_SHARPNESS:
        i = 3;
        property = VideoProcAmp_Sharpness;
        break;
    default:
        param->errCode = EINVAL;
        return;
    }
    set_val = value_convert_from_guest(qctrl_tbl[i].min,
            qctrl_tbl[i].max, (long)param->stack[1]);
    hr = SetVideoProcAmp(property, set_val);
    if (FAILED(hr)) {
        param->errCode = EINVAL;
        ERR("failed to set video control value, [HRESULT : 0x%x]\n", hr);
        return;
    }
}

void marucam_device_g_ctrl(MaruCamState* state)
{
    HRESULT hr;
    uint32_t i;
    long property, get_val;
    MaruCamParam *param = state->param;

    param->top = 0;
    switch (param->stack[0]) {
    case V4L2_CID_BRIGHTNESS:
        i = 0;
        property = VideoProcAmp_Brightness;
        break;
    case V4L2_CID_CONTRAST:
        i = 1;
        property = VideoProcAmp_Contrast;
        break;
    case V4L2_CID_SATURATION:
        i = 2;
        property = VideoProcAmp_Saturation;
        break;
    case V4L2_CID_SHARPNESS:
        i = 3;
        property = VideoProcAmp_Sharpness;
        break;
    default:
        param->errCode = EINVAL;
        return;
    }

    hr = GetVideoProcAmp(property, &get_val);
    if (FAILED(hr)) {
        param->errCode = EINVAL;
        ERR("failed to get video control value!!!, [HRESULT : 0x%x]\n", hr);
        return;
    }
    param->stack[0] = (uint32_t)value_convert_to_guest(qctrl_tbl[i].min,
                qctrl_tbl[i].max, get_val);
}

void marucam_device_enum_fsizes(MaruCamState* state)
{
    uint32_t index, pixfmt, i;
    MaruCamParam *param = state->param;

    param->top = 0;
    index = param->stack[0];
    pixfmt = param->stack[1];

    if (index >= ARRAY_SIZE(supported_dst_frames)) {
        param->errCode = EINVAL;
        return;
    }
    for (i = 0; i < ARRAY_SIZE(supported_dst_pixfmts); i++) {
        if (supported_dst_pixfmts[i].fmt == pixfmt)
            break;
    }

    if (i == ARRAY_SIZE(supported_dst_pixfmts)) {
        param->errCode = EINVAL;
        return;
    }

    param->stack[0] = supported_dst_frames[index].width;
    param->stack[1] = supported_dst_frames[index].height;
}

void marucam_device_enum_fintv(MaruCamState* state)
{
    MaruCamParam *param = state->param;

    param->top = 0;

    /* switch by index(param->stack[0]) */
    switch (param->stack[0]) {
    case 0:
        param->stack[1] = 30; /* denominator */
        break;
    default:
        param->errCode = EINVAL;
        return;
    }
    param->stack[0] = 1; /* numerator */
}

void v4lconvert_yuyv_to_yuv420(const unsigned char *src, unsigned char *dest,
        uint32_t width, uint32_t height, uint32_t yvu)
{
    uint32_t i, j;
    const unsigned char *src1;
    unsigned char *udest, *vdest;

    /* copy the Y values */
    src1 = src;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j += 2) {
            *dest++ = src1[0];
            *dest++ = src1[2];
            src1 += 4;
        }
    }

    /* copy the U and V values */
    src++;              /* point to V */
    src1 = src + width * 2;     /* next line */
    if (yvu) {
        vdest = dest;
        udest = dest + width * height / 4;
    } else {
        udest = dest;
        vdest = dest + width * height / 4;
    }
    for (i = 0; i < height; i += 2) {
        for (j = 0; j < width; j += 2) {
            *udest++ = ((int) src[0] + src1[0]) / 2;    /* U */
            *vdest++ = ((int) src[2] + src1[2]) / 2;    /* V */
            src += 4;
            src1 += 4;
        }
        src = src1;
        src1 += width * 2;
    }
}
