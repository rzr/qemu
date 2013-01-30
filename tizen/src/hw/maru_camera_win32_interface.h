/*
 * Interface definition header for Windows host.
 *
 * Copyright (c) 2011 - 2013 Samsung Electronics Co., Ltd All Rights Reserved
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#ifndef _MARU_CAMERA_INTERFACE_H_
#define _MARU_CAMERA_INTERFACE_H_

extern int hax_enabled(void);

static const WCHAR HWCPinName[] = L"HWCInputPin\0";
static const WCHAR HWCFilterName[] = L"HWCFilter\0";

/* Forward Declarations */
FWD_DECL(IBaseFilter);
FWD_DECL(IFilterGraph);

/* defines */
#define MAX_PIN_NAME     128
#define MAX_FILTER_NAME  128

#define DECLARE_INTERFACE2(i) \
   _COM_interface i { CONST_VTABLE struct i##Vtbl *lpVtbl; }; \
   typedef CONST_VTABLE struct i##Vtbl i##Vtbl; \
   CONST_VTABLE struct i##Vtbl
#define DECLARE_INTERFACE2_(i, b) DECLARE_INTERFACE2(i)

typedef LONGLONG REFERENCE_TIME;
typedef long OAFilterState;
typedef DWORD_PTR HSEMAPHORE;
typedef DWORD_PTR HEVENT;

typedef enum _FilterState {
  State_Stopped,
  State_Paused,
  State_Running
} FILTER_STATE;

typedef struct _FilterInfo {
  WCHAR achName[MAX_FILTER_NAME];
  IFilterGraph *pGraph;
} FILTER_INFO;

typedef enum _PinDirection {
    PINDIR_INPUT    = 0,
    PINDIR_OUTPUT   = (PINDIR_INPUT + 1)
} PIN_DIRECTION;

typedef struct _PinInfo {
  IBaseFilter *pFilter;
  PIN_DIRECTION dir;
  WCHAR achName[MAX_PIN_NAME];
} PIN_INFO;

typedef struct _AllocatorProperties {
  long cBuffers;
  long cbBuffer;
  long cbAlign;
  long cbPrefix;
} ALLOCATOR_PROPERTIES;

typedef struct _AMMediaType {
  GUID majortype;
  GUID subtype;
  BOOL bFixedSizeSamples;
  BOOL bTemporalCompression;
  ULONG lSampleSize;
  GUID formattype;
  IUnknown *pUnk;
  ULONG cbFormat;
  BYTE *pbFormat;
} AM_MEDIA_TYPE;

typedef enum tagVideoProcAmpFlags {
    VideoProcAmp_Flags_Auto = 0x0001,
    VideoProcAmp_Flags_Manual = 0x0002
} VideoProcAmpFlags;

typedef enum tagVideoProcAmpProperty {
    VideoProcAmp_Brightness,
    VideoProcAmp_Contrast,
    VideoProcAmp_Hue,
    VideoProcAmp_Saturation,
    VideoProcAmp_Sharpness,
    VideoProcAmp_Gamma,
    VideoProcAmp_ColorEnable,
    VideoProcAmp_WhiteBalance,
    VideoProcAmp_BacklightCompensation,
    VideoProcAmp_Gain
} VideoProcAmpProperty;

typedef struct tagVIDEOINFOHEADER {
    RECT rcSource;
    RECT rcTarget;
    DWORD dwBitRate;
    DWORD dwBitErrorRate;
    REFERENCE_TIME AvgTimePerFrame;
    BITMAPINFOHEADER bmiHeader;
} VIDEOINFOHEADER;

typedef struct _VIDEO_STREAM_CONFIG_CAPS {
  GUID guid;
  ULONG VideoStandard;
  SIZE InputSize;
  SIZE MinCroppingSize;
  SIZE MaxCroppingSize;
  int CropGranularityX;
  int CropGranularityY;
  int CropAlignX;
  int CropAlignY;
  SIZE MinOutputSize;
  SIZE MaxOutputSize;
  int OutputGranularityX;
  int OutputGranularityY;
  int StretchTapsX;
  int StretchTapsY;
  int ShrinkTapsX;
  int ShrinkTapsY;
  LONGLONG MinFrameInterval;
  LONGLONG MaxFrameInterval;
  LONG MinBitsPerSecond;
  LONG MaxBitsPerSecond;
} VIDEO_STREAM_CONFIG_CAPS;


/* Interface & Class GUIDs */
static const IID IID_IGrabCallback   = {0x4C337035,0xC89E,0x4B42,{0x9B,0x0C,0x36,0x74,0x44,0xDD,0x70,0xDD}};

EXTERN_C const IID IID_IBaseFilter;
EXTERN_C const IID IID_ICreateDevEnum;
EXTERN_C const IID IID_IGraphBuilder;
EXTERN_C const IID IID_IMediaSeeking;
EXTERN_C const IID IID_IMediaEventSink;
EXTERN_C const IID IID_IMemInputPin;
EXTERN_C const IID IID_IEnumPins;
EXTERN_C const IID IID_IMediaFilter;
EXTERN_C const IID IID_IEnumMediaTypes;
EXTERN_C const IID IID_IMemAllocator;
EXTERN_C const IID IID_IPin;
EXTERN_C const IID IID_ICaptureGraphBuilder2;
EXTERN_C const IID IID_IFileSinkFilter;
EXTERN_C const IID IID_IAMCopyCaptureFileProgress;
EXTERN_C const IID IID_IEnumFilters;
EXTERN_C const IID IID_IMediaSample;
EXTERN_C const IID IID_IMediaControl;
EXTERN_C const IID IID_IAMStreamConfig;
EXTERN_C const IID IID_IAMVideoProcAmp;

EXTERN_C const IID CLSID_CaptureGraphBuilder2;
EXTERN_C const IID CLSID_VideoInputDeviceCategory;
EXTERN_C const IID CLSID_AudioRender;
EXTERN_C const IID CLSID_SystemDeviceEnum;
EXTERN_C const IID CLSID_AudioRendererCategory;
EXTERN_C const IID CLSID_FilterGraph;
EXTERN_C const IID CLSID_InfTee;
EXTERN_C const IID CLSID_VideoMixingRenderer9;
EXTERN_C const IID CLSID_MemoryAllocator;


/* other types GUIDs*/
EXTERN_C const IID MEDIATYPE_Audio;
EXTERN_C const IID MEDIATYPE_Video;
EXTERN_C const IID MEDIATYPE_Stream;
EXTERN_C const IID MEDIASUBTYPE_PCM;
EXTERN_C const IID MEDIASUBTYPE_WAVE;
EXTERN_C const IID MEDIASUBTYPE_Avi;
EXTERN_C const IID MEDIASUBTYPE_RGB32;
EXTERN_C const IID MEDIASUBTYPE_YV12;
EXTERN_C const IID MEDIASUBTYPE_YUY2;
EXTERN_C const IID MEDIASUBTYPE_I420;
EXTERN_C const IID MEDIASUBTYPE_YUYV;
EXTERN_C const IID FORMAT_WaveFormatEx;
EXTERN_C const IID FORMAT_VideoInfo;
EXTERN_C const IID FORMAT_VideoInfo2;
EXTERN_C const IID PIN_CATEGORY_CAPTURE;
EXTERN_C const IID PIN_CATEGORY_PREVIEW;


#define MEDIATYPE_NULL       GUID_NULL
#define MEDIASUBTYPE_NULL    GUID_NULL

#define INTERFACE IGrabCallback
DECLARE_INTERFACE_(IGrabCallback, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(Grab)(THIS_ ULONG,BYTE*) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
#define IGrabCallback_QueryInterface(T,a,b) (T)->lpVtbl->QueryInterface(T,a,b)
#define IGrabCallback_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IGrabCallback_Release(T) (T)->lpVtbl->Release(T)
#define IGrabCallback_Grab(T,a,b) (T)->lpVtbl->Grab(T,a,b)
#endif /* COBJMACROS */

#define INTERFACE IAMCopyCaptureFileProgress
DECLARE_INTERFACE_(IAMCopyCaptureFileProgress, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(Progress)(THIS_ int) PURE;
};
#undef INTERFACE
#define INTERFACE IReferenceClock
DECLARE_INTERFACE_(IReferenceClock, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetTime)(THIS_ REFERENCE_TIME *) PURE;
    STDMETHOD(AdviseTime)(THIS_ REFERENCE_TIME, REFERENCE_TIME, HEVENT, DWORD_PTR *) PURE;
    STDMETHOD(AdvisePeriodic)(THIS_ REFERENCE_TIME, REFERENCE_TIME, HSEMAPHORE, DWORD_PTR *) PURE;
    STDMETHOD(Unadvise)(THIS_ DWORD_PTR) PURE;
};
#undef INTERFACE
#define INTERFACE IEnumFilters
DECLARE_INTERFACE_(IEnumFilters, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(Next)(THIS_ ULONG, IBaseFilter **, ULONG *) PURE;
    STDMETHOD(Skip)(THIS_ ULONG) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone)(THIS_ IEnumFilters **) PURE;
};
#undef INTERFACE
#define INTERFACE IEnumMediaTypes
DECLARE_INTERFACE_(IEnumMediaTypes, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(Next)(THIS_ ULONG, AM_MEDIA_TYPE **, ULONG *) PURE;
    STDMETHOD(Skip)(THIS_ ULONG) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone)(THIS_ IEnumMediaTypes **) PURE;
};
#undef INTERFACE
#define INTERFACE IPin
DECLARE_INTERFACE_(IPin, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(Connect)(THIS_ IPin *, const AM_MEDIA_TYPE *) PURE;
    STDMETHOD(ReceiveConnection)(THIS_ IPin *, const AM_MEDIA_TYPE *) PURE;
    STDMETHOD(Disconnect)(THIS) PURE;
    STDMETHOD(ConnectedTo)(THIS_ IPin **) PURE;
    STDMETHOD(ConnectionMediaType)(THIS_ AM_MEDIA_TYPE *) PURE;
    STDMETHOD(QueryPinInfo)(THIS_ PIN_INFO *) PURE;
    STDMETHOD(QueryDirection)(THIS_ PIN_DIRECTION *) PURE;
    STDMETHOD(QueryId)(THIS_ LPWSTR *) PURE;
    STDMETHOD(QueryAccept)(THIS_ const AM_MEDIA_TYPE *) PURE;
    STDMETHOD(EnumMediaTypes)(THIS_ IEnumMediaTypes **) PURE;
    STDMETHOD(QueryInternalConnections)(THIS_ IPin **, ULONG *) PURE;
    STDMETHOD(EndOfStream)(THIS) PURE;
    STDMETHOD(BeginFlush)(THIS) PURE;
    STDMETHOD(EndFlush)(THIS) PURE;
    STDMETHOD(NewSegment)(THIS_ REFERENCE_TIME, REFERENCE_TIME, double) PURE;
};
#undef INTERFACE
#define INTERFACE IEnumPins
DECLARE_INTERFACE_(IEnumPins, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(Next)(THIS_ ULONG, IPin **, ULONG *) PURE;
    STDMETHOD(Skip)(THIS_ ULONG) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone)(THIS_ IEnumPins **) PURE;
};
#undef INTERFACE
#define INTERFACE IMediaFilter
DECLARE_INTERFACE_(IMediaFilter, IPersist)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetClassID)(THIS_ CLSID*) PURE;
    STDMETHOD(Stop)(THIS) PURE;
    STDMETHOD(Pause)(THIS) PURE;
    STDMETHOD(Run)(THIS_ REFERENCE_TIME) PURE;
    STDMETHOD(GetState)(THIS_ DWORD, FILTER_STATE *) PURE;
    STDMETHOD(SetSyncSource)(THIS_ IReferenceClock *) PURE;
    STDMETHOD(GetSyncSource)(THIS_ IReferenceClock **) PURE;
};
#undef INTERFACE
#define INTERFACE IBaseFilter
DECLARE_INTERFACE2_(IBaseFilter, IMediaFilter)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetClassID)(THIS_ CLSID*) PURE;
    STDMETHOD(Stop)(THIS) PURE;
    STDMETHOD(Pause)(THIS) PURE;
    STDMETHOD(Run)(THIS_ REFERENCE_TIME) PURE;
    STDMETHOD(GetState)(THIS_ DWORD, FILTER_STATE *) PURE;
    STDMETHOD(SetSyncSource)(THIS_ IReferenceClock *) PURE;
    STDMETHOD(GetSyncSource)(THIS_ IReferenceClock **) PURE;
    STDMETHOD(EnumPins)(THIS_ IEnumPins **) PURE;
    STDMETHOD(FindPin)(THIS_ LPCWSTR, IPin **) PURE;
    STDMETHOD(QueryFilterInfo)(THIS_ FILTER_INFO *) PURE;
    STDMETHOD(JoinFilterGraph)(THIS_ IFilterGraph *, LPCWSTR) PURE;
    STDMETHOD(QueryVendorInfo)(THIS_ LPWSTR *) PURE;
};
#undef INTERFACE
#define INTERFACE IFilterGraph
DECLARE_INTERFACE2_(IFilterGraph, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(AddFilter)(THIS_ IBaseFilter *, LPCWSTR) PURE;
    STDMETHOD(RemoveFilter)(THIS_ IBaseFilter *) PURE;
    STDMETHOD(EnumFilters)(THIS_ IEnumFilters **) PURE;
    STDMETHOD(FindFilterByName)(THIS_ LPCWSTR, IBaseFilter **) PURE;
    STDMETHOD(ConnectDirect)(THIS_ IPin *, IPin *, const AM_MEDIA_TYPE *) PURE;
    STDMETHOD(Reconnect)(THIS_ IPin *) PURE;
    STDMETHOD(Disconnect)(THIS_ IPin *) PURE;
    STDMETHOD(SetDefaultSyncSource)(THIS) PURE;
};
#undef INTERFACE
#define INTERFACE IGraphBuilder
DECLARE_INTERFACE_(IGraphBuilder ,IFilterGraph)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(AddFilter)(THIS_ IBaseFilter *, LPCWSTR) PURE;
    STDMETHOD(RemoveFilter)(THIS_ IBaseFilter *) PURE;
    STDMETHOD(EnumFilters)(THIS_ IEnumFilters **) PURE;
    STDMETHOD(FindFilterByName)(THIS_ LPCWSTR, IBaseFilter **) PURE;
    STDMETHOD(ConnectDirect)(THIS_ IPin *, IPin *, const AM_MEDIA_TYPE *) PURE;
    STDMETHOD(Reconnect)(THIS_ IPin *) PURE;
    STDMETHOD(Disconnect)(THIS_ IPin *) PURE;
    STDMETHOD(SetDefaultSyncSource)(THIS) PURE;
    STDMETHOD(Connect)(THIS_ IPin *, IPin *) PURE;
    STDMETHOD(Render)(THIS_ IPin *) PURE;
    STDMETHOD(RenderFile)(THIS_ LPCWSTR, LPCWSTR) PURE;
    STDMETHOD(AddSourceFilter)(THIS_ LPCWSTR, LPCWSTR, IBaseFilter **) PURE;
    STDMETHOD(SetLogFile)(THIS_ DWORD_PTR) PURE;
    STDMETHOD(Abort)(THIS) PURE;
    STDMETHOD(ShouldOperationContinue)(THIS) PURE;
};
#undef INTERFACE
#define INTERFACE ICreateDevEnum
DECLARE_INTERFACE_(ICreateDevEnum, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(CreateClassEnumerator)(THIS_ REFCLSID, IEnumMoniker **, DWORD) PURE;
};
#undef INTERFACE
#define INTERFACE IMediaSample
DECLARE_INTERFACE_(IMediaSample, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetPointer)(THIS_ BYTE **) PURE;
    STDMETHOD_(long, GetSize)(THIS) PURE;
    STDMETHOD(GetTime)(THIS_ REFERENCE_TIME *, REFERENCE_TIME *) PURE;
    STDMETHOD(SetTime)(THIS_ REFERENCE_TIME *, REFERENCE_TIME *) PURE;
    STDMETHOD(IsSyncPoint)(THIS) PURE;
    STDMETHOD(SetSyncPoint)(THIS_ BOOL) PURE;
    STDMETHOD(IsPreroll)(THIS) PURE;
    STDMETHOD(SetPreroll)(THIS_ BOOL) PURE;
    STDMETHOD_(long, GetActualDataLength)(THIS) PURE;
    STDMETHOD(SetActualDataLength)(THIS_ long) PURE;
    STDMETHOD(GetMediaType)(THIS_ AM_MEDIA_TYPE **) PURE;
    STDMETHOD(SetMediaType)(THIS_ AM_MEDIA_TYPE *) PURE;
    STDMETHOD(IsDiscontinuity)(THIS) PURE;
    STDMETHOD(SetDiscontinuity)(THIS_ BOOL) PURE;
    STDMETHOD(GetMediaTime)(THIS_ LONGLONG *, LONGLONG *) PURE;
    STDMETHOD(SetMediaTime)(THIS_ LONGLONG *, LONGLONG *) PURE;
};
#undef INTERFACE
#define INTERFACE IMemAllocator
DECLARE_INTERFACE_(IMemAllocator, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(SetProperties)(THIS_ ALLOCATOR_PROPERTIES *, ALLOCATOR_PROPERTIES *) PURE;
    STDMETHOD(GetProperties)(THIS_ ALLOCATOR_PROPERTIES *) PURE;
    STDMETHOD(Commit)(THIS) PURE;
    STDMETHOD(Decommit)(THIS) PURE;
    STDMETHOD(GetBuffer)(THIS_ IMediaSample **, REFERENCE_TIME *, REFERENCE_TIME *, DWORD) PURE;
    STDMETHOD(ReleaseBuffer)(THIS_ IMediaSample *) PURE;

};
#undef INTERFACE
#define INTERFACE IMemInputPin
DECLARE_INTERFACE_(IMemInputPin, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetAllocator)(THIS_ IMemAllocator **) PURE;
    STDMETHOD(NotifyAllocator)(THIS_ IMemAllocator *, BOOL) PURE;
    STDMETHOD(GetAllocatorRequirements)(THIS_ ALLOCATOR_PROPERTIES *) PURE;
    STDMETHOD(Receive)(THIS_ IMediaSample *) PURE;
    STDMETHOD(ReceiveMultiple)(THIS_ IMediaSample **, long, long *) PURE;
    STDMETHOD(ReceiveCanBlock)(THIS) PURE;
};
#undef INTERFACE
#define INTERFACE IFileSinkFilter
DECLARE_INTERFACE_(IFileSinkFilter, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(SetFileName)(THIS_ LPCOLESTR,const AM_MEDIA_TYPE *) PURE;
    STDMETHOD(GetCurFile)(THIS_ LPOLESTR *,AM_MEDIA_TYPE*) PURE;
};
#undef INTERFACE
#define INTERFACE ICaptureGraphBuilder2
DECLARE_INTERFACE_(ICaptureGraphBuilder2, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(SetFiltergraph)(THIS_ IGraphBuilder*) PURE;
    STDMETHOD(GetFiltergraph)(THIS_ IGraphBuilder**) PURE;
    STDMETHOD(SetOutputFileName)(THIS_ const GUID*,LPCOLESTR,IBaseFilter**,IFileSinkFilter**) PURE;
    STDMETHOD(FindInterface)(THIS_ const GUID*,const GUID*,IBaseFilter*,REFIID,void**) PURE;
    STDMETHOD(RenderStream)(THIS_ const GUID*,const GUID*,IUnknown*,IBaseFilter*,IBaseFilter*) PURE;
    STDMETHOD(ControlStream)(THIS_ const GUID*,const GUID*,IBaseFilter*,REFERENCE_TIME*,REFERENCE_TIME*,WORD,WORD) PURE;
    STDMETHOD(AllocCapFile)(THIS_ LPCOLESTR,DWORDLONG) PURE;
    STDMETHOD(CopyCaptureFile)(THIS_ LPOLESTR,LPOLESTR,int,IAMCopyCaptureFileProgress*) PURE;
    STDMETHOD(FindPin)(THIS_ IUnknown*,PIN_DIRECTION,const GUID*,const GUID*,BOOL,int,IPin**) PURE;
};
#undef INTERFACE
#define INTERFACE IAMStreamConfig
DECLARE_INTERFACE_(IAMStreamConfig, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(SetFormat)(THIS_ AM_MEDIA_TYPE*) PURE;
    STDMETHOD(GetFormat)(THIS_ AM_MEDIA_TYPE**) PURE;
    STDMETHOD(GetNumberOfCapabilities)(THIS_ int*,int*) PURE;
    STDMETHOD(GetStreamCaps)(THIS_ int,AM_MEDIA_TYPE**,BYTE*) PURE;
};
#undef INTERFACE
#define INTERFACE IAMVideoProcAmp
DECLARE_INTERFACE_(IAMVideoProcAmp, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetRange)(THIS_ long,long*,long*,long*,long*,long*) PURE;
    STDMETHOD(Set)(THIS_ long,long,long) PURE;
    STDMETHOD(Get)(THIS_ long,long*,long*) PURE;
};
#undef INTERFACE
#define INTERFACE IMediaControl
DECLARE_INTERFACE_(IMediaControl, IDispatch)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT*);
    STDMETHOD(GetTypeInfo)(THIS_ UINT,LCID,ITypeInfo**);
    STDMETHOD(GetIDsOfNames)(THIS_ REFIID,LPOLESTR*,UINT,LCID,DISPID*);
    STDMETHOD(Invoke)(THIS_ DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
    STDMETHOD(Run)(THIS);
    STDMETHOD(Pause)(THIS);
    STDMETHOD(Stop)(THIS);
    STDMETHOD(GetState)(THIS_ LONG, OAFilterState*);
    STDMETHOD(RenderFile)(THIS_ BSTR);
    STDMETHOD(AddSourceFilter)(THIS_ BSTR,IDispatch**);
    STDMETHOD(get_FilterCollection)(THIS_ IDispatch**);
    STDMETHOD(get_RegFilterCollection)(THIS_ IDispatch**);
    STDMETHOD(StopWhenReady)(THIS);
};
#undef INTERFACE

