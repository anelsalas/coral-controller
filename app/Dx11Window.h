#pragma once
#include "common/Result.h"

#include <d3d11.h>
#include <windows.h>
#include <wrl/client.h>

#include <memory>
#include <string>

namespace coral::app {

// Win32 window + D3D11 swap chain + Dear ImGui Win32/DX11 backends.
class Dx11Window {
public:
    static Result<std::unique_ptr<Dx11Window>> create(const std::wstring& title, int width, int height);
    ~Dx11Window();
    Dx11Window(const Dx11Window&) = delete;
    Dx11Window& operator=(const Dx11Window&) = delete;

    bool pumpMessages();  // false once the window was closed
    void beginFrame();
    Status endFrame();

    ID3D11Device* device() const;
    ID3D11DeviceContext* context() const;

private:
    Dx11Window() = default;
    Status createWindow(const std::wstring& title, int width, int height);
    Status createDevice();
    Status createRenderTarget();
    void resizeIfRequested();
    static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    HWND window_ = nullptr;
    WNDCLASSEXW windowClass_{};
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTarget_;
    UINT pendingWidth_ = 0;
    UINT pendingHeight_ = 0;
    bool imguiWin32Ready_ = false;
    bool imguiDx11Ready_ = false;
};

}  // namespace coral::app