#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path


def minify_html(content: str) -> str:
    content = re.sub(r"<!--.*?-->", "", content, flags=re.S)
    content = re.sub(r">\s+<", "><", content)
    content = re.sub(r"\s+", " ", content).strip()
    return content


def minify_css(content: str) -> str:
    content = re.sub(r"/\*.*?\*/", "", content, flags=re.S)
    content = re.sub(r"\s+", " ", content)
    content = re.sub(r"\s*([{}:;,])\s*", r"\1", content)
    return content.strip()


def minify_js(content: str) -> str:
    lines = []
    for line in content.splitlines():
        stripped = line.strip()
        if stripped.startswith("//") or not stripped:
            continue
        lines.append(stripped)
    content = " ".join(lines)
    content = re.sub(r"/\*.*?\*/", "", content, flags=re.S)
    content = re.sub(r"\s+", " ", content)
    content = re.sub(r"\s*([{}();,:=])\s*", r"\1", content)
    return content.strip()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--src", required=True)
    parser.add_argument("--dst", required=True)
    args = parser.parse_args()

    src = Path(args.src)
    dst = Path(args.dst)
    dst.mkdir(parents=True, exist_ok=True)

    html = (src / "index.html").read_text(encoding="utf-8")
    css = (src / "styles.css").read_text(encoding="utf-8")
    js = (src / "app.js").read_text(encoding="utf-8")

    (dst / "index.html").write_text(minify_html(html), encoding="utf-8")
    (dst / "styles.css").write_text(minify_css(css), encoding="utf-8")
    (dst / "app.js").write_text(minify_js(js), encoding="utf-8")


if __name__ == "__main__":
    main()
