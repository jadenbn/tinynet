#include "game/Hud.h"
#include "game/PitchView.h"
#include <cmath>

namespace {
void Centred(const char *text, float x, float y, int size, Color color) {
  DrawText(text, x - MeasureText(text, size) / 2.0f, y, size, color);
}
} // namespace

void Hud::Add(std::string text, Color color, double now) {
  feed.push_back({std::move(text), color, now});
  while (feed.size() > 6)
    feed.pop_front();
}

void Hud::OnEvent(const GameEvent &event, const std::string &who, bool aboutUs,
                  double now) {
  Color team = TeamColor(event.team);
  std::string score = std::to_string(event.score[0]) + "-" + std::to_string(event.score[1]);
  switch (event.type) {
  case EventType::PlayerJoined:
    Add(who + " joined " + TeamName(event.team), team, now);
    break;
  case EventType::PlayerLeft:
    Add(who + " left", LIGHTGRAY, now);
    break;
  case EventType::Goal:
    goalTeam = event.team;
    goalDetail = event.value ? "own goal by " + who : who + (aboutUs ? " score!" : " scores");
    Add("GOAL! " + goalDetail + "  " + score, team, now);
    break;
  case EventType::FullTime:
    result = event.value ? "a draw, " + score
                         : std::string(TeamName(event.team)) + " win " + score;
    Add("full time: " + result, RAYWHITE, now);
    break;
  case EventType::QuickChat:
    Add(who + ": " + QUICK_CHAT[event.value], team, now);
    break;
  }
}

void Hud::Draw(const MatchState &match, Rectangle area, double now) const {
  // score bar
  float mid = area.x + area.width / 2;
  DrawRectangleRec({area.x, 0, area.width, 52}, {16, 20, 26, 255});
  const char *score = TextFormat("%d  -  %d", match.score[0], match.score[1]);
  Centred(score, mid, 8, 28, RAYWHITE);
  float half = MeasureText(score, 28) / 2.0f + 20;
  DrawText("BLUE", mid - half - MeasureText("BLUE", 22), 12, 22, TeamColor(0));
  DrawText("ORANGE", mid + half, 12, 22, TeamColor(1));
  int seconds = (match.clock + 59) / 60;
  Centred(TextFormat("%d:%02d", seconds / 60, seconds % 60), mid, 36, 12,
          seconds <= 10 && match.phase == MatchPhase::Playing ? RED : LIGHTGRAY);

  // banners
  float centreY = area.y + area.height / 2;
  if (match.phase == MatchPhase::Goal) {
    // a little pulse: scale the text with a sine wave
    int size = 64 + static_cast<int>(6 * std::sin(now * 8));
    Centred("GOAL!", mid, centreY - 60, size, TeamColor(goalTeam));
    Centred(goalDetail.c_str(), mid, centreY + 10, 22, RAYWHITE);
  } else if (match.phase == MatchPhase::FullTime) {
    DrawRectangleRec({area.x, centreY - 70, area.width, 120}, Fade(BLACK, 0.6f));
    Centred("FULL TIME", mid, centreY - 55, 52, RAYWHITE);
    Centred(result.c_str(), mid, centreY + 5, 24, LIGHTGRAY);
  }

  // the feed, fading out after a while
  float y = area.y + area.height - 24;
  for (auto it = feed.rbegin(); it != feed.rend(); ++it) {
    float age = static_cast<float>(now - it->time);
    float alpha = std::clamp(1.5f - age / 4, 0.0f, 1.0f);
    if (alpha > 0)
      DrawText(it->text.c_str(), area.x + 12, y, 14, Fade(it->color, alpha));
    y -= 18;
  }
}
