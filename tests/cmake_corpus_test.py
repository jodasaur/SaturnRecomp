"""Match run_all.ps1's optional game module checks without requiring media."""
from pathlib import Path
import re
import subprocess
import sys


def main():
    config = Path(sys.argv[2])
    match = re.search(r'^\s*disc\s*=\s*"([^"]+)"', config.read_text(encoding="utf-8-sig"), re.MULTILINE)
    if not match:
        print("SKIP: no disc declared")
        return 77
    disc = Path(match[1])
    if not disc.is_absolute():
        disc = config.parent / disc
    if not disc.is_file():
        print("SKIP: disc image not present")
        return 77
    return subprocess.run([sys.argv[1], "modules", str(config)]).returncode


if __name__ == "__main__":
    sys.exit(main())