#ifdef COBJMACROS
#define ICreateDevEnum_QueryInterface(This,riid,ppvObject)  \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define ICreateDevEnum_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define ICreateDevEnum_Release(This)    \
    ((This)->lpVtbl->Release(This))
#define ICreateDevEnum_CreateClassEnumerator(This,clsidDeviceClass,ppEnumMoniker,dwFlags)   \
    ((This)->lpVtbl->CreateClassEnumerator(This,clsidDeviceClass,ppEnumMoniker,dwFlags))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IPin_QueryInterface(This,riid,ppvObject)    \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IPin_AddRef(This)   \
    ((This)->lpVtbl->AddRef(This))
#define IPin_Release(This)  \
    ((This)->lpVtbl->Release(This))
#define IPin_Connect(This,pReceivePin,pmt)  \
    ((This)->lpVtbl->Connect(This,pReceivePin,pmt))
#define IPin_ReceiveConnection(This,pConnector,pmt) \
    ((This)->lpVtbl->ReceiveConnection(This,pConnector,pmt))
#define IPin_Disconnect(This)   \
    ((This)->lpVtbl->Disconnect(This))
#define IPin_ConnectedTo(This,pPin) \
    ((This)->lpVtbl->ConnectedTo(This,pPin))
#define IPin_ConnectionMediaType(This,pmt)  \
    ((This)->lpVtbl->ConnectionMediaType(This,pmt))
