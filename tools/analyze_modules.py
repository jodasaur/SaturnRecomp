#!/usr/bin/env python3
"""Extract candidate modules through SaturnRecomp, preserving evidence separately."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys


def run(cli, *arguments):
    result = subprocess.run([str(cli), *map(str, arguments)],
                            capture_output=True, text=True)
    if result.returncode:
        raise ValueError(result.stderr.strip() or result.stdout.strip()
                         or f"CLI exited {result.returncode}")
    return result.stdout


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def analyze(cli, disc, output, first_read, suffixes):
    inspection = run(cli, "inspect", disc)
    files = []
    for line in inspection.splitlines():
        match = re.fullmatch(r"FILE\s+(\d+)\s+(\d+)\s+(/.*)", line)
        if match:
            path = match[3]
            # Diagnostic annotations are not part of the ISO path.
            path = re.split(r"\s+\[(?:UNREADABLE|CD-DA track, raw 2352)\]$", path)[0]
            files.append({"iso_path": path, "lba": int(match[1]),
                          "size": int(match[2])})
    address = re.search(r"1st-read addr\s+0x([0-9A-Fa-f]+)", inspection)
    if not address or first_read not in {entry["iso_path"] for entry in files}:
        raise ValueError("Saturn first-read header or selected first-read file is missing")
    load = int(address[1], 16)
    if not load:
        raise ValueError("IP.BIN has a zero first-read address; investigate before disassembly")
    candidates = [entry for entry in files if entry["iso_path"] == first_read
                  or PurePosixPath(entry["iso_path"]).suffix.upper() in suffixes]
    # A fresh directory prevents silently mixing evidence from different dumps.
    output.mkdir(parents=True, exist_ok=False)
    (output / "inspection.txt").write_text(inspection, encoding="utf-8")
    binaries = output / "modules"
    binaries.mkdir()
    for index, entry in enumerate(candidates):
        # Numbered names avoid case collisions and unsafe ISO path components.
        name = f"{index:03d}.bin"
        destination = binaries / name
        run(cli, "extract", disc, entry["iso_path"], destination)
        if destination.stat().st_size != entry["size"]:
            raise ValueError(f"Extracted size differs from ISO record: {entry['iso_path']}")
        initial = entry["iso_path"] == first_read
        entry.update(extracted_file=f"modules/{name}", sha256=sha256(destination),
                     role="first-read" if initial else "candidate",
                     load_addr=load if initial else None,
                     entry=None, cpu=None, compression=None)
        # This is a linear preview, not a claim that every word is an instruction.
        if initial:
            preview = run(cli, "disasm", destination, hex(load), "128")
            (output / "first-read-preview.txt").write_text(preview, encoding="utf-8")
    report = {"schema_version": 1, "disc": str(disc),
              "cli_sha256": sha256(cli), "first_read": first_read,
              "selection_suffixes": sorted(suffixes), "files": files,
              "modules": candidates,
              "limitations": ["Suffixes identify candidates, not proven executable modules.",
                              "Only the first-read load address is read from IP.BIN.",
                              "CPU, compression, entry points and overlay addresses need evidence.",
                              "The disassembly preview may include data."]}
    (output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n",
                                          encoding="utf-8")
    return len(candidates)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("disc", type=Path)
    parser.add_argument("--cli", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True,
                        help="New local directory for extracted data and reports")
    parser.add_argument("--first-read", required=True, help="Confirmed ISO path")
    parser.add_argument("--suffix", action="append",
                        help="Candidate suffix; repeat as needed (default: .PRG, .TSK)")
    args = parser.parse_args()
    try:
        count = analyze(args.cli.resolve(strict=True), args.disc.resolve(strict=True),
                        args.out.resolve(), args.first_read,
                        {s.upper() for s in (args.suffix or [".PRG", ".TSK"])})
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    print(f"Extracted {count} candidates. Report: {args.out / 'manifest.json'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
