#include <windows.h>
#include <d3d11.h>
#include <future>
#include <chrono>
#include <string>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "database.h"
#include "queries.h"

#pragma comment(lib, "d3d11.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_swap = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;

void CreateRTV() {
    ID3D11Texture2D* back = nullptr;
    g_swap->GetBuffer(0, IID_PPV_ARGS(&back));
    g_dev->CreateRenderTargetView(back, nullptr, &g_rtv);
    back->Release();
}

LRESULT WINAPI WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    switch (m) {
    case WM_SIZE:
        if (g_dev && w != SIZE_MINIMIZED) {
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
            g_swap->ResizeBuffers(0, LOWORD(l), HIWORD(l), DXGI_FORMAT_UNKNOWN, 0);
            CreateRTV();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0, 0, hi, nullptr, nullptr, nullptr, nullptr, L"App", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Mijn project", WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800, nullptr, nullptr, hi, nullptr);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_swap, &g_dev, &fl, &g_ctx);
    CreateRTV();

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    bool running = true;
    while (running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static Db db;
        static std::future<bool> fut;
        static bool connecting = false;
        static std::string status = "Niet verbonden";

        ImGui::Begin("Database");
        ImGui::TextUnformatted(status.c_str());

        if (!connecting && ImGui::Button("Verbind")) {
            connecting = true;
            status = "Verbinden...";
            fut = std::async(std::launch::async, [] {
                return db.Connect("localhost", "needforspeed", "nfs", "underground");
                });
        }
        if (connecting && fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            connecting = false;
            status = fut.get() ? "Verbonden" : "Fout: " + db.error;
        }

        static std::vector<std::string> cols;
        static std::vector<std::vector<std::string>> rows;
        static std::future<bool> qfut;
        static bool querying = false;

        if (db.conn && !querying && ImGui::Button("Laad auto's")) {
            querying = true;
            qfut = std::async(std::launch::async, [] {
                return db.Query(QUERY_CARS, cols, rows);
                });
        }
        if (querying && qfut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            querying = false;
            if (!qfut.get()) status = "Fout: " + db.error;
        }

        if (!querying && !cols.empty() &&
            ImGui::BeginTable("cars", (int)cols.size(),
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            for (auto& c : cols) ImGui::TableSetupColumn(c.c_str());
            ImGui::TableHeadersRow();
            for (auto& r : rows) {
                ImGui::TableNextRow();
                for (int i = 0; i < (int)r.size(); i++) {
                    ImGui::TableSetColumnIndex(i);
                    ImGui::TextUnformatted(r[i].c_str());
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Render();
        const float clear[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swap->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (g_rtv) g_rtv->Release();
    g_swap->Release(); g_ctx->Release(); g_dev->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hi);
    return 0;
}