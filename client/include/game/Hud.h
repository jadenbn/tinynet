#pragma once
#include "GameTypes.h"
#include "raylib.h"
#include <deque>
#include <string>

constexpr const char *QUICK_CHAT[] = {"nice shot!", "pass it!", "what a save!", "oops"};

// the score, the clock, goal + full time banners, and a feed of everything
// that's happened (from the reliable events)
class Hud {
public:
  // names are worked out by the caller, who knows who's who
  void OnEvent(const GameEvent &event, const std::string &who, bool aboutUs, double now);
  void Draw(const MatchState &match, Rectangle area, double now) const;

private:
  struct Line {
    std::string text;
    Color color;
    double time;
  };
  std::deque<Line> feed;
  std::string goalDetail;
  uint8_t goalTeam = 0;
  std::string result;

  void Add(std::string text, Color color, double now);
};
