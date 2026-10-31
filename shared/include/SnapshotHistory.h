#pragma once
#include "Protocol.h"
#include <deque>
#include <optional>

class SnapshotHistory {
public:
  bool Push(const WorldSnapshot &snapshot, double transitSeconds = 0) {
    if (!history.empty() && !IsNewer(snapshot.tick, history.back().snapshot.tick))
      return false;
    double time = history.empty() ? 0 : LatestTime() +
        static_cast<double>(snapshot.tick-history.back().snapshot.tick)*TICK_DT;
    clock = history.empty() ? time + transitSeconds
                            : std::max(clock, time + transitSeconds);
    history.push_back({snapshot, time});
    while (history.size() > 32)
      history.pop_front();
    return true;
  }

  void Advance(double dt) { if (!history.empty()) clock += dt; }
  double RenderTime() const { return clock - 0.1; }

  double LatestTime() const { return history.empty() ? 0 : history.back().time; }

  std::optional<PlayerState> Sample(PlayerID id, double renderTime) const {
    if (history.empty())
      return std::nullopt;
    auto [left, right] = Bracket(renderTime);
    auto a = left->snapshot.players.find(id);
    auto b = right->snapshot.players.find(id);
    if (b == right->snapshot.players.end())
      return std::nullopt;
    if (a == left->snapshot.players.end() || left->time == right->time)
      return b->second;
    float alpha = std::clamp(static_cast<float>((renderTime-left->time) /
                                              (right->time-left->time)), 0.0f, 1.0f);
    PlayerState state = b->second;
    state.pos = Lerp(a->second.pos, b->second.pos, alpha);
    state.orientation = Slerp(a->second.orientation, b->second.orientation, alpha);
    return state;
  }

  // the ball as everyone else saw it at renderTime: the same moment we draw
  // other players at, so their touches line up with it
  std::optional<BallState> SampleBall(double renderTime) const {
    if (history.empty())
      return std::nullopt;
    auto [left, right] = Bracket(renderTime);
    BallState ball = right->snapshot.ball;
    if (left->time != right->time) {
      float alpha = std::clamp(static_cast<float>((renderTime-left->time) /
                                                (right->time-left->time)), 0.0f, 1.0f);
      ball.pos = Lerp(left->snapshot.ball.pos, right->snapshot.ball.pos, alpha);
    }
    return ball;
  }

private:
  struct Entry { WorldSnapshot snapshot; double time; };
  // the snapshots either side of renderTime (the same one twice at the ends)
  std::pair<const Entry *, const Entry *> Bracket(double renderTime) const {
    const Entry *left = &history.front(), *right = left;
    for (const auto &entry : history) {
      right = &entry;
      if (entry.time >= renderTime)
        break;
      left = &entry;
    }
    return {left, right};
  }
  std::deque<Entry> history;
  double clock = 0;
};
