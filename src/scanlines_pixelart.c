// main.c
//
// (c) 2025 elmerucr

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdint.h>
#include <stdio.h>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;
static SDL_Texture *texture = nullptr;

static uint8_t scanline_alpha = 0xb0;

const char *modes[] = {
    "SDL_SCALEMODE_NEAREST",
    "SDL_SCALEMODE_PIXELART",
    "SDL_SCALEMODE_LINEAR"
};

static int current_scale_mode = 1;
static bool fullscreen = false;

uint8_t rnd8() {
    static uint8_t a, b, c, x;
    x++;
    a = (a ^ c) ^ x;
    b = b + a;
    c = (c + ((b >> 1) | (b << 7))) ^ a;
    return c;
}

#define TEXTURE_WIDTH   320
#define TEXTURE_HEIGHT  360

static int window_width;
static int window_height;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    printf("[SDL] Version %i.%i.%i\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

    SDL_SetAppMetadata("scanlines_pixelart", "1.0", "elmerucr");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

	int num_displays = 0;
	SDL_DisplayID *displays = SDL_GetDisplays(&num_displays);
	printf("[SDL] Number of displays: %i\n", num_displays);
	const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(*displays);
	printf("[SDL] Desktop display mode: %ix%i\n", mode->w, mode->h);
	int mag = mode->w / TEXTURE_WIDTH;
	if ((mode->w % TEXTURE_WIDTH) == 0) {
		mag--;
	}
	window_width = mag * TEXTURE_WIDTH;
	window_height = mag * TEXTURE_WIDTH * 9 / 16;
	printf("[SDL] Window size %ix%i\n", window_width, window_height);
	SDL_free(displays);

    if (!SDL_CreateWindowAndRenderer("scanlines_pixelart", window_width, window_height, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Surface *icon = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_ARGB8888);
    if (icon->pixels) {
        const uint8_t pixels[] = {
            "                "
            "   *            "
            "  *./           "
            "  *.//          "
            " *./../         "
            " *.///..        "
            " *./////.       "
            " *.///../.      "
            " *./..//./.     "
            "  *.////././    "
            "  *.///.///./   "
            "   *.//.///.//  "
            "    *../////..* "
            "     **.....**  "
            "       *****    "
            "                "
        };
        for (unsigned int y = 0; y < 64; y++) {
            for (unsigned int x = 0; x < 64; x++) {
                switch (pixels[(16 * (y >> 2)) + (x >> 2)]) {
                    case '*': ((uint32_t *)icon->pixels)[(y * 64) + x] = 0xff'34'68'56; break;
                    case '/': ((uint32_t *)icon->pixels)[(y * 64) + x] = 0xff'88'c0'70; break;
                    case '.': ((uint32_t *)icon->pixels)[(y * 64) + x] = 0xff'e0'f8'd0; break;
                    default : ((uint32_t *)icon->pixels)[(y * 64) + x] = 0x00'00'00'00; break;
                }
            }
        }
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
    }

    SDL_SetRenderVSync(renderer, 1);
    int i;
    SDL_GetRenderVSync(renderer, &i);
    if (i) {
        printf("[SDL] VSync every %i frame(s)\n", i);
    } else {
        printf("[SDL] VSync is off\n");
    }

    SDL_SetRenderLogicalPresentation(renderer, 2*TEXTURE_WIDTH, 2*TEXTURE_HEIGHT/2, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, TEXTURE_WIDTH, TEXTURE_HEIGHT);
    if (!texture) {
        SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        SDL_ScaleMode sm;
        SDL_GetTextureScaleMode(texture, &sm);
        if (sm == SDL_SCALEMODE_LINEAR) {
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
            current_scale_mode = 0;
        } else if (sm == SDL_SCALEMODE_NEAREST) {
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
            current_scale_mode = 1;
        } else if (sm == SDL_SCALEMODE_PIXELART) {
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
            current_scale_mode = 2;
        }
    }

	if (event->type == SDL_EVENT_KEY_DOWN) {
		switch (event->key.scancode) {
		case SDL_SCANCODE_UP:
			scanline_alpha += 2;
			break;
		case SDL_SCANCODE_DOWN:
			scanline_alpha -= 2;
			break;
		case SDL_SCANCODE_F:
			fullscreen = !fullscreen;
			SDL_SetWindowFullscreen(window, fullscreen);
		default:
			break;
		}
	}

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    void *pixels;
    int pitch;

    SDL_LockTexture(texture, NULL, &pixels, &pitch);

    for (int y=0; y<TEXTURE_HEIGHT; y += 2) {
        for (int x=0; x<TEXTURE_WIDTH; x++) {
            //uint8_t lum = rnd8();
            //((uint32_t *)pixels)[(y*pitch/4) + x] = 0xff00f030;   // green
            ((uint32_t *)pixels)[(y*pitch/4) + x] = 0xff000000 | (rnd8() << 16) | /*(rnd8() << 8) |*/ rnd8(); // random color
            //((uint32_t *)pixels)[(y*pitch/4) + x] = 0xff000000 | (lum << 16) | (lum << 8) | lum; // random color
        }
    }

    for (int y=1; y<(TEXTURE_HEIGHT - 2); y += 2) {
        for (int x=0; x<TEXTURE_WIDTH; x++) {
            uint32_t col_top  = ((uint32_t *)pixels)[((y-1)*pitch/4) + x];
            uint32_t col_down = ((uint32_t *)pixels)[((y+1)*pitch/4) + x];
            uint32_t effect =
                scanline_alpha << 24 |
                (((col_top & 0x00ff0000) + (col_down & 0x00ff0000)) >> 1) & 0x00ff0000 |
                (((col_top & 0x0000ff00) + (col_down & 0x0000ff00)) >> 1) & 0x0000ff00 |
                (((col_top & 0x000000ff) + (col_down & 0x000000ff)) >> 1) & 0x000000ff;
            ((uint32_t *)pixels)[(y*pitch/4) + x] = effect;
        }
    }
    for (int x = 0; x < TEXTURE_WIDTH; x++) {
        ((uint32_t *)pixels)[((TEXTURE_HEIGHT - 1) * pitch/4) + x] =
            (((uint32_t *)pixels)[((TEXTURE_HEIGHT - 2) * pitch/4) + x] & 0x00ffffff) | (scanline_alpha << 24);
    }

    SDL_UnlockTexture(texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);  /* start with a blank canvas. */
    SDL_RenderTexture(renderer, texture, NULL, NULL);

	SDL_SetRenderDrawColor(renderer, 0x00, 0xf0, 0x30, 0xff);
	SDL_RenderDebugTextFormat(renderer, 8,  8, "Scanline alpha: 0x%02x", scanline_alpha);
	SDL_RenderDebugTextFormat(renderer, 8, 18, "Scalemode:      %s", modes[current_scale_mode]);

    SDL_RenderPresent(renderer);  /* put it all on the screen! */
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyTexture(texture);
}
