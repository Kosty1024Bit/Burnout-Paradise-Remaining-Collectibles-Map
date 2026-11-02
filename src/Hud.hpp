#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

struct HudSettings {
    float scale = 1.0f;
    float bottomMargin = 32.0f;
    float opacity = 0.82f;
};

inline int MphToKmh(int mph) {
    return static_cast<int>(std::lround(std::abs(static_cast<double>(mph)) * 1.609344));
}

inline void DrawSpeedometer(int kmh, const HudSettings& settings, ImFont* font) {
    const ImGuiIO& io = ImGui::GetIO();
    const float s = std::clamp(io.DisplaySize.y / 1080.0f, 0.5f, 4.0f) * settings.scale;
    const float cx = io.DisplaySize.x * 0.5f;
    const float bottom = io.DisplaySize.y - settings.bottomMargin * s;
    const float top = bottom - 112.0f * s;
    const ImVec2 a(cx - 100.0f * s, top), b(cx + 100.0f * s, bottom);
    auto* draw = ImGui::GetForegroundDrawList();
    draw->AddRectFilled(ImVec2(a.x+3*s,a.y+4*s), ImVec2(b.x+3*s,b.y+4*s), IM_COL32(0,0,0,80), 12*s);
    draw->AddRectFilled(a, b, IM_COL32(10,17,24,static_cast<int>(settings.opacity*255)), 12*s);
    draw->AddRect(a, b, IM_COL32(255,255,255,45), 12*s);
    const ImU32 accent = IM_COL32(75,207,255,255);
    draw->AddLine(ImVec2(cx-64*s,top+2*s), ImVec2(cx+64*s,top+2*s), accent, 3*s);
    char digits[16];
    std::snprintf(digits, sizeof(digits), "%03d", std::clamp(kmh, 0, 999));
    const float size = 62.0f*s;
    const ImVec2 extent = font->CalcTextSizeA(size, FLT_MAX, 0, digits);
    draw->AddText(font, size, ImVec2(cx-extent.x/2, top+7*s), IM_COL32(248,252,255,255), digits);
    const char* unit = "\xD0\x9A\xD0\x9C/\xD0\xA7"; // КМ/Ч (UTF-8)
    const float unitSize = 17.0f*s;
    const ImVec2 unitExtent = font->CalcTextSizeA(unitSize, FLT_MAX, 0, unit);
    draw->AddText(font, unitSize, ImVec2(cx-unitExtent.x/2, top+78*s), accent, unit);
}

inline ImFont* LoadHudFont() {
    ImGuiIO& io = ImGui::GetIO();
    // Windows system font, not a redistributed game asset.
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arialbd.ttf", 64.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    return font ? font : io.Fonts->AddFontDefault();
}
