#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <atomic>
#include <vector>
#include "Hud.hpp"
#include "Collectibles.hpp"

namespace {
HMODULE selfModule;
std::wstring configPath, logPath;
std::mutex renderMutex;
using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
PresentFn originalPresent = nullptr;
ImGuiContext* hudContext = nullptr;
ID3D11Device* hudDevice = nullptr;
ID3D11DeviceContext* hudDeviceContext = nullptr;
IDXGISwapChain* hudSwapChain = nullptr; // Borrowed; do not retain back buffers across resize.
ImFont* hudFont = nullptr;
HudSettings settings;
bool enabled = true;
std::atomic<bool> collectiblesEnabled{false};
bool captureMap = false;
std::wstring capturePath;
uintptr_t gameBase = 0;

void Log(const char* message) {
    FILE* file = nullptr;
    if (_wfopen_s(&file, logPath.c_str(), L"a") == 0 && file) {
        SYSTEMTIME t{}; GetLocalTime(&t);
        std::fprintf(file, "%02u:%02u:%02u %s\n", t.wHour,t.wMinute,t.wSecond,message);
        std::fclose(file);
    }
}

void CaptureBackBuffer(IDXGISwapChain* swap) {
    ID3D11Texture2D* source=nullptr;
    if(FAILED(swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&source))))return;
    D3D11_TEXTURE2D_DESC td{};source->GetDesc(&td);
    if(td.Format!=DXGI_FORMAT_R8G8B8A8_UNORM && td.Format!=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB && td.Format!=DXGI_FORMAT_B8G8R8A8_UNORM){source->Release();Log("Map capture: unsupported format.");return;}
    td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;td.MiscFlags=0;
    ID3D11Texture2D* staging=nullptr;
    if(FAILED(hudDevice->CreateTexture2D(&td,nullptr,&staging))){source->Release();return;}
    hudDeviceContext->CopyResource(staging,source);source->Release();
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if(SUCCEEDED(hudDeviceContext->Map(staging,0,D3D11_MAP_READ,0,&mapped))) {
        FILE* file=nullptr;_wfopen_s(&file,capturePath.c_str(),L"wb");
        if(file) {
            BITMAPFILEHEADER fh{};fh.bfType=0x4d42;fh.bfOffBits=54;fh.bfSize=54+td.Width*td.Height*4;
            BITMAPINFOHEADER ih{};ih.biSize=40;ih.biWidth=td.Width;ih.biHeight=-static_cast<LONG>(td.Height);ih.biPlanes=1;ih.biBitCount=32;
            fwrite(&fh,sizeof(fh),1,file);fwrite(&ih,sizeof(ih),1,file);
            std::vector<unsigned char> row(td.Width*4);
            for(UINT y=0;y<td.Height;++y){auto* pixels=static_cast<unsigned char*>(mapped.pData)+y*mapped.RowPitch;
                for(UINT x=0;x<td.Width;++x){row[x*4]=pixels[x*4+(td.Format==DXGI_FORMAT_B8G8R8A8_UNORM?0:2)];row[x*4+1]=pixels[x*4+1];row[x*4+2]=pixels[x*4+(td.Format==DXGI_FORMAT_B8G8R8A8_UNORM?2:0)];row[x*4+3]=255;}
                fwrite(row.data(),row.size(),1,file);
            }
            fclose(file);Log("Map capture saved to Collectibles-map.bmp.");
        }
        hudDeviceContext->Unmap(staging,0);
    }
    staging->Release();
}

template<class T> bool Read(uintptr_t address, T& value) {
    SIZE_T bytes = 0;
    return address >= 0x10000 && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address), &value, sizeof(T), &bytes) && bytes == sizeof(T);
}

// Source of offsets: matty-ross/bpr-mods-repository, DashboardHud.cpp (GPLv3).
bool ReadSpeed(int& kmh) {
    uintptr_t module = 0;
    if (!Read(gameBase + (0x013FC8E0 - 0x00400000), module) || module == 0) return false;
    uintptr_t raceMainHudState = 0;
    if (!Read(module + 0x7FABBC, raceMainHudState) || raceMainHudState == 0) return false;
    unsigned char drivingHud = 0;
    int engineState = 0, mph = 0;
    if (!Read(raceMainHudState + 0x14C, drivingHud) || drivingHud != 1) return false;
    if (!Read(module + 0x8EFEC0 + 0x48, engineState) || engineState <= 0 || engineState > 16) return false;
    if (!Read(module + 0x8EFEC0 + 0x30, mph) || mph < -620 || mph > 620) return false;
    kmh = MphToKmh(mph);
    return true;
}

