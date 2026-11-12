#!/usr/bin/env python3
"""Fetch the Third Strike Upper (3s') design spec and store it as plain text.

Source: https://mikezsez.blogspot.com/2015/12/what-id-change-for-third-strike-upper-3s.html

The blog post is the normative design document for the 3s' balance patch. This
script extracts the post body so the spec can live in the repository and be
diffed as it changes.

Usage:
    python3 tools/fetch_design_spec.py [--check]

    --check   Do not write; exit non-zero if the stored spec differs from the
              upstream post (useful in CI to detect upstream edits).
"""

import argparse
import html
import re
import sys
import urllib.request
from pathlib import Path

SPEC_URL = "https://mikezsez.blogspot.com/2015/12/what-id-change-for-third-strike-upper-3s.html"
POST_BODY_ID = "post-body-8824080525238959540"
OUTPUT_PATH = Path("docs/upper3s/design-spec.md")

HEADER = """# Third Strike Upper (3s') - Design Specification

> Source: <{url}>
>
> Retrieved by `tools/fetch_design_spec.py`. This is the normative design
> document for the 3s' balance patch; the implementation status of each item
> is tracked in `docs/upper3s/spec-map.md`.
>
> **Note on numbers:** where this document disagrees with the decompiled retail
> data, the *intent* of the change governs. Concrete values are re-derived from
> the retail tables rather than transcribed, and the reasoning is recorded in
> `docs/upper3s/spec-map.md`.

---

"""


def fetch(url: str) -> str:
    request = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0 (3s-decomp spec fetcher)"})
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.read().decode("utf-8", errors="replace")


def extract_body(page: str) -> str:
    """Pull the post body out of the Blogger HTML."""
    match = re.search(
        r'<div class=[\'"]post-body entry-content[\'"] id=[\'"]%s[\'"][^>]*>(.*?)<div class=[\'"]post-footer[\'"]>' % POST_BODY_ID,
        page,
        flags=re.S,
    )
    if match is None:
        # Fall back to a looser match if the surrounding markup shifted.
        match = re.search(r'id=[\'"]%s[\'"][^>]*>(.*?)<div class=[\'"]post-footer[\'"]>' % POST_BODY_ID, page, flags=re.S)
    if match is None:
        raise SystemExit("could not locate the post body in the fetched page")
    return match.group(1)


def to_text(fragment: str) -> str:
    """Convert the post body HTML into readable plain text."""
    text = fragment
    text = re.sub(r"<br\s*/?>", "\n", text, flags=re.I)
    text = re.sub(r"</li>", "\n", text, flags=re.I)
    text = re.sub(r"<li[^>]*>", "- ", text, flags=re.I)
    text = re.sub(r"</(p|div|h3|h4|ul|ol|blockquote)>", "\n", text, flags=re.I)
    text = re.sub(r"<h3[^>]*>", "\n### ", text, flags=re.I)
    text = re.sub(r"<h4[^>]*>", "\n#### ", text, flags=re.I)
    text = re.sub(r"<b>(.*?)</b>", r"**\1**", text, flags=re.S | re.I)
    text = re.sub(r"<i>(.*?)</i>", r"_\1_", text, flags=re.S | re.I)
    text = re.sub(r"<[^>]+>", "", text)
    text = html.unescape(text)
    text = text.replace("\xa0", " ")
    text = re.sub(r"[ \t]+\n", "\n", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    body = to_text(extract_body(fetch(SPEC_URL)))
    document = HEADER.format(url=SPEC_URL) + body + "\n"

    if args.check:
        if not OUTPUT_PATH.exists():
            print(f"{OUTPUT_PATH} is missing", file=sys.stderr)
            return 1
        if OUTPUT_PATH.read_text(encoding="utf-8") != document:
            print(f"{OUTPUT_PATH} differs from the upstream spec", file=sys.stderr)
            return 1
        print(f"{OUTPUT_PATH} is up to date")
        return 0

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(document, encoding="utf-8")
    print(f"wrote {OUTPUT_PATH} ({len(document)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
