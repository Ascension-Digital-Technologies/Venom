# Limitations

Venom cannot make client-delivered code permanently secret. An analyst with control of the browser and enough time can observe inputs, outputs, memory, workers, WebAssembly, and host interactions.

Venom does not replace:

- server-side authorization;
- secure credential storage;
- TLS and deployment hardening;
- dependency and vulnerability management;
- code review and application security testing.

Best results come from protecting cohesive data-oriented logic and keeping authoritative secrets or irreversible decisions on a trusted server.