void ReleaseRenderer() {
    if (hudContext) {
        ImGui::SetCurrentContext(hudContext);
        ImGui_ImplDX11_Shutdown();
        ImGui::DestroyContext(hudContext);
    }
    if (hudDeviceContext) hudDeviceContext->Release();
    if (hudDevice) hudDevice->Release();
    hudContext = nullptr; hudDevice = nullptr; hudDeviceContext = nullptr; hudSwapChain = nullptr;
}

bool InitializeRenderer(IDXGISwapChain* swap) {
    if (FAILED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&hudDevice)))) return false;
    hudDevice->GetImmediateContext(&hudDeviceContext);
    hudContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; io.LogFilename = nullptr;
    hudFont = LoadHudFont();
    if (!ImGui_ImplDX11_Init(hudDevice, hudDeviceContext)) { ReleaseRenderer(); return false; }
    hudSwapChain = swap;
    Log("D3D11 speedometer renderer initialized.");
    return true;
}

HRESULT STDMETHODCALLTYPE HookPresent(IDXGISwapChain* swap, UINT sync, UINT flags) {
    if (!(flags & DXGI_PRESENT_TEST)) {
        std::lock_guard<std::mutex> guard(renderMutex);
        ImGuiContext* previousContext = ImGui::GetCurrentContext();
        if (hudSwapChain && hudSwapChain != swap) {
            // Ignore secondary swapchains rather than moving resources between windows.
        } else if (hudContext || InitializeRenderer(swap)) {
            ImGui::SetCurrentContext(hudContext);
            DXGI_SWAP_CHAIN_DESC desc{};
            swap->GetDesc(&desc);
            RECT client{}; GetClientRect(desc.OutputWindow, &client);
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(static_cast<float>(client.right), static_cast<float>(client.bottom));
            LARGE_INTEGER now{}, frequency{}; QueryPerformanceCounter(&now); QueryPerformanceFrequency(&frequency);
            static LONGLONG lastTick = 0;
            io.DeltaTime = lastTick ? std::clamp(static_cast<float>(now.QuadPart-lastTick)/frequency.QuadPart,0.001f,0.1f) : 1.0f/60.0f;
            lastTick = now.QuadPart;
            if ((enabled || collectiblesEnabled) && io.DisplaySize.x > 0 && io.DisplaySize.y > 0) {
                int kmh = 0;
                const bool valid = ReadSpeed(kmh);
                static bool firstTelemetry = true;
                static ULONGLONG lastLog = 0;
                if (valid && (firstTelemetry || GetTickCount64()-lastLog > 5000)) {
                    char message[100]; std::snprintf(message,sizeof(message),"Driving telemetry: %d km/h; viewport %.0fx%.0f",kmh,io.DisplaySize.x,io.DisplaySize.y);
                    Log(message); firstTelemetry = false; lastLog = GetTickCount64();
                }
                ImGui_ImplDX11_NewFrame();
                ImGui::NewFrame();
                if (valid && enabled) DrawSpeedometer(kmh, settings, hudFont);
                bool mapDrawn=collectiblesEnabled && DrawCollectibles(desc.OutputWindow, hudFont);
                ImGui::Render();
                ID3D11Texture2D* backBuffer = nullptr;
                ID3D11RenderTargetView* target = nullptr;
                if (SUCCEEDED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)))) {
                    hudDevice->CreateRenderTargetView(backBuffer,nullptr,&target);
                    backBuffer->Release();
                }
                if (target) {
                    ID3D11RenderTargetView* oldTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
                    ID3D11DepthStencilView* oldDepth = nullptr;
                    hudDeviceContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,oldTargets,&oldDepth);
                    hudDeviceContext->OMSetRenderTargets(1,&target,nullptr);
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                    static unsigned mapFrames=0;
                    if(mapDrawn && captureMap && ++mapFrames==45) CaptureBackBuffer(swap);
                    hudDeviceContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,oldTargets,oldDepth);
                    for (auto* old : oldTargets) if(old) old->Release();
                    if (oldDepth) oldDepth->Release();
                    target->Release();
                }
            }
        }
        ImGui::SetCurrentContext(previousContext);
    }
    return originalPresent(swap,sync,flags);
}

