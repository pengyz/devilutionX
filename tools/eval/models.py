"""Strict YAML schema for D1-eval cases.

Ports osbot's models.py pattern: every block has an allowed-field frozenset,
and an unknown field is a hard error. This is what makes a typo'd assertion
(e.g. `passed_mim: 1`) fail loudly instead of silently passing.

Case file: eval/cases/<category>/<id>.yaml
"""

from __future__ import annotations

from pathlib import Path

import yaml

# --------------------------------------------------------------------------- allowed fields

_TOP_LEVEL = frozenset({
    'id', 'name', 'category', 'difficulty', 'tags', 'quarantine',
    'setup', 'run', 'expected', 'scoring', 'description',
})

_SETUP = frozenset({'mpq_required', 'side_effect', 'timeout', 'env', 'needs_build'})

_RUN = frozenset({'backend', 'binary', 'filter', 'args', 'command', 'harness'})

_EXPECTED = frozenset({
    'exit_code', 'failed', 'passed_min', 'skipped_max',
    'output_contains', 'output_not_contains', 'metrics',
})

_SCORING = frozenset({'pass', 'output_contains', 'output_not_contains'})

_DIFFICULTY = {'basic', 'medium', 'advanced', 'expert'}
_BACKENDS = {'gtest'}  # 'command' | 'harness' reserved for phase 2

DEFAULT_SCORING = {'pass': 10, 'output_contains': 5, 'output_not_contains': 3}


class EvalError(Exception):
    """Schema or runtime error in a case definition."""


class EvalCase:
    """A parsed, validated eval case."""

    __slots__ = (
        'path', 'id', 'name', 'category', 'difficulty', 'tags',
        'quarantine', 'setup', 'run', 'expected', 'scoring', 'description',
    )

    def __init__(self, path: Path, data: dict):
        self.path = path
        self.id = data['id']
        self.name = data['name']
        self.category = data['category']
        self.difficulty = data.get('difficulty', 'basic')
        self.tags = data.get('tags', [])
        self.quarantine = data.get('quarantine', False)
        self.setup = data.get('setup', {})
        self.run = data['run']
        self.expected = data['expected']
        self.scoring = {**DEFAULT_SCORING, **data.get('scoring', {})}
        self.description = data.get('description', '')

    # -- convenient accessors -----------------------------------------------
    @property
    def mpq_required(self) -> bool:
        return self.setup.get('mpq_required', False)

    @property
    def side_effect(self) -> bool:
        return self.setup.get('side_effect', False)

    @property
    def timeout(self) -> int:
        return self.setup.get('timeout', 120)

    @property
    def needs_build(self) -> bool:
        return self.setup.get('needs_build', False)

    @property
    def env(self) -> dict:
        return self.setup.get('env', {})

    @property
    def binary(self) -> str | None:
        return self.run.get('binary')

    @property
    def gtest_filter(self) -> str | None:
        return self.run.get('filter')

    @property
    def run_args(self) -> list:
        return self.run.get('args', [])

    @property
    def max_score(self) -> int:
        """Maximum possible score for this case (assertions that could pass)."""
        score = self.scoring['pass']
        score += len(self.expected.get('output_contains', [])) * self.scoring['output_contains']
        score += len(self.expected.get('output_not_contains', [])) * self.scoring['output_not_contains']
        return score


def _check_fields(block: dict, allowed: frozenset, where: str, path: Path) -> None:
    unknown = set(block) - allowed
    if unknown:
        raise EvalError(f"{path}: unknown field(s) in {where}: {sorted(unknown)}")


def _validate_expected(expected: dict, path: Path) -> None:
    _check_fields(expected, _EXPECTED, 'expected', path)
    for key in ('exit_code', 'failed', 'passed_min', 'skipped_max'):
        if key in expected and not isinstance(expected[key], (int, type(None))):
            raise EvalError(f"{path}: expected.{key} must be an int")
    for key in ('output_contains', 'output_not_contains'):
        if key in expected and not isinstance(expected[key], list):
            raise EvalError(f"{path}: expected.{key} must be a list")
    if 'metrics' in expected and not isinstance(expected['metrics'], dict):
        raise EvalError(f"{path}: expected.metrics must be a dict")


def load_case(path: Path) -> EvalCase:
    """Load and strictly validate one case YAML file. Raises EvalError."""
    if not path.exists():
        raise EvalError(f"case file not found: {path}")
    try:
        data = yaml.safe_load(path.read_text(encoding='utf-8'))
    except yaml.YAMLError as e:
        raise EvalError(f"{path}: invalid YAML: {e}") from e
    if not isinstance(data, dict):
        raise EvalError(f"{path}: case must be a YAML mapping")

    _check_fields(data, _TOP_LEVEL, 'case', path)
    for required in ('id', 'name', 'category', 'run', 'expected'):
        if required not in data:
            raise EvalError(f"{path}: missing required field '{required}'")
    if 'run' not in data or 'expected' not in data:
        raise EvalError(f"{path}: 'run' and 'expected' are required")

    run = data['run']
    _check_fields(run, _RUN, 'run', path)
    backend = run.get('backend', 'gtest')
    if backend not in _BACKENDS:
        raise EvalError(f"{path}: run.backend '{backend}' not supported (gtest only in MVP)")
    if 'binary' not in run:
        raise EvalError(f"{path}: run.binary is required for gtest backend")

    setup = data.get('setup', {})
    if setup:
        _check_fields(setup, _SETUP, 'setup', path)
    if data.get('scoring'):
        _check_fields(data['scoring'], _SCORING, 'scoring', path)

    difficulty = data.get('difficulty', 'basic')
    if difficulty not in _DIFFICULTY:
        raise EvalError(f"{path}: difficulty '{difficulty}' not in {sorted(_DIFFICULTY)}")

    _validate_expected(data['expected'], path)
    return EvalCase(path, data)


def load_cases_dir(directory: Path) -> list[EvalCase]:
    """Load all *.yaml cases under a directory (recursive), skipping _*.yaml suite files."""
    cases: list[EvalCase] = []
    if not directory.exists():
        return cases
    for path in sorted(directory.rglob('*.yaml')):
        if path.name.startswith('_'):
            continue  # suite files
        cases.append(load_case(path))
    return cases


def load_suite(suite_path: Path, cases_dir: Path) -> list[EvalCase]:
    """Load a suite file (flat list of case ids) and resolve to cases."""
    if not suite_path.exists():
        raise EvalError(f"suite not found: {suite_path}")
    data = yaml.safe_load(suite_path.read_text(encoding='utf-8'))
    if not isinstance(data, dict) or 'cases' not in data:
        raise EvalError(f"{suite_path}: suite must be {{cases: [id, ...]}}")
    ids = data['cases']
    by_id = {c.id: c for c in load_cases_dir(cases_dir)}
    missing = [i for i in ids if i not in by_id]
    if missing:
        raise EvalError(f"{suite_path}: case id(s) not found: {missing}")
    return [by_id[i] for i in ids]
