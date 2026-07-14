# Trust boundaries

## Trusted release inputs

Compiler source, pinned toolchains, runtime source, signing keys, and release workflow configuration.

## Untrusted environment

The browser owner, browser extensions, local storage, network responses not bound by the release, and arbitrary hosting configuration.

## Important consequence

Venom provides reverse-engineering resistance and tamper evidence—not permanent secrecy against a fully controlling local analyst. Credentials and server-authoritative decisions must remain server-side.
