# Fixture-driven compatibility

Venom uses application-owned `venom.browser.json` manifests to describe observable browser behavior. Each manifest has a stable fixture identifier and one or more route scenarios containing waits, actions, and assertions. The generic Playwright runner executes the same manifest in Chromium, Firefox, and WebKit.

Reports bind together the fixture identifier, manifest SHA-256, and complete distribution SHA-256. This makes the result specific to both the behavioral contract and the exact files that were tested.

Supported assertion types currently include element existence, exact or partial text, attributes, classes, and URL paths. Supported actions include click and fill. New assertion types should remain deterministic and browser-neutral.

A passing fixture is evidence for the scenarios declared by that fixture only. It does not justify broad framework compatibility claims unless representative production bundles and user journeys are included and continuously passing.
