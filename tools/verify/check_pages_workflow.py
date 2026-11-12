#!/usr/bin/env python3
"""Check that the Pages workflow only deploys from main, and that a branch
push is still a build check.

The github-pages environment of this repository has a deployment branch
policy that lists the branches that may deploy.  A run of a branch that is
not on that list is rejected by the policy: the build job succeeds and the
deploy job fails with

    Branch "<name>" is not allowed to deploy to github-pages due to
    environment protection rules.

which is what made every push to a review branch show up as a failed run
even though the game built.  The deploy job therefore carries a guard that
restricts it to main.  The guard is checked here by *evaluating* it, the way
GitHub does, against a set of refs:

* refs/heads/main                  -> the deploy job runs;
* any other branch, a tag, a fork   -> the deploy job is skipped;
* the build job runs in every case, so a branch push still compiles the
  game and still fails the run when the game does not build.

Usage: python3 tools/verify/check_pages_workflow.py [--workflow PATH]
Exits 0 when every check passes, 1 otherwise.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent.parent
WORKFLOW = ROOT / ".github" / "workflows" / "pages.yml"

# The guard is evaluated with eval(), so only the shape of an expression
# that is allowed to appear in a job level "if" is accepted here.
SAFE_EXPRESSION = re.compile(
    r"^[A-Za-z_][A-Za-z0-9_.]*\s*(==|!=)\s*'[^']*'"
    r"|^\s*[A-Za-z_][A-Za-z0-9_.]*\s*(==|!=)\s*'[^']*'\s*(&&|\|\|)\s*[A-Za-z_][A-Za-z0-9_.]*\s*(==|!=)\s*'[^']*'\s*$"
)

# The refs the guard is evaluated against.  The last one is the ref of a
# pull request, which is not a branch at all.
REFS = [
    "refs/heads/main",
    "refs/heads/ccr-579fa5e1-azpt7p",
    "refs/heads/feat/auto-20261020-abd7ca61",
    "refs/tags/v1.0",
    "refs/pull/85/merge",
]


class Github:
    """The part of the workflow context the guard is allowed to read."""

    def __init__(self, ref: str) -> None:
        self.ref = ref
        self.ref_name = ref.rsplit("/", 1)[-1]


def evaluate(expression: str, ref: str) -> bool:
    """Evaluate a job level "if" expression the way GitHub does."""
    expression = expression.strip()
    if expression.startswith("${{") and expression.endswith("}}"):
        expression = expression[3:-2].strip()
    if not SAFE_EXPRESSION.match(expression):
        raise ValueError(f"the guard expression is not of a known shape: {expression!r}")
    return bool(eval(expression, {"__builtins__": {}}, {"github": Github(ref)}))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workflow", type=Path, default=WORKFLOW)
    args = parser.parse_args()

    failures: list[str] = []
    passed = 0

    def check(condition: bool, message: str) -> None:
        nonlocal passed
        if condition:
            passed += 1
        else:
            failures.append(message)

    text = args.workflow.read_text(encoding="utf-8")
    workflow = yaml.safe_load(text)
    jobs = workflow["jobs"]

    check("build" in jobs, "the workflow has no build job")
    check("deploy" in jobs, "the workflow has no deploy job")

    build = jobs.get("build", {})
    deploy = jobs.get("deploy", {})

    check(
        "if" not in build,
        "the build job is guarded, so a push to a branch does not build the game",
    )
    check(
        deploy.get("needs") == "build",
        "the deploy job does not need the build job",
    )
    check(
        deploy.get("environment", {}).get("name") == "github-pages",
        "the deploy job does not deploy to the github-pages environment",
    )

    guard = deploy.get("if")
    if guard is None:
        failures.append(
            "the deploy job has no guard, so a push to a branch is rejected by the "
            "deployment branch policy of the github-pages environment and the run fails"
        )
    else:
        check(
            "refs/heads/main" in guard,
            f"the guard does not mention the main branch: {guard!r}",
        )
        for ref in REFS:
            try:
                result = evaluate(guard, ref)
            except ValueError as error:
                failures.append(str(error))
                break
            if ref == "refs/heads/main":
                check(result, f"the guard does not deploy on main: {guard!r}")
            else:
                check(
                    not result,
                    f"the guard deploys on {ref}, which the environment rejects: {guard!r}",
                )

    for failure in failures:
        print(f"FAIL: {failure}")
    print(f"{passed} checks passed, {len(failures)} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
