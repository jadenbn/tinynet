#include "raylib.h"
#include "resource_dir.h"

int main()
{
	constexpr int WIDTH = 1920;
	constexpr int HEIGHT = 1080;

	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(WIDTH, HEIGHT, "tinynet");

	SearchAndSetResourceDir("resources");
	Texture testSprite = LoadTexture("github.png");

	// game loop
	while (!WindowShouldClose()) // run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLACK);

		// draw some text using the default font
		DrawText("test", 200, 200, 20, WHITE);

		// draw our texture to the screen
		DrawTexture(testSprite, WIDTH / 2, HEIGHT / 2, WHITE);

		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// cleanup
	// unload our texture so it can be cleaned up
	UnloadTexture(testSprite);

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
