"""Canonical public-example registry used by launch, certification, docs, and CI."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from .io import read_json

EXPECTED_SCHEMA = "VENOM_EXAMPLE_REGISTRY_V2"


@dataclass(frozen=True)
class ExampleSpec:
    id: str
    display_name: str
    path: str
    launcher: str
    aliases: tuple[str, ...]
    port: int
    kind: str
    certify: bool
    requires_real_turbojs: bool
    leak_scan: bool
    browser: bool
    requires_loopback_compiler: bool
    requires_vite: bool

    @property
    def directory(self) -> str:
        return Path(self.path).name

    @property
    def launch_names(self) -> tuple[str, ...]:
        return (self.launcher, *self.aliases)

    @property
    def chrome_extension(self) -> bool:
        return self.kind == "chrome-extension"

    @property
    def playground(self) -> bool:
        return self.requires_loopback_compiler


@dataclass(frozen=True)
class ExampleRegistry:
    path: Path
    build_profile: str
    examples: tuple[ExampleSpec, ...]

    def lookup(self) -> dict[str, ExampleSpec]:
        return {
            name: spec
            for spec in self.examples
            for name in spec.launch_names
        }

    def certifiable(self) -> tuple[ExampleSpec, ...]:
        return tuple(spec for spec in self.examples if spec.certify)

    def launcher_names(self) -> tuple[str, ...]:
        return tuple(spec.launcher for spec in self.examples)


def _required_string(record: dict, key: str, context: str) -> str:
    value = record.get(key)
    if not isinstance(value, str) or not value.strip():
        raise ValueError(f"{context}.{key} must be a non-empty string")
    return value


def _boolean(record: dict, key: str, default: bool = False) -> bool:
    value = record.get(key, default)
    if not isinstance(value, bool):
        raise ValueError(f"example.{key} must be true or false")
    return value


def _validate_unique(values: Iterable[str], label: str) -> None:
    seen: set[str] = set()
    duplicates: set[str] = set()
    for value in values:
        if value in seen:
            duplicates.add(value)
        seen.add(value)
    if duplicates:
        raise ValueError(f"duplicate {label}: {', '.join(sorted(duplicates))}")


def load_example_registry(repo_root: Path, contract: Path | None = None) -> ExampleRegistry:
    root = repo_root.resolve()
    path = (contract or root / "contracts" / "examples.json").resolve()
    payload = read_json(path)
    if payload.get("schema") != EXPECTED_SCHEMA:
        raise ValueError(f"unsupported example registry schema in {path}: {payload.get('schema')!r}")
    if payload.get("version") != 2:
        raise ValueError(f"unsupported example registry version in {path}: {payload.get('version')!r}")
    profile = payload.get("buildProfile")
    if profile != "prod":
        raise ValueError("example registry buildProfile must be 'prod'")
    raw_examples = payload.get("examples")
    if not isinstance(raw_examples, list) or not raw_examples:
        raise ValueError("example registry must contain at least one example")

    examples: list[ExampleSpec] = []
    for index, record in enumerate(raw_examples, start=1):
        if not isinstance(record, dict):
            raise ValueError(f"examples[{index}] must be an object")
        context = f"examples[{index}]"
        aliases_raw = record.get("aliases", [])
        if not isinstance(aliases_raw, list) or not all(isinstance(value, str) and value for value in aliases_raw):
            raise ValueError(f"{context}.aliases must be an array of non-empty strings")
        port = record.get("port")
        if not isinstance(port, int) or port < 0 or port > 65535:
            raise ValueError(f"{context}.port must be from 0 to 65535")
        kind = _required_string(record, "kind", context)
        if kind not in {"browser", "chrome-extension"}:
            raise ValueError(f"{context}.kind must be browser or chrome-extension")
        if kind == "chrome-extension" and port != 0:
            raise ValueError(f"{context}.port must be 0 for a Chrome extension")
        if kind == "browser" and port == 0:
            raise ValueError(f"{context}.port must be non-zero for a browser example")
        examples.append(ExampleSpec(
            id=_required_string(record, "id", context),
            display_name=_required_string(record, "displayName", context),
            path=_required_string(record, "path", context),
            launcher=_required_string(record, "launcher", context),
            aliases=tuple(aliases_raw),
            port=port,
            kind=kind,
            certify=_boolean(record, "certify", True),
            requires_real_turbojs=_boolean(record, "requiresRealTurboJs", True),
            leak_scan=_boolean(record, "leakScan", True),
            browser=_boolean(record, "browser", kind == "browser"),
            requires_loopback_compiler=_boolean(record, "requiresLoopbackCompiler"),
            requires_vite=_boolean(record, "requiresVite"),
        ))

    _validate_unique((spec.id for spec in examples), "example ids")
    _validate_unique((spec.path for spec in examples), "example paths")
    _validate_unique((spec.launcher for spec in examples), "launcher names")
    _validate_unique((name for spec in examples for name in spec.launch_names), "launcher names and aliases")
    _validate_unique((str(spec.port) for spec in examples if spec.port), "browser ports")
    return ExampleRegistry(path=path, build_profile=profile, examples=tuple(examples))
