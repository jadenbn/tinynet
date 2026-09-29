#include "NetDebugOverlay.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {

constexpr float PANEL_WIDTH = 320.0f;
constexpr float PAD = 10.0f;
constexpr int FONT = 10;
constexpr int LINE = 14;

constexpr Color PANEL = {20, 20, 24, 235};
constexpr Color TEXT = {220, 220, 220, 255};
constexpr Color DIM = {130, 130, 130, 255};
constexpr Color EMPTY = {55, 55, 60, 255};
constexpr Color UP = {90, 170, 255, 255};
constexpr Color DOWN = {120, 220, 120, 255};
constexpr Color GOOD = {80, 200, 120, 255};
constexpr Color WAITING = {230, 200, 80, 255};
constexpr Color BAD = {230, 80, 80, 255};

// rounds up to a tidy number for a graph's top edge, e.g. 183 -> 200
float NiceMax(float value, float minimum) {
  value = std::max(value, minimum);
  float step = std::pow(10.0f, std::floor(std::log10(value)));
  return std::ceil(value / step) * step;
}

// immediate mode slider: draws itself and hands back the (maybe dragged)
// value. no slider objects to keep in sync, the value is the state
float Slider(float x, float y, const char *label, float value, float max,
             float displayScale, const char *unit) {
  Rectangle track = {x + 70, y + 1, 160, 8};
  Rectangle hitbox = {track.x, track.y - 4, track.width, track.height + 8};

  Vector2 mouse = GetMousePosition();
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
      CheckCollisionPointRec(mouse, hitbox)) {
    value = Clamp((mouse.x - track.x) / track.width, 0.0f, 1.0f) * max;
  }

  DrawText(label, x, y, FONT, TEXT);
  DrawRectangleRec(track, EMPTY);
  DrawRectangleRec({track.x, track.y, track.width * value / max, track.height},
                   UP);
  DrawText(TextFormat("%.0f %s", value * displayScale, unit),
           track.x + track.width + 8, y, FONT, TEXT);
  return value;
}

// a little "[] label" key, returns where the next one should go
float Legend(float x, float y, const char *label, Color color) {
  DrawRectangle(x, y + 2, 6, 6, color);
  DrawText(label, x + 9, y, FONT, DIM);
  return x + 9 + MeasureText(label, FONT) + 8;
}

} // namespace

void NetDebugOverlay::Update(Client &client) {
  if (IsKeyPressed(KEY_TAB)) {
    visible = !visible;
  }

  // presets, so a demo can jump straight to an interesting network
  if (IsKeyPressed(KEY_ONE))
    client.SetConditions({});
  if (IsKeyPressed(KEY_TWO))
    client.SetConditions({20.0f, 5.0f, 0.01f, 0.0f});
  if (IsKeyPressed(KEY_THREE))
    client.SetConditions({60.0f, 20.0f, 0.05f, 0.01f});
  if (IsKeyPressed(KEY_FOUR))
    client.SetConditions({150.0f, 50.0f, 0.2f, 0.05f});

  double now = GetTime();
  if (now - lastSampleTime < SAMPLE_INTERVAL) {
    return;
  }

  // the connection only keeps running totals; rates come from how far they
  // moved since the last sample
  ConnectionStats stats = client.GetConnection().GetStats();
  float seconds = static_cast<float>(now - lastSampleTime);
  history[head] = {
      stats.rtt,
      (stats.bytesSent - lastStats.bytesSent) * 8 / 1000.0f / seconds,
      (stats.bytesReceived - lastStats.bytesReceived) * 8 / 1000.0f / seconds,
      (stats.packetsSent - lastStats.packetsSent) / seconds,
      (stats.packetsReceived - lastStats.packetsReceived) / seconds,
  };
  head = (head + 1) % HISTORY;
  lastStats = stats;
  lastSampleTime = now;
}

const NetDebugOverlay::Sample &NetDebugOverlay::Latest() const {
  return history[(head + HISTORY - 1) % HISTORY];
}

