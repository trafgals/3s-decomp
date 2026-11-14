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
        # The events and the refs are written out here on purpose: a test
        # that takes them from the checker checks the checker against
        # itself.
        for event in ("push", "workflow_dispatch"):
            self.assertTrue(C.evaluate(expression, event, "refs/heads/main"), event)
            for ref in ("refs/heads/ccr-579fa5e1-azpt7p", "refs/tags/v1.0"):
                self.assertFalse(C.evaluate(expression, event, ref), f"{event} {ref}")

    def test_the_checker_knows_the_events_of_the_workflow(self):
        self.assertEqual(("push", "workflow_dispatch"), tuple(C.DEPLOY_EVENTS))
        self.assertEqual("refs/heads/main", C.MAIN)

    def test_the_checker_knows_a_sample_of_the_refs(self):
        self.assertTrue(C.BRANCH_REFS)
        self.assertTrue(C.TAG_REFS)

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
        self.assertTrue(C.evaluate(expression, C.PUSH, "refs/heads/main"))
        self.assertFalse(C.evaluate(expression, C.PUSH, "refs/heads/other"))

    def test_the_or_operator_is_evaluated(self):
        expression = "github.ref == 'refs/heads/main' || github.ref == 'refs/heads/other'"
        self.assertTrue(C.evaluate(expression, C.PUSH, "refs/heads/main"))
        self.assertTrue(C.evaluate(expression, C.PUSH, "refs/heads/other"))
        self.assertFalse(C.evaluate(expression, C.PUSH, "refs/heads/third"))

    def test_the_not_operator_is_evaluated(self):
        expression = "!(github.ref == 'refs/heads/main')"
        self.assertFalse(C.evaluate(expression, C.PUSH, "refs/heads/main"))
        self.assertTrue(C.evaluate(expression, C.PUSH, "refs/heads/other"))

    def test_the_parentheses_are_evaluated(self):
        expression = (
            "(github.ref == 'refs/heads/main' || github.ref == 'refs/heads/other') "
            "&& github.ref_name != 'other'"
        )
        self.assertTrue(C.evaluate(expression, C.PUSH, "refs/heads/main"))
        self.assertFalse(C.evaluate(expression, C.PUSH, "refs/heads/other"))

    def test_the_expression_wrapper_is_removed(self):
        self.assertTrue(C.evaluate("${{ github.ref == 'refs/heads/main' }}", C.PUSH, "refs/heads/main"))

    def test_the_boolean_literals_are_evaluated(self):
        self.assertTrue(C.evaluate("true", C.PUSH, "refs/heads/main"))
        self.assertFalse(C.evaluate("false", C.PUSH, "refs/heads/main"))
        self.assertTrue(C.evaluate("github.ref == 'refs/heads/main' && true", C.PUSH, "refs/heads/main"))

    def test_the_success_function_is_evaluated(self):
        self.assertTrue(C.evaluate("success()", C.PUSH, "refs/heads/main"))

    def test_the_status_functions_that_depend_on_the_build_are_refused(self):
        # always(), failure() and cancelled() change the meaning of the
        # guard with the state of the build, which the checker does not
        # know: a guard that uses them is refused.
        for function in ("always()", "failure()", "cancelled()"):
            with self.assertRaises(C.GuardError):
                C.evaluate(function, C.PUSH, "refs/heads/main")

    def test_a_guard_with_text_after_the_expression_is_refused(self):
        # The guard is read in full: a guard that is only half read is a
        # guard that is checked in a way that does not match GitHub's.
        with self.assertRaises(C.GuardError):
            C.evaluate("github.ref == 'refs/heads/main' && evil()", C.PUSH, "refs/heads/main")

    def test_a_guard_that_is_not_an_expression_is_refused(self):
        with self.assertRaises(C.GuardError):
            C.evaluate("github.ref == 'refs/heads/main'; import os", C.PUSH, "refs/heads/main")

    def test_a_guard_that_reads_an_unknown_name_is_refused(self):
        with self.assertRaises(C.GuardError):
            C.evaluate("github.event.pull_request.number == 1", C.PUSH, "refs/heads/main")

    def test_a_guard_that_calls_an_unknown_function_is_refused(self):
        with self.assertRaises(C.GuardError):
            C.evaluate("contains(github.ref, 'main')", C.PUSH, "refs/heads/main")

    def test_the_head_and_the_base_ref_are_empty(self):
        # head_ref and base_ref are set for a pull request only, and the
        # workflow runs on a push and on a manual run, so they are empty:
        # a guard that reads them does not deploy.
        self.assertFalse(C.evaluate("github.head_ref == 'main'", C.PUSH, "refs/heads/main"))
        self.assertFalse(C.evaluate("github.base_ref == 'main'", C.PUSH, "refs/heads/main"))
        self.assertFalse(C.evaluate("github.base_ref == 'main'", C.DISPATCH, "refs/heads/main"))

    def test_the_ref_name_is_the_short_name_of_the_ref(self):
        context = C.github_context(C.PUSH, "refs/heads/feature/x")
        self.assertEqual("feature/x", context["ref_name"])
        self.assertEqual("branch", context["ref_type"])
        context = C.github_context(C.PUSH, "refs/tags/v1.0")
        self.assertEqual("v1.0", context["ref_name"])
        self.assertEqual("tag", context["ref_type"])

    def test_the_event_name_is_the_name_of_the_event(self):
        self.assertTrue(C.evaluate("github.event_name == 'push'", C.PUSH, "refs/heads/main"))
        self.assertFalse(C.evaluate("github.event_name == 'push'", C.DISPATCH, "refs/heads/main"))

    def test_a_string_literal_in_double_quotes_is_refused(self):
        # GitHub throws an error on a string literal in double quotes.
        with self.assertRaises(C.GuardError):
            C.evaluate('github.ref == "refs/heads/main"', C.PUSH, "refs/heads/main")

    def test_the_case_of_the_strings_is_ignored(self):
        # GitHub ignores the case when it compares strings.
        self.assertTrue(C.evaluate("github.ref == 'REFS/HEADS/MAIN'", C.PUSH, "refs/heads/main"))
        self.assertTrue(C.evaluate("github.ref_name == 'MAIN'", C.PUSH, "refs/heads/main"))


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

    def test_the_workflow_with_a_guard_that_names_a_tag_fails(self):
        # A guard that matches the name of a tag is a guard that deploys
        # on that tag, and the environment rejects a tag.
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' || github.ref_type == 'tag'\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("refs/tags" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_names_a_tag_by_name_fails(self):
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' || github.ref_name == 'v1.0'\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("refs/tags/v1.0" in failure for failure in failures), failures)

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

    def test_the_workflow_with_a_guard_that_uses_a_status_function_fails(self):
        # always() and failure() make the deploy job run even when the
        # build failed, and a run of a branch that is then rejected by the
        # environment is a failed run.
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' || failure()\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("cannot be evaluated" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_reads_the_head_ref_fails(self):
        # head_ref is empty on a push, so the guard would not deploy on a
        # push of main.
        text = workflow_text().replace(
            GUARD,
            "    if: github.head_ref == 'main'\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("does not deploy" in failure for failure in failures), failures)

    def test_the_workflow_with_a_guard_that_reads_the_event_fails(self):
        # The deploy job has to deploy on a manual run of main as well.
        text = workflow_text().replace(
            GUARD,
            "    if: github.ref == 'refs/heads/main' && github.event_name == 'push'\n",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(
            any("workflow_dispatch" in failure for failure in failures), failures
        )

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
        self.assertTrue(
            any("check_pages_workflow.py" in failure for failure in failures), failures
        )

    def test_the_workflow_that_does_not_run_on_a_change_of_the_requirements_fails(self):
        text = workflow_text().replace(
            '      - "tools/requirements-python.txt"\n',
            "",
        )
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("requirements" in failure for failure in failures), failures)

    def test_the_workflow_that_does_not_run_on_a_change_of_the_tests_fails(self):
        text = workflow_text().replace('      - "tests/verify/**"\n', "")
        failures = self.check(text)
        self.assertTrue(failures)
        self.assertTrue(any("tests/verify" in failure for failure in failures), failures)

    def test_a_workflow_that_is_not_yaml_fails(self):
        failures = self.check("jobs: [\n")
        self.assertTrue(failures)
        self.assertTrue(any("YAML" in failure for failure in failures), failures)


if __name__ == "__main__":
    unittest.main()
