/* API placeholders for builds that retain only the SDL software frontend. */
#include "vulkan_renderer.h"
#include <stdio.h>

static void unavailable(char *error, size_t size)
{
    if (error && size) snprintf(error, size, "Vulkan is disabled in this build");
}

saturn_vk_renderer *saturn_vk_create(SDL_Window *window, saturn *s,
                                     char *error, size_t error_size)
{
    (void)window;
    (void)s;
    unavailable(error, error_size);
    return NULL;
}

int  saturn_vk_render(saturn_vk_renderer *r, saturn *s, int width, int height,
                      char *error, size_t error_size)
{
    (void)r;
    (void)s;
    (void)width;
    (void)height;
    unavailable(error, error_size);
    return 0;
}

int  saturn_vk_present(saturn_vk_renderer *r, char *error, size_t error_size)
{
    (void)r;
    unavailable(error, error_size);
    return 0;
}

int saturn_vk_readback(saturn_vk_renderer *r, uint32_t *pixels, int width, int height,
                       char *error, size_t error_size)
{
    (void)r;
    (void)pixels;
    (void)width;
    (void)height;
    unavailable(error, error_size);
    return 0;
}

int saturn_vk_readback_presented(saturn_vk_renderer *r,uint32_t *pixels,int width,int height,
                               char *error,size_t error_size)
{
    (void)r;
    (void)pixels;
    (void)width;
    (void)height;
    unavailable(error, error_size);
    return 0;
}

int saturn_vk_replay_geometry(saturn_vk_renderer *r, saturn *s,
    const saturn_vk_vdp1_op *ops,unsigned count,int w,int h,char *error,size_t error_size)
{
    (void)r;
    (void)s;
    (void)ops;
    (void)count;
    (void)w;
    (void)h;
    unavailable(error, error_size);
    return 0;
}

int saturn_vk_interpolation_enable(saturn_vk_renderer *r)
{
    (void)r;
    return 0;
}

void saturn_vk_interpolation_disable(saturn_vk_renderer *r)
{
    (void)r;
}

int saturn_vk_set_quality(saturn_vk_renderer *r,const saturn_runtime_settings *settings,char *error,size_t size)
{
    (void)r;
    (void)settings;
    unavailable(error, size);
    return 0;
}

int saturn_vk_presentation_active(const saturn_vk_renderer *r)
{
    (void)r;
    return 0;
}

int saturn_vk_attach_overlay(saturn_vk_renderer *r,game_overlay *overlay,char *error,size_t size)
{
    (void)r;
    (void)overlay;
    unavailable(error, size);
    return 0;
}

void saturn_vk_overlay_draw(saturn_vk_renderer *r,const game_overlay_draw_data *draw)
{
    (void)r;
    (void)draw;
}

void saturn_vk_interpolation_stats(const saturn_vk_renderer *r,unsigned *matched,unsigned *span)
{
    (void)r;
    if (matched) *matched = 0;
    if (span) *span = 0;
}

int saturn_vk_interpolation_begin(saturn_vk_renderer *r,saturn *s,int w,int h,char *error,size_t size)
{
    (void)r;
    (void)s;
    (void)w;
    (void)h;
    unavailable(error, size);
    return 0;
}

int saturn_vk_interpolation_render(saturn_vk_renderer *r,saturn *s,float alpha,int w,int h,char *error,size_t size)
{
    (void)r;
    (void)s;
    (void)alpha;
    (void)w;
    (void)h;
    unavailable(error, size);
    return 0;
}

void saturn_vk_destroy(saturn_vk_renderer *r)
{
    (void)r;
}

const char *saturn_vk_device_name(const saturn_vk_renderer *r)
{
    (void)r;
    return "SDL software renderer";
}

