# Windows Emscripten invocation

The controller invokes the resolved compiler directly. It never preserves quote characters as part of the executable name. PowerShell checks `$LASTEXITCODE` after each external command.
