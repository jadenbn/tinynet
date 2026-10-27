#include "ClientGame.h"
#include "Ball.h"
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "GameTypes.h"
#include "Packets.h"
#include "Protocol.h"
#include "game/ClientWorld.h"
#include "raylib.h"
#include "resource_dir.h"
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

constexpr static int MAX_ATTEMPTS = 100;

ClientGame::ClientGame(Address &clientAddress_c, Address &serverAddress_c)
    : client(clientAddress_c, serverAddress_c), clientWorld(),
      replicationSystem(clientWorld, simulation) {};

void ClientGame::NetworkInit() {
  client.Initialize();

  // handshake
  bool connectionConfirmed = false;
  int attempts = 0;

  uint8_t scratch[MAX_PACKET_SIZE];
  Buffer buff{scratch, 0, 0, sizeof(scratch)};
  while (!connectionConfirmed && attempts < MAX_ATTEMPTS) {
    client.SendPacket(ConnectionRequest{});
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    if (client.ReceiveFromServer(buff) > 0) {
      PacketType type = static_cast<PacketType>(packets::ReadChar(buff));

      if (type == PacketType::ConnectionRejected)
        throw std::runtime_error("Server is full");
      if (type == PacketType::ConnectionAccepted && buff.length - buff.index >= 4) {
        connectionConfirmed = true;
        PlayerID id = packets::ReadInteger(buff);
        this->id = id;
        replicationSystem.localID = id;
        clientWorld.localID = id;
        return;
      }
    }
    attempts++;
  }

  throw std::runtime_error("Couldn't connect to server.");
}

void ClientGame::GameInit() {
  constexpr int WIDTH = 1280; // 960 for the pitch + 320 for the net overlay
  constexpr int HEIGHT = 720;

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
  InitWindow(WIDTH, HEIGHT, "tinynet: tiny football");

  SearchAndSetResourceDir("resources");

  SetTargetFPS(144);
}

Rectangle ClientGame::PitchArea() const {
  float width = GetScreenWidth() - (overlay.Visible() ? NetDebugOverlay::WIDTH : 0);
  return {0, 56, width, GetScreenHeight() - 56.0f - 30};
}

KickCommand ClientGame::ChargedKick() const {
  double held = std::clamp((GetTime() - chargeStart) / CHARGE_SECONDS, 0.0, 1.0);
  // hold E or Q while kicking to bend it right or left
  int curve = (IsKeyDown(KEY_E) ? 127 : 0) - (IsKeyDown(KEY_Q) ? 127 : 0);
  return {charging, static_cast<uint8_t>(std::lround(held * 255)),
          static_cast<int8_t>(curve)};
}

void ClientGame::HandleFrameInput() {
  if (charging == KickType::None) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
      charging = KickType::Ground;
    else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
      charging = KickType::Chip;
    chargeStart = GetTime();
  } else {
    int button = charging == KickType::Ground ? MOUSE_BUTTON_LEFT : MOUSE_BUTTON_RIGHT;
    if (!IsMouseButtonDown(button)) {
      queuedKick = ChargedKick();
      charging = KickType::None;
    }
  }

  for (int phrase = 0; phrase < QUICK_CHAT_PHRASES; phrase++)
    if (IsKeyPressed(KEY_FIVE + phrase))
      client.SendReliable(EventMessage{{EventType::QuickChat, NO_PLAYER, 0,
                                        static_cast<uint8_t>(phrase)}});
}

void ClientGame::HandleInput() {
  // WASD moves you around the screen; the mouse is where you face
  Vec3 world{static_cast<float>(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)), 0,
             static_cast<float>(IsKeyDown(KEY_W) - IsKeyDown(KEY_S))};
  Vec3 aim = view.ToWorld(GetMousePosition()) - simulation.local.pos;
  aim.y = 0;
  if (Dot(aim, aim) > 0.01f)
    yaw = YawTowards(aim);

  // the server wants directions relative to where we face
  Vec3 direction = RelativeDirection(Normalize(world), yaw);
  simulation.AddInput(direction, yaw, std::exchange(queuedKick, {}));

  PlayerInputPacket packet;
  for (const auto &input : simulation.pending) {
    if (packet.inputs.size() == INPUTS_PER_PACKET)
      break;
    packet.inputs.push_back(input);
  }
  client.SendPacket(packet);
}

