#ifndef RECT_HPP
#define RECT_HPP

#include "Vec2.hpp"

struct Rect {
    float x, y, w, h;
    bool intersects(const Rect& other) const {
        return (x < other.x + other.w && x + w > other.x &&
                y < other.y + other.h && y + h > other.y);
    }
    bool contains(const Vec2& p) const {
        return (p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h);
    }
    Vec2 center() const { return {x + w / 2, y + h / 2}; }
};

#endif
