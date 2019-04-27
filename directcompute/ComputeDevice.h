#pragma once

#include <vector>
#include <winrt/base.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <d3dcompiler.h>

class ComputeDevice {
public:

    void Init(LPCWSTR pSrcFile, LPCSTR pFunctionName, UINT uSize) {
        buffer_size_ = uSize * 4;
        Init_();
        LoadShader_(pSrcFile, pFunctionName);
        CreateOutputBuffer_();
    }

    ~ComputeDevice() {
        if (is_out_buffer_mapped_)
            context_->Unmap(out_buffer_handle_.get(), 0);
    }

    void Run(UINT threadGroups) {
        context_->Dispatch(threadGroups, 1, 1);
    }

    UINT* GetOutput() {
        D3D11_BUFFER_DESC desc = {};
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.ByteWidth = buffer_size_;
        desc.MiscFlags = 0;
        winrt::check_hresult(device_->CreateBuffer(&desc, nullptr, out_buffer_handle_.put()));
#if defined(_DEBUG) || defined(PROFILE)
        out_buffer_handle_->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("OutBuffer") - 1, "OutBuffer");
#endif
        context_->CopyResource(out_buffer_handle_.get(), calc_buffer_handle_.get());
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        winrt::check_hresult(context_->Map(out_buffer_handle_.get(), 0, D3D11_MAP_READ, 0, &MappedResource));
        is_out_buffer_mapped_ = true;
        return static_cast<UINT*>(MappedResource.pData);
    }

    HRESULT GetError() {
        return device_->GetDeviceRemovedReason();
    }

private:
    void Init_() {
        UINT uCreationFlags = D3D11_CREATE_DEVICE_SINGLETHREADED; // (main cpu) program is single-threaded
#ifdef _DEBUG
        uCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL flOut;
        static const D3D_FEATURE_LEVEL flvl[] = { D3D_FEATURE_LEVEL_11_0 };

        winrt::check_hresult(D3D11CreateDevice(nullptr,  // Use default graphics card
            D3D_DRIVER_TYPE_HARDWARE,         // Try to create a hardware accelerated device
            nullptr,                          // Do not use external software rasterizer module
            uCreationFlags,                   // Device creation flags
            flvl,
            sizeof(flvl) / sizeof(D3D_FEATURE_LEVEL),
            D3D11_SDK_VERSION,                // SDK version
            device_.put(),                      // Device out
            &flOut,                           // Actual feature level created
            context_.put()));                    // Context out

        // A hardware accelerated device has been created, so check for Compute Shader support
        // If we have a device >= D3D_FEATURE_LEVEL_11_0 created, full CS5.0 support is guaranteed, no need for further checks
        if (flOut < D3D_FEATURE_LEVEL_11_0)
            throw std::runtime_error("Compute device does not support DX11 and CS5.0");

        // Double-precision support is an optional feature of CS 5.0
        D3D11_FEATURE_DATA_DOUBLES hwopts;
        device_->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &hwopts, sizeof(hwopts));
        if (!hwopts.DoublePrecisionFloatShaderOps)
            throw std::runtime_error("Compute device does not support double-precision");
    }

    void LoadShader_(LPCWSTR pSrcFile, LPCSTR pFunctionName) {
        DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        dwShaderFlags |= D3DCOMPILE_DEBUG;
        dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        const D3D_SHADER_MACRO defines[] =
        {
            nullptr, nullptr
        };
        winrt::com_ptr<ID3DBlob> pErrorBlob;
        winrt::com_ptr<ID3DBlob> pBlob;
        if (FAILED(D3DCompileFromFile(
            pSrcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, pFunctionName, "cs_5_0",
            dwShaderFlags, 0, pBlob.put(), pErrorBlob.put()))) {
            if (pErrorBlob)
                throw std::runtime_error((char*)pErrorBlob->GetBufferPointer());
            throw std::runtime_error("Compiling shader failed");
        }

        winrt::check_hresult(device_->CreateComputeShader(
            pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, shader_.put()));

#if defined(_DEBUG) || defined(PROFILE)
        shader_->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(pFunctionName), pFunctionName);
#endif

        context_->CSSetShader(shader_.get(), nullptr, 0);
    }

    void CreateOutputBuffer_() {
        D3D11_BUFFER_DESC desc = {};
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        desc.ByteWidth = buffer_size_;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        std::vector<UCHAR> out_buffer_init(buffer_size_, 0);
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = out_buffer_init.data();
        winrt::check_hresult(device_->CreateBuffer(&desc, &InitData, calc_buffer_handle_.put()));
#if defined(_DEBUG) || defined(PROFILE)
        calc_buffer_handle_->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("Result") - 1, "Result");
#endif

        D3D11_UNORDERED_ACCESS_VIEW_DESC desc_view = {};
        desc_view.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        desc_view.Buffer.FirstElement = 0;
        desc_view.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
        desc_view.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
        desc_view.Buffer.NumElements = buffer_size_ / 4;
        winrt::check_hresult(device_->CreateUnorderedAccessView(calc_buffer_handle_.get(), &desc_view, calc_buffer_view_.put()));

        ID3D11UnorderedAccessView* views[]{ calc_buffer_view_.get() };
        context_->CSSetUnorderedAccessViews(0, 1, views, nullptr);
    }

    winrt::com_ptr<ID3D11Device> device_;
    winrt::com_ptr<ID3D11DeviceContext> context_;
    winrt::com_ptr<ID3D11ComputeShader> shader_;
    winrt::com_ptr <ID3D11Buffer> calc_buffer_handle_;
    winrt::com_ptr <ID3D11UnorderedAccessView> calc_buffer_view_;
    winrt::com_ptr <ID3D11Buffer> out_buffer_handle_;
    UINT buffer_size_;
    bool is_out_buffer_mapped_{ false };
};
