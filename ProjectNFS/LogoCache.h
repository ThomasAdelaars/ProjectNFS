#pragma once
#include <cstddef>
#include <unordered_map>

struct ID3D11Device;
struct ID3D11ShaderResourceView;

struct LogoTexture
{
    ID3D11ShaderResourceView* srv = nullptr;
    int                       width = 0;
    int                       height = 0;
};

bool InitImageDecoding();
void ShutdownImageDecoding();

class LogoCache
{
public:
    explicit LogoCache(ID3D11Device* device) : device(device) {}
    ~LogoCache() { clear(); }

    LogoCache(const LogoCache&) = delete;   // owns COM refs, don't copy
    LogoCache& operator=(const LogoCache&) = delete;

    // Decodes on first call for this id, returns the cached texture afterwards.
    // Returns {nullptr,0,0} if the blob is empty or not a valid image.
    LogoTexture get(int brandId, const unsigned char* data, std::size_t size);

    void clear();

private:
    ID3D11Device* device = nullptr;
    std::unordered_map<int, LogoTexture> cache;
};