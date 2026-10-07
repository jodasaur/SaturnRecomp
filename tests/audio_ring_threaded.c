#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "saturn.h"
#include <assert.h>

static saturn system_state;
static SDL_atomic_t finished;

static int produce(void *unused)
{
    (void)unused;
    for (unsigned i = 0; i < 2000; i++) {
        sound_run(&system_state, MASTER_CLOCK_NTSC / 100u);
        if (i % 31u == 0) sound_clock_change_reset(&system_state);
        if (i % 8u == 0) SDL_Delay(0);
    }
    SDL_AtomicSet(&finished, 1);
    return 0;
}

int main(void)
{
    int16_t samples[512];
    saturn_init(&system_state);
    SDL_Thread *producer = SDL_CreateThread(produce, "ring producer", NULL);
    assert(producer);
    do {
        unsigned count = sound_drain(&system_state, samples, 256);
        for (unsigned i = 0; i < count * 2u; i++) assert(samples[i] == 0);
        if (!count) SDL_Delay(0);
    } while (!SDL_AtomicGet(&finished) || sound_queued(&system_state));
    SDL_WaitThread(producer, NULL);
    assert(sound_queued(&system_state) == 0);
    assert(atomic_load(&system_state.snd_wp) > SATURN_AUDIO_RING_FRAMES * 2u);
    puts("audio_ring_threaded: concurrent production, draining, wrap and reset passed");
    return 0;
}
