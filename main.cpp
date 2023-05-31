#include <iostream>
#include <format>

#include "memory.h"

#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>
#include <windowsx.h>

#include <thread>


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
        return 0L;
    }

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0L;
    }
    switch (message)
    {
    case WM_NCHITTEST:
    {
        // Allow the window to be moved by treating the title bar area as the caption
        const LONG borderWidth = GetSystemMetrics(SM_CXSIZEFRAME);
        const LONG titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
        POINT cursorPos = { GET_X_LPARAM(w_param), GET_Y_LPARAM(l_param) };
        RECT windowRect;
        GetWindowRect(window, &windowRect);

        if (cursorPos.y >= windowRect.top && cursorPos.y < windowRect.top + titleBarHeight)
            return HTCAPTION;

        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(window, message, w_param, l_param);
}


int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_procedure;
    wc.hInstance = instance;
    wc.lpszClassName = L"External Overlay Class";

    RegisterClassExW(&wc);

    const HWND window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName,
        L"Crosshair",
        WS_POPUP,
        0,
        0,
        screenWidth,
        screenHeight,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    {
        RECT client_area{};
        GetClientRect(window, &client_area);

        RECT window_area{};
        GetWindowRect(window, &window_area);

        POINT diff{};
        ClientToScreen(window, &diff);

        const MARGINS margins{
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom,

        };

        DwmExtendFrameIntoClientArea(window, &margins);
    }


    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.RefreshRate.Numerator = 60U;
    sd.BufferDesc.RefreshRate.Denominator = 1U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2U;
    sd.OutputWindow = window;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    constexpr D3D_FEATURE_LEVEL levels[2]{
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    ID3D11Device* device{ nullptr };
    ID3D11DeviceContext* device_context{ nullptr };
    IDXGISwapChain* swap_chain{ nullptr };
    ID3D11RenderTargetView* render_target_view{ nullptr };
    D3D_FEATURE_LEVEL level{};

    //create device and that
    D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0U,
        levels,
        2U,
        D3D11_SDK_VERSION,
        &sd,
        &swap_chain,
        &device,
        &level,
        &device_context
    );

    ID3D11Texture2D* back_buffer{ nullptr };
    swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

    if (back_buffer) {
        device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
        back_buffer->Release();
    }
    else {
        return 1;
    }

    ShowWindow(window, cmd_show);
    UpdateWindow(window);

    ImGui::CreateContext();

    //set up ImGui style andcontent
    ImGui::StyleColorsDark();

    //Display the additional window

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);

    bool running = true;

    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    float crosshairSize = 10.0f;
    float lineWidth = 1.5f;
    float gapSize = 4.0f;
    float dotSize = 2.0f;

    while (running) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                running = false;
            }
        }

        if (!running) {
            break;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        //drawList->AddCircleFilled({ static_cast<float>(screenWidth) / 2, static_cast<float>(screenHeight) / 2 }, 5.f, ImColor(1.f, 0.f, 0.f));
        // Draw horizontal line
        drawList->AddLine({ centerX - crosshairSize - gapSize, centerY }, { centerX - gapSize, centerY }, ImColor(255, 255, 255), lineWidth);
        drawList->AddLine({ centerX + gapSize, centerY }, { centerX + crosshairSize + gapSize, centerY }, ImColor(255, 255, 255), lineWidth);

        // Draw vertical line
        drawList->AddLine({ centerX, centerY - crosshairSize - gapSize }, { centerX, centerY - gapSize }, ImColor(255, 255, 255), lineWidth);
        drawList->AddLine({ centerX, centerY + gapSize }, { centerX, centerY + crosshairSize + gapSize }, ImColor(255, 255, 255), lineWidth);

        // Draw dot in the center
        drawList->AddCircleFilled({ centerX, centerY }, dotSize, ImColor(255, 255, 255));


        //Rendering :)
        ImGui::Render();


        constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
        device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
        device_context->ClearRenderTargetView(render_target_view, color);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swap_chain->Present(1U, 0U);

        
    }


    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();

    if (swap_chain) {
        swap_chain->Release();
    }

    if (device_context) {
        device_context->Release();
    }

    if (device) {
        device->Release();
    }

    if (render_target_view) {
        render_target_view->Release();
    }

    DestroyWindow(window);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

