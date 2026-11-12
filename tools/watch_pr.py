#!/usr/bin/env python3
"""Block until something new happens on a GitHub pull request.

The watcher is the agent's alarm clock for the pull request. It never asks the
agent to look: one process sleeps inside itself and *exits* the moment an
event appears, so the completion of the background task is what wakes the
agent up. Nothing in the shell polls.

Events that wake the watcher:

  comment  a new conversation comment from a watched author (default: the
           repository owner). The agent's own progress reports carry the
           ``<!-- mavis:progress -->`` marker and never wake it.
  review   a new review, or new inline review comments, on the pull request.
  ci       a check run that started or reached a conclusion, or a commit
           status that changed.
  commit   the head of the pull request moved to a new commit.

Usage:

    python tools/watch_pr.py --repo trafgals/3s-decomp --pr 1
    python tools/watch_pr.py --repo trafgals/3s-decomp --pr 1 --once
    python tools/watch_pr.py --self-test

Exit codes: 0 an event was found, 3 the wait timed out with nothing to
report, 1 the watcher could not reach the API.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

API = "https://api.github.com"
PROGRESS_MARKER = "<!-- mavis:progress -->"
USER_AGENT = "3s-decomp-pr-watch"


# --------------------------------------------------------------------------
# GitHub access
# --------------------------------------------------------------------------
class GitHubError(RuntimeError):
    pass


def gh_token() -> str:
    """The token, from the environment or from the GitHub CLI."""
    for name in ("GH_TOKEN", "GITHUB_TOKEN"):
        value = os.environ.get(name)
        if value and value.strip():
            return value.strip()
    try:
        done = subprocess.run(
            ["gh", "auth", "token"], capture_output=True, text=True, timeout=60
        )
    except (OSError, subprocess.SubprocessError) as err:
        raise GitHubError(f"no token in the environment and gh failed: {err}")
    if done.returncode == 0 and done.stdout.strip():
        return done.stdout.strip()
    raise GitHubError("no token in the environment and gh auth token failed")


class GitHub:
    """A very small REST client that keeps the ETags it was given."""

    def __init__(self, token: str):
        self.token = token

    def _request(self, path: str, etag: str | None = None):
        request = urllib.request.Request(
            API + path,
            headers={
                "Authorization": f"Bearer {self.token}",
                "Accept": "application/vnd.github+json",
                "X-GitHub-Api-Version": "2022-11-28",
                "User-Agent": USER_AGENT,
            },
        )
        if etag:
            request.add_header("If-None-Match", etag)
        try:
            with urllib.request.urlopen(request, timeout=60) as response:
                body = response.read().decode("utf-8", "replace")
                return response.status, body, response.headers.get("ETag")
        except urllib.error.HTTPError as err:
            if err.code == 304:
                return 304, "", etag
            if err.code in (401, 403):
                raise GitHubError(f"{path}: GitHub said {err.code} {err.reason}")
            raise GitHubError(f"{path}: GitHub said {err.code} {err.reason}")
        except urllib.error.URLError as err:
            raise GitHubError(f"{path}: {err.reason}")
        except OSError as err:
            # a socket that was reset, a timeout of the read, a name that
            # did not resolve: the watcher has to survive them
            raise GitHubError(f"{path}: {err}")

    def get(self, path: str, etag: str | None = None):
        """One request. Returns (status, parsed body, etag)."""
        status, body, new_etag = self._request(path, etag)
        if status == 304:
            return 304, None, new_etag
        return status, json.loads(body) if body else None, new_etag

    def get_object_list(self, path: str, key: str) -> list:
        """A list that GitHub wraps in an object, like check-runs."""
        status, body, _ = self.get(path)
        if status == 304 or not body:
            return []
        if isinstance(body, dict):
            return body.get(key) or []
        return body

    def get_all(self, path: str, etag: str | None = None):
        """Every page of a list endpoint. Returns (items, etag).

        The ETag belongs to one URL, so it is only offered on the first
        page; the later pages are asked for without it.
        """
        items: list = []
        last_etag = etag
        page = 1
        while True:
            if page == 1:
                request_path = path
                etag_used = etag
            else:
                separator = "&" if "?" in path else "?"
                request_path = f"{path}{separator}page={page}"
                etag_used = None
            status, raw, new_etag = self._request(request_path, etag_used)
            if status == 304:
                return items, last_etag
            last_etag = new_etag
            try:
                body = json.loads(raw) if raw else None
            except json.JSONDecodeError as err:
                raise GitHubError(f"{request_path}: the answer was not JSON ({err})")
            if isinstance(body, list):
                items.extend(body)
                # A short page is the last one.
                if len(body) < 100:
                    break
            elif body is not None:
                items.append(body)
                break
            page += 1
        return items, last_etag


# --------------------------------------------------------------------------
# The state machine
# --------------------------------------------------------------------------
class Watcher:
    """Turns a snapshot of the pull request into a list of events."""

    def __init__(self, comment_authors: list[str], review_authors: list[str]):
        self.comment_authors = [a.lower() for a in comment_authors]
        self.review_authors = [a.lower() for a in review_authors]
        self.seen_comments: set[int] = set()
        self.seen_reviews: set[int] = set()
        self.seen_review_comments: set[int] = set()
        self.seen_check_runs: set[int] = set()
        self.seen_statuses: set[str] = set()
        self.head_sha: str | None = None
        self.primed = False

    # -- helpers ---------------------------------------------------------
    @staticmethod
    def _text(item: dict) -> str:
        return (item.get("body") or item.get("output") or "").strip()

    @staticmethod
    def _headline(text: str, limit: int = 110) -> str:
        first = text.splitlines()[0] if text else ""
        return first[:limit]

    def _watched_comment(self, comment: dict) -> bool:
        login = (comment.get("user") or {}).get("login", "")
        if login.lower() not in self.comment_authors:
            return False
        if PROGRESS_MARKER in (comment.get("body") or ""):
            return False
        return True

    def _watched_review(self, review: dict) -> bool:
        if self.review_authors and "any" not in self.review_authors:
            login = (review.get("user") or {}).get("login", "")
            return login.lower() in self.review_authors
        return True

    # -- the scan --------------------------------------------------------
    def _prime(self, snapshot: dict) -> None:
        """The first scan is the baseline: everything that exists is old."""
        for comment in snapshot.get("comments", []):
            self.seen_comments.add(comment.get("id"))
        for review in snapshot.get("reviews", []):
            self.seen_reviews.add(review.get("id"))
        for comment in snapshot.get("review_comments", []):
            self.seen_review_comments.add(comment.get("id"))
        for run in snapshot.get("check_runs", []):
            self.seen_check_runs.add(
                (run.get("id"), run.get("status"), run.get("conclusion"))
            )
        for status in snapshot.get("statuses", []):
            self.seen_statuses.add(f"{status.get('id')}:{status.get('state')}")
        self.primed = True

    def scan(self, snapshot: dict) -> list[dict]:
        """Compare the snapshot with what was seen and return the events."""
        events: list[dict] = []

        head = snapshot.get("head_sha")
        if self.head_sha is None:
            self.head_sha = head
        elif head and head != self.head_sha:
            events.append(
                {
                    "kind": "commit",
                    "title": f"the head of the pull request moved to {head[:7]}",
                    "items": [
                        {
                            "who": "git",
                            "text": f"{self.head_sha[:7]} -> {head[:7]}",
                        }
                    ],
                }
            )
            self.head_sha = head

        if not self.primed:
            self._prime(snapshot)
            return []

        for comment in snapshot.get("comments", []):
            cid = comment.get("id")
            if cid in self.seen_comments:
                continue
            self.seen_comments.add(cid)
            if not self._watched_comment(comment):
                continue
            who = (comment.get("user") or {}).get("login", "?")
            events.append(
                {
                    "kind": "comment",
                    "title": f"a new comment from {who}",
                    "items": [
                        {
                            "who": who,
                            "text": self._text(comment),
                            "created_at": comment.get("created_at"),
                            "id": cid,
                        }
                    ],
                }
            )

        for review in snapshot.get("reviews", []):
            rid = review.get("id")
            if rid in self.seen_reviews:
                continue
            self.seen_reviews.add(rid)
            if not self._watched_review(review):
                continue
            who = (review.get("user") or {}).get("login", "?")
            events.append(
                {
                    "kind": "review",
                    "title": f"a new review from {who} ({review.get('state')})",
                    "items": [
                        {
                            "who": who,
                            "text": self._text(review),
                            "created_at": review.get("submitted_at"),
                            "id": rid,
                            "commit_id": review.get("commit_id"),
                        }
                    ],
                }
            )

        for comment in snapshot.get("review_comments", []):
            cid = comment.get("id")
            if cid in self.seen_review_comments:
                continue
            self.seen_review_comments.add(cid)
            who = (comment.get("user") or {}).get("login", "?")
            where = f"{comment.get('path')}:{comment.get('line') or comment.get('original_line')}"
            events.append(
                {
                    "kind": "review-comment",
                    "title": f"an inline review comment from {who} on {where}",
                    "items": [
                        {
                            "who": who,
                            "text": self._text(comment),
                            "created_at": comment.get("created_at"),
                            "id": cid,
                            "where": where,
                            "commit_id": comment.get("commit_id"),
                        }
                    ],
                }
            )

        for run in snapshot.get("check_runs", []):
            # The key holds the state of the run as well as its name, so
            # that a run that was seen while it was still running is
            # reported again when it reaches a conclusion.
            rid = run.get("id")
            key = (rid, run.get("status"), run.get("conclusion"))
            if key in self.seen_check_runs:
                continue
            self.seen_check_runs.add(key)
            name = run.get("name", "?")
            events.append(
                {
                    "kind": "ci",
                    "title": f"check {name}: {run.get('status')} {run.get('conclusion') or ''}".strip(),
                    "items": [
                        {
                            "who": name,
                            "text": (run.get("output") or {}).get("title")
                            or (run.get("output") or {}).get("summary")
                            or "",
                            "url": run.get("html_url"),
                            "id": rid,
                        }
                    ],
                }
            )

        for status in snapshot.get("statuses", []):
            key = f"{status.get('id')}:{status.get('state')}"
            if key in self.seen_statuses:
                continue
            self.seen_statuses.add(key)
            events.append(
                {
                    "kind": "ci",
                    "title": f"commit status {status.get('context')}: {status.get('state')}",
                    "items": [
                        {
                            "who": status.get("context", "?"),
                            "text": status.get("description") or "",
                            "url": status.get("target_url"),
                        }
                    ],
                }
            )

        self.primed = True
        return events



# --------------------------------------------------------------------------
# The snapshot
# --------------------------------------------------------------------------
def take_snapshot(gh: GitHub, repo: str, pr: int) -> dict:
    head = gh.get(f"/repos/{repo}/pulls/{pr}")[1] or {}
    sha = (head.get("head") or {}).get("sha", "")

    comments, _ = gh.get_all(f"/repos/{repo}/issues/{pr}/comments?per_page=100")
    reviews, _ = gh.get_all(f"/repos/{repo}/pulls/{pr}/reviews?per_page=100")
    review_comments, _ = gh.get_all(f"/repos/{repo}/pulls/{pr}/comments?per_page=100")
    check_runs = gh.get_object_list(
        f"/repos/{repo}/commits/{sha}/check-runs?per_page=100", "check_runs"
    )
    statuses = gh.get_object_list(
        f"/repos/{repo}/commits/{sha}/status?per_page=100", "statuses"
    )

    return {
        "head_sha": sha,
        "comments": comments,
        "reviews": reviews,
        "review_comments": review_comments,
        "check_runs": check_runs,
        "statuses": statuses,
    }


# --------------------------------------------------------------------------
# The report
# --------------------------------------------------------------------------
def render_report(events: list[dict]) -> str:
    lines = ["# Something new happened on the pull request", ""]
    for event in events:
        lines.append(f"## {event['kind']}: {event['title']}")
        lines.append("")
        for item in event["items"]:
            lines.append(f"- **{item.get('who', '?')}**")
            if item.get("where"):
                lines.append(f"  - at `{item['where']}`")
            if item.get("created_at"):
                lines.append(f"  - at {item['created_at']}")
            if item.get("url"):
                lines.append(f"  - {item['url']}")
            text = item.get("text") or ""
            if text:
                lines.append("")
                lines.append("```text")
                lines.append(text)
                lines.append("```")
            lines.append("")
    return "\n".join(lines)


def print_summary(events: list[dict]) -> None:
    print(f"WATCH: {len(events)} event(s) on the pull request")
    for event in events:
        for item in event["items"]:
            print(f"  [{event['kind']}] {event['title']}")
            headline = (item.get("text") or "").splitlines()
            if headline and headline[0]:
                print(f"      {headline[0][:160]}")
            if item.get("where"):
                print(f"      {item['where']}")
            if item.get("url"):
                print(f"      {item['url']}")


def valid_wait(interval: float, max_wait: float) -> bool:
    """The watcher has to wait, and it has to wait for a while."""
    return interval > 0 and max_wait > 0


def write_report(path: str, text: str) -> tuple:
    """Write the report, making the folder of the file when it is not there.

    The folder of the default path is the build folder, which a clean
    checkout does not have, and a report that is not written is a report
    that is lost.  The answer is the text of the error, so that a path that
    cannot be written stays diagnosable.
    """
    try:
        folder = os.path.dirname(os.path.abspath(path))
        os.makedirs(folder, exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(text)
        return True, ""
    except OSError as err:
        return False, str(err)


# --------------------------------------------------------------------------
# The self test
# --------------------------------------------------------------------------
def self_test() -> int:
    """The event machine is a pure function, so it can be tested without a network."""
    failures = 0
    checks = 0

    def check(name: str, condition: bool) -> None:
        nonlocal failures, checks
        checks += 1
        if not condition:
            failures += 1
            print(f"FAIL {name}")
        else:
            print(f"ok   {name}")

    def comment(cid: int, login: str, body: str, created: str = "2026-10-22T00:00:00Z"):
        return {
            "id": cid,
            "user": {"login": login},
            "body": body,
            "created_at": created,
        }

    def review(rid: int, login: str, body: str, state: str = "COMMENTED"):
        return {
            "id": rid,
            "user": {"login": login},
            "body": body,
            "state": state,
            "submitted_at": "2026-10-22T00:00:00Z",
            "commit_id": "a" * 40,
        }

    # 1. The first scan primes the watcher: nothing is an event yet.
    watcher = Watcher(["trafgals"], ["any"])
    first = watcher.scan(
        {
            "head_sha": "a" * 40,
            "comments": [comment(1, "trafgals", "hello")],
            "reviews": [review(10, "cubic-dev-ai[bot]", "old review")],
            "review_comments": [],
            "check_runs": [],
            "statuses": [],
        }
    )
    check("the first scan reports nothing", first == [])

    # 2. A new comment from the watched author is an event.
    events = watcher.scan(
        {
            "head_sha": "a" * 40,
            "comments": [comment(1, "trafgals", "hello"), comment(2, "trafgals", "please fix X")],
            "reviews": [],
            "review_comments": [],
            "check_runs": [],
            "statuses": [],
        }
    )
    check("a comment from trafgals wakes the watcher", len(events) == 1)
    check("the comment is reported as a comment", events and events[0]["kind"] == "comment")
    check(
        "the comment body is kept",
        events and "please fix X" in events[0]["items"][0]["text"],
    )

    # 3. The same comment does not wake the watcher twice.
    events = watcher.scan(
        {
            "head_sha": "a" * 40,
            "comments": [comment(1, "trafgals", "hello"), comment(2, "trafgals", "please fix X")],
            "reviews": [],
            "review_comments": [],
            "check_runs": [],
            "statuses": [],
        }
    )
    check("a comment is only reported once", events == [])

    # 4. A comment from somebody else does not wake the watcher.
    events = watcher.scan(
        {
            "head_sha": "a" * 40,
            "comments": [comment(1, "trafgals", "hello"), comment(3, "someone-else", "hi")],
            "reviews": [],
            "review_comments": [],
            "check_runs": [],
            "statuses": [],
        }
    )
    check("a comment from another author is ignored", events == [])

    # 5. The agent's own progress report does not wake the watcher.
    events = watcher.scan(
        {
            "head_sha": "a" * 40,
            "comments": [comment(4, "trafgals", PROGRESS_MARKER + "\nall done")],
            "reviews": [],
            "review_comments": [],
            "check_runs": [],
            "statuses": [],
        }
    )
    check("the agent's own report is ignored", events == [])

    # 6. A new review wakes the watcher.
    events = watcher.scan(
        {
            "head_sha": "a" * 40,
            "comments": [],
            "reviews": [review(11, "cubic-dev-ai[bot]", "11 issues found")],
            "review_comments": [],
            "check_runs": [],
            "statuses": [],
        }
    )
    check("a review wakes the watcher", len(events) == 1 and events[0]["kind"] == "review")

    # 7. A new inline review comment wakes the watcher.
    events = watcher.scan(
        {
            "head_sha": "a" * 40,
            "comments": [],
            "reviews": [],
            "review_comments": [
                {
                    "id": 20,
                    "user": {"login": "cubic-dev-ai[bot]"},
                    "body": "P0: this is wrong",
                    "path": "src/foo.c",
                    "line": 12,
                    "created_at": "2026-10-22T00:00:00Z",
                }
            ],
            "check_runs": [],
            "statuses": [],
        }
    )
    check("an inline review comment wakes the watcher", len(events) == 1)
    check(
        "the inline comment names the place",
        events and events[0]["items"][0].get("where") == "src/foo.c:12",
    )

    # 8. A finished check run wakes the watcher.
    events = watcher.scan(
        {
            "head_sha": "a" * 40,
            "comments": [],
            "reviews": [],
            "review_comments": [],
            "check_runs": [
                {
                    "id": 30,
                    "name": "cubic",
                    "status": "completed",
                    "conclusion": "success",
                    "html_url": "https://example.invalid/run",
                    "output": {"title": "all good"},
                }
            ],
            "statuses": [],
        }
    )
    check("a check run wakes the watcher", len(events) == 1 and events[0]["kind"] == "ci")

    # 9. A moved head wakes the watcher.
    events = watcher.scan(
        {
            "head_sha": "b" * 40,
            "comments": [],
            "reviews": [],
            "review_comments": [],
            "check_runs": [],
            "statuses": [],
        }
    )
    check("a moved head wakes the watcher", len(events) == 1 and events[0]["kind"] == "commit")

    # 10. The report renders the bodies.
    report = render_report(
        [
            {
                "kind": "comment",
                "title": "a new comment from trafgals",
                "items": [{"who": "trafgals", "text": "the body"}],
            }
        ]
    )
    check("the report keeps the body", "the body" in report)
    check("the report names the kind", "## comment:" in report)

    # 11. A check run that is seen while it runs is reported again when it
    #     reaches a conclusion.
    running = Watcher(["trafgals"], ["any"])
    running.scan(
        {
            "head_sha": "a" * 40,
            "comments": [],
            "reviews": [],
            "review_comments": [],
            "check_runs": [
                {"id": 31, "name": "cubic", "status": "in_progress", "conclusion": None}
            ],
            "statuses": [],
        }
    )
    events = running.scan(
        {
            "head_sha": "a" * 40,
            "comments": [],
            "reviews": [],
            "review_comments": [],
            "check_runs": [
                {"id": 31, "name": "cubic", "status": "completed", "conclusion": "success"}
            ],
            "statuses": [],
        }
    )
    check("a check run that reaches a conclusion is reported", len(events) == 1)
    check("the conclusion is a ci event", events and events[0]["kind"] == "ci")

    # 12. The same state of a check run is not reported twice.
    events = running.scan(
        {
            "head_sha": "a" * 40,
            "comments": [],
            "reviews": [],
            "review_comments": [],
            "check_runs": [
                {"id": 31, "name": "cubic", "status": "completed", "conclusion": "success"}
            ],
            "statuses": [],
        }
    )
    check("the same state of a check run is not reported twice", events == [])

    # 13. The watcher has to wait.
    check("an interval of zero is refused", not valid_wait(0, 100))
    check("a negative interval is refused", not valid_wait(-1, 100))
    check("a wait of zero is refused", not valid_wait(45, 0))
    check("a sane interval is accepted", valid_wait(45, 3600))

    # 14. The report is written even when the folder of the file is not
    #     there: a clean checkout has no build folder.
    folder = tempfile.mkdtemp(prefix="watch_pr_self_test_")
    path = os.path.join(folder, "the_report.md")
    written, error = write_report(path, "the body")
    check("the report is written", written)
    check("the report is on the disk", os.path.isfile(path))
    if os.path.isfile(path):
        with open(path, "r", encoding="utf-8") as handle:
            check("the report holds the body", handle.read() == "the body")
    shutil.rmtree(folder, ignore_errors=True)

    print(f"\n{checks - failures}/{checks} checks passed")
    return 0 if failures == 0 else 1


# --------------------------------------------------------------------------
# The main loop
# --------------------------------------------------------------------------
def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default="trafgals/3s-decomp")
    parser.add_argument("--pr", type=int, default=1)
    parser.add_argument(
        "--comment-authors",
        default="trafgals",
        help="comma separated logins whose comments wake the watcher",
    )
    parser.add_argument(
        "--review-authors",
        default="any",
        help="comma separated logins whose reviews wake the watcher, or any",
    )
    parser.add_argument("--interval", type=float, default=45.0)
    parser.add_argument("--max-wait", type=float, default=43200.0)
    parser.add_argument("--report", default="build/pr_watch_event.md")
    parser.add_argument("--once", action="store_true", help="print a snapshot and exit")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)

    if args.self_test:
        return self_test()

    if not valid_wait(args.interval, args.max_wait):
        print("WATCH: the interval and the wait have to be longer than zero", file=sys.stderr)
        return 1

    comment_authors = [a.strip() for a in args.comment_authors.split(",") if a.strip()]
    review_authors = [a.strip() for a in args.review_authors.split(",") if a.strip()]

    try:
        gh = GitHub(gh_token())
    except GitHubError as err:
        print(f"WATCH: {err}", file=sys.stderr)
        return 1

    watcher = Watcher(comment_authors, review_authors)

    if args.once:
        snapshot = take_snapshot(gh, args.repo, args.pr)
        print(f"head      {snapshot['head_sha']}")
        print(f"comments  {len(snapshot['comments'])}")
        for comment in snapshot["comments"]:
            who = (comment.get("user") or {}).get("login", "?")
            body = (comment.get("body") or "").splitlines()
            print(f"  {comment.get('created_at')} {who} :: {body[0][:90] if body else ''}")
        print(f"reviews   {len(snapshot['reviews'])}")
        for review in snapshot["reviews"]:
            who = (review.get("user") or {}).get("login", "?")
            body = (review.get("body") or "").splitlines()
            print(
                f"  {review.get('submitted_at')} {who} {review.get('state')} :: "
                f"{body[0][:90] if body else ''}"
            )
        print(f"inline    {len(snapshot['review_comments'])}")
        for comment in snapshot["review_comments"][-12:]:
            who = (comment.get("user") or {}).get("login", "?")
            body = (comment.get("body") or "").splitlines()
            print(
                f"  {comment.get('created_at')} {who} "
                f"{comment.get('path')}:{comment.get('line')} :: "
                f"{body[0][:90] if body else ''}"
            )
        print(f"checks    {len(snapshot['check_runs'])}")
        for run in snapshot["check_runs"]:
            print(f"  {run.get('name')} {run.get('status')} {run.get('conclusion')}")
        return 0

    deadline = time.time() + args.max_wait
    print(
        f"WATCH: watching {args.repo}#{args.pr} for a comment from "
        f"{', '.join(comment_authors)}, a review or a check run"
    )
    sys.stdout.flush()

    backoff = args.interval
    while time.time() < deadline:
        try:
            snapshot = take_snapshot(gh, args.repo, args.pr)
            backoff = args.interval
        except GitHubError as err:
            print(f"WATCH: {err}; retrying", file=sys.stderr)
            time.sleep(min(backoff, 300.0))
            backoff = min(backoff * 2, 300.0)
            continue

        events = watcher.scan(snapshot)
        if events:
            report = render_report(events)
            written, error = write_report(args.report, report)
            if written:
                print(f"WATCH: the full report is in {args.report}")
            else:
                print(f"WATCH: could not write {args.report}: {error}", file=sys.stderr)
            print_summary(events)
            sys.stdout.flush()
            return 0

        time.sleep(args.interval)

    print("WATCH: timed out with nothing to report")
    return 3


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
