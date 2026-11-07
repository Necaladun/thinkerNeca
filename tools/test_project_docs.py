#!/usr/bin/env python3
"""Check maintainer docs/navigation after the thinkerNeca documentation split.

Only local Markdown link paths and required files are validated. This does not
validate external URLs, semantics or rendered GitHub anchors.
"""
from pathlib import Path
import re
from urllib.parse import unquote


REQUIRED = [
    "Readme.md", "Details.md", "Technical.md", "Changelog.md",
    "Fork-Changelog.md", "PROJECT.md",
    "project/STATUS.md", "project/WORK-QUEUE.md",
    "project/DECISIONS.md", "project/IDEAS.md", "project/TESTING.md",
    "project/technical/minor-terrain.md",
    "project/technical/rendering.md",
    "project/technical/sprite-atlas.md",
    "project/technical/map-generation.md",
    "project/technical/configuration.md",
]
OBSOLETE = [
    "docs/minor-terrain-yields.md",
    "docs/minor-terrain-graphics.md",
]


def main():
    for path in REQUIRED:
        assert Path(path).is_file(), f"Missing required project document: {path}"
    for path in OBSOLETE:
        assert not Path(path).exists(), f"Old shipped document not migrated: {path}"

    # The historical upstream Details/Technical/Changelog are not reformatted;
    # their existing external links are left untouched.
    errors = []
    checked = 0
    for source in [Path("PROJECT.md"), Path("Readme.md"),
                   Path("Fork-Changelog.md"), *sorted(Path("project").rglob("*.md"))]:
        text = source.read_text(encoding="utf-8")
        for match in re.finditer(r"(?<!!)\[[^\]]+\]\(([^)]+)\)", text):
            target = match.group(1).strip().split(" ", 1)[0].strip("<>")
            if not target or target.startswith(("#", "/", "mailto:")):
                continue
            if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target):
                continue
            filepart = unquote(target.split("#", 1)[0].split("?", 1)[0])
            if not filepart:
                continue
            checked += 1
            targetpath = source.parent / filepart
            if not targetpath.exists():
                line = text[:match.start()].count("\n") + 1
                errors.append(f"{source}:{line} -> {target} (missing)")

    assert not errors, "Broken local project doc links:\n" + "\n".join(errors)
    print(f"Project docs contract passed: {len(REQUIRED)} files, {checked} local links")


if __name__ == "__main__":
    main()
