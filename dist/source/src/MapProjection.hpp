#pragma once
#include <cmath>

namespace MapProjection {
struct Point { float x,y; };
struct Matrix { float row[12]; };
inline Point Transform(Point p,const Matrix& m) {
    return {p.x*m.row[0]+p.y*m.row[4]+m.row[8],p.x*m.row[1]+p.y*m.row[5]+m.row[9]};
}
inline bool Finite(const Matrix& m) {
    for(float f:m.row)if(!std::isfinite(f) || std::abs(f)>100000.f)return false;
    return true;
}
inline bool Inside(Point p,float insetX=0.f,float insetY=0.f) {
    return std::isfinite(p.x) && std::isfinite(p.y) && p.x>=insetX && p.x<=1.f-insetX && p.y>=insetY && p.y<=1.f-insetY;
}
inline Point GuiToSurface(Point gui,Point guiExtent,Point renderExtent) {
    return {gui.x*renderExtent.x/guiExtent.x,gui.y*renderExtent.y/guiExtent.y};
}
}