float NetDebugOverlay::Max(float Sample::*field) const {
  float max = 0.0f;
  for (const Sample &sample : history) {
    max = std::max(max, sample.*field);
  }
  return max;
}

// one line per sample, oldest on the left. `field` picks which number in
// the sample to plot, so the same code draws rtt, up, and down
void NetDebugOverlay::Plot(Rectangle bounds, float Sample::*field, float max,
                           Color color) const {
  float step = bounds.width / (HISTORY - 1);
  for (int i = 1; i < HISTORY; i++) {
    float a = history[(head + i - 1) % HISTORY].*field / max;
    float b = history[(head + i) % HISTORY].*field / max;
    Vector2 from = {bounds.x + (i - 1) * step,
                    bounds.y + bounds.height * (1.0f - std::min(a, 1.0f))};
    Vector2 to = {bounds.x + i * step,
                  bounds.y + bounds.height * (1.0f - std::min(b, 1.0f))};
    DrawLineV(from, to, color);
  }
}

// the last 64 packets we sent, newest on the right, coloured by whether the
// other side has acked them yet
int NetDebugOverlay::DrawSentStrip(Rectangle bounds, Connection &conn) {
  uint32_t next = conn.GetSequenceNumber();
  float cell = bounds.width / STRIP;
  int lost = 0;

  for (int i = 0; i < STRIP; i++) {
    uint32_t back = STRIP - i; // how many packets ago
    Color color = EMPTY;
    if (next >= back) {
      switch (conn.GetSentStatus(next - back)) {
      case PacketStatus::Acked:
        color = GOOD;
        break;
      case PacketStatus::Pending:
        color = WAITING;
        break;
      case PacketStatus::Lost:
        color = BAD;
        lost++;
        break;
      case PacketStatus::Unknown:
        break;
      }
    }
    DrawRectangleRec({bounds.x + i * cell, bounds.y, cell - 1, bounds.height},
                     color);
  }

  return lost;
}

// the last 64 sequence numbers the other side sent, and whether they got
// here. late packets (jitter) show up as holes that fill in a moment later
int NetDebugOverlay::DrawReceivedStrip(Rectangle bounds, Connection &conn) {
  uint32_t newest = conn.GetRemoteSequenceNumber();
  float cell = bounds.width / STRIP;
  int missing = 0;

  for (int i = 0; i < STRIP; i++) {
    uint32_t back = STRIP - 1 - i;
    Color color = EMPTY;
    if (newest >= back) {
      bool received = conn.WasReceived(newest - back);
      color = received ? GOOD : BAD;
      missing += received ? 0 : 1;
    }
    DrawRectangleRec({bounds.x + i * cell, bounds.y, cell - 1, bounds.height},
                     color);
  }

  return missing;
}

