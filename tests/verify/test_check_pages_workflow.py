"""The tests of the checker of the Pages workflow (see README.md).

The checker decides whether the deploy job of the workflow may run, and it
decides that by evaluating the guard of the job.  These tests check the
checker against the workflow of the repository and against copies of it
that are broken on purpose: a guard that is removed, a guard that points at
another branch, a guard that is written in a way the checker does not
understand.  A checker that accepts a broken guard is worse than no
checker, so every one of those copies has to be reported as a failure.
"""

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / "tools/verify"))

import check_pages_workflow as C  # noqa: E402

WORKFLOW = ROOT / ".github" / "workflows" / "pages.yml"

# The guard of the deploy job in the workflow of the repository.
GUARD = "    if: github.ref == 'refs/heads/main'\n"


def workflow_text() -> str:
    return WORKFLOW.read_text(encoding="utf-8")


class CheckerTest(unittest.TestCase):
    """The checker of the workflow of the repository."""

    def test_the_workflow_of_the_repository_passes(self):
        passed, failures = C.check_workflow(WORKFLOW)
        self.assertEqual([], failures)
        self.assertGreater(passed, 0)

    def test_the_guard_deploys_on_main_only(self):
        text = workflow_text()
        self.assertIn(GUARD, text)
        expression = GUARD.strip()[len("if:"):].strip()
        self.assertTrue(C.evaluate(expression, "refs/heads/main"))
        for ref in C.REFS:
            if ref == "refs/heads/main":
                continue
            self.assertFalse(C.evaluate(expression, ref), ref)

    def test_pyyaml_is_in_the_requirements(self):
        # The checker reads the workflow with PyYAML, so PyYAML has to be in
        # the requirements that the setup of the repository installs.
        requirements = (ROOT / "tools" / "requirements-python.txt").read_text(encoding="utf-8")
        names = [line.strip().lower() for line in requirements.splitlines()]
        self.assertTrue(
            any(name == "pyyaml" or name.startswith("pyyaml") for name in names),
            "pyyaml is not in tools/requirements-python.txt",
        )

    def test_the_checker_does_not_use_eval(self):
        # The guard is read by the parser of the checker and not by eval(),
        # so that the operators of the workflow language are not the
        # operators of Python and so that a guard cannot run code.
        source = (ROOT / "tools" / "verify" / "check_pages_workflow.py").read_text(encoding="utf-8")
        self.assertNotIn("eval(", source)


class ExpressionTest(unittest.TestCase):
    """The evaluation of the guard."""

    def test_the_and_operator_is_evaluated(self):
        # "&&" is an operator of the workflow language and not of Python.
        expression = "github.ref == 'refs/heads/main' && github.ref_name == 'main'"
        self.assertTrue(C.evaluate(expression, "refs/heads/main"))
        self.assertFalse(C.evaluate(expression, "refs/heads/other"))

    def test_the_or_operator_is_evaluated(self):
        expression = "github.ref == 'refs/heads/main' || github.ref == 'refs/heads/other'"
        self.assertTrue(C.evaluate(expression, "refs/heads/main"))
        self.assertTrue(C.evaluate(expression, "refs/heads/other"))
        self.assertFalse(C.evaluate(expression, "refs/heads/third"))

    def test_the_not_operator_is_evaluated(self):
        expression = "!(github.ref == 'refs/heads/main')"
        self.assertFalse(C.evaluate(expression, "refs/heads/main"))
        self.assertTrue(C.evaluate(expression, "refs/heads/other"))

    def test_the_parentheses_are_evaluated(self):
        expression = "(github.ref == 'refs/heads/main' || github.ref == 'refs/heads/other') && github.ref_name != 'other'"
        self.assertTrue(C.evaluate(expression, "refs/heads/main"))
        self.assertFalse(C.evaluate(expression, "refs/heads/other"))

    def test_the_expression_wrapper_is_removed(self):
        self.assertTrue(C.evaluate("${{ github.ref == 'refs/heads/main' }}", "refs/heads/main"))

    def test_the_boolean_literals_are_evaluated(self):
        self.assertTrue(C.evaluate("true", "refs/heads/main"))
        self.assertFalse(C.evaluate("false", "refs/heads/main"))
        self.assertTrue(C.evaluate("github.ref == 'refs/heads/main' && true", "refs/heads/main"))

    def test_a_guard_with_text_after_the_expression_is_refused(self):
        # The guard is read in full: a guard that is only half read is a
        # guard that is checked in a way that does not match GitHub's.
        with self.assertRaises(C.GuardError):
            C.evaluate("github.ref == 'refs/heads/main' && evil()", "refs/heads/main")

    def test_a_guard_that_is_not_an_expression_is_refused(self):
        with self.assertRaises(C.GuardError):
            C.evaluate("github.ref == 'refs/heads/main'; import os", "refs/heads/main")

    def test_a_guard_that_reads_an_unknown_name_is_refused(self):
        with self.assertRaises(C.GuardError):
            C.evaluate("github.event.pull_request.number == 1", "refs/heads/main")

    def test_a_guard_that_calls_an_unknown_function_is_refused(self):
        with self.assertRaises(C.GuardError):
            C.evaluate("contains(github.ref, 'main')", "refs/heads/main")


