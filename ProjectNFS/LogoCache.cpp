#include "LogoCache.h"

#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib") 

using Microsoft::WRL::ComPtr;

bool InitImageDecoding()
{
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
}

void ShutdownImageDecoding()
{
    ::CoUninitialize();
}

static bool DecodeToTexture(ID3D11Device* device,
    const unsigned char* data, std::size_t size,
    LogoTexture* out)
{
    if (!device || !data || size == 0 || !out)
        return false;

    ComPtr<IWICImagingFactory> factory;
    if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        return false;

    ComPtr<IWICStream> stream;
    if (FAILED(factory->CreateStream(&stream)))
        return false;
    if (FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(data), (DWORD)size)))
        return false;

    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr,
        WICDecodeMetadataCacheOnLoad, &decoder)))
        return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame)))
        return false;

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter)))
        return false;
    if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0,
        WICBitmapPaletteTypeCustom)))
        return false;

    UINT w = 0, h = 0;
    if (FAILED(converter->GetSize(&w, &h)) || w == 0 || h == 0)
        return false;

    const UINT stride = w * 4;
    std::vector<BYTE> pixels((std::size_t)stride * h);
    if (FAILED(converter->CopyPixels(nullptr, stride, (UINT)pixels.size(), pixels.data())))
        return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub = {};
    sub.pSysMem = pixels.data();
    sub.SysMemPitch = stride;

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(device->CreateTexture2D(&desc, &sub, &tex)))
        return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    if (FAILED(device->CreateShaderResourceView(tex.Get(), &srvDesc, &srv)))
        return false;

    out->srv = srv;
    out->width = (int)w;
    out->height = (int)h;
    return true;
}

LogoTexture LogoCache::get(int brandId, const unsigned char* data, std::size_t size)
{
    auto it = cache.find(brandId);
    if (it != cache.end())
        return it->second;

    LogoTexture lt;
    DecodeToTexture(device, data, size, &lt);
    cache.emplace(brandId, lt);
    return lt;
}

void LogoCache::clear()
{
    for (auto& kv : cache)
        if (kv.second.srv)
            kv.second.srv->Release();
    cache.clear();
}