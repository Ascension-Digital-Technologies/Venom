# Deployment

Venom production output is static and can be hosted by an ordinary HTTP server, CDN, object-storage website, or reverse proxy.

## Required server behavior

- Serve `.wasm` as `application/wasm`.
- Serve JavaScript with a JavaScript MIME type.
- Do not rewrite hashed runtime assets to HTML.
- Preserve immutable hashed filenames.
- Use HTTPS in production.
- Configure SPA route fallback only for application routes, not asset paths.

## Recommended headers

- `Content-Security-Policy` tailored to the application
- `X-Content-Type-Options: nosniff`
- `Referrer-Policy`
- `Permissions-Policy`
- long-lived immutable caching for hashed assets
- short caching for `index.html` and release metadata

Always run `venom verify dist` before deployment.
