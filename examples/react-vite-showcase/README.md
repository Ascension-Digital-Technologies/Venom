# Venom React + Vite showcase

A React and TypeScript application compiled by Vite, then ingested and protected by Venom.

```bash
npm install
npm run build
```

Vite writes its normal production output to `dist/`. The Venom Vite plugin then protects that compiled output and writes the sealed distribution to `dist-venom/`. JSX/TSX, React Fast Refresh, package resolution, CSS imports, and chunk generation remain Vite responsibilities; Venom protects the final browser-ready graph.
