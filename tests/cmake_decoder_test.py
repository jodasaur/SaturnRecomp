"""Run the existing decoder oracle in an isolated directory for CTest."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    try:
        from capstone import CS_ARCH_SH, CS_MODE_SH2  # noqa: F401
    except ImportError:
        print("SKIP: install Python Capstone with SH-2 support")
        return 77
    source = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix="saturn-decoder-") as directory:
        root = Path(directory)
        (root / "tests/oracle").mkdir(parents=True)
        shutil.copy(source / "oracle/capstone_lenient.txt", root / "tests/oracle")
        commands = [
            [sys.executable, str(source / "oracle/capstone_sh2_dump.py"), "tests/oracle_sh2.txt"],
            [sys.argv[1], "tests/ours_sh2.txt"],
            [sys.executable, str(source / "oracle/compare.py")],
        ]
        for command in commands:
            result = subprocess.run(command, cwd=root)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == "__main__":
    sys.exit(main())
