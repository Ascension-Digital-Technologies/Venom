# Getting Started

## Install or build Venom

Use an installable platform package when available, or build the repository with CMake. Confirm the toolchain:

```powershell
venom --version
venom doctor
```

Normal website users do not need Emscripten. Venom embeds the qualified QuickJS/WASM runtime and can install a versioned copy locally:

```powershell
venom runtime install
venom runtime verify
venom runtime path
```

## Create a project

```powershell
venom new my-site
cd my-site
venom dev . --open
```

The generated project separates browser and protected code:

```text
my-site/
├── index.html
├── assets/
│   ├── css/app.css
│   └── js/app.js
├── protected/engine.js
├── venom.toml
└── README.md
```

`protected/engine.js` is a typed protected module. `assets/js/app.js` imports its generated browser facade normally.

## Initialize an existing website

From the website root:

```powershell
venom init
venom config validate
venom dev . --open
```

`venom init` does not overwrite existing files unless `--force` is supplied.

## Build production output

```powershell
venom build . --profile prod --out dist
venom analyze-dist dist
venom verify-runtime dist --require-real-engine
```

Deploy `index.html` and `assets/` to an ordinary static web server.