void NetDebugOverlay::Draw(Client &client) {
  if (!visible) {
    DrawText("[tab] net debug", PAD, GetScreenHeight() - 20, FONT, DIM);
    return;
  }

  Connection &conn = client.GetConnection();
  ConnectionStats stats = conn.GetStats();
  const Sample &now = Latest();

  float left = GetScreenWidth() - PANEL_WIDTH;
  DrawRectangle(left, 0, PANEL_WIDTH, GetScreenHeight(), PANEL);

  float x = left + PAD;
  float width = PANEL_WIDTH - 2 * PAD;
  float y = PAD;

  // header
  bool connected = client.GetIsConnected();
  DrawText("tinynet", x, y, 20, TEXT);
  DrawText(connected ? "connected" : "timed out", x + 90, y + 6, FONT,
           connected ? GOOD : BAD);
  DrawText("[tab] hide", x + width - MeasureText("[tab] hide", FONT), y + 6,
           FONT, DIM);
  y += 30;

  // numbers
  DrawText(TextFormat("rtt %.1f ms", now.rtt), x, y, FONT, TEXT);
  y += LINE;
  DrawText(TextFormat("up %.1f kbps  %.0f pkt/s", now.upKbps, now.upPackets),
           x, y, FONT, UP);
  DrawText(
      TextFormat("down %.1f kbps  %.0f pkt/s", now.downKbps, now.downPackets),
      x + width / 2, y, FONT, DOWN);
  y += LINE;
  DrawText(TextFormat("sent %llu  acked %llu  lost %llu", stats.packetsSent,
                      stats.packetsAcked, stats.packetsLost),
           x, y, FONT, TEXT);
  y += LINE;
  DrawText(TextFormat("received %llu  out of order %llu  dupes %llu",
                      stats.packetsReceived, stats.outOfOrderReceived,
                      stats.duplicatesReceived),
           x, y, FONT, TEXT);
  y += LINE + 8;

  // rtt graph
  float rttMax = NiceMax(Max(&Sample::rtt), 50.0f);
  DrawText("rtt (ms)", x, y, FONT, DIM);
  DrawText(TextFormat("%.0f", rttMax), x + width - 30, y, FONT, DIM);
  y += LINE;
  Rectangle rttGraph = {x, y, width, 50};
  DrawRectangleRec(rttGraph, EMPTY);
  Plot(rttGraph, &Sample::rtt, rttMax, WAITING);
  y += rttGraph.height + 8;

  // bandwidth graph
  float kbpsMax = NiceMax(std::max(Max(&Sample::upKbps), Max(&Sample::downKbps)),
                          10.0f);
  float legend = Legend(x, y, "up kbps", UP);
  Legend(legend, y, "down kbps", DOWN);
  DrawText(TextFormat("%.0f", kbpsMax), x + width - 30, y, FONT, DIM);
  y += LINE;
  Rectangle kbpsGraph = {x, y, width, 50};
  DrawRectangleRec(kbpsGraph, EMPTY);
  Plot(kbpsGraph, &Sample::upKbps, kbpsMax, UP);
  Plot(kbpsGraph, &Sample::downKbps, kbpsMax, DOWN);
  y += kbpsGraph.height + 10;

  // what happened to our last 64 packets
  DrawText("sent by us", x, y, FONT, TEXT);
  legend = Legend(x + 70, y, "acked", GOOD);
  legend = Legend(legend, y, "waiting", WAITING);
  Legend(legend, y, "lost", BAD);
  y += LINE;
  int lost = DrawSentStrip({x, y, width, 12}, conn);
  const char *lostText = TextFormat("%d lost", lost);
  DrawText(lostText, x + width - MeasureText(lostText, FONT), y - LINE, FONT,
           lost > 0 ? BAD : DIM);
  y += 12 + 8;

  // what happened to the server's last 64 packets
  DrawText("sent by server", x, y, FONT, TEXT);
  legend = Legend(x + 90, y, "received", GOOD);
  Legend(legend, y, "missing", BAD);
  y += LINE;
  int missing = DrawReceivedStrip({x, y, width, 12}, conn);
  const char *missingText = TextFormat("%d missing", missing);
  DrawText(missingText, x + width - MeasureText(missingText, FONT), y - LINE,
           FONT, missing > 0 ? BAD : DIM);
  y += 12 + 14;

  // fake network conditions
  DrawText("network simulator", x, y, FONT, TEXT);
  DrawText("[1-4] presets", x + width - MeasureText("[1-4] presets", FONT), y,
           FONT, DIM);
  y += LINE + 2;
  NetworkConditions conditions = client.GetConditions();
  conditions.latency =
      Slider(x, y, "latency", conditions.latency, 300.0f, 1.0f, "ms");
  y += LINE + 2;
  conditions.jitter =
      Slider(x, y, "jitter", conditions.jitter, 100.0f, 1.0f, "ms");
  y += LINE + 2;
  conditions.loss = Slider(x, y, "loss", conditions.loss, 0.5f, 100.0f, "%");
  y += LINE + 2;
  conditions.duplicate =
      Slider(x, y, "duplicate", conditions.duplicate, 0.2f, 100.0f, "%");
  client.SetConditions(conditions);
  y += LINE + 2;
  DrawText("latency + jitter are added in each direction", x, y, FONT, DIM);
}
