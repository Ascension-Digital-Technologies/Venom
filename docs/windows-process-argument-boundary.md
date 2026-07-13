# Windows process argument boundary — v1.0.2

Venom launches external build tools directly and never through `cmd.exe` or a shell.

## Failure fixed

The former Windows `_spawnvp` path did not preserve an argument containing spaces in every environment. The remote-vendor user agent:

```text
Venom/1.0.1 remote-vendor
```

could reach `curl.exe` as two command-line tokens. Curl then treated `remote-vendor` as another URL, streamed the real script body to the console, and failed with `Could not resolve host: remote-vendor`.

## Current contract

The Windows runner now uses `CreateProcessW` and builds one mutable Unicode command line using the quoting contract consumed by `CommandLineToArgvW` and the Microsoft C runtime:

- empty arguments remain present;
- whitespace remains inside its original argument;
- embedded quotes are escaped;
- backslashes before quotes are doubled correctly;
- trailing backslashes survive a closing quote;
- executable and argument paths containing spaces remain one token;
- the child inherits the current environment;
- no command shell is involved;
- Venom waits for and returns the exact child exit code.

The process input is interpreted as UTF-8 first, with a Windows active-code-page fallback for legacy path sources.

## Regression coverage

The fake downloader verifies the exact user-agent value and rejects additional positional arguments. The remote-vendor suite also uses a cache path containing spaces. On Windows, these checks fail if argument boundaries regress.
