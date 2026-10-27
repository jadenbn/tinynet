#include "game/ClientWorld.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr Color GRASS{46, 125, 62, 255};
constexpr Color GRASS_STRIPE{52, 136, 68, 255};
constexpr Color LINE{235, 240, 235, 220};
constexpr Color BOARDS{28, 34, 40, 255};
constexpr Color NET{230, 230, 230, 70};

void Line(const PitchView &view, Vec3 a, Vec3 b, float thick = 2) {
  DrawLineEx(view.ToScreen(a), view.ToScreen(b), thick, LINE);
}
} // namespace

ClientWorld::ClientWorld() {}

bool ClientWorld::AddPlayer(PlayerID playerID) {
  return players.try_emplace(playerID, playerID).second;
}

const char *ClientWorld::Name(PlayerID id) const {
  if (id == localID)
    return "you";
  auto player = players.find(id);
  if (player != players.end() && player->second.state.keeper)
    return TextFormat("the %s keeper", TeamName(player->second.state.team));
  return id == NO_PLAYER ? "someone" : TextFormat("player %u", id);
}

void ClientWorld::DrawPitch(const PitchView &view) const {
  const float l = PITCH_HALF_LENGTH, w = PITCH_HALF_WIDTH;
  Vector2 topLeft = view.ToScreen({-l, 0, w});
  float s = view.scale;

  // the boards around the outside, then grass mown in stripes
  DrawRectangleRec({topLeft.x - 0.6f * s, topLeft.y - 0.6f * s, (2 * l + 1.2f) * s,
                    (2 * w + 1.2f) * s}, BOARDS);
  DrawRectangleRec({topLeft.x, topLeft.y, 2 * l * s, 2 * w * s}, GRASS);
  for (int stripe = 0; stripe < 10; stripe += 2) {
    Vector2 from = view.ToScreen({-l + stripe * 4.0f, 0, w});
    DrawRectangleRec({from.x, from.y, 4 * s, 2 * w * s}, GRASS_STRIPE);
  }

  // markings
  DrawRectangleLinesEx({topLeft.x, topLeft.y, 2 * l * s, 2 * w * s}, 2, LINE);
  Line(view, {0, 0, -w}, {0, 0, w});
  Vector2 spot = view.ToScreen({});
  DrawRing(spot, 3 * s - 1, 3 * s + 1, 0, 360, 64, LINE);
  DrawCircleV(spot, 3, LINE);
  for (float side : {-1.0f, 1.0f}) {
    // futsal style area: quarter circles off each post joined by a line
    float line = side * l;
    for (float post : {-GOAL_HALF_WIDTH, GOAL_HALF_WIDTH}) {
      // raylib angles go clockwise from screen-right, and +z is up the
      // screen, so each post gets the quarter that faces into the pitch
      Vector2 centre = view.ToScreen({line, 0, post});
      float start = side < 0 ? (post < 0 ? 0 : 270) : (post < 0 ? 90 : 180);
      DrawRing(centre, 6 * s - 1, 6 * s + 1, start, start + 90, 24, LINE);
    }
    Line(view, {line - side * 6, 0, -GOAL_HALF_WIDTH}, {line - side * 6, 0, GOAL_HALF_WIDTH});
    DrawCircleV(view.ToScreen({line - side * 6, 0, 0}), 2.5f, LINE);

    // the goal: net behind the line, posts on it
    Vector2 back = view.ToScreen({line + side * GOAL_DEPTH, 0, GOAL_HALF_WIDTH});
    Vector2 mouth = view.ToScreen({line, 0, -GOAL_HALF_WIDTH});
    Rectangle net{std::min(back.x, mouth.x), back.y, std::abs(mouth.x - back.x),
                  mouth.y - back.y};
    DrawRectangleRec(net, Fade(BLACK, 0.35f));
    for (float x = 0; x <= net.width; x += 5)
      DrawLineV({net.x + x, net.y}, {net.x + x, net.y + net.height}, NET);
    for (float y = 0; y <= net.height; y += 5)
      DrawLineV({net.x, net.y + y}, {net.x + net.width, net.y + y}, NET);
    DrawRectangleLinesEx(net, 1.5f, Fade(RAYWHITE, 0.6f));
    Line(view, {line, 0, -GOAL_HALF_WIDTH}, {line, 0, GOAL_HALF_WIDTH}, 4);
    for (float post : {-GOAL_HALF_WIDTH, GOAL_HALF_WIDTH})
      DrawCircleV(view.ToScreen({line, 0, post}), std::max(3.0f, POST_RADIUS * s * 1.5f), RAYWHITE);
  }
}

void ClientWorld::DrawBall(const PitchView &view) {
  // rolling without slipping: it turns by (distance / radius) radians
  Vec3 moved = ball - lastBall;
  float distance = std::sqrt(moved.x * moved.x + moved.z * moved.z);
  if (distance < 2) // not a teleport
    roll += distance / BALL_RADIUS;
  lastBall = ball;

  float height = std::max(0.0f, ball.y - BALL_RADIUS);
  float radius = BALL_RADIUS * view.scale * (1 + height * 0.12f);
  Vector2 shadow = view.ToScreen({ball.x, 0, ball.z});
  DrawEllipse(shadow.x + 1, shadow.y + 2, BALL_RADIUS * view.scale, BALL_RADIUS * view.scale * 0.7f,
              Fade(BLACK, std::max(0.12f, 0.4f - height * 0.08f)));

  Vector2 centre = view.ToScreenRaised(ball);
  DrawCircleV(centre, radius + 1, Fade(BLACK, 0.7f));
  DrawCircleV(centre, radius, RAYWHITE);
  DrawPoly(centre, 5, radius * 0.45f, roll * RAD2DEG, Fade(BLACK, 0.75f));
}

void ClientWorld::Draw(const PitchView &view, bool debug) {
  DrawPitch(view);

  // where the server last had us + the ball. the gap is how far ahead we're
  // predicting
  if (debug && hasLocal) {
    DrawCircleLinesV(view.ToScreen(authoritative), PLAYER_RADIUS * view.scale + 3, RED);
    DrawCircleLinesV(view.ToScreenRaised(serverBall), BALL_RADIUS * view.scale + 2, RED);
  }

  for (size_t i = 0; i < preview.size(); i++) {
    float fade = 1 - static_cast<float>(i) / preview.size();
    DrawCircleV(view.ToScreenRaised(preview[i]), 2.5f, Fade(RAYWHITE, 0.15f + 0.6f * fade));
  }

  for (auto &[id, player] : players)
    if (id != localID)
      player.DrawPlayer(view, false);
  if (auto local = players.find(localID); local != players.end())
    local->second.DrawPlayer(view, true);
  DrawBall(view);
}
