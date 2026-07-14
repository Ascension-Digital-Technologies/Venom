# Threat model

Venom is designed against casual source copying, browser-source inspection, generic deobfuscation, static package scraping, low-effort modification, and reuse of one stable extraction layout across releases.

A determined analyst controlling the browser can instrument workers, WebAssembly, memory, and host calls. Venom aims to make that work specialized, multi-layered, and increasingly build-specific—not impossible.
