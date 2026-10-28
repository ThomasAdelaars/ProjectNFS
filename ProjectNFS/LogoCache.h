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

    LogoCache(const LogoCache&) = delete;
    LogoCache& operator=(const LogoCache&) = delete;

    LogoTexture get(int brandId, const unsigned char* data, std::size_t size);

    void clear();

private:
    ID3D11Device* device = nullptr;
    std::unordered_map<int, LogoTexture> cache;
};