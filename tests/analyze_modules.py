"""Exercise extraction and evidence reporting using the real CLI and a synthetic ISO."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from cmake_frontend_test import directory_record


def main():
    cli = Path(sys.argv[1]).resolve()
    tool = Path(__file__).resolve().parents[1] / "tools/analyze_modules.py"
    with tempfile.TemporaryDirectory(prefix="saturn-analysis-") as temporary:
        root = Path(temporary)
        image = bytearray(25 * 2048)
        image[:15] = b"SEGA SEGASATURN"
        struct.pack_into(">I", image, 0xF0, 0x06004000)
        pvd = 16 * 2048
        image[pvd:pvd + 7] = b"\x01CD001\x01"
        struct.pack_into("<I", image, pvd + 80, 25)
        struct.pack_into(">I", image, pvd + 84, 25)
        directory = directory_record(b"\x00", 20, 2048, 2)
        image[pvd + 156:pvd + 156 + len(directory)] = directory
        records = (directory + directory_record(b"\x01", 20, 2048, 2)
                   + directory_record(b"BOOT.PRG;1", 21, 4)
                   + directory_record(b"NEXT.PRG;1", 22, 3)
                   + directory_record(b"DATA.DAT;1", 23, 4))
        image[20 * 2048:20 * 2048 + len(records)] = records
        code = bytes.fromhex("affe0009")
        image[21 * 2048:21 * 2048 + 4] = code
        image[22 * 2048:22 * 2048 + 3] = b"abc"
        image[23 * 2048:23 * 2048 + 4] = b"data"
        disc = root / "disc with spaces.iso"
        disc.write_bytes(image)
        output = root / "report"
        command = [sys.executable, str(tool), str(disc), "--cli", str(cli),
                   "--out", str(output), "--first-read", "/BOOT.PRG"]
        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr
        manifest = json.loads((output / "manifest.json").read_text())
        assert len(manifest["files"]) == 3
        assert len(manifest["modules"]) == 2
        boot, overlay = manifest["modules"]
        assert boot["sha256"] == hashlib.sha256(code).hexdigest()
        assert boot["load_addr"] == 0x06004000
        assert (output / boot["extracted_file"]).read_bytes() == code
        assert (output / overlay["extracted_file"]).read_bytes() == b"abc"
        assert overlay["load_addr"] is None and overlay["entry"] is None
        assert "06004000" in (output / "first-read-preview.txt").read_text()
        original = (output / "manifest.json").read_bytes()
        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode != 0
        assert (output / "manifest.json").read_bytes() == original
        command[command.index(str(output))] = str(root / "invalid")
        command[-1] = "/ABSENT.PRG"
        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode != 0
        assert not (root / "invalid").exists()
    print("Module analysis: extraction, hashes, unknown overlays and overwrite checks passed")


if __name__ == "__main__":
    main()
