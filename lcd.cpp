// Displaying the picture on the LCD
#include "pch.h"

int lcd_scale = 4;
int lcd_fpslimit = 1;
int lcd_effect = 1; // possible values: 0,1
int lcd_border = 0;

int screen_width, screen_height;
static uint32_t* pbuf;

/* milk to cofee */
uint32_t dmg_pal[] = {
	0xffe78f,		// color #0 (milk)
	0xdfb05f,		// color #1
	0x90783f,		// color #2
	0x4f381f,		// color #3 (cofee)
};

SDL_Surface* output_surface = nullptr;
SDL_Window* output_window = nullptr;

void lcd_refresh(int line)
{
	int i;
	uint32_t* p = (uint32_t*)pbuf + 160 * line;
	if (lcd_effect == 1)
		for (i = 0; i < 160; i++)
			p[i] = (0x7F7F7F & (p[i] >> 1)) + (0x7F7F7F & (((uint32_t*)dmg_pal)[mainpal[(linebuffer + 8)[i] & 0x3F]] >> 1));
	else
		for (i = 0; i < 160; i++)
			p[i] = ((uint32_t*)dmg_pal)[mainpal[(linebuffer + 8)[i] & 0x3F]];
}

void sdl_win_init(int width, int height)
{
	screen_width = width;
	screen_height = height;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		__log ("SDL video could not initialize! SDL_Error: %s\n", SDL_GetError());
		return;
	}

	char title[128];
	sprintf(title, "GameBoy - %s", cart.title);

	SDL_Window* window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_width * lcd_scale + 2 * lcd_border * lcd_scale, screen_height * lcd_scale + 2 * lcd_border * lcd_scale,
		0);

	if (window == NULL) {
		__log ("SDL_CreateWindow failed: %s\n", SDL_GetError());
		return;
	}

	SDL_Surface* surface = SDL_GetWindowSurface(window);

	if (surface == NULL) {
		__log ("SDL_GetWindowSurface failed: %s\n", SDL_GetError());
		return;
	}

	// Initialize window to all black
	//SDL_FillSurfaceRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
	SDL_UpdateWindowSurface(window);

	output_window = window;
	output_surface = surface;

	pbuf = new uint32_t[screen_width * screen_height];
	memset(pbuf, 0, screen_width * screen_height * sizeof(uint32_t));
}

void sdl_win_shutdown()
{
	SDL_DestroyWindow(output_window);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	delete[] pbuf;
}

void sdl_win_update()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {

		switch (event.type) {
			case SDL_QUIT:
				gb_shutdown();
				exit(0);
				break;

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				bool pressed = event.type == SDL_KEYDOWN;
				switch (event.key.keysym.scancode) {
					case SDL_SCANCODE_F12:
						if (!pressed) {
							(sound_enabled) ? apu_shutdown() : apu_init(44100);
							sound_enabled ^= 1;
						}
						break;
					case SDL_SCANCODE_F8:
						if (!pressed) {
							lcd_fpslimit ^= 1;
						}
						break;
					case SDL_SCANCODE_F9:
						if (!pressed) {
							lcd_effect ^= 1;
						}
						break;

					default:
						pad_sdl_process(event.key.keysym.scancode, pressed);
						break;
				}
				break;
		}
	}
}

void sdl_win_blit()
{
	int w = screen_width;
	int h = screen_height;
	int ScaleFactor = lcd_scale;

	Uint32* const pixels = (Uint32*)output_surface->pixels;

	if (lcd_border != 0) {
		for (int n = 0; n < output_surface->w * output_surface->h; n++) {
			pixels[n] = dmg_pal[0];
		}
	}

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			uint32_t color = pbuf[w * y + x];

			for (int s = 0; s < ScaleFactor; s++) {
				for (int t = 0; t < ScaleFactor; t++) {
					pixels[ScaleFactor * (x + lcd_border) + s + ((ScaleFactor * (y + lcd_border) + t) * output_surface->w)] = color;
				}
			}
		}
	}

	SDL_UpdateWindowSurface(output_window);
}

void sdl_win_update_title(char* title)
{
	if (!output_window)
		return;
	SDL_SetWindowTitle(output_window, title);
}
