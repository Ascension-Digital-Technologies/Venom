"""Consistent subprocess execution for Venom's Python controllers."""
from __future__ import annotations

import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping, Sequence


@dataclass(frozen=True)
class CommandResult:
    command: tuple[str, ...]
    returncode: int
    duration_ms: int
    output: str

    @property
    def passed(self) -> bool:
        return self.returncode == 0


def run_command(
    command: Sequence[object],
    *,
    cwd: Path,
    timeout: int | float | None = None,
    env: Mapping[str, str] | None = None,
    input_bytes: bytes | None = None,
    stream: bool = False,
) -> CommandResult:
    argv = tuple(str(value) for value in command)
    started = time.perf_counter()
    try:
        if stream:
            completed = subprocess.run(argv, cwd=cwd, env=env, timeout=timeout)
            output = ""
        else:
            completed = subprocess.run(
                argv,
                cwd=cwd,
                env=env,
                input=input_bytes,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=timeout,
            )
            output = completed.stdout.decode("utf-8", errors="replace") if completed.stdout else ""
        return CommandResult(
            command=argv,
            returncode=completed.returncode,
            duration_ms=round((time.perf_counter() - started) * 1000),
            output=output,
        )
    except subprocess.TimeoutExpired as exc:
        output = ""
        if exc.stdout:
            output += exc.stdout.decode("utf-8", errors="replace") if isinstance(exc.stdout, bytes) else exc.stdout
        if exc.stderr:
            output += exc.stderr.decode("utf-8", errors="replace") if isinstance(exc.stderr, bytes) else exc.stderr
        if output and not output.endswith("\n"):
            output += "\n"
        output += "step timed out"
        return CommandResult(
            command=argv,
            returncode=124,
            duration_ms=round((time.perf_counter() - started) * 1000),
            output=output,
        )
