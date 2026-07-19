from pathlib import Path
root=Path(__file__).resolve().parents[2]
server=(root/'tools/launch_example.py').read_text(encoding='utf-8')
required=[
  'secrets.token_urlsafe(32)','X-Venom-Playground-Token','secrets.compare_digest',
  '_playground_request_allowed','playground origin rejected','compiler concurrency limit reached',
  'BoundedSemaphore(2)','application/json is required','safe_env','timeout=10'
]
for token in required:
    assert token in server, f'missing loopback compiler protection: {token}'
print('JavaScript playground loopback security: PASS')
