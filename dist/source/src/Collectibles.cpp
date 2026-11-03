#include "Collectibles.hpp"
#include <MinHook.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <set>
#include <vector>

namespace {
struct Marker { uint32_t id, kind; float x,z; };
static_assert(sizeof(Marker)==16);
struct MapEvent { float world[4], view[4]; uint32_t textures; float zoom; int type; unsigned char active; unsigned char pad[3]; };
static_assert(sizeof(MapEvent)==48);
using MapFn=void(__thiscall*)(void*,const MapEvent*);
using LoadFn=void(__thiscall*)(void*,const void*,const void*);
using DoneFn=bool(__thiscall*)(void*,int,uint64_t);
MapFn originalMap; LoadFn originalLoad; DoneFn originalDone;
uintptr_t base;
std::atomic<uintptr_t> profile{};
std::mutex mapMutex;
MapEvent latestMap{};
ULONGLONG mapTick{};
void (*writeLog)(const char*);
std::vector<Marker> markers;
std::array<std::set<uint32_t>,3> completed;
bool shown=true, categories[3]{true,true,true}, ready=false;
float markerScale=1;

bool ReadBytes(uintptr_t a,void* out,size_t n) {
    SIZE_T got=0; return a>=0x10000 && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(a),out,n,&got) && got==n;
}
template<class T> bool Read(uintptr_t a,T& out) { return ReadBytes(a,&out,sizeof(out)); }
uintptr_t VA(uintptr_t a) { return base+a-0x400000; }

void __fastcall OnMap(void* self,void*,const MapEvent* event) {
    originalMap(self,event);
    MapEvent copy{};
    if (!Read(reinterpret_cast<uintptr_t>(event),copy)) return;
    std::lock_guard<std::mutex> lock(mapMutex);
    latestMap=copy; mapTick=GetTickCount64();
}
void __fastcall OnLoad(void* self,void*,const void* image,const void* upgrade) {
    originalLoad(self,image,upgrade);
    profile.store(reinterpret_cast<uintptr_t>(self));
}
bool __fastcall OnDone(void* self,void*,int kind,uint64_t id) {
    profile.store(reinterpret_cast<uintptr_t>(self));
    return originalDone(self,kind,id);
}

bool Hook(uintptr_t address,void* fn,void** original,const unsigned char* expected,size_t size) {
    unsigned char bytes[16]{};
    if(size>sizeof(bytes) || !ReadBytes(VA(address),bytes,size) || std::memcmp(bytes,expected,size)) {
        char message[180];std::snprintf(message,sizeof(message),"Collectibles hook %08X: bytes %02X %02X %02X %02X %02X %02X (layout mismatch)",static_cast<unsigned>(address),bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5]);writeLog(message);return false;
    }
    auto created=MH_CreateHook(reinterpret_cast<void*>(VA(address)),fn,original);
    auto activated=created==MH_OK?MH_EnableHook(reinterpret_cast<void*>(VA(address))):MH_ERROR_NOT_CREATED;
    char message[180];std::snprintf(message,sizeof(message),"Collectibles hook %08X: create=%d enable=%d",static_cast<unsigned>(address),created,activated);writeLog(message);
    return created==MH_OK && activated==MH_OK;
}

bool RefreshProgress() {
    const uintptr_t p=profile.load(); uint32_t version=0;
    if (!p || !Read(p,version) || version!=31) return false;
    std::array<std::set<uint32_t>,3> next;
    auto append=[&](int k,uintptr_t entries,uintptr_t length,uint32_t max,bool lowOnly) {
        uint32_t count=0,after=0;
        if(!Read(length,count) || count>max) return false;
        std::vector<uint64_t> ids(count);
        if(count && !ReadBytes(entries,ids.data(),count*8)) return false;
        if(!Read(length,after) || after!=count) return false;
        for(auto id:ids) {
            if(!lowOnly && (id>>32)!=0 && (id>>32)!=0xffffffffu) return false;
            next[k].insert(static_cast<uint32_t>(id));
        }
        return true;
    };
    for(int k=0;k<3;++k)
        if(!append(k,p+0x7770+k*0x1008,p+0x8770+k*0x1008,512,false))return false;
    const uintptr_t island=p+0x282f8;
    if(!append(2,island,island+0x168,45,true) ||
       !append(1,island+0x16c,island+0x3c4,75,true) ||
       !append(0,island+0x3c8,island+0x440,15,true))return false;
    if(!ready || next!=completed) {
        char line[180]; std::snprintf(line,sizeof(line),"Collectibles profile: jumps %zu/65; smashes %zu/475; billboards %zu/165",next[0].size(),next[1].size(),next[2].size());
        writeLog(line);
    }
    completed=std::move(next); return true;
}
}

