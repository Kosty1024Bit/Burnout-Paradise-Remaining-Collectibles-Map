#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <imgui.h>

bool InitializeCollectibles(uintptr_t gameBase, const std::wstring& directory, void (*log)(const char*));
bool DrawCollectibles(HWND gameWindow, ImFont* font, bool drivingHud);