void ClientGame::HandleNetwork() {
  client.UpdateConnection();
  uint8_t scratch[MAX_PACKET_SIZE];
  Buffer packet{scratch, 0, 0, sizeof(scratch)};
  while (client.ReceiveFromServer(packet) > 0)
    replicationSystem.HandlePacket(packet, client.GetRTT()/2000.0);
  uint8_t message[ReliableChannel::MAX_MESSAGE_SIZE];
  int size;
  while ((size = client.GetConnection().ReceiveReliable(message, sizeof(message))) > 0) {
    Buffer messageBuff{message, 0, size, size};
    replicationSystem.HandleMessage(messageBuff);
  }
  for (const GameEvent &event : replicationSystem.events)
    hud.OnEvent(event, clientWorld.Name(event.player), event.player == id, GetTime());
  replicationSystem.events.clear();
}

void ClientGame::UpdatePreview() {
  // run the same physics as the server on a copy of the ball, as if we let
  // go right now. that's the dotted line, curve and all
  clientWorld.preview.clear();
  if (charging == KickType::None || !simulation.PredictingBall())
    return;
  PlayerState shooter = simulation.local;
  shooter.orientation = Yaw(yaw);
  BallState ball = simulation.ball;
  if (!TryKick(shooter, ChargedKick(), ball))
    return;
  for (int i = 0; i < 75; i++) {
    SimulateBall(ball);
    if (i % 3 == 0)
      clientWorld.preview.push_back(ball.pos);
    if (GoalLineCrossed(ball))
      break;
  }
}

void ClientGame::DrawCharge() const {
  auto local = clientWorld.players.find(id);
  if (charging == KickType::None || local == clientWorld.players.end())
    return;
  KickCommand kick = ChargedKick();
  Vector2 centre = view.ToScreen(local->second.state.pos);
  float radius = PLAYER_RADIUS * view.scale + 6;
  Color color = charging == KickType::Chip ? SKYBLUE : GOLD;
  DrawRing(centre, radius, radius + 4, -90, -90 + 360 * kick.power / 255.0f, 36, color);
  if (kick.curve != 0)
    DrawText(kick.curve > 0 ? "curl >" : "< curl", centre.x - 14, centre.y + radius + 6, 10, color);
}

void ClientGame::GameLoop() {
  SetExitKey(KEY_ESCAPE);
  double previous = GetTime(), accumulator = 0;
  while (!WindowShouldClose()) {
    double now = GetTime();
    double dt = std::min(now-previous, 0.25);
    accumulator += dt;
    previous = now;
    view = PitchView::Fit(PitchArea());

    HandleNetwork();
    HandleFrameInput();
    while (accumulator >= TICK_DT) {
      HandleInput();
      accumulator -= TICK_DT;
    }
    replicationSystem.Update(dt, static_cast<float>(accumulator / TICK_DT));
    UpdatePreview();
    overlay.Update(client);

    BeginDrawing();
    ClearBackground({22, 27, 33, 255});
    clientWorld.Draw(view, overlay.Visible());
    DrawCharge();
    Rectangle area = PitchArea();
    hud.Draw(clientWorld.match, area, now);
    DrawText("WASD move   mouse aim   hold LMB shoot / RMB chip   Q/E curl   5-8 chat   TAB net",
             12, GetScreenHeight() - 22, 12, GRAY);
    if (!client.GetIsConnected()) {
      const char *text = client.WasDisconnected() ? "the server closed" : "connection lost";
      DrawText(text, area.width / 2 - MeasureText(text, 30) / 2.0f, area.y + 40, 30, RED);
    }
    auto cm = [](Vec3 v) { return std::sqrt(Dot(v, v)) * 100; };
    overlay.Draw(client, {cm(simulation.BallError()), cm(simulation.LocalError()),
                          static_cast<int>(simulation.pending.size())});
    EndDrawing();
  }
  client.Disconnect();
  CloseWindow();
}
