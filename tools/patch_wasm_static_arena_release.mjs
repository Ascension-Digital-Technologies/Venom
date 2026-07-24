#!/usr/bin/env node
import fs from 'node:fs';

const [inputPath, outputPath, exportName = 'venom_tjs_script_buffer_free'] = process.argv.slice(2);
if (!inputPath || !outputPath) {
  throw new Error('usage: patch_wasm_static_arena_release.mjs INPUT OUTPUT [EXPORT]');
}

const bytes = new Uint8Array(fs.readFileSync(inputPath));
if (bytes.length < 8 || String.fromCharCode(...bytes.subarray(0, 4)) !== '\0asm') {
  throw new Error('input is not a WebAssembly module');
}

function uleb(offset) {
  let value = 0;
  let shift = 0;
  let cursor = offset;
  for (;;) {
    if (cursor >= bytes.length || shift > 35) throw new Error('invalid LEB128 value');
    const byte = bytes[cursor++];
    value |= (byte & 0x7f) << shift;
    if (!(byte & 0x80)) return { value: value >>> 0, next: cursor };
    shift += 7;
  }
}

function stringAt(offset) {
  const length = uleb(offset);
  const end = length.next + length.value;
  if (end > bytes.length) throw new Error('invalid string range');
  return { value: new TextDecoder().decode(bytes.subarray(length.next, end)), next: end };
}

let importedFunctions = 0;
let exportedFunction = -1;
let codeStart = -1;
let codeEnd = -1;
let cursor = 8;
while (cursor < bytes.length) {
  const id = bytes[cursor++];
  const size = uleb(cursor);
  const start = size.next;
  const end = start + size.value;
  if (end > bytes.length) throw new Error('invalid section range');
  if (id === 2) {
    let p = start;
    const count = uleb(p); p = count.next;
    for (let i = 0; i < count.value; ++i) {
      const moduleName = stringAt(p); p = moduleName.next;
      const fieldName = stringAt(p); p = fieldName.next;
      const kind = bytes[p++];
      if (kind === 0) { const type = uleb(p); p = type.next; ++importedFunctions; }
      else if (kind === 1) { p++; const flags = uleb(p); p = flags.next; const initial = uleb(p); p = initial.next; if (flags.value & 1) { const maximum = uleb(p); p = maximum.next; } }
      else if (kind === 2) { const flags = uleb(p); p = flags.next; const initial = uleb(p); p = initial.next; if (flags.value & 1) { const maximum = uleb(p); p = maximum.next; } }
      else if (kind === 3) { p += 2; }
      else throw new Error('unsupported import kind');
    }
  } else if (id === 7) {
    let p = start;
    const count = uleb(p); p = count.next;
    for (let i = 0; i < count.value; ++i) {
      const name = stringAt(p); p = name.next;
      const kind = bytes[p++];
      const index = uleb(p); p = index.next;
      if (kind === 0 && name.value === exportName) exportedFunction = index.value;
    }
  } else if (id === 10) {
    codeStart = start;
    codeEnd = end;
  }
  cursor = end;
}

if (exportedFunction < importedFunctions || codeStart < 0) throw new Error(`function export not found: ${exportName}`);
const localIndex = exportedFunction - importedFunctions;
let p = codeStart;
const bodyCount = uleb(p); p = bodyCount.next;
if (localIndex >= bodyCount.value) throw new Error('exported function has no code body');
let bodyStart = -1;
let bodyEnd = -1;
for (let i = 0; i < bodyCount.value; ++i) {
  const bodySize = uleb(p);
  const start = bodySize.next;
  const end = start + bodySize.value;
  if (end > codeEnd) throw new Error('invalid function body range');
  if (i === localIndex) { bodyStart = start; bodyEnd = end; break; }
  p = end;
}

let instructionStart = bodyStart;
const localGroups = uleb(instructionStart); instructionStart = localGroups.next;
for (let i = 0; i < localGroups.value; ++i) {
  const count = uleb(instructionStart); instructionStart = count.next;
  if (instructionStart >= bodyEnd) throw new Error('invalid local declaration');
  instructionStart++;
}
if (bodyEnd - instructionStart < 3) throw new Error('function body is too small to patch');

// i32.const 1; nops; end.  Preserve the original body and section lengths so
// every custom/provenance section remains byte-for-byte positioned.
bytes[instructionStart] = 0x41;
bytes[instructionStart + 1] = 0x01;
bytes.fill(0x01, instructionStart + 2, bodyEnd - 1);
bytes[bodyEnd - 1] = 0x0b;

fs.writeFileSync(outputPath, bytes);
const module = new WebAssembly.Module(bytes);
const exported = WebAssembly.Module.exports(module).some((item) => item.kind === 'function' && item.name === exportName);
if (!exported) throw new Error('patched module lost the target export');
console.log(`patched ${exportName} as a constant-success compatibility no-op`);
