# HTML script classification

Venom classifies every HTML `script` element before JavaScript discovery.

Executable script types are:

- an omitted or empty `type` attribute;
- `type="module"`;
- standard JavaScript and ECMAScript MIME types.

Non-executable data blocks such as `application/json`, `application/ld+json`, `importmap`, `speculationrules`, `text/plain`, templates, and unknown MIME types are not parsed, compiled, protected, or added to the JavaScript module graph. Their raw text is preserved when the script element participates in a compiled HTML route.

Tag boundaries are scanned with quote awareness, so `>` inside an attribute value does not prematurely terminate a script start tag.
