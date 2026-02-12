#ifndef VEC2_HPP
#define VEC2_HPP

#include <cmath>
#include <algorithm>

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}
    Vec2 operator+(const Vec2& v) const { return {x + v.x, y + v.y}; }
    Vec2 operator-(const Vec2& v) const { return {x - v.x, y - v.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const {
        float l = length();
        return (l > 0.0001f) ? Vec2(x / l, y / l) : Vec2(0, 0);
    }
    float distance(const Vec2& v) const { return (*this - v).length(); }
};

#endif
