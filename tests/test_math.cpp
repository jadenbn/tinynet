#include "VectorMath.h"
#include <cassert>
#include <iostream>
#include <numbers>

bool Near(float a, float b) { return std::abs(a-b) < 0.00001f; }
void Check(Vec3 a, Vec3 b) {
  assert(Near(a.x, b.x) && Near(a.y, b.y) && Near(a.z, b.z));
}

int main() {
  assert(Near(Dot(Vec3{1,2,3}, Vec3{4,5,6}), 32));
  Check(Cross({1,0,0}, {0,1,0}), {0,0,1});
  Check(Normalize(Vec3{}), {});
  Check(Normalize(Vec3{3,0,4}), {0.6f,0,0.8f});
  Check(Lerp(Vec3{0,0,0}, Vec3{2,4,6}, 0.5f), {1,2,3});
  assert(Near(Normalize(Quat{0,0,0,0}).w, 1));
  Quat turn = Yaw(std::numbers::pi_v<float>/2);
  Check(Rotate(turn, {0,0,1}), {1,0,0});
  Check(Rotate(Slerp({}, turn, 0), {0,0,1}), {0,0,1});
  Check(Rotate(Slerp({}, turn, 1), {0,0,1}), {1,0,0});
  Check(Rotate(Slerp({}, turn, 0.5f), {0,0,1}),
        {std::sqrt(0.5f),0,std::sqrt(0.5f)});
  Quat opposite{-turn.x,-turn.y,-turn.z,-turn.w};
  assert(Near(std::abs(Dot(Slerp(turn, opposite, 0.5f), turn)), 1));
  assert(Near(Dot(Slerp({}, Yaw(0.0001f), 0.5f),
                       Slerp({}, Yaw(0.0001f), 0.5f)), 1));
  std::cout << "All math tests passed!\n";
}
