#include "Engine/Rendering/Backends/DirectX12RenderBackend.h"

#if !defined(_WIN32)

#include "Engine/Core/Log.h"

namespace rg
{
DirectX12RenderBackend::DirectX12RenderBackend() = default;
DirectX12RenderBackend::~DirectX12RenderBackend() = default;

bool DirectX12RenderBackend::Initialize(ResourceManager& resources, const RenderBackendContext& context)
{
    (void)resources;
    (void)context;
    Log::Write(LogLevel::Warning, "DirectX12 backend is only available on Windows.");
    return false;
}

void DirectX12RenderBackend::Render(
    const World& world,
    const minecraft::VoxelWorld* voxelWorld,
    const std::uint64_t frameIndex,
    const UiRenderCallback& uiCallback)
{
    (void)world;
    (void)voxelWorld;
    (void)frameIndex;
    (void)uiCallback;
}

const char* DirectX12RenderBackend::Name() const
{
    return "DirectX12";
}
} // namespace rg

#else

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <DirectXCollision.h>
#include <DirectXMath.h>

#include "Engine/Core/Log.h"
#include "Engine/Scene/Components.h"
#include "Game/Minecraft/VoxelMesher.h"
#include "Game/Minecraft/VoxelWorld.h"

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#endif

namespace rg
{
namespace
{
using Microsoft::WRL::ComPtr;

constexpr UINT kFrameCount = 2;
constexpr DXGI_FORMAT kBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D32_FLOAT;
constexpr float kCameraFovRadians = 1.1519173f;
constexpr float kCameraNear = 0.1f;
constexpr float kCameraFar = 1200.0f;

struct FrameConstants
{
    DirectX::XMFLOAT4X4 viewProj {};
};

struct ChunkGpuMesh
{
    int chunkX = 0;
    int chunkZ = 0;
    std::uint32_t indexCount = 0;
    DirectX::XMFLOAT3 boundsMin {};
    DirectX::XMFLOAT3 boundsMax {};
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexView {};
    D3D12_INDEX_BUFFER_VIEW indexView {};
};

std::string HrMessage(const char* stage, const HRESULT hr)
{
    std::ostringstream oss;
    oss << "[DirectX12] " << stage << " failed (HRESULT=0x" << std::hex << std::uppercase
        << static_cast<std::uint32_t>(hr) << ")";
    return oss.str();
}

bool LogIfFailed(const char* stage, const HRESULT hr)
{
    if (SUCCEEDED(hr))
    {
        return false;
    }

    Log::Write(LogLevel::Error, HrMessage(stage, hr));
    return true;
}

const char* kVoxelShaderSource = R"(
cbuffer FrameCB : register(b0)
{
    float4x4 uViewProj;
};

struct VSInput
{
    float3 position : POSITION;
    uint color : COLOR0;
};

struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

float4 DecodeColor(uint c)
{
    float r = ((c >> 16u) & 255u) / 255.0;
    float g = ((c >> 8u) & 255u) / 255.0;
    float b = (c & 255u) / 255.0;
    float a = ((c >> 24u) & 255u) / 255.0;
    return float4(r, g, b, a);
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0), uViewProj);
    output.color = DecodeColor(input.color);
    return output;
}

float4 PSMain(PSInput input) : SV_Target
{
    return input.color;
}
)";
} // namespace

