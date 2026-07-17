from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/compiler/pipeline/js.cpp").read_text(encoding="utf-8")
assert "batch:batchProtected" in source
assert "batching:'parallel-v1'" in source
assert "Protected batch exceeds the pending-call limit" in source
assert "Unknown protected export in batch" in source
print("batched protected calls smoke: PASS")
