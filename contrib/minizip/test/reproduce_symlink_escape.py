"""Reproduce miniunz writes through existing symlinks inside a temporary tree.

By default this is a regression test: escapes must be blocked and ordinary
extraction must work. Use --expect-vulnerable to reproduce the original bug.
All affected files are confined to a temporary tree.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
import zipfile


def create_link(link, target, directory, link_type):
    if link_type == "symlink":
        link.symlink_to(target, target_is_directory=directory)
    else:
        # mklink is a cmd.exe builtin. Both paths come from our temporary tree.
        result = subprocess.run(
            f'cmd.exe /d /c mklink /J "{link}" "{target}"',
            capture_output=True, text=True, errors="replace", check=False,
        )
        if result.returncode != 0:
            raise OSError(f"junction setup failed: {result.stdout} {result.stderr}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("miniunz", type=Path)
    parser.add_argument("--expect-vulnerable", action="store_true")
    parser.add_argument("--link-type", choices=("symlink", "junction"),
                        default="symlink")
    parser.add_argument("--json", type=Path, help="write case results as JSON")
    args = parser.parse_args()
    if args.link_type == "junction" and os.name != "nt":
        parser.error("junction tests require native Windows Python")
    executable = args.miniunz.resolve(strict=True)
    passed = []
    records = []
    unavailable = False
    kinds = ["file", "directory", "new-directory", "directory-entry",
             "regular", "overwrite"]
    if args.link_type == "junction":
        kinds.remove("file")  # Junctions are directory links only.
    with tempfile.TemporaryDirectory(prefix="miniunz-review-") as temp:
        root = Path(temp)
        for kind in kinds:
            case = root / kind
            dest = case / "extract"
            outside = case / "outside"
            dest.mkdir(parents=True)
            outside.mkdir()
            victim = outside / "marker.txt"
            victim.write_bytes(b"original")
            link = None
            if kind == "file":
                link = dest / "marker.txt"
                target = victim
                member = "marker.txt"
            elif kind in ("directory", "new-directory", "directory-entry"):
                link = dest / "link"
                target = outside
                member = {"directory": "link/marker.txt",
                          "new-directory": "link/new/marker.txt",
                          "directory-entry": "link/new/"}[kind]
            else:
                member = "nested/marker.txt"
                if kind == "overwrite":
                    (dest / "nested").mkdir()
                    (dest / member).write_bytes(b"previous")
            if link is not None:
                try:
                    create_link(link, target, kind != "file", args.link_type)
                except OSError as error:
                    unavailable = True
                    print(f"{kind}: UNAVAILABLE (link setup: {error})")
                    records.append({"case": kind, "status": "unavailable",
                                    "error": str(error)})
                    continue
            archive = case / "input.zip"
            with zipfile.ZipFile(archive, "w") as out:
                info = zipfile.ZipInfo(member, date_time=(2000, 1, 2, 3, 4, 6))
                out.writestr(info, b"archive-controlled")
            command = [str(executable), "-o", str(archive)]
            try:
                result = subprocess.run(
                    command, cwd=dest, capture_output=True, text=True,
                    errors="replace", timeout=10, check=False,
                )
            finally:
                # Remove junctions explicitly so cleanup never traverses them.
                if link is not None:
                    if args.link_type == "junction":
                        link.rmdir()
                    else:
                        link.unlink()
            escaped = (victim.read_bytes() != b"original" or
                       (outside / "new").exists())
            if kind in ("regular", "overwrite"):
                ok = (not escaped and result.returncode == 0 and
                      (dest / member).read_bytes() == b"archive-controlled" and
                      int((dest / member).stat().st_mtime) ==
                      int(time.mktime((2000, 1, 2, 3, 4, 6, 0, 0, -1))))
            elif args.expect_vulnerable:
                ok = escaped
            else:
                ok = not escaped and result.returncode != 0
            print(f"{kind}: {'PASS' if ok else 'FAIL'}"
                  f" (escaped={escaped}, miniunz exit={result.returncode})")
            if not ok:
                print(result.stdout, result.stderr)
            passed.append(ok)
            records.append({"case": kind, "status": "pass" if ok else "fail",
                            "escaped": escaped, "command": command,
                            "returncode": result.returncode,
                            "stdout": result.stdout, "stderr": result.stderr})
    # A permission problem is an incomplete experiment, never a successful test.
    code = 1 if not all(passed) else (2 if unavailable else 0)
    if args.json:
        args.json.write_text(json.dumps({
            "executable": str(executable), "platform": os.name,
            "link_type": args.link_type,
            "expect_vulnerable": args.expect_vulnerable,
            "exit_code": code, "cases": records,
        }, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")
    return code


if __name__ == "__main__":
    raise SystemExit(main())