struct DirectX12RenderBackend::Impl
{
    [[nodiscard]] bool Initialize(const RenderBackendContext& context);
    void Shutdown();
    void WaitForGpu();
    [[nodiscard]] bool BeginFrame();
    [[nodiscard]] bool EndFrame();
    [[nodiscard]] bool CreateVoxelPipeline();
    [[nodiscard]] bool RebuildChunkMeshes(const minecraft::VoxelWorld& world);
    void DrawVoxelWorld(const World& world);
    void DrawEditorUi(const UiRenderCallback& uiCallback);

private:
    [[nodiscard]] bool CreateUploadBuffer(
        const void* sourceData,
        std::size_t sourceSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outResource) const;
    [[nodiscard]] bool CompileShader(
        const char* entryPoint,
        const char* profile,
        Microsoft::WRL::ComPtr<ID3DBlob>& outBlob) const;
    void BuildCamera(
        const World& world,
        DirectX::XMMATRIX& outView,
        DirectX::XMMATRIX& outProj,
        DirectX::BoundingFrustum& outWorldFrustum) const;

public:
    HWND hwnd = nullptr;
    std::uint32_t width = 1;
    std::uint32_t height = 1;
    bool vsync = true;

    ComPtr<IDXGIFactory6> factory;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<IDXGISwapChain3> swapChain;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    std::array<ComPtr<ID3D12Resource>, kFrameCount> backBuffers;
    std::array<ComPtr<ID3D12CommandAllocator>, kFrameCount> commandAllocators;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12Resource> depthBuffer;
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = nullptr;
    std::array<std::uint64_t, kFrameCount> fenceValues {};
    UINT frameIndex = 0;
    UINT rtvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
    ComPtr<ID3D12Resource> frameConstantBuffer;
    FrameConstants* mappedFrameConstants = nullptr;

    std::uint64_t voxelRevision = 0;
    std::vector<ChunkGpuMesh> chunkMeshes;

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    ComPtr<ID3D12DescriptorHeap> imguiSrvHeap;
    bool imguiInitialized = false;
#endif
};

DirectX12RenderBackend::DirectX12RenderBackend() : m_impl(std::make_unique<Impl>())
{
}

DirectX12RenderBackend::~DirectX12RenderBackend()
{
    if (m_impl != nullptr)
    {
        m_impl->Shutdown();
    }
}