#define IPin_QueryPinInfo(This,pInfo)   \
    ((This)->lpVtbl->QueryPinInfo(This,pInfo))
#define IPin_QueryDirection(This,pPinDir)   \
    ((This)->lpVtbl->QueryDirection(This,pPinDir))
#define IPin_QueryId(This,Id)   \
    ((This)->lpVtbl->QueryId(This,Id))
#define IPin_QueryAccept(This,pmt)  \
    ((This)->lpVtbl->QueryAccept(This,pmt))
#define IPin_EnumMediaTypes(This,ppEnum)    \
    ((This)->lpVtbl->EnumMediaTypes(This,ppEnum))
#define IPin_QueryInternalConnections(This,apPin,nPin)  \
    ((This)->lpVtbl->QueryInternalConnections(This,apPin,nPin))
#define IPin_EndOfStream(This)  \
    ((This)->lpVtbl->EndOfStream(This))
#define IPin_BeginFlush(This)   \
    ((This)->lpVtbl->BeginFlush(This))
#define IPin_EndFlush(This) \
    ((This)->lpVtbl->EndFlush(This))
#define IPin_NewSegment(This,tStart,tStop,dRate)    \
    ((This)->lpVtbl->NewSegment(This,tStart,tStop,dRate))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IEnumPins_QueryInterface(This,riid,ppvObject)   \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IEnumPins_AddRef(This)  \
    ((This)->lpVtbl->AddRef(This))
