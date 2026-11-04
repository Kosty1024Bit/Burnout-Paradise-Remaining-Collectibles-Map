#include "MapProjection.hpp"
#include <cstdio>
using namespace MapProjection;
bool Close(Point a,Point b){return std::abs(a.x-b.x)<.001f && std::abs(a.y-b.y)<.001f;}
int main() {
    // A 1000m square around world (2000,-1000), north-up.
    const Matrix north{{.001f,0,0,0, 0,.001f,0,0, -1.5f,1.5f,1,0}};
    const Matrix viewport{{196,0,0,0, 0,145,0,0, 996,479,1,0}};
    if(!Close(Transform(Transform({2000,-1000},north),viewport),{1094,551.5f}))return 1;
    if(!Close(Transform(Transform({1500,-1500},north),viewport),{996,479}))return 2;
    // Same centre and extent after a quarter turn: east now points screen-up.
    const Matrix rotated{{0,-.001f,0,0, .001f,0,0,0, 1.5f,2.5f,1,0}};
    if(!Close(Transform(Transform({2000,-1000},rotated),viewport),{1094,551.5f}))return 3;
    if(!Close(Transform(Transform({2500,-1000},rotated),viewport),{1094,479}))return 4;
    if(Inside(Transform({2501,-1000},rotated)))return 5;
    if(Inside({.01f,.5f},.02f,.02f) || !Inside({.5f,.5f},.02f,.02f))return 6;
    Matrix bad=viewport;bad.row[0]=NAN;
    if(Finite(bad))return 7;
    // Regression: a 2560x1440 backbuffer presented into a 2560x1400 client.
    // The minimap anchor must first land at native buffer y=958, then DXGI scales
    // the whole frame to client y=931.3889. Client-sized overlay drew it twice too high.
    const auto surface=GuiToSurface({996,479},{1280,720},{2560,1440});
    if(!Close(surface,{1992,958}))return 8;
    if(std::abs(surface.y*1400.f/1440.f-931.3889f)>.001f)return 9;
    const auto full=GuiToSurface({1192,624},{1280,720},{2560,1440});
    if(!Close(full,{2384,1248}))return 10;
    std::puts("Projection checks passed: world mapping, rotation, clipping, invalid matrix, stretched-window regression.");
}
