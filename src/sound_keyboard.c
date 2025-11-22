#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *stream = NULL;
static int current_sine_sample = 0;

static const bool *sdl_keyboard_state = NULL;
static bool alt_pressed = false;
static bool fullscreen = false;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    printf("[SDL] Version %i.%i.%i\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

    SDL_AudioSpec spec;

    SDL_SetAppMetadata("Example Audio Simple Playback", "1.0", "com.example.audio-simple-playback");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* we don't _need_ a window for audio-only things but it's good policy to have one. */
    if (!SDL_CreateWindowAndRenderer("examples/audio/simple-playback", 320, 180, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderVSync(renderer, 1);
    int i;
    SDL_GetRenderVSync(renderer, &i);
    if (i) {
        printf("[SDL] VSync every %i frame(s)\n", i);
    } else {
        printf("[SDL] VSync is off\n");
    }

    SDL_SetRenderLogicalPresentation(renderer, 320, 180, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    /* We're just playing a single thing here, so we'll use the simplified option.
       We are always going to feed audio in as mono, float32 data at 48000Hz.
       The stream will convert it to whatever the hardware wants on the other side. */
    spec.channels = 1;
    spec.format = SDL_AUDIO_F32;
    spec.freq = 48000;
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        SDL_Log("[SDL] Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start! */
    SDL_ResumeAudioStreamDevice(stream);

	int numkeys = 0;

	sdl_keyboard_state = SDL_GetKeyboardState(&numkeys);

	if (!sdl_keyboard_state) {
		printf("[SDL] Error getting keyboard state: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_KEY_DOWN) {
		switch (event->key.scancode) {
		case SDL_SCANCODE_F:
			if (alt_pressed) {
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen);
			}
			break;
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
	alt_pressed = sdl_keyboard_state[SDL_SCANCODE_LALT] || sdl_keyboard_state[SDL_SCANCODE_RALT];

    /* see if we need to feed the audio stream more data yet.
       We're being lazy here, but if there's less than half a second queued, generate more.
       A sine wave is unchanging audio--easy to stream--but for video games, you'll want
       to generate significantly _less_ audio ahead of time! */
    const int minimum_audio = (48000 * sizeof (float)) / 2;  /* 48000 float samples per second. Half of that. */
    if (SDL_GetAudioStreamQueued(stream) < minimum_audio) {
        static float samples[1024];  /* this will feed 1024 samples each frame until we get to our maximum. */
        int i;

        /* generate a 440Hz pure tone */
        for (i = 0; i < SDL_arraysize(samples); i++) {
            const int freq = 440;
            const float phase = current_sine_sample * freq / 48000.0f;
            samples[i] = SDL_sinf(phase * 2 * SDL_PI_F);
            current_sine_sample++;
        }

        /* wrapping around to avoid floating-point errors */
        current_sine_sample %= 48000;

        /* feed the new data to the stream. It will queue at the end, and trickle out as the hardware needs more data. */
        SDL_PutAudioStreamData(stream, samples, sizeof (samples));

		SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 0, 240, 32, 255);
		SDL_RenderDebugText(renderer, 8, 8, "sound_sdl3");
		if (alt_pressed) {
			SDL_RenderDebugText(renderer, 8, 16, "alt pressed");
		}
		SDL_RenderPresent(renderer);
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}