bool DirectX12RenderBackend::Impl::Initialize(const RenderBackendContext& context)
{
    hwnd = static_cast<HWND>(context.nativeWindowHandle);
    if (hwnd == nullptr)
    {
        Log::Write(LogLevel::Error, "[DirectX12] Native Win32 window handle is null.");
        return false;
    }

    width = std::max<std::uint32_t>(1U, context.width);
    height = std::max<std::uint32_t>(1U, context.height);
    vsync = context.vsync;

    if ((context.width == 0U) || (context.height == 0U))
    {
        RECT clientRect {};
        if (GetClientRect(hwnd, &clientRect))
        {
            width = std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(clientRect.right - clientRect.left));
            height = std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(clientRect.bottom - clientRect.top));
        }
    }

    if (LogIfFailed("CreateDXGIFactory2", CreateDXGIFactory2(0U, IID_PPV_ARGS(&factory))))
    {
        return false;
    }

    ComPtr<IDXGIAdapter1> selectedAdapter;
    for (UINT adapterIndex = 0; factory->EnumAdapters1(adapterIndex, &selectedAdapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 description {};
        selectedAdapter->GetDesc1(&description);
        if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0U)
        {
            selectedAdapter.Reset();
            continue;
        }

        if (!LogIfFailed(
                "D3D12CreateDevice",
                D3D12CreateDevice(selectedAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
        {
            break;
        }

        selectedAdapter.Reset();
    }

    if (device == nullptr)
    {
        if (LogIfFailed("D3D12CreateDevice(DefaultAdapter)", D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
        {
            return false;
        }
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (LogIfFailed("CreateCommandQueue", device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))))
    {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = kBackBufferFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = kFrameCount;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Stereo = FALSE;

    ComPtr<IDXGISwapChain1> swapChain1;
    if (LogIfFailed(
            "CreateSwapChainForHwnd",
            factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1)))
    {
        return false;
    }
    if (LogIfFailed("MakeWindowAssociation", factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER)))
    {
        return false;
    }
    if (LogIfFailed("Query IDXGISwapChain3", swapChain1.As(&swapChain)))
    {
        return false;
    }
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc {};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = kFrameCount;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (LogIfFailed("CreateDescriptorHeap(RTV)", device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap))))
    {
        return false;
    }
    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < kFrameCount; ++i)
    {
        if (LogIfFailed("GetSwapChainBuffer", swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]))))
        {
            return false;
        }
        device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += static_cast<SIZE_T>(rtvDescriptorSize);

        if (LogIfFailed(
                "CreateCommandAllocator",
                device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i]))))
        {
            return false;
        }
    }

    if (LogIfFailed(
            "CreateCommandList",
            device->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                commandAllocators[frameIndex].Get(),
                nullptr,
                IID_PPV_ARGS(&commandList))))
    {
        return false;
    }
    if (LogIfFailed("Close(InitialCommandList)", commandList->Close()))
    {
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc {};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (LogIfFailed("CreateDescriptorHeap(DSV)", device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap))))
    {
        return false;
    }

    D3D12_HEAP_PROPERTIES depthHeapProps {};
    depthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC depthDesc {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = kDepthFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClear {};
    depthClear.Format = kDepthFormat;
    depthClear.DepthStencil.Depth = 1.0f;
    depthClear.DepthStencil.Stencil = 0;

    if (LogIfFailed(
            "CreateCommittedResource(Depth)",
            device->CreateCommittedResource(
                &depthHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &depthDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthClear,
                IID_PPV_ARGS(&depthBuffer))))
    {
        return false;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc {};
    dsvDesc.Format = kDepthFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

    if (LogIfFailed("CreateFence", device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
    {
        return false;
    }
    fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
    {
        Log::Write(LogLevel::Error, "[DirectX12] CreateEventW failed.");
        return false;
    }

    const UINT64 frameConstantsSize = (sizeof(FrameConstants) + 255ULL) & ~255ULL;
    D3D12_HEAP_PROPERTIES constantsHeapProps {};
    constantsHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC constantsDesc {};
    constantsDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantsDesc.Width = frameConstantsSize;
    constantsDesc.Height = 1;
    constantsDesc.DepthOrArraySize = 1;
    constantsDesc.MipLevels = 1;
    constantsDesc.SampleDesc.Count = 1;
    constantsDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (LogIfFailed(
            "CreateCommittedResource(FrameConstants)",
            device->CreateCommittedResource(
                &constantsHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &constantsDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&frameConstantBuffer))))
    {
        return false;
    }

    D3D12_RANGE noReadRange {0, 0};
    if (LogIfFailed(
            "Map(FrameConstants)",
            frameConstantBuffer->Map(0, &noReadRange, reinterpret_cast<void**>(&mappedFrameConstants))))
    {
        return false;
    }
    std::memset(mappedFrameConstants, 0, sizeof(FrameConstants));

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    D3D12_DESCRIPTOR_HEAP_DESC imguiHeapDesc {};
    imguiHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    imguiHeapDesc.NumDescriptors = 1;
    imguiHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (LogIfFailed("CreateDescriptorHeap(ImGui)", device->CreateDescriptorHeap(&imguiHeapDesc, IID_PPV_ARGS(&imguiSrvHeap))))
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(hwnd))
    {
        Log::Write(LogLevel::Error, "[DirectX12] ImGui_ImplWin32_Init failed.");
        return false;
    }
    if (!ImGui_ImplDX12_Init(
            device.Get(),
            static_cast<int>(kFrameCount),
            kBackBufferFormat,
            imguiSrvHeap.Get(),
            imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
            imguiSrvHeap->GetGPUDescriptorHandleForHeapStart()))
    {
        Log::Write(LogLevel::Error, "[DirectX12] ImGui_ImplDX12_Init failed.");
        ImGui_ImplWin32_Shutdown();
        return false;
    }

    imguiInitialized = true;
#endif

    return true;
}

void DirectX12RenderBackend::Impl::Shutdown()
{
    WaitForGpu();

#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (imguiInitialized)
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized = false;
    }
#endif

    if ((frameConstantBuffer != nullptr) && (mappedFrameConstants != nullptr))
    {
        frameConstantBuffer->Unmap(0, nullptr);
        mappedFrameConstants = nullptr;
    }

    chunkMeshes.clear();

    if (fenceEvent != nullptr)
    {
        CloseHandle(fenceEvent);
        fenceEvent = nullptr;
    }
}

