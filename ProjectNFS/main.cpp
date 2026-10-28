#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#pragma comment(lib, "d3d11.lib")

#include "CarBrandUI.h"
#include "DatabaseConnection.h"
#include "brandRepository.h"
#include "carRepository.h"

#include <iostream>
#include <exception>
#include <string>

#include "LogoCache.h"
#include <unordered_map>

// ---------------------------------------------------------------- D3D globals
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;

static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Forward declare the ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
    WPARAM wParam, LPARAM lParam);

// ----------------------------------------------------------------------- main
int main()
{
    // ---------- 1. Win32 window ----------
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
                       GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                       L"CarManagerClass", nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName,
        L"Need For Speed - Car Manager",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        std::cerr << "Kon geen D3D11 device aanmaken.\n";
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // ---------- 2. ImGui ----------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    int exitCode = 0;

    try
    {
        // ---------- 3. database ----------
        DatabaseConnection database("127.0.0.1", "needforspeed", "nfs", "underground");
        database.connect();
        std::cout << "Verbinding met MariaDB gelukt!\n";

        BrandRepository brandRepository(database);
        CarRepository   carRepository(database);

        CarBrandUI ui;
        LogoCache  logos(g_pd3dDevice);
        std::unordered_map<int, Brand> brandById;   // keeps the logo bytes alive

        auto loadBrands = [&]()
            {
                logos.clear();          // blobs may have changed -> drop cached textures
                brandById.clear();
                ui.setLogo((ImTextureID)0, 0, 0);

                std::vector<BrandItem> items;
                for (const Brand& b : brandRepository.getAll())
                {
                    items.push_back({ b.getId(), b.getName() });
                    brandById.emplace(b.getId(), b);
                }

                ui.setBrands(items);
                ui.setStatus("Merken geladen: " + std::to_string(items.size()));
            };

        ui.onRefresh = loadBrands;

        ui.onBrandSelected = [&](int brandId)
            {
                try
                {
                    auto it = brandById.find(brandId);
                    if (it == brandById.end())
                        return;

                    const Brand& b = it->second;

                    ui.setBrandInfo(
                        "Merk: " + b.getName(),
                        "Stad: " + b.getCityName(),
                        "Land: " + b.getCountryName(),
                        "Logo: " + std::to_string(b.getLogo().size()) + " bytes");

                    // ---- logo straight from the brand object ----
                    const auto& blob = b.getLogo();
                    LogoTexture lt = logos.get(brandId,
                        blob.empty() ? nullptr : blob.data(),
                        blob.size());
                    ui.setLogo((ImTextureID)lt.srv, lt.width, lt.height);

                    // ---- cars ----
                    std::vector<CarItem> carItems;
                    for (const Car& c : carRepository.getCarsByBrandId(brandId))
                    {
                        carItems.push_back({
                            c.getId(),
                            c.getModelName() + " - " + c.getTypeName() +
                            " - " + std::to_string(c.getPowerHP()) + " PK"
                            });
                    }
                    ui.setCars(carItems);
                    ui.setStatus("Auto's geladen: " + std::to_string(carItems.size()));
                }
                catch (const std::exception& e)
                {
                    ui.setStatus(std::string("Fout: ") + e.what());
                }
            };

        loadBrands();   // initial fill

        // ---------- 5. main loop ----------
        bool done = false;
        while (!done)
        {
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            // Window minimized / occluded -> don't burn CPU
            if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
            {
                ::Sleep(10);
                continue;
            }
            g_SwapChainOccluded = false;

            // Handle pending resize
            if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
            {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
                g_ResizeWidth = g_ResizeHeight = 0;
                CreateRenderTarget();
            }

            // ----- frame -----
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ui.render();                      // <-- your interface

            ImGui::Render();

            const float clear_color[4] = { 0.10f, 0.11f, 0.13f, 1.00f };
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            HRESULT hr = g_pSwapChain->Present(1, 0);   // 1 = vsync
            g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
        }

        database.disconnect();
    }
    catch (const sql::SQLException& e)
    {
        std::cerr << "Databasefout: " << e.what() << '\n';
        exitCode = 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fout: " << e.what() << '\n';
        exitCode = 1;
    }

    // ---------- 6. cleanup ----------
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return exitCode;
}

// ------------------------------------------------------------ D3D11 helpers
static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED)  // fall back to WARP software renderer
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION,
            &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release();        g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release();        g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// --------------------------------------------------------------- window proc
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)   // disable ALT application menu
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}