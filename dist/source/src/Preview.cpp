#include <Windows.h>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <cassert>
#include <cstdio>
#include <vector>
#include "Hud.hpp"

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int) {
    if(MphToKmh(0)!=0 || MphToKmh(100)!=161 || MphToKmh(-100)!=161 || MphToKmh(200)!=322) return 2;
    WNDCLASSW wc{}; wc.lpfnWndProc=DefWindowProcW; wc.hInstance=instance; wc.lpszClassName=L"SpeedometerPreview";
    RegisterClassW(&wc);
    RECT size{0,0,1280,720}; AdjustWindowRect(&size,WS_OVERLAPPEDWINDOW,FALSE);
    HWND window=CreateWindowW(wc.lpszClassName,L"BPR Center Speedometer - renderer preview",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,size.right-size.left,size.bottom-size.top,nullptr,nullptr,instance,nullptr);
    DXGI_SWAP_CHAIN_DESC desc{}; desc.BufferCount=1; desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow=window; desc.SampleDesc.Count=1; desc.Windowed=TRUE;
    IDXGISwapChain* swap=nullptr; ID3D11Device* device=nullptr; ID3D11DeviceContext* context=nullptr;
    if(FAILED(D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&swap,&device,nullptr,&context))) return 1;
    ID3D11Texture2D* buffer=nullptr; swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&buffer));
    ID3D11RenderTargetView* target=nullptr; device->CreateRenderTargetView(buffer,nullptr,&target); buffer->Release();
    ImGui::CreateContext(); ImGui::GetIO().IniFilename=nullptr;
    ImFont* font=LoadHudFont(); ImGui_ImplDX11_Init(device,context);
    ShowWindow(window,SW_SHOWNOACTIVATE);
    for(int frame=0; frame<3; ++frame) {
        MSG message; while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
        ImGui::GetIO().DisplaySize=ImVec2(1280,720); ImGui::GetIO().DeltaTime=1.0f/60.0f;
        ImGui_ImplDX11_NewFrame(); ImGui::NewFrame(); DrawSpeedometer(187,HudSettings{},font); ImGui::Render();
        float clear[]{0.1f,0.14f,0.18f,1.0f}; context->OMSetRenderTargets(1,&target,nullptr); context->ClearRenderTargetView(target,clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if(frame==2) {
            ID3D11Texture2D* source=nullptr; swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&source));
            D3D11_TEXTURE2D_DESC td{}; source->GetDesc(&td);
            td.Usage=D3D11_USAGE_STAGING; td.BindFlags=0; td.CPUAccessFlags=D3D11_CPU_ACCESS_READ; td.MiscFlags=0;
            ID3D11Texture2D* staging=nullptr;
            if(FAILED(device->CreateTexture2D(&td,nullptr,&staging)))return 3;
            context->CopyResource(staging,source); source->Release();
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if(FAILED(context->Map(staging,0,D3D11_MAP_READ,0,&mapped)))return 4;
            BITMAPFILEHEADER fileHeader{}; fileHeader.bfType=0x4D42; fileHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER); fileHeader.bfSize=fileHeader.bfOffBits+td.Width*td.Height*4;
            BITMAPINFOHEADER info{}; info.biSize=sizeof(info);info.biWidth=td.Width;info.biHeight=-static_cast<LONG>(td.Height);info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;
            FILE* out=nullptr; fopen_s(&out,"artifacts/preview.bmp","wb"); if(!out)return 5;
            fwrite(&fileHeader,sizeof(fileHeader),1,out);fwrite(&info,sizeof(info),1,out);
            std::vector<unsigned char> row(td.Width*4);
            for(UINT y=0;y<td.Height;++y) {
                auto* pixels=static_cast<unsigned char*>(mapped.pData)+y*mapped.RowPitch;
                for(UINT x=0;x<td.Width;++x){row[x*4]=pixels[x*4+2];row[x*4+1]=pixels[x*4+1];row[x*4+2]=pixels[x*4];row[x*4+3]=255;}
                fwrite(row.data(),row.size(),1,out);
            }
            fclose(out);context->Unmap(staging,0);staging->Release();
        }
        swap->Present(1,0);
    }
    ImGui_ImplDX11_Shutdown(); ImGui::DestroyContext(); target->Release(); context->Release(); device->Release(); swap->Release();
    if(IsWindow(window))DestroyWindow(window);
    return 0;
}
