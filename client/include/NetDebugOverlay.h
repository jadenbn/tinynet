#pragma once
#include "Client.h"
#include "raylib.h"
#include <array>

// how the client's prediction is doing, for the overlay to show
struct PredictionStats {
  float ballCorrection = 0;  // cm still being smoothed out
  float localCorrection = 0; // cm
  int pendingInputs = 0;     // how far ahead of the server we are
};

// draws what the networking layer is doing under the hood: rtt, bandwidth,
// which packets made it, plus sliders for faking a bad network
class NetDebugOverlay {
public:
  // samples stats and handles the overlay's keys. call once a frame
  void Update(Client &client);
  void Draw(Client &client, const PredictionStats &prediction);
  bool Visible() const { return visible; }

  // perfect, decent wifi, mobile, awful. keys 1-4, or --preset on the
  // command line
  static constexpr NetworkConditions PRESETS[4] = {
      {}, {20.0f, 5.0f, 0.01f, 0.0f}, {60.0f, 20.0f, 0.05f, 0.01f},
      {150.0f, 50.0f, 0.2f, 0.05f}};
  static constexpr float WIDTH = 320.0f;

private:
  struct Sample {
    float rtt; // ms
    float upKbps;
    float downKbps;
    float upPackets; // per second
    float downPackets;
  };

  static constexpr int HISTORY = 120;            // samples on the graphs
  static constexpr double SAMPLE_INTERVAL = 0.1; // seconds, so 12s of history
  static constexpr int STRIP = 64;               // packets on the ack strips

  bool visible = true;
  std::array<Sample, HISTORY> history = {};
  int head = 0; // where the next sample goes
  double lastSampleTime = 0.0;
  ConnectionStats lastStats;

  const Sample &Latest() const;
  float Max(float Sample::*field) const;
  void Plot(Rectangle bounds, float Sample::*field, float max,
            Color color) const;
  int DrawSentStrip(Rectangle bounds, Connection &conn);
  int DrawReceivedStrip(Rectangle bounds, Connection &conn);
};