#define IEnumPins_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IEnumPins_Next(This,cPins,ppPins,pcFetched) \
    ((This)->lpVtbl->Next(This,cPins,ppPins,pcFetched))
#define IEnumPins_Skip(This,cPins)  \
    ((This)->lpVtbl->Skip(This,cPins))
#define IEnumPins_Reset(This)   \
    ((This)->lpVtbl->Reset(This))
#define IEnumPins_Clone(This,ppEnum)    \
    ((This)->lpVtbl->Clone(This,ppEnum))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IAMStreamConfig_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IAMStreamConfig_AddRef(This)    \
    ((This)->lpVtbl->AddRef(This))
#define IAMStreamConfig_Release(This)   \
    ((This)->lpVtbl->Release(This))
#define IAMStreamConfig_SetFormat(This,pmt) \
    ((This)->lpVtbl->SetFormat(This,pmt))
#define IAMStreamConfig_GetFormat(This,ppmt)    \
    ((This)->lpVtbl->GetFormat(This,ppmt))
#define IAMStreamConfig_GetNumberOfCapabilities(This,piCount,piSize)    \
    ((This)->lpVtbl->GetNumberOfCapabilities(This,piCount,piSize))
#define IAMStreamConfig_GetStreamCaps(This,iIndex,ppmt,pSCC)    \
    ((This)->lpVtbl->GetStreamCaps(This,iIndex,ppmt,pSCC))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IFilterGraph_QueryInterface(This,riid,ppvObject)    \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IFilterGraph_AddRef(This)   \
    ((This)->lpVtbl->AddRef(This))