bool InitializeCollectibles(uintptr_t gameBase,const std::wstring& directory,void (*log)(const char*)) {
    base=gameBase;writeLog=log;
    for(auto address:{0x004f9780u,0x032c4ea0u,0x00a078f0u,0x03215650u}) {
        unsigned char b[8]{};ReadBytes(VA(address),b,sizeof(b));char message[180];
        std::snprintf(message,sizeof(message),"Collectibles entry %08X runtime: %02X %02X %02X %02X %02X %02X %02X %02X",address,b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7]);log(message);
    }
    const auto config=directory+L"Collectibles.ini";
    if(!GetPrivateProfileIntW(L"Collectibles",L"Enabled",1,config.c_str()))return false;
    shown=true;
    categories[0]=GetPrivateProfileIntW(L"Collectibles",L"Jumps",1,config.c_str())!=0;
    categories[1]=GetPrivateProfileIntW(L"Collectibles",L"Smashes",1,config.c_str())!=0;
    categories[2]=GetPrivateProfileIntW(L"Collectibles",L"Billboards",1,config.c_str())!=0;
    FILE* file=nullptr; _wfopen_s(&file,(directory+L"Collectibles.bin").c_str(),L"rb");
    if(!file){log("Collectibles: local cache missing; run prepare_collectibles.py. Disabled.");return false;}
    uint32_t h[3]{};
    bool ok=fread(h,sizeof(h),1,file)==1 && h[0]==0x43525042 && h[1]==1 && h[2]>0 && h[2]<=2000;
    if(ok){markers.resize(h[2]);ok=fread(markers.data(),sizeof(Marker),markers.size(),file)==markers.size() && fgetc(file)==EOF;}
    fclose(file);
    std::array<std::set<uint32_t>,3> keys;
    for(auto m:markers) {
        if(m.kind>2 || !std::isfinite(m.x) || !std::isfinite(m.z) || std::abs(m.x)>100000 || std::abs(m.z)>100000)ok=false;
        else keys[m.kind].insert(m.id);
    }
    if(!ok || keys[0].size()!=65 || keys[1].size()!=475 || keys[2].size()!=165) {log("Collectibles: invalid marker cache. Disabled.");markers.clear();return false;}
    const unsigned char mapBytes[]{0x53,0x89,0xe3,0x83,0xec,0x08};
    const unsigned char loadBytes[]{0x55,0x8b,0xec,0x83,0xec,0x14};
    const unsigned char doneBytes[]{0x55,0x89,0xe5,0x51,0x53};
    // Measured in the installed PC executable. All addresses rebased for ASLR.
    if(!Hook(0x032c4ea0,reinterpret_cast<void*>(OnMap),reinterpret_cast<void**>(&originalMap),mapBytes,sizeof(mapBytes))) {log("Collectibles: unsupported map callback. Disabled.");return false;}
    bool load=Hook(0x00a078f0,reinterpret_cast<void*>(OnLoad),reinterpret_cast<void**>(&originalLoad),loadBytes,sizeof(loadBytes));
    bool done=Hook(0x03215650,reinterpret_cast<void*>(OnDone),reinterpret_cast<void**>(&originalDone),doneBytes,sizeof(doneBytes));
    if(!load && !done){log("Collectibles: no profile observation hook. Disabled.");return false;}
    log("Collectibles: native menu-map hooks ready; 65 jumps, 475 smashes, 165 billboards; F6/F7/F8/F9.");
    return true;
}

