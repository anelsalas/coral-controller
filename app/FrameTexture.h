#pragma once
#include "common/Result.h"

#include <d3d11.h>
#include <imgui.h>
#include <opencv2/core.hpp>
#include <wrl/client.h>

namespace coral::app {

// GPU texture showing the latest processed frame (any 8-bit 1/3/4-channel image).
class FrameTexture {
public:
    FrameTexture(ID3D11Device* device, ID3D11DeviceContext* context);

    Status upload(const cv::Mat& image);
    bool hasImage() const;
    ImTextureID textureId() const;
    ImVec2 size() const;

private:
    Status recreate(int width, int height);

    ID3D11Device* device_;
    ID3D11DeviceContext* context_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_;
    int width_ = 0;
    int height_ = 0;
    cv::Mat rgba_;
};

}  // namespace coral::app