class BrokenWorkflowTest(unittest.TestCase):
    """The checker against copies of the workflow that are broken."""

    def check(self, text: str) -> list[str]:
        with tempfile.TemporaryDirectory(prefix="pages-workflow-") as directory:
            path = Path(directory) / "pages.yml"
            path.write_text(text, encoding="utf-8")
            _, failures = C.check_workflow(path)
            return failures

    def test_the_workflow_without_a_guard_fails(self):
        failures = self.check(workflow_text().replace(GUARD, ""))
        self.assertTrue(failures)
        self.assertTrue(any("guard" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_of_another_branch_fails(self):
        text = workflow_text().replace(GUARD, "    if: github.ref == 'refs/heads/other'\n")
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("main" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_deploys_everywhere_fails(self):
        text = workflow_text().replace(GUARD, "    if: github.event_name == 'push'\n")
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("deploys on" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_names_another_branch_fails(self):
        # The branch that the guard names is not one of the refs the guard
        # is evaluated against, and the environment rejects it as well.
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' || github.ref == 'refs/heads/other'\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("refs/heads/other" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_names_another_branch_by_name_fails(self):
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' || github.ref_name == 'other'\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("refs/heads/other" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_excludes_a_branch_fails(self):
        # A guard that only excludes a branch deploys on every other
        # branch, and the environment rejects those.
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref != 'refs/heads/other'\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("deploys on" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_excludes_a_branch_of_its_own_passes(self):
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' && github.ref != 'refs/heads/other'\n",
        )
        self.assertEqual([], self.check(text))

    def test_the_workflow_with_a_compound_guard_is_evaluated(self):
        # The guard is a compound expression of the workflow language: it
        # has to be evaluated, and not refused because of its shape.
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' && github.ref_name == 'main'\n",
        )
        self.assertEqual([], self.check(text))

    def test_the_workflow_with_a_compound_guard_that_is_wrong_fails(self):
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' && github.ref_name == 'other'\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("main" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_is_not_read_in_full_fails(self):
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' && evil()\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("cannot be evaluated" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guarded_build_job_fails(self):
        text = workflow_text().replace(
            "  build:\n    runs-on: ubuntu-latest\n",
            "  build:\n    if: github.ref == 'refs/heads/main'\n    runs-on: ubuntu-latest\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("build job is guarded" in failure for failure in failures), failures)

    def test_the_workflow_without_the_deploy_job_fails(self):
        text = workflow_text().replace("  deploy:\n", "  deploy2:\n")
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("deploy job" in failure for failure in failures), failures)

    def test_the_workflow_that_does_not_run_on_a_change_of_the_checker_fails(self):
        text = workflow_text().replace(
            '      - "tools/verify/check_pages_workflow.py"\n',
            "",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("checker" in failure for failure in failures), failures)

    def test_a_workflow_that_is_not_yaml_fails(self):
        failures = self.check("jobs: [\n")
        self.assertTrue(failures)
        self.assertTrue(any("YAML" in failure for failure in failures), failures)


if __name__ == "__main__":
    unittest.main()
