# The tests of the tools

```
make verify-test
```

runs the tests of the tools of the repository.  They need no game, no disc
image and no display: they check the tools themselves, and they are run by
the workflows that use the tools.

## The checker of the Pages workflow

```
python3 tools/verify/check_pages_workflow.py
```

checks that the workflow of the Pages deployment (`.github/workflows/pages.yml`)
only deploys from `main`: the `github-pages` environment of the repository has
a deployment branch policy, and a run of a branch that is not on that list is
rejected by the policy, so a push to a review branch shows up as a failed run
even though the game built.

The checker reads the guard of the deploy job and *evaluates* it the way GitHub
does, against a set of refs: the deploy job has to run on `main` and to be
skipped on every other branch, on a tag and on the ref of a pull request.

The guard is an expression of the GitHub Actions language, and the checker
parses it instead of handing it to Python's `eval`: the operators of the
language (`&&` and `||`) are not the operators of Python, and a guard that is
only half read is a guard that is checked in a way that does not match
GitHub's.  A guard of a shape that the parser does not know is reported as a
failure, and not as a pass.

`tests/verify/test_check_pages_workflow.py` is the test of that checker: it
runs the checker against the workflow of the repository, and against copies of
it that are broken on purpose - a guard that is removed, a guard that points at
another branch, a guard that deploys on every branch, a guard that is written
in a way the parser does not read in full.  Every one of those copies has to
be reported as a failure, because a checker that lets a broken guard through
is worse than no checker at all.
