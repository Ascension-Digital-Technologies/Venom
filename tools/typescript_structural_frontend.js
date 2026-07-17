#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");

function fail(message, diagnostics = []) {
  process.stderr.write(JSON.stringify({ ok: false, message, diagnostics }) + "\n");
  process.exit(2);
}

if (process.argv.length !== 5) {
  fail("usage: typescript_structural_frontend.js <input> <output-js> <output-meta>");
}

const [inputPath, outputPath, metaPath] = process.argv.slice(2);
const root = path.resolve(__dirname, "..");
const ts = require(path.join(root, "third_party", "typescript", "lib", "typescript.js"));
const source = fs.readFileSync(inputPath, "utf8");
const filename = path.basename(inputPath).replace(/\.venom-input$/, "");
const isTsx = /\.tsx$/i.test(filename);

const compilerOptions = {
  target: ts.ScriptTarget.ES2022,
  module: ts.ModuleKind.ESNext,
  moduleResolution: ts.ModuleResolutionKind.Bundler,
  sourceMap: true,
    inlineSources: true,
    importsNotUsedAsValues: ts.ImportsNotUsedAsValues.Remove,
    preserveValueImports: false,
    useDefineForClassFields: true,
    isolatedModules: true,
    verbatimModuleSyntax: false,
  newLine: ts.NewLineKind.LineFeed
};
if (isTsx) compilerOptions.jsx = ts.JsxEmit.Preserve;
const result = ts.transpileModule(source, {
  fileName: filename,
  reportDiagnostics: true,
  compilerOptions
});

const diagnostics = (result.diagnostics || []).map((diagnostic) => {
  const message = ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n");
  let line = 0;
  let column = 0;
  if (diagnostic.file && typeof diagnostic.start === "number") {
    const pos = diagnostic.file.getLineAndCharacterOfPosition(diagnostic.start);
    line = pos.line + 1;
    column = pos.character + 1;
  }
  return { code: diagnostic.code, category: diagnostic.category, line, column, message };
});

const errors = diagnostics.filter((item) => item.category === ts.DiagnosticCategory.Error);
if (errors.length) fail("TypeScript structural lowering failed", diagnostics);

let javascript = result.outputText;
let sourceMap = result.sourceMapText || "";
const mapMatch = javascript.match(/\/\/# sourceMappingURL=([^\s]+)\s*$/);
if (mapMatch) {
  javascript = javascript.slice(0, mapMatch.index).replace(/\s+$/, "") + "\n";
}

fs.writeFileSync(outputPath, javascript, "utf8");
fs.writeFileSync(outputPath + ".map", sourceMap, "utf8");
fs.writeFileSync(metaPath, JSON.stringify({
  ok: true,
  frontend: "typescript-compiler-api",
  version: ts.version,
  tsx: isTsx,
  diagnostics
}), "utf8");
