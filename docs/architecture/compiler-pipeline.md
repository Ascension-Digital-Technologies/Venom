# Compiler pipeline

```text
site discovery
→ HTML/CSS/asset graph
→ JavaScript discovery and module graph
→ realm planning and extraction analysis
→ protected module/function rewriting
→ QuickJS bytecode compilation
→ route/package assembly
→ runtime metadata and integrity binding
→ production hardening and hashed output
→ release verification
```

The compiler keeps public headers organized under commands, core, pipeline, and services. Generated runtime sources live separately from handwritten compiler code.
