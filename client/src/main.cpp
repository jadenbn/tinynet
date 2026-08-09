#include "raylib.h"
#include "raymath.h"
#include "resource_dir.h"
#include <iostream>
#include "string"

class Player
{
private:
	float scaleFactor = 2.0f;

public:
	Vector2 position;
	Texture sprite;

	Player(Vector2 initialPosition, Texture playerSprite)
	{
		position = initialPosition;
		sprite = playerSprite;
	}

	void DrawPlayer()
	{
		// todo: make this cleaner
		// DrawTextureEx(sprite, {this->position.x, this->position.y}, 0.0f, scaleFactor, BLUE);
		DrawTexture(sprite, this->position.x, this->position.y, BLUE);
	}
};

void MovePlayer(Player *p, int speed);

int main()
{
	constexpr int WIDTH = 1920;
	constexpr int HEIGHT = 1080;

	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(WIDTH, HEIGHT, "tinynet client");

	SearchAndSetResourceDir("resources");

	// main player starts in center of screen
	Texture mainPlayerSprite = LoadTexture("github.png");
	Player mainPlayer({(float)(WIDTH / 2), (float)(HEIGHT / 2)}, mainPlayerSprite);

	// game loop
	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(WHITE);

		MovePlayer(&mainPlayer, 200);

		// draw main player
		mainPlayer.DrawPlayer();

		EndDrawing();
	}

	// cleanup
	UnloadTexture(mainPlayerSprite);
	CloseWindow();
	return 0;
}

void MovePlayer(Player *p, int speed)
{
	Vector2 direction;

	if (IsKeyDown(KEY_W))
		direction.y -= 1.0f;
	if (IsKeyDown(KEY_A))
		direction.x -= 1.0f;
	if (IsKeyDown(KEY_S))
		direction.y += 1.0f;
	if (IsKeyDown(KEY_D))
		direction.x += 1.0f;

	if (direction.x != 0.0f || direction.y != 0.0f)
	{
		direction = Vector2Normalize(direction);
	}

	// account for offset of sprite
	DrawText(std::string(std::to_string(direction.x) + std::to_string(direction.y)).c_str(), p->position.x - (p->sprite.width / 2), p->position.y - 25 - (p->sprite.height / 2), 12, BLACK);

	float deltaTime = GetFrameTime();
	p->position.x += direction.x * speed * deltaTime;
	p->position.y += direction.y * speed * deltaTime;

	direction = {0, 0};
}
