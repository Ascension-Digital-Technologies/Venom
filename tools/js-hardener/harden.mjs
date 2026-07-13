import fs from 'node:fs';
import path from 'node:path';
let minify;
let JavaScriptObfuscator;
try {
  ({ minify } = await import('terser'));
  const obfuscatorModule = await import('javascript-obfuscator');
  JavaScriptObfuscator = obfuscatorModule.default ?? obfuscatorModule;
} catch (error) {
  console.error('[venom] JavaScript hardener dependencies are unavailable or corrupt.');
  console.error('[venom] Run scripts/setup-js-hardener.ps1 on Windows or scripts/setup-js-hardener.sh on Unix.');
  console.error(`[venom] ${error?.message ?? error}`);
  process.exit(3);
}

const [, , inputPath, outputPath, kind = 'runtime', seedText = '0'] = process.argv;
if (!inputPath || !outputPath) {
  console.error('usage: node harden.mjs <input> <output> [kind] [seed]');
  process.exit(2);
}
const source = fs.readFileSync(inputPath, 'utf8');
const bindingMatch = kind === 'loader' ? source.match(/bindingToken\s*:\s*['\"]([^'\"]+)['\"]/) : null;
const seed = Number.parseInt(seedText, 10) >>> 0;
const moduleKind = kind === 'loader' || kind === 'engine' || kind === 'runtime';
const minified = await minify(source, {
  module: moduleKind,
  compress: {
    passes: 3,
    booleans_as_integers: true,
    drop_console: false,
    keep_fargs: false,
    unsafe_arrows: true,
    unsafe_methods: true,
    unsafe_proto: false,
    unsafe_regexp: false
  },
  mangle: {
    toplevel: true,
    keep_classnames: false,
    keep_fnames: false
  },
  format: {
    comments: false,
    ascii_only: true,
    semicolons: true,
    wrap_iife: true
  }
});
if (!minified.code) throw new Error('terser produced no output');
const hardened = JavaScriptObfuscator.obfuscate(minified.code, {
  target: 'browser-no-eval',
  compact: true,
  seed,
  identifierNamesGenerator: 'hexadecimal',
  identifiersPrefix: '_v',
  renameGlobals: false,
  renameProperties: false,
  stringArray: true,
  stringArrayEncoding: ['rc4'],
  stringArrayThreshold: kind === 'runtime' ? 0.42 : 0.58,
  stringArrayCallsTransform: kind === 'loader',
  stringArrayCallsTransformThreshold: 0.35,
  stringArrayIndexesType: ['hexadecimal-number'],
  stringArrayRotate: true,
  stringArrayShuffle: true,
  stringArrayWrappersCount: 1,
  stringArrayWrappersChainedCalls: true,
  stringArrayWrappersParametersMaxCount: 4,
  stringArrayWrappersType: 'function',
  splitStrings: kind === 'loader',
  splitStringsChunkLength: 12,
  numbersToExpressions: kind !== 'worker',
  simplify: kind !== 'worker',
  transformObjectKeys: kind !== 'worker',
  unicodeEscapeSequence: false,
  controlFlowFlattening: kind === 'loader',
  controlFlowFlatteningThreshold: kind === 'loader' ? 0.22 : 0.12,
  deadCodeInjection: kind === 'loader',
  deadCodeInjectionThreshold: kind === 'loader' ? 0.035 : 0.02,
  selfDefending: kind !== 'worker',
  debugProtection: false,
  disableConsoleOutput: false,
  sourceMap: false
}).getObfuscatedCode();
fs.mkdirSync(path.dirname(outputPath), { recursive: true });
const bindingMarker = bindingMatch ? `;void\"vbind:${bindingMatch[1]}\";` : '';
fs.writeFileSync(outputPath, hardened + bindingMarker, 'utf8');