#define IFilterGraph_Release(This)  \
    ((This)->lpVtbl->Release(This))
#define IFilterGraph_AddFilter(This,pFilter,pName)  \
    ((This)->lpVtbl->AddFilter(This,pFilter,pName))
#define IFilterGraph_RemoveFilter(This,pFilter) \
    ((This)->lpVtbl->RemoveFilter(This,pFilter))
#define IFilterGraph_EnumFilters(This,ppEnum)   \
    ((This)->lpVtbl->EnumFilters(This,ppEnum))
#define IFilterGraph_FindFilterByName(This,pName,ppFilter)  \
    ((This)->lpVtbl->FindFilterByName(This,pName,ppFilter))
#define IFilterGraph_ConnectDirect(This,ppinOut,ppinIn,pmt) \
    ((This)->lpVtbl->ConnectDirect(This,ppinOut,ppinIn,pmt))
#define IFilterGraph_Reconnect(This,ppin)   \
    ((This)->lpVtbl->Reconnect(This,ppin))
#define IFilterGraph_Disconnect(This,ppin)  \
    ((This)->lpVtbl->Disconnect(This,ppin))
#define IFilterGraph_SetDefaultSyncSource(This) \
    ((This)->lpVtbl->SetDefaultSyncSource(This))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IMediaFilter_QueryInterface(This,riid,ppvObject)    \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IMediaFilter_AddRef(This)   \
    ((This)->lpVtbl->AddRef(This))
#define IMediaFilter_Release(This)  \
    ((This)->lpVtbl->Release(This))
#define IMediaFilter_GetClassID(This,pClassID)  \
    ((This)->lpVtbl->GetClassID(This,pClassID))
#define IMediaFilter_Stop(This) \
    ((This)->lpVtbl->Stop(This))
#define IMediaFilter_Pause(This)    \
    ((This)->lpVtbl->Pause(This))
#define IMediaFilter_Run(This,tStart)   \
    ((This)->lpVtbl->Run(This,tStart))
