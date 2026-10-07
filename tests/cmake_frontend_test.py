"""Run the real SDL software frontend on a generated, BIOS-free SH-2 loop."""
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile


def directory_record(name, lba, size, flags=0):
    record = bytearray(33 + len(name) + (len(name) % 2 == 0))
    record[0] = len(record)
    struct.pack_into("<I", record, 2, lba)
    struct.pack_into(">I", record, 6, lba)
    struct.pack_into("<I", record, 10, size)
    struct.pack_into(">I", record, 14, size)
    record[25] = flags
    record[28:32] = b"\x01\x00\x00\x01"
    record[32] = len(name)
    record[33:33 + len(name)] = name
    return record


def main():
    executable = str(Path(sys.argv[1]).resolve())
    software_only = "--software-only" in sys.argv[2:]
    with tempfile.TemporaryDirectory(prefix="saturn-sdl-test-") as temporary:
        root = Path(temporary)
        image = bytearray(24 * 2048)
        image[:15] = b"SEGA SEGASATURN"
        struct.pack_into(">I", image, 0xE8, 0x06100000)
        struct.pack_into(">I", image, 0xF0, 0x06004000)
        pvd = 16 * 2048
        image[pvd:pvd + 7] = b"\x01CD001\x01"
        struct.pack_into("<I", image, pvd + 80, 24)
        struct.pack_into(">I", image, pvd + 84, 24)
        root_record = directory_record(b"\x00", 20, 2048, 2)
        image[pvd + 156:pvd + 156 + len(root_record)] = root_record
        entries = (root_record + directory_record(b"\x01", 20, 2048, 2)
                   + directory_record(b"TEST.BIN;1", 21, 4))
        image[20 * 2048:20 * 2048 + len(entries)] = entries
        # BRA to self, with a NOP delay slot. No firmware or game code.
        image[21 * 2048:21 * 2048 + 4] = bytes.fromhex("affe0009")
        (root / "test.iso").write_bytes(image)
        config = root / "game.toml"
        config.write_text('[game]\nname="Synthetic SDL test"\ndisc="test.iso"\n'
                          '[[module]]\nname="test"\nfile="/TEST.BIN"\n'
                          'first_read=true\nload_addr=0x06004000\n')
        environment = {key: value for key, value in os.environ.items()
                       if not key.startswith("SATURN_")}
        environment.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                           SATURN_IMASK="15", SATURN_PAD="0", SATURN_UNCAP="1",
                           SATURN_MAX_FRAMES="3",
                           SATURN_SETTINGS_FILE=str(root / "settings.ini"))
        if not software_only:
            environment["SATURN_RENDERER"] = "software"
        result = subprocess.run([executable, str(config)], env=environment,
                                cwd=root, capture_output=True, text=True, timeout=20)
        assert result.returncode == 0, result.stdout + result.stderr
        assert "[runtime] fields 3 " in result.stderr, result.stderr
        assert "HALTED" not in result.stdout, result.stdout
        if software_only:
            environment["SATURN_RENDERER"] = "vulkan"
            result = subprocess.run([executable, str(config)], env=environment,
                                    cwd=root, capture_output=True, text=True, timeout=20)
            assert result.returncode != 0, result.stdout + result.stderr
            assert "Vulkan is disabled" in result.stderr, result.stderr
    print("SDL software frontend: three synthetic fields passed")


if __name__ == "__main__":
    main()
