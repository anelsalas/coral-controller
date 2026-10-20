#include "FrameTexture.h"

#include <opencv2/imgproc.hpp>

#include <format>

namespace coral::app {

FrameTexture::FrameTexture(ID3D11Device* device, ID3D11DeviceContext* context) : device_(device), context_(context) {}

Status FrameTexture::upload(const cv::Mat& image) {
    if (image.empty() || image.depth() != CV_8U) return Status::failure("display needs a non-empty 8-bit image");
    switch (image.channels()) {
        case 1: cv::cvtColor(image, rgba_, cv::COLOR_GRAY2RGBA); break;
        case 3: cv::cvtColor(image, rgba_, cv::COLOR_BGR2RGBA); break;
        case 4: cv::cvtColor(image, rgba_, cv::COLOR_BGRA2RGBA); break;
        default: return Status::failure(std::format("cannot display {} channels", image.channels()));
    }
    if (rgba_.cols != width_ || rgba_.rows != height_) {
        if (auto status = recreate(rgba_.cols, rgba_.rows); !status.ok()) return status;
    }
    context_->UpdateSubresource(texture_.Get(), 0, nullptr, rgba_.data, static_cast<UINT>(rgba_.step), 0);
    return okStatus();
}

Status FrameTexture::recreate(int width, int height) {
    view_.Reset();
    texture_.Reset();
    width_ = height_ = 0;
    D3D11_TEXTURE2D_DESC description{};
    description.Width = static_cast<UINT>(width);
    description.Height = static_cast<UINT>(height);
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    HRESULT result = device_->CreateTexture2D(&description, nullptr, &texture_);
    if (FAILED(result)) return Status::failure(std::format("CreateTexture2D failed (0x{:08X})", static_cast<unsigned>(result)));
    result = device_->CreateShaderResourceView(texture_.Get(), nullptr, &view_);
    if (FAILED(result)) return Status::failure(std::format("CreateShaderResourceView failed (0x{:08X})", static_cast<unsigned>(result)));
    width_ = width;
    height_ = height;
    return okStatus();
}

bool FrameTexture::hasImage() const { return view_ != nullptr; }
ImTextureID FrameTexture::textureId() const { return (ImTextureID)(intptr_t)view_.Get(); }
ImVec2 FrameTexture::size() const { return ImVec2(static_cast<float>(width_), static_cast<float>(height_)); }

}  // namespace coral::app