#define IMediaFilter_GetState(This,dwMilliSecsTimeout,State)    \
    ((This)->lpVtbl->GetState(This,dwMilliSecsTimeout,State))
#define IMediaFilter_SetSyncSource(This,pClock) \
    ((This)->lpVtbl->SetSyncSource(This,pClock))
#define IMediaFilter_GetSyncSource(This,pClock) \
    ((This)->lpVtbl->GetSyncSource(This,pClock))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IBaseFilter_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IBaseFilter_AddRef(This)    \
    ((This)->lpVtbl->AddRef(This))
#define IBaseFilter_Release(This)   \
    ((This)->lpVtbl->Release(This))
#define IBaseFilter_GetClassID(This,pClassID)   \
    ((This)->lpVtbl->GetClassID(This,pClassID))
#define IBaseFilter_Stop(This)  \
    ((This)->lpVtbl->Stop(This))
#define IBaseFilter_Pause(This) \
    ((This)->lpVtbl->Pause(This))
#define IBaseFilter_Run(This,tStart)    \
    ((This)->lpVtbl->Run(This,tStart))
#define IBaseFilter_GetState(This,dwMilliSecsTimeout,State) \
    ((This)->lpVtbl->GetState(This,dwMilliSecsTimeout,State))
#define IBaseFilter_SetSyncSource(This,pClock)  \
    ((This)->lpVtbl->SetSyncSource(This,pClock))
#define IBaseFilter_GetSyncSource(This,pClock)  \
    ((This)->lpVtbl->GetSyncSource(This,pClock))
#define IBaseFilter_EnumPins(This,ppEnum)   \
    ((This)->lpVtbl->EnumPins(This,ppEnum))
#define IBaseFilter_FindPin(This,Id,ppPin)  \
    ((This)->lpVtbl->FindPin(This,Id,ppPin))
#define IBaseFilter_QueryFilterInfo(This,pInfo) \
    ((This)->lpVtbl->QueryFilterInfo(This,pInfo))
#define IBaseFilter_JoinFilterGraph(This,pGraph,pName)  \
    ((This)->lpVtbl->JoinFilterGraph(This,pGraph,pName))
#define IBaseFilter_QueryVendorInfo(This,pVendorInfo)   \
    ((This)->lpVtbl->QueryVendorInfo(This,pVendorInfo))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IMediaSample_QueryInterface(This,riid,ppvObject)    \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IMediaSample_AddRef(This)   \
    ((This)->lpVtbl->AddRef(This))
#define IMediaSample_Release(This)  \
    ((This)->lpVtbl->Release(This))
#define IMediaSample_GetPointer(This,ppBuffer)  \
    ((This)->lpVtbl->GetPointer(This,ppBuffer))
#define IMediaSample_GetSize(This)  \
        ((This)->lpVtbl->GetSize(This))
#define IMediaSample_GetTime(This,pTimeStart,pTimeEnd)  \
    ((This)->lpVtbl->GetTime(This,pTimeStart,pTimeEnd))
#define IMediaSample_SetTime(This,pTimeStart,pTimeEnd)  \
    ((This)->lpVtbl->SetTime(This,pTimeStart,pTimeEnd))
#define IMediaSample_IsSyncPoint(This)  \
    ((This)->lpVtbl->IsSyncPoint(This))
#define IMediaSample_SetSyncPoint(This,bIsSyncPoint)    \
    ((This)->lpVtbl->SetSyncPoint(This,bIsSyncPoint))
#define IMediaSample_IsPreroll(This)    \
    ((This)->lpVtbl->IsPreroll(This))
#define IMediaSample_SetPreroll(This,bIsPreroll)    \
    ((This)->lpVtbl->SetPreroll(This,bIsPreroll))
#define IMediaSample_GetActualDataLength(This)  \
    ((This)->lpVtbl->GetActualDataLength(This))
#define IMediaSample_SetActualDataLength(This,length)   \
    ((This)->lpVtbl->SetActualDataLength(This,length))
#define IMediaSample_GetMediaType(This,ppMediaType) \
    ((This)->lpVtbl->GetMediaType(This,ppMediaType))
#define IMediaSample_SetMediaType(This,pMediaType)  \
    ((This)->lpVtbl->SetMediaType(This,pMediaType))
#define IMediaSample_IsDiscontinuity(This)  \
    ((This)->lpVtbl->IsDiscontinuity(This))
#define IMediaSample_SetDiscontinuity(This,bDiscontinuity)  \
    ((This)->lpVtbl->SetDiscontinuity(This,bDiscontinuity))
#define IMediaSample_GetMediaTime(This,pTimeStart,pTimeEnd) \
    ((This)->lpVtbl->GetMediaTime(This,pTimeStart,pTimeEnd))
#define IMediaSample_SetMediaTime(This,pTimeStart,pTimeEnd) \
    ((This)->lpVtbl->SetMediaTime(This,pTimeStart,pTimeEnd))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IEnumFilters_QueryInterface(This,riid,ppvObject)    \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IEnumFilters_AddRef(This)   \
    ((This)->lpVtbl->AddRef(This))
#define IEnumFilters_Release(This)  \
    ((This)->lpVtbl->Release(This))
#define IEnumFilters_Next(This,cFilters,ppFilter,pcFetched) \
    ((This)->lpVtbl->Next(This,cFilters,ppFilter,pcFetched))
#define IEnumFilters_Skip(This,cFilters)    \
    ((This)->lpVtbl->Skip(This,cFilters))
#define IEnumFilters_Reset(This)    \
    ((This)->lpVtbl->Reset(This))
#define IEnumFilters_Clone(This,ppEnum) \
    ((This)->lpVtbl->Clone(This,ppEnum))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IMemAllocator_QueryInterface(This,riid,ppvObject)   \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IMemAllocator_AddRef(This)  \
    ((This)->lpVtbl->AddRef(This))
