#!/usr/bin/env python3
"""Differential oracle for the V4-M8 source-direct typed SSA subset.

All inputs are deterministic generator-produced JavaScript, not untrusted code.
The Node runtime evaluates these test functions only. The test compares the
uint32 representation of the signed-int32 / Boolean result against the VM.
"""
import json
import random
import subprocess
import sys


def lit(n: int) -> str:
    return str(n) if n >= 0 else f"({n})"


def cases():
    randomizer = random.Random(0x4D380088)
    examples = [
        "function main(){return 42;}",
        "function main(){const a=7;let b=3;b=b+a;return b*2;}",
        "function main(){return true === !false;}",
        "function main(){return (5+3*2)^7;}",
        "function main(){let n=1;n=n+1;n=n+3;return n;}",
    ]
    operations = ["+", "-", "*", "^", "<", "<=", ">", ">=", "===", "!=="]
    for i in range(1200):
        a, b, c = [randomizer.randrange(-120, 121) for _ in range(3)]
        op = operations[i % len(operations)]
        mode = i % 8
        if mode == 0:
            src = f"function main(){{let a={lit(a)};let b={lit(b)};return a {op} b;}}"
        elif mode == 1:
            src = f"function main(){{const x={lit(a)};let y={lit(b)};y=y+{lit(c)};return x+y;}}"
        elif mode == 2:
            src = f"function main(){{let a={lit(a)};a=a*{lit(c)};return a-{lit(b)};}}"
        elif mode == 3:
            src = f"function main(){{let a={lit(a)};let b={lit(b)};return (a<b) === (a>=b);}}"
        elif mode == 4:
            src = f"function main(){{let a={lit(a)};return !((a<0)===false);}}"
        elif mode == 5:
            src = f"function main(){{const a={lit(a)};const b={lit(b)};return (a ^ b) + {lit(c)};}}"
        elif mode == 6:
            src = f"function main(){{let a={lit(a)};let b={lit(b)};a=b;b={lit(c)};return a-b;}}"
        else:
            src = f"function main(){{return ({lit(a)} + {lit(b)}) * {lit(c)};}}"
        examples.append(src)
    return examples


def main(executable: str) -> None:
    source_cases = cases()
    oracle_script = r"""
const fs = require('node:fs');
const cases = JSON.parse(fs.readFileSync(0, 'utf8'));
const answers = cases.map(source => {
  const value = Function(source + ';return main();')();
  if (typeof value !== 'number' && typeof value !== 'boolean')
    throw Error('Unexpected JavaScript return type');
  if (typeof value === 'number' && (!Number.isInteger(value) ||
      value > 2147483647 || value < -2147483648))
    throw Error('Oracle outside signed int32');
  return (Number(value) >>> 0);
});
process.stdout.write(JSON.stringify(answers));
"""
    oracle = subprocess.run(["node", "-e", oracle_script],
                            input=json.dumps(source_cases),
                            text=True, capture_output=True, check=True)
    answers = json.loads(oracle.stdout)
    if len(answers) != len(source_cases):
        raise AssertionError("Incomplete oracle answers")
    for i, (source, expected) in enumerate(zip(source_cases, answers)):
        run = subprocess.run([executable, str(i + 7001)],
                             input=source, text=True, capture_output=True)
        if run.returncode != 0:
            raise AssertionError(f"Case {i} VM rejected:\n{run.stderr}\n{source}")
        got = int(run.stdout.strip())
        if got != expected:
            raise AssertionError(f"Case {i}: Node={expected}, VM={got}\n{source}")
    print(f"V4-M8 typed SSA vs Node.js: {len(answers)} cases PASS")


if __name__ == "__main__":
    main(sys.argv[1])