bool DrawCollectibles(HWND window,ImFont* font) {
    if(markers.empty() || !originalMap)return false;
    if(GetForegroundWindow()==window) {
        static bool held[4]{};
        for(int i=0;i<4;++i) {
            bool down=(GetAsyncKeyState(VK_F6+i)&0x8000)!=0;
            if(down && !held[i]){if(i==0)shown=!shown;else categories[i-1]=!categories[i-1];}
            held[i]=down;
        }
    }
    if(!shown)return false;
    MapEvent event{}; ULONGLONG tick=0;
    {std::lock_guard<std::mutex> lock(mapMutex);event=latestMap;tick=mapTick;}
    if(!tick || GetTickCount64()-tick>200 || event.active!=1 || event.type!=0)return false;
    uintptr_t game=0;unsigned char online=1;
    if(!Read(VA(0x13fc8e0),game) || !Read(game+0xb6d415,online) || online)return false;
    for(auto f:event.world)if(!std::isfinite(f) || std::abs(f)>100000)return false;
    for(auto f:event.view)if(!std::isfinite(f) || std::abs(f)>10)return false;
    float dx=event.world[2]-event.world[0], dz=event.world[3]-event.world[1];
    if(std::abs(dx)<10 || std::abs(dz)<10)return false;
    static ULONGLONG lastRefresh=0;
    if(GetTickCount64()-lastRefresh>250){ready=RefreshProgress();lastRefresh=GetTickCount64();}
    float matrix[12]{};
    if(!Read(VA(0x149a1f0),matrix))return false;
    for(auto f:matrix)if(!std::isfinite(f) || std::abs(f)>100000)return false;
    auto screen=[&](float x,float y){return ImVec2(x*matrix[0]+y*matrix[4]+matrix[8],x*matrix[1]+y*matrix[5]+matrix[9]);};
    ImVec2 lo=screen(event.view[0],event.view[1]), hi=screen(event.view[2],event.view[3]);
    const ImVec2 display=ImGui::GetIO().DisplaySize;
    // The game's transform produces GUI pixels, with its live resolution in the device rect.
    // Normalised->device transform itself gives the full device span.
    ImVec2 fullA=screen(0,0),fullB=screen(1,1);
    float guiW=std::abs(fullB.x-fullA.x),guiH=std::abs(fullB.y-fullA.y);
    if(guiW<100 || guiH<100)return false;
    float sx=display.x/guiW,sy=display.y/guiH;
    auto fit=[&](ImVec2 p){return ImVec2(p.x*sx,p.y*sy);};
    lo=fit(lo);hi=fit(hi);
    ImVec2 clipMin(std::max(0.f,std::min(lo.x,hi.x)),std::max(0.f,std::min(lo.y,hi.y)));
    ImVec2 clipMax(std::min(display.x,std::max(lo.x,hi.x)),std::min(display.y,std::max(lo.y,hi.y)));
    if(clipMax.x<=clipMin.x || clipMax.y<=clipMin.y)return false;
    const float s=std::clamp(display.y/1080.f,.5f,4.f), r=4.2f*s*markerScale;
    auto* draw=ImGui::GetForegroundDrawList();
    const ImU32 colors[3]{IM_COL32(90,210,255,255),IM_COL32(255,206,65,255),IM_COL32(255,105,160,255)};
    draw->PushClipRect(clipMin,clipMax,true);
    int visible=0;
    if(ready)for(auto m:markers) {
        if(!categories[m.kind] || completed[m.kind].count(m.id))continue;
        float u=(m.x-event.world[0])/dx,v=(m.z-event.world[1])/dz;
        if(u<0 || u>1 || v<0 || v>1)continue;
        ImVec2 p=fit(screen(event.view[0]+u*(event.view[2]-event.view[0]),event.view[1]+v*(event.view[3]-event.view[1])));
        if(p.x<clipMin.x || p.x>clipMax.x || p.y<clipMin.y || p.y>clipMax.y)continue;
        draw->AddCircleFilled(p,r+1.6f*s,IM_COL32(0,0,0,220),12);
        if(m.kind==0)draw->AddTriangleFilled(ImVec2(p.x,p.y-r-1*s),ImVec2(p.x-r,p.y+r),ImVec2(p.x+r,p.y+r),colors[0]);
        else if(m.kind==1)draw->AddRectFilled(ImVec2(p.x-r,p.y-r*.6f),ImVec2(p.x+r,p.y+r*.6f),colors[1],s);
        else {ImVec2 a(p.x,p.y-r),b(p.x+r,p.y),c(p.x,p.y+r),d(p.x-r,p.y);draw->AddQuadFilled(a,b,c,d,colors[2]);}
        ++visible;
    }
    draw->PopClipRect();
    const float fs=15*s;
    ImVec2 a(24*s,display.y-112*s),b(535*s,display.y-38*s);
    draw->AddRectFilled(a,b,IM_COL32(8,15,23,225),8*s);
    const char* names[3]{"F7  Прыжки","F8  Заграждения","F9  Билборды"};
    const int totals[3]{65,475,165};
    for(int k=0;k<3;++k){char text[120];
        if(ready)std::snprintf(text,sizeof(text),"%s  %d",names[k],std::max(0,totals[k]-static_cast<int>(completed[k].size())));
        else std::snprintf(text,sizeof(text),"%s  ...",names[k]);
        draw->AddText(font,fs,ImVec2(a.x+12*s+k*165*s,a.y+12*s),categories[k]?colors[k]:IM_COL32(130,130,130,255),text);
    }
    draw->AddText(font,14*s,ImVec2(a.x+12*s,a.y+43*s),IM_COL32(225,230,235,255),ready?"Осталось собрать • F6: скрыть метки":"Ожидание данных текущего профиля...");
    static ULONGLONG lastLog=0;
    if(GetTickCount64()-lastLog>5000){char line[240];std::snprintf(line,sizeof(line),"Collectibles map: active=%u type=%d markers=%d ready=%d world %.1f %.1f %.1f %.1f; GUI %.0fx%.0f",event.active,event.type,visible,ready,event.world[0],event.world[1],event.world[2],event.world[3],guiW,guiH);writeLog(line);lastLog=GetTickCount64();}
    return ready;
}
