#pragma once
#include <algorithm>
#include <cmath>

struct Vec3 {
  float x = 0, y = 0, z = 0;
  Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
  Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
  Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};

inline float Dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b) {
  return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
inline Vec3 Normalize(Vec3 v) {
  float length = std::sqrt(Dot(v, v));
  return length > 0 ? v * (1 / length) : Vec3{};
}
inline Vec3 Lerp(Vec3 a, Vec3 b, float t) { return a * (1-t) + b * t; }
inline bool IsFinite(Vec3 v) {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

struct Quat {
  float x = 0, y = 0, z = 0, w = 1;
};

inline float Dot(Quat a, Quat b) {
  return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}
inline Quat Normalize(Quat q) {
  float length = std::sqrt(Dot(q, q));
  return length > 0 ? Quat{q.x/length, q.y/length, q.z/length, q.w/length}
                    : Quat{};
}
inline Quat Lerp(Quat a, Quat b, float t) {
  return {a.x*(1-t)+b.x*t, a.y*(1-t)+b.y*t,
          a.z*(1-t)+b.z*t, a.w*(1-t)+b.w*t};
}
inline Quat Slerp(Quat a, Quat b, float t) {
  a = Normalize(a);
  b = Normalize(b);
  float dot = Dot(a, b);
  // q and -q are the same rotation; take the shorter arc.
  if (dot < 0) {
    b = {-b.x, -b.y, -b.z, -b.w};
    dot = -dot;
  }
  dot = std::clamp(dot, 0.0f, 1.0f);
  if (dot > 0.9995f)
    return Normalize(Lerp(a, b, t));
  float angle = std::acos(dot);
  float left = std::sin((1-t)*angle) / std::sin(angle);
  float right = std::sin(t*angle) / std::sin(angle);
  return Normalize({a.x*left+b.x*right, a.y*left+b.y*right,
                    a.z*left+b.z*right, a.w*left+b.w*right});
}
inline Quat Yaw(float radians) {
  return {0, std::sin(radians/2), 0, std::cos(radians/2)};
}
inline Vec3 Rotate(Quat q, Vec3 v) {
  q = Normalize(q);
  Vec3 axis{q.x, q.y, q.z};
  Vec3 twiceCross = Cross(axis, v) * 2;
  return v + twiceCross * q.w + Cross(axis, twiceCross);
}
