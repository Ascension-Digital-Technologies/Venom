#!/usr/bin/env python3
"""Node.js oracle for the accepted signed-int32 JavaScript grammar subset.

Only generated trusted source is executed by Node. Do not feed untrusted JS to
this test harness: the oracle is a real JavaScript evaluator.
"""
import json
import random
import subprocess
import sys


def lit(n: int) -> str:
    return str(n) if n >= 0 else f"({n})"


def cases():
    rng = random.Random(0x560005)
    examples = [
        "function main(){ return (1 + 2 * 3) ^ 5; }",
        "function main(){ return 7===7; }",
        "function main(){ return 7!==8; }",
        "function main(){ return (2>1)===true; }",
        "function main(){ let n=3; while(n>0) { n=n-1; } return n; }",
        "function main(){ let n=3; while(n>0) { if(n>1){n=n-1;} else {n=n-1;} } return n; }",
    ]
    for n in range(720):
        a = rng.randrange(-200, 201)
        b = rng.randrange(-200, 201)
        c = rng.randrange(0, 41)
        d = rng.randrange(0, 41)
        op = rng.choice(["+", "-", "*", "^", "<", "<=", ">", ">=", "===", "!=="])
        form = n % 6
        if form == 0:
            src = f"function main(){{ let a={lit(a)};let b={lit(b)};return a {op} b; }}"
        elif form == 1:
            src = f"function main(){{ let a={lit(a)};let b={lit(b)};if(a {op} b){{return 71;}}else{{return 37;}} }}"
        elif form == 2:
            src = f"function main(){{let n={c};let s=0;while(n<{d}){{s=s+3;n=n+1;}}return s;}}"
        elif form == 3:
            src = f"function main(){{let n={c};let s=0;while(n>0){{s=s+n;n=n-1;}}return s;}}"
        elif form == 4:
            src = f"function main(){{ let a={lit(a)}; let b={lit(b)};let q=a^b; if(q<0){{q=q+13;}}else{{q=q-11;}}return q;}}"
        else:
            src = f"function main(){{ const a={lit(a)};let q={lit(b)};if((a===q)===false){{q=q+12;}}else{{q=q-12;}}return q;}}"
        examples.append(src)
    return examples


def main(exe: str) -> None:
    scripts = cases()
    node_oracle = r"""
const fs = require('node:fs');
const cases = JSON.parse(fs.readFileSync(0, 'utf8'));
const result = cases.map(src => {
  const result = Function(src + ';return main();')();
  if (typeof result !== 'number' && typeof result !== 'boolean') throw Error('Unexpected JS value');
  if (typeof result === 'number' && (!Number.isInteger(result) || result < -2147483648 || result > 2147483647))
    throw Error('Oracle out of int32 range');
  return (Number(result) >>> 0);
});
process.stdout.write(JSON.stringify(result));
"""
    js = subprocess.run(["node", "-e", node_oracle], input=json.dumps(scripts),
                        text=True, capture_output=True, check=True)
    reference = json.loads(js.stdout)
    for index, (script, expected) in enumerate(zip(scripts, reference)):
        actual = subprocess.run([exe, str(index+200)], input=script,
                                text=True, capture_output=True)
        if actual.returncode:
            raise AssertionError(f"Case {index} rejected: {actual.stderr}\n{script}")
        if int(actual.stdout.strip()) != expected:
            raise AssertionError(f"Case {index} mismatch: VM={actual.stdout} Node={expected}\n{script}")
    print(f"V4-M5 Node.js oracle: {len(scripts)} source programs PASS")


if __name__ == "__main__":
    main(sys.argv[1])
