#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
example = root / 'examples' / 'bot-detection'
browser_files = [example/'assets/js/fingerprint.js', example/'assets/js/app.js']
protected = example/'protected/bot-engine.js'
for path in browser_files+[protected]:
    if not path.is_file(): raise SystemExit(f'missing required file: {path}')
browser_text='\n'.join(p.read_text(encoding='utf-8') for p in browser_files)
protected_text=protected.read_text(encoding='utf-8')
for marker in ('COMMON_AUTOMATION_GLOBALS','scoreIntegrity','scoreCoherence','scoreBehavior','scoreTiming','scoreNetwork','uniform-event-cadence','uniform-pointer-speed','validateTelemetryEnvelope','SESSION_TTL_MS'):
    if marker in browser_text: raise SystemExit(f'browser-side bot policy leak: {marker}')
for marker in ('// @venom: protected module','export function beginAssessmentSession','validateTelemetryEnvelope','deriveTemporalBehavior','uniform-event-cadence','uniform-pointer-speed','export function assessClient'):
    if marker not in protected_text: raise SystemExit(f'protected marker missing: {marker}')
for marker in ('runtime.call("beginAssessmentSession"','schemaVersion: 2','buildTelemetryEnvelope','samples: state.samples.slice()'):
    if marker not in browser_text: raise SystemExit(f'telemetry transport marker missing: {marker}')
print('bot detection protection smoke: PASS')
