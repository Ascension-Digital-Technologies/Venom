(function (global) {
  "use strict";

  function normalizeDiagnostic(diagnostic) {
    var line = 0;
    var column = 0;
    if (diagnostic.file && typeof diagnostic.start === "number") {
      var position = diagnostic.file.getLineAndCharacterOfPosition(diagnostic.start);
      line = position.line + 1;
      column = position.character + 1;
    }
    return {
      code: diagnostic.code >>> 0,
      category: diagnostic.category >>> 0,
      line: line,
      column: column,
      message: ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n")
    };
  }

  global.__venomTranspileTypeScript = function (source, filename) {
    var isTsx = /\.tsx$/i.test(filename);
    var compilerOptions = {
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

    var result = ts.transpileModule(source, {
      fileName: filename,
      reportDiagnostics: true,
      compilerOptions: compilerOptions
    });

    var javascript = result.outputText || "";
    javascript = javascript.replace(/\n?\/\/# sourceMappingURL=[^\s]+\s*$/, "\n");
    return {
      javascript: javascript,
      sourceMap: result.sourceMapText || "",
      version: ts.version || "unknown",
      tsx: isTsx,
      diagnostics: (result.diagnostics || []).map(normalizeDiagnostic)
    };
  };
})(globalThis);
