# Venom TSX Showcase

A small typed UI application compiled through Venom's native TypeScript and JSX frontends. The browser-side `app.tsx` is lowered to classic `React.createElement` calls, linked with a local lightweight JSX runtime, and calls a protected TypeScript pricing module executed through TurboJS/WASM.

## Demonstrated features

- TSX function components and typed props
- Intrinsic elements, components, fragments, nested JSX and expression children
- String, boolean, expression and spread-compatible props
- Generic TypeScript DOM calls
- Relative browser-module linking
- Protected TypeScript imports from TSX
- JSON `<script>` data preserved as non-executable HTML content
- No `.ts` or `.tsx` source shipped in the distribution

## Run

Windows PowerShell from the repository root:

```powershell
.\scripts\windows\build-and-launch-tsx-showcase.bat
```

Windows PowerShell does not search the current directory for commands, so the
leading `.\` is required. From inside `scripts\windows`, run:

```powershell
.\build-and-launch-tsx-showcase.bat
```

Linux/macOS:

```bash
./scripts/linux/build-and-launch-tsx-showcase.sh
```

The launcher serves the example at `http://127.0.0.1:8084/`.

## JSX runtime

Venom emits classic JSX calls:

```js
React.createElement(type, props, ...children)
```

Projects may use React itself or provide a compatible local runtime, as this example does.
