#include "Dx11Window.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <format>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

namespace coral::app {
namespace {
constexpr wchar_t kWindowClassName[] = L"CoralControllerWindow";
constexpr UINT kSwapChainBuffers = 2;
constexpr UINT kVsyncInterval = 1;
constexpr float kClearColor[4] = {0.08f, 0.08f, 0.10f, 1.0f};

std::string hresultText(const char* action, HRESULT result) {
    return std::format("{} failed (HRESULT 0x{:08X})", action, static_cast<unsigned>(result));
}
}  // namespace

Result<std::unique_ptr<Dx11Window>> Dx11Window::create(const std::wstring& title, int width, int height) {
    using WindowResult = Result<std::unique_ptr<Dx11Window>>;
    std::unique_ptr<Dx11Window> window(new Dx11Window());
    if (auto status = window->createWindow(title, width, height); !status.ok()) return WindowResult::failure(status.error());
    if (auto status = window->createDevice(); !status.ok()) return WindowResult::failure(status.error());
    if (auto status = window->createRenderTarget(); !status.ok()) return WindowResult::failure(status.error());

    ShowWindow(window->window_, SW_SHOWDEFAULT);
    UpdateWindow(window->window_);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    window->imguiWin32Ready_ = ImGui_ImplWin32_Init(window->window_);
    window->imguiDx11Ready_ = ImGui_ImplDX11_Init(window->device_.Get(), window->context_.Get());
    if (!window->imguiWin32Ready_ || !window->imguiDx11Ready_) return WindowResult::failure("ImGui backend init failed");
    return WindowResult::success(std::move(window));
}

Dx11Window::~Dx11Window() {
    if (imguiDx11Ready_) ImGui_ImplDX11_Shutdown();
    if (imguiWin32Ready_) ImGui_ImplWin32_Shutdown();
    if (ImGui::GetCurrentContext() != nullptr) ImGui::DestroyContext();
    if (window_ != nullptr) DestroyWindow(window_);
    UnregisterClassW(windowClass_.lpszClassName, windowClass_.hInstance);
}

Status Dx11Window::createWindow(const std::wstring& title, int width, int height) {
    windowClass_ = {sizeof(windowClass_), CS_CLASSDC, windowProcedure, 0, 0, GetModuleHandleW(nullptr),
                    nullptr, LoadCursorW(nullptr, IDC_ARROW), nullptr, nullptr, kWindowClassName, nullptr};
    if (RegisterClassExW(&windowClass_) == 0) return Status::failure(hresultText("RegisterClassEx", HRESULT_FROM_WIN32(GetLastError())));
    window_ = CreateWindowW(kWindowClassName, title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width,
                            height, nullptr, nullptr, windowClass_.hInstance, nullptr);
    if (window_ == nullptr) return Status::failure(hresultText("CreateWindow", HRESULT_FROM_WIN32(GetLastError())));
    SetWindowLongPtrW(window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    return okStatus();
}

Status Dx11Window::createDevice() {
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferCount = kSwapChainBuffers;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.OutputWindow = window_;
    description.SampleDesc.Count = 1;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL obtained{};
    HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
                                                   D3D11_SDK_VERSION, &description, &swapChain_, &device_, &obtained,
                                                   &context_);
    if (result == DXGI_ERROR_UNSUPPORTED) {  // no GPU driver: fall back to the WARP software rasterizer
        result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2,
                                               D3D11_SDK_VERSION, &description, &swapChain_, &device_, &obtained,
                                               &context_);
    }
    if (FAILED(result)) return Status::failure(hresultText("D3D11CreateDeviceAndSwapChain", result));
    return okStatus();
}

Status Dx11Window::createRenderTarget() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT result = swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(result)) return Status::failure(hresultText("IDXGISwapChain::GetBuffer", result));
    result = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTarget_);
    if (FAILED(result)) return Status::failure(hresultText("CreateRenderTargetView", result));
    return okStatus();
}

void Dx11Window::resizeIfRequested() {
    if (pendingWidth_ == 0 || pendingHeight_ == 0) return;
    renderTarget_.Reset();
    swapChain_->ResizeBuffers(0, pendingWidth_, pendingHeight_, DXGI_FORMAT_UNKNOWN, 0);
    pendingWidth_ = pendingHeight_ = 0;
    if (const auto status = createRenderTarget(); !status.ok()) OutputDebugStringA(status.error().c_str());
}

bool Dx11Window::pumpMessages() {
    MSG message;
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
        if (message.message == WM_QUIT) return false;
    }
    return true;
}

void Dx11Window::beginFrame() {
    resizeIfRequested();
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

Status Dx11Window::endFrame() {
    ImGui::Render();
    ID3D11RenderTargetView* targets[] = {renderTarget_.Get()};
    context_->OMSetRenderTargets(1, targets, nullptr);
    context_->ClearRenderTargetView(renderTarget_.Get(), kClearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    const HRESULT result = swapChain_->Present(kVsyncInterval, 0);
    if (FAILED(result)) return Status::failure(hresultText("IDXGISwapChain::Present", result));
    return okStatus();
}

ID3D11Device* Dx11Window::device() const { return device_.Get(); }
ID3D11DeviceContext* Dx11Window::context() const { return context_.Get(); }

LRESULT CALLBACK Dx11Window::windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam)) return TRUE;
    auto* self = reinterpret_cast<Dx11Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
        case WM_SIZE:
            if (self != nullptr && wParam != SIZE_MINIMIZED) {
                self->pendingWidth_ = LOWORD(lParam);
                self->pendingHeight_ = HIWORD(lParam);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wParam, lParam);
    }
}

}  // namespace coral::app