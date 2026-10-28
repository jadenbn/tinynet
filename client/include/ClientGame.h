// for handling gamestate
#pragma once
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "NetDebugOverlay.h"
#include "game/ClientWorld.h"
#include "game/Hud.h"
#include "game/PitchView.h"

class ClientGame {
public:
  ClientGame(Address &clientAddress, Address &serverAddress, bool bot = false);
  void GameLoop();
  void NetworkInit();
  void GameInit();
  void HandleNetwork();
  void HandleInput();      // once per tick: one input for the server
  void HandleFrameInput(); // once per frame: charging kicks, quick chat

private:
  static constexpr double CHARGE_SECONDS = 0.8; // to a full power kick

  Client client;
  ClientWorld clientWorld;
  ClientSimulation simulation;
  ClientReplicationSystem replicationSystem;
  NetDebugOverlay overlay;
  Hud hud;
  PitchView view;
  PlayerID id = 0;
  float yaw = 0;
  bool bot; // the computer plays, through exactly the same inputs
  uint32_t ticks = 0;

  // holding a mouse button winds up a kick; letting go queues it for the
  // next tick's input
  KickType charging = KickType::None;
  double chargeStart = 0;
  KickCommand queuedKick;

  KickCommand ChargedKick() const;
  void UpdatePreview();
  void DrawCharge() const;
  Rectangle PitchArea() const;
};
