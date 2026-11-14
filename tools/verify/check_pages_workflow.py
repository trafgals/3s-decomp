#!/usr/bin/env python3
"""Check that the Pages workflow only deploys from main, and that a branch
push is still a build check.

The github-pages environment of this repository has a deployment branch
policy that lists the branches that may deploy.  A run of a branch that
is not on that list is rejected by the policy: the build job succeeds and
the deploy job fails with

    Branch "<name>" is not allowed to deploy to github-pages due to
    environment protection rules.

which is what made every push to a review branch show up as a failed run
even though the game built.  The deploy job therefore carries a guard that
restricts it to main.  The guard is checked here by *evaluating* it, the way
GitHub does, against the events and the refs of the workflow:

* a push of main and a manual run of main   -> the deploy job runs;
* a push of another branch, a tag, a manual
  run of another branch                     -> the deploy job is skipped;
* the build job runs in every case, so a branch push still compiles the
  game and still fails the run when the game does not build.

The workflow runs on a push and on a manual run, so the refs it can have are
branches and tags.  The refs that the guard names are checked as well, so
that a guard that deploys on a branch of its own is reported.

The guard is an expression of the GitHub Actions language, and it is parsed
here instead of being handed to Python's eval.  The parser reads the whole
expression, so a guard of a shape that is not known is reported as a
failure instead of being read in a way that does not match GitHub's: the
operators of the language (&& and ||) are not the operators of Python, and
a guard with text after the expression is not the guard that was written.

The workflow is read with PyYAML, which is in tools/requirements-python.txt.

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

# The main branch, which is the only ref that may deploy.
MAIN = "refs/heads/main"
MAIN_NAME = "main"

# The events the workflow runs on: a push and a manual run.  The workflow
# does not run on a pull request, so the refs it can have are branches and
# tags.
PUSH = "push"
DISPATCH = "workflow_dispatch"

# The events in which the deploy job has to run.
DEPLOY_EVENTS = (PUSH, DISPATCH)

# The refs the guard is evaluated against: the main branch, two other
# branches and a tag.
BRANCH_REFS = ("refs/heads/ccr-579fa5e1-azpt7p", "refs/heads/feat/auto-20261020-abd7ca61")
TAG_REFS = ("refs/tags/v1.0",)

# The files that the workflow has to run on, so that a change of them is
# checked: the checker itself, its tests, the requirements it needs and the
# workflow that carries it.
WATCHED_PATHS = (
    "tools/verify/check_pages_workflow.py",
    "tests/verify/**",
    "tools/requirements-python.txt",
    ".github/workflows/pages.yml",
)

# The tokens of the subset of the workflow language that a job level "if"
# is written in: a string, a number, a name of the context, and the
# operators ==, !=, &&, ||, !, ( and ).
#
# A string literal is written in single quotes: "Wrapping with double quotes
# will throw an error" is what the documentation of the language says, so a
# guard with a double quoted string is a guard that GitHub refuses, and it
# is refused here as well.
TOKEN = re.compile(
    r"""
      (?P<string>'(?:[^']|'')*')
    | (?P<number>[0-9]+)
    | (?P<name>[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*)
    | (?P<op>&&|\|\||==|!=|\(|\)|!)
    """,
    re.VERBOSE,
)


class GuardError(ValueError):
    """The guard is not an expression this checker knows."""


def github_context(event: str, ref: str) -> dict:
    """The github context of an event and a ref.

    These are the parts of the context a guard may read, and the values
    are the ones GitHub sets for the events and the refs the guard is
    evaluated against:

    * ref_name is the short name of the ref: the name of the branch or of
      the tag, with the "refs/heads/" or the "refs/tags/" taken off.
    * head_ref and base_ref are empty: they are set for a pull request
      only, and the workflow runs on a push and on a manual run.
    * event_name is the name of the event, so a guard that reads it is
      checked in every event: the deploy job has to run on a push of main
      and on a manual run of main.
    """
    parts = ref.split("/")
    if len(parts) >= 3 and parts[0] == "refs" and parts[1] == "heads":
        ref_type = "branch"
    elif len(parts) >= 3 and parts[0] == "refs" and parts[1] == "tags":
        ref_type = "tag"
    elif len(parts) >= 3 and parts[0] == "refs" and parts[1] == "pull":
        ref_type = "pull_request"
    else:
        ref_type = "unknown"
    return {
        "ref": ref,
        "ref_name": "/".join(parts[2:]) if len(parts) > 2 else ref,
        "ref_type": ref_type,
        "event_name": event,
        "head_ref": "",
        "base_ref": "",
        "repository": "trafgals/3s-decomp",
        "actor": "trafgals",
        "workflow": "the web build",
        "job": "deploy",
        "sha": "0" * 40,
        "run_id": "1",
        "run_number": "1",
    }


# The status functions of the workflow language.  A job level "if" is
# "success() and the expression" unless the expression holds a status
# function of its own, and the state of the build is not something this
# checker knows, so only success() is read here: always(), failure() and
# cancelled() change the meaning of the guard with the state of the build,
# and a guard that uses them is refused instead of being read as if the
# build had succeeded.
STATUS_FUNCTIONS = {
    "success": True,
}

# The status functions that depend on the state of the build.
BUILD_FUNCTIONS = ("always", "failure", "cancelled")


def unquote(text: str) -> str:
    """The value of a string literal of the workflow language.

    A single quote is written twice inside a single quoted string.
    """
    return text[1:-1].replace("''", "'")


def tokenize(expression: str) -> list[tuple[str, object]]:
    """The tokens of an expression of the workflow language."""
    tokens: list[tuple[str, object]] = []
    position = 0
    while position < len(expression):
        if expression[position].isspace():
            position += 1
            continue
        match = TOKEN.match(expression, position)
        if not match:
            raise GuardError(
                f"the guard cannot be read at {expression[position:position + 20]!r}"
            )
        position = match.end()
        kind = match.lastgroup
        text = match.group(kind)
        if kind == "string":
            tokens.append(("string", unquote(text)))
        elif kind == "number":
            tokens.append(("number", int(text)))
        elif kind == "name":
            tokens.append(("name", text))
        else:
            tokens.append(("op", text))
    return tokens


class Parser:
    """The parser of the subset of the workflow language a guard is written in.

    The operators of the language are && and ||, and ! binds tighter than a
    comparison, and the precedence is the one of the language and not the
    one of Python.  A guard that the parser does not read in full is
    refused: a guard that is only half read is a guard that is checked in a
    way that does not match GitHub's.
    """

    def __init__(self, tokens: list[tuple[str, object]]) -> None:
        self.tokens = tokens
        self.position = 0

    def peek(self) -> tuple[str, object]:
        if self.position < len(self.tokens):
            return self.tokens[self.position]
        return ("end", None)

    def take(self) -> tuple[str, object]:
        token = self.peek()
        self.position += 1
        return token

    def parse(self) -> tuple:
        node = self.parse_or()
        if self.position != len(self.tokens):
            raise GuardError("the guard has text that is not part of the expression")
        return node

    def parse_or(self) -> tuple:
        node = self.parse_and()
        while self.peek() == ("op", "||"):
            self.take()
            node = ("or", node, self.parse_and())
        return node

    def parse_and(self) -> tuple:
        node = self.parse_unary()
        while self.peek() == ("op", "&&"):
            self.take()
            node = ("and", node, self.parse_unary())
        return node

    def parse_unary(self) -> tuple:
        if self.peek() == ("op", "!"):
            self.take()
            return ("not", self.parse_unary())
        return self.parse_primary()

    def parse_primary(self) -> tuple:
        kind, value = self.peek()
        if kind == "op" and value == "(":
            self.take()
            node = self.parse_or()
            if self.take() != ("op", ")"):
                raise GuardError("the guard has an opening parenthesis without a closing one")
            return node
        if kind in ("string", "number"):
            self.take()
            left = (kind, value)
        elif kind == "name":
            self.take()
            if value in ("true", "false"):
                left = ("boolean", value == "true")
            elif self.peek() == ("op", "("):
                self.take()
                if self.take() != ("op", ")"):
                    raise GuardError("the guard calls a function with an argument")
                left = ("call", value)
            else:
                left = ("context", value)
        else:
            raise GuardError("the guard does not start with a value")
        kind, value = self.peek()
        if kind == "op" and value in ("==", "!="):
            self.take()
            right = self.parse_value()
            return (value, left, right)
        return left

    def parse_value(self) -> tuple:
        kind, value = self.take()
        if kind in ("string", "number"):
            return (kind, value)
        if kind == "name":
            if value in ("true", "false"):
                return ("boolean", value == "true")
            return ("context", value)
        raise GuardError("the guard compares with something that is not a value")


def lookup(context: dict, path: str) -> object:
    """The value of a name of the context."""
    parts = path.split(".")
    if parts[0] != "github":
        raise GuardError(f"the guard reads {path!r}, which this checker does not know")
    value: object = context
    for part in parts[1:]:
        if not isinstance(value, dict) or part not in value:
            raise GuardError(f"the guard reads {path!r}, which this checker does not know")
        value = value[part]
    return value


def evaluate_node(node: tuple, context: dict) -> object:
    """The value of a parsed expression."""
    kind = node[0]
    if kind in ("string", "number", "boolean"):
        return node[1]
    if kind == "context":
        return lookup(context, node[1])
    if kind == "call":
        name = node[1]
        if name in STATUS_FUNCTIONS:
            return STATUS_FUNCTIONS[name]
        if name in BUILD_FUNCTIONS:
            raise GuardError(
                f"the guard calls {name}(), which changes with the state of the build, "
                f"so this checker cannot read it"
            )
        raise GuardError(f"the guard calls {name}(), which this checker does not know")
    if kind == "not":
        return not evaluate_node(node[1], context)
    if kind in ("==", "!="):
        left = evaluate_node(node[1], context)
        right = evaluate_node(node[2], context)
        if isinstance(left, str) and isinstance(right, str):
            # GitHub ignores the case when it compares strings.
            left, right = left.lower(), right.lower()
        return left == right if kind == "==" else left != right
    if kind == "and":
        return bool(evaluate_node(node[1], context)) and bool(evaluate_node(node[2], context))
    if kind == "or":
        return bool(evaluate_node(node[1], context)) or bool(evaluate_node(node[2], context))
    raise GuardError(f"the guard cannot be read: {node!r}")


def parse_guard(expression: str) -> tuple:
    """The parsed guard of a job."""
    expression = expression.strip()
    if expression.startswith("${{") and expression.endswith("}}"):
        expression = expression[3:-2].strip()
    return Parser(tokenize(expression)).parse()


def evaluate(expression: str, event: str, ref: str) -> bool:
    """Evaluate a job level "if" expression the way GitHub does."""
    return bool(evaluate_node(parse_guard(expression), github_context(event, ref)))


def mentioned_refs(node: tuple) -> set[str]:
    """The refs that the guard names.

    A guard that deploys on a branch it names is a guard that the
    deployment branch policy of the environment rejects, and that branch
    is not necessarily one of the refs the guard is evaluated against.

    Only the refs that the workflow can run on are candidates: the
    workflow runs on a push and on a manual run, so it can run on a
    branch and on a tag, and on nothing else.
    """
    refs: set[str] = set()
    if node[0] in ("==", "!="):
        for value, other in ((node[1], node[2]), (node[2], node[1])):
            if value[0] != "string" or other[0] != "context":
                continue
            if other[1] == "github.ref":
                refs.add(value[1].lower())
            elif other[1] == "github.ref_name":
                # The name of a ref is the name of a branch and the name of
                # a tag, so both are checked: a guard that matches the
                # name of a tag is a guard that deploys on that tag.  The
                # name of the main branch is not checked as a tag: a tag
                # that is named like the main branch is not a thing, and
                # the name of the main branch is the name that the guard
                # has to match.
                refs.add("refs/heads/" + value[1].lower())
                if value[1].lower() != MAIN_NAME:
                    refs.add("refs/tags/" + value[1].lower())
        return refs
    for child in node[1:]:
        if isinstance(child, tuple):
            refs |= mentioned_refs(child)
    return refs


def evaluation_refs(named: set[str]) -> list[str]:
    """The refs the guard is evaluated against.

    The main branch, a sample of the other branches and of the tags, and
    the refs that the guard names: a guard that deploys on a ref of its
    own is a guard that the environment rejects.
    """
    refs = [MAIN]
    refs += [ref for ref in BRANCH_REFS + TAG_REFS if ref != MAIN]
    refs += sorted(
        ref
        for ref in named
        if ref not in refs and (ref.startswith("refs/heads/") or ref.startswith("refs/tags/"))
    )
    return refs


def check_workflow(path: Path) -> tuple[int, list[str]]:
    """The checks of a workflow: the number that passed and the failures."""
    failures: list[str] = []
    passed = 0

    def check(condition: bool, message: str) -> None:
        nonlocal passed
        if condition:
            passed += 1
        else:
            failures.append(message)

    try:
        text = path.read_text(encoding="utf-8")
    except OSError as error:
        return 0, [f"the workflow {path} cannot be read: {error}"]

    try:
        workflow = yaml.safe_load(text)
    except yaml.YAMLError as error:
        return 0, [f"the workflow {path} is not valid YAML: {error}"]

    if not isinstance(workflow, dict):
        return 0, [f"the workflow {path} is not a mapping"]

    jobs = workflow.get("jobs") or {}
    check("build" in jobs, "the workflow has no build job")
    check("deploy" in jobs, "the workflow has no deploy job")

    build = jobs.get("build") or {}
    deploy = jobs.get("deploy") or {}

    check(
        "if" not in build,
        "the build job is guarded, so a push to a branch does not build the game",
    )
    check(
        deploy.get("needs") == "build",
        "the deploy job does not need the build job",
    )
    check(
        (deploy.get("environment") or {}).get("name") == "github-pages"
        if isinstance(deploy.get("environment"), dict)
        else deploy.get("environment") == "github-pages",
        "the deploy job does not deploy to the github-pages environment",
    )

    # PyYAML reads the "on" key of a workflow as the boolean true, because
    # "on" is a boolean in YAML 1.1.
    triggers = workflow.get("on", workflow.get(True)) or {}
    paths = (triggers.get("push") or {}).get("paths") or []
    for watched in WATCHED_PATHS:
        check(
            watched in paths,
            f"the workflow does not run on a change of {watched}, so a change of "
            f"it is not checked",
        )

    guard = deploy.get("if")
    if guard is None:
        failures.append(
            "the deploy job has no guard, so a push to a branch is rejected by the "
            "deployment branch policy of the github-pages environment and the run fails"
        )
    elif not isinstance(guard, str):
        failures.append(f"the guard is not an expression: {guard!r}")
    else:
        check(
            MAIN_NAME in guard.lower(),
            f"the guard does not mention the main branch: {guard!r}",
        )
        try:
            node = parse_guard(guard)
            named = mentioned_refs(node)
            results = []
            for event in DEPLOY_EVENTS:
                for ref in evaluation_refs(named):
                    results.append((event, ref, bool(evaluate_node(node, github_context(event, ref)))))
        except GuardError as error:
            failures.append(f"the guard cannot be evaluated: {error}")
        else:
            for event, ref, result in results:
                if ref == MAIN:
                    check(
                        result,
                        f"the guard does not deploy on {event} of {ref}: {guard!r}",
                    )
                else:
                    check(
                        not result,
                        f"the guard deploys on {event} of {ref}, which the environment "
                        f"rejects: {guard!r}",
                    )

    return passed, failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--workflow", type=Path, default=WORKFLOW)
    args = parser.parse_args()

    passed, failures = check_workflow(args.workflow)

    for failure in failures:
        print(f"FAIL: {failure}")
    print(f"{passed} checks passed, {len(failures)} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
