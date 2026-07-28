"""Consistent subprocess execution for Venom's Python controllers."""
from __future__ import annotations

import os
import shutil
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


def _resolve_command(argv: tuple[str, ...], env: Mapping[str, str] | None) -> tuple[str, ...]:
    """Resolve command shims such as npm.cmd without invoking a shell."""
    if not argv or os.path.dirname(argv[0]) or Path(argv[0]).is_absolute():
        return argv
    search_path = env.get("PATH") if env is not None else None
    resolved = shutil.which(argv[0], path=search_path)
    if resolved is None and os.name == "nt" and not Path(argv[0]).suffix:
        # Some Python/Windows combinations do not apply PATHEXT consistently.
        for extension in (".exe", ".cmd", ".bat", ".com"):
            resolved = shutil.which(argv[0] + extension, path=search_path)
            if resolved is not None:
                break
    return (resolved, *argv[1:]) if resolved is not None else argv


def run_command(
    command: Sequence[object],
    *,
    cwd: Path,
    timeout: int | float | None = None,
    env: Mapping[str, str] | None = None,
    input_bytes: bytes | None = None,
    stream: bool = False,
) -> CommandResult:
    argv = _resolve_command(tuple(str(value) for value in command), env)
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
    except FileNotFoundError as exc:
        return CommandResult(
            command=argv,
            returncode=127,
            duration_ms=round((time.perf_counter() - started) * 1000),
            output=f"command not found: {argv[0]} ({exc})",
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
