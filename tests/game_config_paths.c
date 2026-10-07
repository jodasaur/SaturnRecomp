#include "game_config.h"
#include "disc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check(const char *config_path, const char *disc, const char *bios,
                  const char *expected_disc, const char *expected_bios)
{
    static game_config config;
    FILE *file = fopen(config_path, "w");
    assert(file);
    fprintf(file, "[game]\nname = \"Path test\"\ndisc = \"%s\"\nbios = \"%s\"\n", disc, bios);
    assert(fclose(file) == 0);
    assert(gc_load(&config, config_path) == 0);
    assert(strcmp(config.disc, expected_disc) == 0);
    assert(strcmp(config.bios, expected_bios) == 0);
    assert(remove(config_path) == 0);
}

static void check_cue(const char *cue_path, const char *bin_path)
{
    static disc image;
    unsigned char sector[2048] = {0}, actual[2048];
    FILE *file = fopen(bin_path, "wb");
    assert(file);
    sector[0] = 0x5a;
    assert(fwrite(sector, 1, sizeof sector, file) == sizeof sector);
    assert(fclose(file) == 0);
    file = fopen(cue_path, "w");
    assert(file);
    fputs("FILE \"path_test.bin\" BINARY\n  TRACK 01 MODE1/2048\n    INDEX 01 00:00:00\n", file);
    assert(fclose(file) == 0);
    assert(disc_open(&image, cue_path) == 0);
    assert(disc_read_sector(&image, 0, actual, NULL) == 0);
    assert(memcmp(actual, sector, sizeof sector) == 0);
    disc_close(&image);
    assert(remove(cue_path) == 0);
    assert(remove(bin_path) == 0);
}

int main(void)
{
    check("game_config_paths.toml", "disc.bin", "bios.bin", "disc.bin", "bios.bin");
    check("out/game_config_paths.toml", "../disc.bin", "bios.bin",
          "out/../disc.bin", "out/bios.bin");
    check("out\\game_config_paths.toml", "disc.bin", "bios.bin",
          "out\\disc.bin", "out\\bios.bin");
    check("out/game_config_paths.toml", "/disc.bin", "C:\\bios.bin",
          "/disc.bin", "C:\\bios.bin");
    check("out/game_config_paths.toml", "\\\\server\\disc.bin", "",
          "\\\\server\\disc.bin", "out/");
    check_cue("path_test.cue", "path_test.bin");
    check_cue("out/path_test.cue", "out/path_test.bin");
    check_cue("out\\path_test.cue", "out\\path_test.bin");
    puts("game_config_paths: configuration and CUE paths passed");
    return 0;
}
