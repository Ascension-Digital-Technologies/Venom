// @venom: browser
export const examples = [
  {
    name: "Hello TurboJS",
    description: "Console output and a returned object.",
    code: `console.log("Hello from TurboJS/WASM");\nconst values = [3, 8, 13, 21];\nconsole.info("values", values);\n({ total: values.reduce((a, b) => a + b, 0), count: values.length });`
  },
  {
    name: "Prime numbers",
    description: "Compute primes without touching the browser runtime.",
    code: `function isPrime(value) {\n  if (value < 2) return false;\n  for (let i = 2; i * i <= value; i++) {\n    if (value % i === 0) return false;\n  }\n  return true;\n}\nconst primes = Array.from({ length: 100 }, (_, i) => i).filter(isPrime);\nconsole.log("primes", primes);\nprimes;`
  },
  {
    name: "Error diagnostics",
    description: "See a protected runtime stack trace.",
    code: `console.warn("about to throw");\nfunction explode() {\n  throw new TypeError("Demonstration failure");\n}\nexplode();`
  },
  {
    name: "JSON transformation",
    description: "Use the playground input object.",
    code: `console.log("input", input);\nconst base = Number(input.value || 0);\n({ original: base, doubled: base * 2, label: input.label || "unlabeled" });`
  }
];