#define IMemAllocator_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IMemAllocator_SetProperties(This,pRequest,pActual)  \
    ((This)->lpVtbl->SetProperties(This,pRequest,pActual))
#define IMemAllocator_GetProperties(This,pProps)    \
    ((This)->lpVtbl->GetProperties(This,pProps))
#define IMemAllocator_Commit(This)  \
    ((This)->lpVtbl->Commit(This))
#define IMemAllocator_Decommit(This)    \
    ((This)->lpVtbl->Decommit(This))
#define IMemAllocator_GetBuffer(This,ppBuffer,pStartTime,pEndTime,dwFlags)  \
    ((This)->lpVtbl->GetBuffer(This,ppBuffer,pStartTime,pEndTime,dwFlags))
#define IMemAllocator_ReleaseBuffer(This,pBuffer)   \
    ((This)->lpVtbl->ReleaseBuffer(This,pBuffer))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IMemInputPin_QueryInterface(This,riid,ppvObject)    \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IMemInputPin_AddRef(This)   \
    ((This)->lpVtbl->AddRef(This))
#define IMemInputPin_Release(This)  \
    ((This)->lpVtbl->Release(This))
#define IMemInputPin_GetAllocator(This,ppAllocator) \
    ((This)->lpVtbl->GetAllocator(This,ppAllocator))
#define IMemInputPin_NotifyAllocator(This,pAllocator,bReadOnly) \
    ((This)->lpVtbl->NotifyAllocator(This,pAllocator,bReadOnly))
#define IMemInputPin_GetAllocatorRequirements(This,pProps)  \
    ((This)->lpVtbl->GetAllocatorRequirements(This,pProps))
#define IMemInputPin_Receive(This,pSample)  \
    ((This)->lpVtbl->Receive(This,pSample))
#define IMemInputPin_ReceiveMultiple(This,pSamples,nSamples,nSamplesProcessed)  \
    ((This)->lpVtbl->ReceiveMultiple(This,pSamples,nSamples,nSamplesProcessed))
#define IMemInputPin_ReceiveCanBlock(This)  \
    ((This)->lpVtbl->ReceiveCanBlock(This))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IGraphBuilder_QueryInterface(This,riid,ppvObject)   \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IGraphBuilder_AddRef(This)  \
    ((This)->lpVtbl->AddRef(This))
#define IGraphBuilder_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IGraphBuilder_AddFilter(This,pFilter,pName) \
    ((This)->lpVtbl->AddFilter(This,pFilter,pName))
#define IGraphBuilder_RemoveFilter(This,pFilter)    \
    ((This)->lpVtbl->RemoveFilter(This,pFilter))
#define IGraphBuilder_EnumFilters(This,ppEnum)  \
    ((This)->lpVtbl->EnumFilters(This,ppEnum))
#define IGraphBuilder_FindFilterByName(This,pName,ppFilter) \
    ((This)->lpVtbl->FindFilterByName(This,pName,ppFilter))
#define IGraphBuilder_ConnectDirect(This,ppinOut,ppinIn,pmt)    \
    ((This)->lpVtbl->ConnectDirect(This,ppinOut,ppinIn,pmt))
#define IGraphBuilder_Reconnect(This,ppin)  \
    ((This)->lpVtbl->Reconnect(This,ppin))
#define IGraphBuilder_Disconnect(This,ppin) \
    ((This)->lpVtbl->Disconnect(This,ppin))
#define IGraphBuilder_SetDefaultSyncSource(This)    \
    ((This)->lpVtbl->SetDefaultSyncSource(This))
#define IGraphBuilder_Connect(This,ppinOut,ppinIn)  \
    ((This)->lpVtbl->Connect(This,ppinOut,ppinIn))
#define IGraphBuilder_Render(This,ppinOut)  \
    ((This)->lpVtbl->Render(This,ppinOut))
#define IGraphBuilder_RenderFile(This,lpcwstrFile,lpcwstrPlayList)  \
    ((This)->lpVtbl->RenderFile(This,lpcwstrFile,lpcwstrPlayList))
#define IGraphBuilder_AddSourceFilter(This,lpcwstrFileName,lpcwstrFilterName,ppFilter)  \
    ((This)->lpVtbl->AddSourceFilter(This,lpcwstrFileName,lpcwstrFilterName,ppFilter))
#define IGraphBuilder_SetLogFile(This,hFile)    \
    ((This)->lpVtbl->SetLogFile(This,hFile))
#define IGraphBuilder_Abort(This)   \
    ((This)->lpVtbl->Abort(This))
#define IGraphBuilder_ShouldOperationContinue(This) \
    ((This)->lpVtbl->ShouldOperationContinue(This))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IEnumMediaTypes_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IEnumMediaTypes_AddRef(This)    \
    ((This)->lpVtbl->AddRef(This))
#define IEnumMediaTypes_Release(This)   \
    ((This)->lpVtbl->Release(This))
#define IEnumMediaTypes_Next(This,cMediaTypes,ppMediaTypes,pcFetched)   \
    ((This)->lpVtbl->Next(This,cMediaTypes,ppMediaTypes,pcFetched))
#define IEnumMediaTypes_Skip(This,cMediaTypes)  \
    ((This)->lpVtbl->Skip(This,cMediaTypes))
#define IEnumMediaTypes_Reset(This) \
    ((This)->lpVtbl->Reset(This))
#define IEnumMediaTypes_Clone(This,ppEnum)  \
    ((This)->lpVtbl->Clone(This,ppEnum))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IMediaControl_QueryInterface(This,riid,ppvObject)   \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IMediaControl_AddRef(This)  \
    ((This)->lpVtbl->AddRef(This))
#define IMediaControl_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IMediaControl_GetTypeInfoCount(This,pctinfo)    \
    ((This)->lpVtbl->GetTypeInfoCount(This,pctinfo))