DWORD WINAPI StartMod(void*) {
    wchar_t path[MAX_PATH]{}; GetModuleFileNameW(selfModule,path,MAX_PATH);
    std::wstring directory(path); directory.resize(directory.find_last_of(L"\\/")+1);
    configPath = directory + L"CenterSpeedometer.ini";
    logPath = directory + L"CenterSpeedometer.log";
    capturePath=directory+L"Collectibles-map.bmp";
    captureMap=GetPrivateProfileIntW(L"Collectibles",L"CaptureMap",0,(directory+L"Collectibles.ini").c_str())!=0;
    wchar_t host[MAX_PATH]{}; GetModuleFileNameW(nullptr,host,MAX_PATH);
    const wchar_t* filename = wcsrchr(host,L'\\'); filename = filename ? filename+1 : host;
    if (_wcsicmp(filename,L"BurnoutPR.exe") != 0) return 0;
    Log("Center Speedometer 1.0 loaded; read-only speed telemetry, km/h, bottom center.");
    gameBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(gameBase);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(gameBase+dos->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != 0x5B365B73 || nt->OptionalHeader.SizeOfImage != 0x07F7E000) {
        Log("Unsupported executable layout; telemetry disabled. No hook installed."); return 0;
    }
    enabled = GetPrivateProfileIntW(L"Speedometer",L"Enabled",1,configPath.c_str()) != 0;
    wchar_t value[64];
    auto readFloat = [&](const wchar_t* key, const wchar_t* fallback) {
        GetPrivateProfileStringW(L"Speedometer",key,fallback,value,64,configPath.c_str());
        float number = static_cast<float>(_wtof(value)); return std::isfinite(number) ? number : 1.0f;
    };
    settings.scale = std::clamp(readFloat(L"Scale",L"1.0"),0.5f,2.0f);
    settings.bottomMargin = std::clamp(readFloat(L"BottomMargin",L"32"),0.0f,300.0f);
    settings.opacity = std::clamp(readFloat(L"Opacity",L"0.82"),0.1f,1.0f);
    WNDCLASSW wc{}; wc.lpfnWndProc = DefWindowProcW; wc.hInstance = selfModule; wc.lpszClassName = L"BPRSpeedometerHookProbe";
    RegisterClassW(&wc);
    HWND probe = CreateWindowW(wc.lpszClassName,L"",WS_OVERLAPPED,0,0,100,100,nullptr,nullptr,selfModule,nullptr);
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount=1; desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow=probe; desc.SampleDesc.Count=1; desc.Windowed=TRUE; desc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
    IDXGISwapChain* swap=nullptr; ID3D11Device* device=nullptr; ID3D11DeviceContext* context=nullptr;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&desc,&swap,&device,nullptr,&context);
    if (SUCCEEDED(hr)) {
        void* present = (*reinterpret_cast<void***>(swap))[8];
        if (MH_Initialize() == MH_OK && MH_CreateHook(present,reinterpret_cast<void*>(&HookPresent),reinterpret_cast<void**>(&originalPresent)) == MH_OK && MH_EnableHook(present) == MH_OK) {
            Log("DXGI Present hook installed.");
            collectiblesEnabled=InitializeCollectibles(gameBase,directory,Log);
        } else Log("Unable to install Present hook.");
    } else Log("Unable to create D3D11 hook probe.");
    if(context) context->Release(); if(device) device->Release(); if(swap) swap->Release();
    if(probe) DestroyWindow(probe); UnregisterClassW(wc.lpszClassName,selfModule);
    return 0;
}

HMODULE RealInput() {
    static HMODULE module = [] {
        wchar_t system[MAX_PATH]{}; GetSystemDirectoryW(system,MAX_PATH);
        std::wstring path(system); path += L"\\dinput8.dll";
        return LoadLibraryW(path.c_str());
    }();
    return module;
}
}

extern "C" HRESULT WINAPI ProxyDirectInput8Create(HINSTANCE instance,DWORD version,REFIID iid,LPVOID* output,LPUNKNOWN outer) {
    using Fn = HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
    auto fn=reinterpret_cast<Fn>(GetProcAddress(RealInput(),"DirectInput8Create"));
    return fn ? fn(instance,version,iid,output,outer) : E_FAIL;
}
extern "C" HRESULT WINAPI ProxyDllCanUnloadNow() {
    using Fn=HRESULT(WINAPI*)(); auto fn=reinterpret_cast<Fn>(GetProcAddress(RealInput(),"DllCanUnloadNow")); return fn ? fn() : S_FALSE;
}
extern "C" HRESULT WINAPI ProxyDllGetClassObject(REFCLSID cls,REFIID iid,LPVOID* object) {
    using Fn=HRESULT(WINAPI*)(REFCLSID,REFIID,LPVOID*); auto fn=reinterpret_cast<Fn>(GetProcAddress(RealInput(),"DllGetClassObject")); return fn ? fn(cls,iid,object) : E_FAIL;
}
extern "C" HRESULT WINAPI ProxyDllRegisterServer() {
    using Fn=HRESULT(WINAPI*)(); auto fn=reinterpret_cast<Fn>(GetProcAddress(RealInput(),"DllRegisterServer")); return fn ? fn() : E_FAIL;
}
extern "C" HRESULT WINAPI ProxyDllUnregisterServer() {
    using Fn=HRESULT(WINAPI*)(); auto fn=reinterpret_cast<Fn>(GetProcAddress(RealInput(),"DllUnregisterServer")); return fn ? fn() : E_FAIL;
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        selfModule=instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread=CreateThread(nullptr,0,StartMod,nullptr,0,nullptr);
        if(thread) CloseHandle(thread);
    }
    return TRUE;
}