void DirectX12RenderBackend::Impl::WaitForGpu()
{
    if ((commandQueue == nullptr) || (fence == nullptr) || (fenceEvent == nullptr))
    {
        return;
    }

    const std::uint64_t value = fenceValues[frameIndex] + 1ULL;
    if (FAILED(commandQueue->Signal(fence.Get(), value)))
    {
        return;
    }

    fenceValues[frameIndex] = value;
    if (fence->GetCompletedValue() < value)
    {
        fence->SetEventOnCompletion(value, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

bool DirectX12RenderBackend::Impl::CompileShader(
    const char* entryPoint,
    const char* profile,
    ComPtr<ID3DBlob>& outBlob) const
{
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG;
    compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> errorBlob;
    const HRESULT hr = D3DCompile(
        kVoxelShaderSource,
        std::strlen(kVoxelShaderSource),
        nullptr,
        nullptr,
        nullptr,
        entryPoint,
        profile,
        compileFlags,
        0,
        &outBlob,
        &errorBlob);

    if (SUCCEEDED(hr))
    {
        return true;
    }

    std::string details = HrMessage("D3DCompile", hr);
    if (errorBlob != nullptr)
    {
        details += " | ";
        details.append(
            static_cast<const char*>(errorBlob->GetBufferPointer()),
            static_cast<std::size_t>(errorBlob->GetBufferSize()));
    }
    Log::Write(LogLevel::Error, details);
    return false;
}

bool DirectX12RenderBackend::Impl::CreateVoxelPipeline()
{
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    if (!CompileShader("VSMain", "vs_5_0", vertexShader))
    {
        return false;
    }
    if (!CompileShader("PSMain", "ps_5_0", pixelShader))
    {
        return false;
    }

    D3D12_ROOT_PARAMETER rootParameter {};
    rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameter.Descriptor.ShaderRegister = 0;
    rootParameter.Descriptor.RegisterSpace = 0;
    rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc {};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = &rootParameter;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> serializedRootSignature;
    ComPtr<ID3DBlob> rootSignatureErrors;
    if (LogIfFailed(
            "D3D12SerializeRootSignature",
            D3D12SerializeRootSignature(
                &rootSignatureDesc,
                D3D_ROOT_SIGNATURE_VERSION_1,
                &serializedRootSignature,
                &rootSignatureErrors)))
    {
        if (rootSignatureErrors != nullptr)
        {
            Log::Write(
                LogLevel::Error,
                std::string("[DirectX12] RootSignature: ") +
                    static_cast<const char*>(rootSignatureErrors->GetBufferPointer()));
        }
        return false;
    }

    if (LogIfFailed(
            "CreateRootSignature",
            device->CreateRootSignature(
                0,
                serializedRootSignature->GetBufferPointer(),
                serializedRootSignature->GetBufferSize(),
                IID_PPV_ARGS(&rootSignature))))
    {
        return false;
    }

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32_UINT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_RASTERIZER_DESC rasterizer {};
    rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blend {};
    blend.AlphaToCoverageEnable = FALSE;
    blend.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultBlendTarget {
        FALSE,
        FALSE,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE,
        D3D12_BLEND_ZERO,
        D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL};
    for (auto& target : blend.RenderTarget)
    {
        target = defaultBlendTarget;
    }

    D3D12_DEPTH_STENCIL_DESC depthStencil {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc {};
    pipelineDesc.pRootSignature = rootSignature.Get();
    pipelineDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipelineDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipelineDesc.BlendState = blend;
    pipelineDesc.SampleMask = UINT_MAX;
    pipelineDesc.RasterizerState = rasterizer;
    pipelineDesc.DepthStencilState = depthStencil;
    pipelineDesc.InputLayout = {inputLayout, _countof(inputLayout)};
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = kBackBufferFormat;
    pipelineDesc.DSVFormat = kDepthFormat;
    pipelineDesc.SampleDesc.Count = 1;

    if (LogIfFailed("CreateGraphicsPipelineState", device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState))))
    {
        return false;
    }

    return true;
}

bool DirectX12RenderBackend::Impl::CreateUploadBuffer(
    const void* sourceData,
    const std::size_t sourceSize,
    ComPtr<ID3D12Resource>& outResource) const
{
    if ((sourceData == nullptr) || (sourceSize == 0U))
    {
        return false;
    }

    D3D12_HEAP_PROPERTIES heapProps {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC desc {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = static_cast<UINT64>(sourceSize);
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (LogIfFailed(
            "CreateCommittedResource(Upload)",
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&outResource))))
    {
        return false;
    }

    void* mapped = nullptr;
    D3D12_RANGE noReadRange {0, 0};
    if (LogIfFailed("Map(Upload)", outResource->Map(0, &noReadRange, &mapped)))
    {
        return false;
    }

    std::memcpy(mapped, sourceData, sourceSize);
    outResource->Unmap(0, nullptr);
    return true;
}

bool DirectX12RenderBackend::Impl::RebuildChunkMeshes(const minecraft::VoxelWorld& world)
{
    chunkMeshes.clear();
    chunkMeshes.reserve(world.LoadedChunkCount());

    std::size_t totalTriangles = 0;
    const auto chunks = world.ChunkCoordinates();
    for (const auto& [chunkX, chunkZ] : chunks)
    {
        minecraft::VoxelChunkMesh mesh = minecraft::VoxelMesher::BuildChunkMesh(world, chunkX, chunkZ);
        if (mesh.indices.empty())
        {
            continue;
        }

        const std::size_t vertexBytes = mesh.vertices.size() * sizeof(minecraft::VoxelVertex);
        const std::size_t indexBytes = mesh.indices.size() * sizeof(std::uint32_t);
        if ((vertexBytes > std::numeric_limits<UINT>::max()) || (indexBytes > std::numeric_limits<UINT>::max()))
        {
            continue;
        }

        ChunkGpuMesh gpuMesh;
        gpuMesh.chunkX = chunkX;
        gpuMesh.chunkZ = chunkZ;
        gpuMesh.indexCount = static_cast<std::uint32_t>(mesh.indices.size());
        gpuMesh.boundsMin = {mesh.boundsMin.x, mesh.boundsMin.y, mesh.boundsMin.z};
        gpuMesh.boundsMax = {mesh.boundsMax.x, mesh.boundsMax.y, mesh.boundsMax.z};

        if (!CreateUploadBuffer(mesh.vertices.data(), vertexBytes, gpuMesh.vertexBuffer))
        {
            continue;
        }
        if (!CreateUploadBuffer(mesh.indices.data(), indexBytes, gpuMesh.indexBuffer))
        {
            continue;
        }

        gpuMesh.vertexView.BufferLocation = gpuMesh.vertexBuffer->GetGPUVirtualAddress();
        gpuMesh.vertexView.StrideInBytes = sizeof(minecraft::VoxelVertex);
        gpuMesh.vertexView.SizeInBytes = static_cast<UINT>(vertexBytes);

        gpuMesh.indexView.BufferLocation = gpuMesh.indexBuffer->GetGPUVirtualAddress();
        gpuMesh.indexView.Format = DXGI_FORMAT_R32_UINT;
        gpuMesh.indexView.SizeInBytes = static_cast<UINT>(indexBytes);

        totalTriangles += (mesh.indices.size() / 3U);
        chunkMeshes.push_back(std::move(gpuMesh));
    }

    voxelRevision = world.Revision();
    Log::Write(
        LogLevel::Info,
        "[DirectX12] Rebuilt voxel meshes: chunks=" + std::to_string(chunkMeshes.size()) +
            ", triangles=" + std::to_string(totalTriangles));
    return true;
}

void DirectX12RenderBackend::Impl::BuildCamera(
    const World& world,
    DirectX::XMMATRIX& outView,
    DirectX::XMMATRIX& outProj,
    DirectX::BoundingFrustum& outWorldFrustum) const
{
    DirectX::XMFLOAT3 cameraPos {0.0f, 46.0f, -64.0f};
    DirectX::XMFLOAT3 cameraDir {0.25f, -0.35f, 1.0f};
    bool hasPlayer = false;

    world.ForEach<VoxelPlayerComponent, TransformComponent>([&](Entity /*entity*/, const VoxelPlayerComponent&, const TransformComponent& transform)
    {
        if (hasPlayer)
        {
            return;
        }

        cameraPos = {transform.position.x, transform.position.y + 1.7f, transform.position.z};
        cameraDir = {0.0f, -0.2f, 1.0f};
        hasPlayer = true;
    });

    const DirectX::XMVECTOR eye = DirectX::XMVectorSet(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
    const DirectX::XMVECTOR direction =
        DirectX::XMVector3Normalize(DirectX::XMVectorSet(cameraDir.x, cameraDir.y, cameraDir.z, 0.0f));
    const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    outView = DirectX::XMMatrixLookToLH(eye, direction, up);
    const float aspect = static_cast<float>(width) / static_cast<float>(std::max<std::uint32_t>(1U, height));
    outProj = DirectX::XMMatrixPerspectiveFovLH(kCameraFovRadians, aspect, kCameraNear, kCameraFar);

    DirectX::BoundingFrustum viewFrustum;
    DirectX::BoundingFrustum::CreateFromMatrix(viewFrustum, outProj);

    const DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, outView);
    viewFrustum.Transform(outWorldFrustum, invView);
}

void DirectX12RenderBackend::Impl::DrawVoxelWorld(const World& world)
{
    if ((pipelineState == nullptr) || (rootSignature == nullptr) || (mappedFrameConstants == nullptr))
    {
        return;
    }

    DirectX::XMMATRIX view;
    DirectX::XMMATRIX proj;
    DirectX::BoundingFrustum worldFrustum;
    BuildCamera(world, view, proj, worldFrustum);

    DirectX::XMStoreFloat4x4(&mappedFrameConstants->viewProj, DirectX::XMMatrixTranspose(view * proj));

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetGraphicsRootConstantBufferView(0, frameConstantBuffer->GetGPUVirtualAddress());

    for (const ChunkGpuMesh& mesh : chunkMeshes)
    {
        DirectX::BoundingBox chunkBounds;
        const DirectX::XMVECTOR minPoint = DirectX::XMLoadFloat3(&mesh.boundsMin);
        const DirectX::XMVECTOR maxPoint = DirectX::XMLoadFloat3(&mesh.boundsMax);
        DirectX::BoundingBox::CreateFromPoints(chunkBounds, minPoint, maxPoint);

        if (!worldFrustum.Intersects(chunkBounds))
        {
            continue;
        }

        commandList->IASetVertexBuffers(0, 1, &mesh.vertexView);
        commandList->IASetIndexBuffer(&mesh.indexView);
        commandList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
    }
}

void DirectX12RenderBackend::Impl::DrawEditorUi(const UiRenderCallback& uiCallback)
{
#if defined(RG_WITH_IMGUI) && RG_WITH_IMGUI
    if (!imguiInitialized)
    {
        return;
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (uiCallback)
    {
        uiCallback();
    }

    ImGui::Render();
    ID3D12DescriptorHeap* heaps[] = {imguiSrvHeap.Get()};
    commandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
#else
    (void)uiCallback;
#endif
}

bool DirectX12RenderBackend::Impl::BeginFrame()
{
    if ((fence != nullptr) && (fenceEvent != nullptr) && (fence->GetCompletedValue() < fenceValues[frameIndex]))
    {
        if (FAILED(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent)))
        {
            return false;
        }
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    if (FAILED(commandAllocators[frameIndex]->Reset()))
    {
        return false;
    }
    if (FAILED(commandList->Reset(commandAllocators[frameIndex].Get(), nullptr)))
    {
        return false;
    }

    D3D12_RESOURCE_BARRIER toRenderTarget {};
    toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRenderTarget.Transition.pResource = backBuffers[frameIndex].Get();
    toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &toRenderTarget);

    D3D12_VIEWPORT viewport {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MaxDepth = 1.0f;
    commandList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor {};
    scissor.right = static_cast<LONG>(width);
    scissor.bottom = static_cast<LONG>(height);
    commandList->RSSetScissorRects(1, &scissor);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += static_cast<SIZE_T>(frameIndex) * static_cast<SIZE_T>(rtvDescriptorSize);
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();

    const float clearColor[4] = {0.55f, 0.73f, 0.93f, 1.0f};
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    return true;
}

bool DirectX12RenderBackend::Impl::EndFrame()
{
    D3D12_RESOURCE_BARRIER toPresent {};
    toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent.Transition.pResource = backBuffers[frameIndex].Get();
    toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &toPresent);

    if (FAILED(commandList->Close()))
    {
        return false;
    }

    ID3D12CommandList* lists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(1, lists);

    if (FAILED(swapChain->Present(vsync ? 1U : 0U, 0U)))
    {
        return false;
    }

    const std::uint64_t signalValue = fenceValues[frameIndex] + 1ULL;
    if (FAILED(commandQueue->Signal(fence.Get(), signalValue)))
    {
        return false;
    }
    fenceValues[frameIndex] = signalValue;
    frameIndex = swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool DirectX12RenderBackend::Initialize(ResourceManager& resources, const RenderBackendContext& context)
{
    (void)resources;

    if (m_impl == nullptr)
    {
        return false;
    }

    if (!m_impl->Initialize(context))
    {
        return false;
    }

    m_pipelineReady = m_impl->CreateVoxelPipeline();
    if (!m_pipelineReady)
    {
        Log::Write(LogLevel::Warning, "[DirectX12] Voxel render pipeline failed to initialize. Scene draw will be skipped.");
    }

    m_reportedRenderFailure = false;
    Log::Write(LogLevel::Info, "[DirectX12] Backend initialized.");
    return true;
}

void DirectX12RenderBackend::Render(
    const World& world,
    const minecraft::VoxelWorld* voxelWorld,
    const std::uint64_t frameIndex,
    const UiRenderCallback& uiCallback)
{
    (void)frameIndex;

    if (m_impl == nullptr)
    {
        return;
    }

    if (!m_impl->BeginFrame())
    {
        if (!m_reportedRenderFailure)
        {
            Log::Write(LogLevel::Error, "[DirectX12] BeginFrame failed.");
            m_reportedRenderFailure = true;
        }
        return;
    }

    if (m_pipelineReady && (voxelWorld != nullptr))
    {
        if (voxelWorld->Revision() != m_impl->voxelRevision)
        {
            if (!m_impl->RebuildChunkMeshes(*voxelWorld))
            {
                m_pipelineReady = false;
            }
        }
        m_impl->DrawVoxelWorld(world);
    }

    m_impl->DrawEditorUi(uiCallback);

    if (!m_impl->EndFrame())
    {
        if (!m_reportedRenderFailure)
        {
            Log::Write(LogLevel::Error, "[DirectX12] EndFrame failed.");
            m_reportedRenderFailure = true;
        }
        return;
    }

    m_reportedRenderFailure = false;
}

const char* DirectX12RenderBackend::Name() const
{
    return "DirectX12";
}
} // namespace rg

#endif