#define IMediaControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo) \
    ((This)->lpVtbl->GetTypeInfo(This,iTInfo,lcid,ppTInfo))
#define IMediaControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)   \
    ((This)->lpVtbl->GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId))
#define IMediaControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) \
    ((This)->lpVtbl->Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr))
#define IMediaControl_Run(This) \
    ((This)->lpVtbl->Run(This))
#define IMediaControl_Pause(This)   \
    ((This)->lpVtbl->Pause(This))
#define IMediaControl_Stop(This)    \
    ((This)->lpVtbl->Stop(This))
#define IMediaControl_GetState(This,msTimeout,pfs)  \
    ((This)->lpVtbl->GetState(This,msTimeout,pfs))
#define IMediaControl_RenderFile(This,strFilename)  \
    ((This)->lpVtbl->RenderFile(This,strFilename))
#define IMediaControl_AddSourceFilter(This,strFilename,ppUnk)   \
    ((This)->lpVtbl->AddSourceFilter(This,strFilename,ppUnk))
#define IMediaControl_get_FilterCollection(This,ppUnk)  \
    ((This)->lpVtbl->get_FilterCollection(This,ppUnk))
#define IMediaControl_get_RegFilterCollection(This,ppUnk)   \
    ((This)->lpVtbl->get_RegFilterCollection(This,ppUnk))
#define IMediaControl_StopWhenReady(This)   \
    ((This)->lpVtbl->StopWhenReady(This))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IAMVideoProcAmp_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IAMVideoProcAmp_AddRef(This)    \
    ((This)->lpVtbl->AddRef(This))
#define IAMVideoProcAmp_Release(This)   \
    ((This)->lpVtbl->Release(This))
#define IAMVideoProcAmp_GetRange(This,Property,pMin,pMax,pSteppingDelta,pDefault,pCapsFlags)    \
    ((This)->lpVtbl->GetRange(This,Property,pMin,pMax,pSteppingDelta,pDefault,pCapsFlags))
#define IAMVideoProcAmp_Set(This,Property,lValue,Flags) \
    ((This)->lpVtbl->Set(This,Property,lValue,Flags))
#define IAMVideoProcAmp_Get(This,Property,lValue,Flags) \
    ((This)->lpVtbl->Get(This,Property,lValue,Flags))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IFileSinkFilter_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IFileSinkFilter_AddRef(This)    \
    ((This)->lpVtbl->AddRef(This))
#define IFileSinkFilter_Release(This)   \
    ((This)->lpVtbl->Release(This))
#define IFileSinkFilter_SetFileName(This,pszFileName,pmt)   \
    ((This)->lpVtbl->SetFileName(This,pszFileName,pmt))
#define IFileSinkFilter_GetCurFile(This,ppszFileName,pmt)   \
    ((This)->lpVtbl->GetCurFile(This,ppszFileName,pmt))
#endif /* COBJMACROS */

#ifdef COBJMACROS
#define IAMCopyCaptureFileProgress_QueryInterface(This,riid,ppvObject)  \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IAMCopyCaptureFileProgress_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IAMCopyCaptureFileProgress_Release(This)    \
    ((This)->lpVtbl->Release(This))
#define IAMCopyCaptureFileProgress_Progress(This,iProgress) \
    ((This)->lpVtbl->Progress(This,iProgress))
#endif /* COBJMACROS */


#ifdef COBJMACROS
#define ICaptureGraphBuilder2_QueryInterface(This,riid,ppvObject)   \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define ICaptureGraphBuilder2_AddRef(This)  \
    ((This)->lpVtbl->AddRef(This))
#define ICaptureGraphBuilder2_Release(This) \
    ((This)->lpVtbl->Release(This))
#define ICaptureGraphBuilder2_SetFiltergraph(This,pfg)  \
    ((This)->lpVtbl->SetFiltergraph(This,pfg))
#define ICaptureGraphBuilder2_GetFiltergraph(This,ppfg) \
    ((This)->lpVtbl->GetFiltergraph(This,ppfg))
#define ICaptureGraphBuilder2_SetOutputFileName(This,pType,lpstrFile,ppf,ppSink)    \
    ((This)->lpVtbl->SetOutputFileName(This,pType,lpstrFile,ppf,ppSink))
#define ICaptureGraphBuilder2_FindInterface(This,pCategory,pType,pf,riid,ppint) \
    ((This)->lpVtbl->FindInterface(This,pCategory,pType,pf,riid,ppint))
#define ICaptureGraphBuilder2_RenderStream(This,pCategory,pType,pSource,pfCompressor,pfRenderer)    \
    ((This)->lpVtbl->RenderStream(This,pCategory,pType,pSource,pfCompressor,pfRenderer))
#define ICaptureGraphBuilder2_ControlStream(This,pCategory,pType,pFilter,pstart,pstop,wStartCookie,wStopCookie) \
    ((This)->lpVtbl->ControlStream(This,pCategory,pType,pFilter,pstart,pstop,wStartCookie,wStopCookie))
#define ICaptureGraphBuilder2_AllocCapFile(This,lpstr,dwlSize)  \
    ((This)->lpVtbl->AllocCapFile(This,lpstr,dwlSize))
#define ICaptureGraphBuilder2_CopyCaptureFile(This,lpwstrOld,lpwstrNew,fAllowEscAbort,pCallback)    \
    ((This)->lpVtbl->CopyCaptureFile(This,lpwstrOld,lpwstrNew,fAllowEscAbort,pCallback))
#define ICaptureGraphBuilder2_FindPin(This,pSource,pindir,pCategory,pType,fUnconnected,num,ppPin)   \
    ((This)->lpVtbl->FindPin(This,pSource,pindir,pCategory,pType,fUnconnected,num,ppPin))
#endif /* COBJMACROS */

#endif /* _MARU_CAMERA_INTERFACE_H_ */
