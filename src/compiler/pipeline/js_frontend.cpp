#include "compiler/pipeline/js_frontend.hpp"
#include "compiler/core/diagnostic.hpp"

#include "quickjs.h"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace venom::compiler::frontend {
namespace {

#include "compiler/pipeline/embedded_js_hardener_bundles.inc"

std::string value_text(JSContext* ctx, JSValueConst value) {
  const char* text = JS_ToCString(ctx, value);
  if (!text) return "<unprintable QuickJS value>";
  std::string result(text);
  JS_FreeCString(ctx, text);
  return result;
}

std::string exception_text(JSContext* ctx) {
  JSValue exception = JS_GetException(ctx);
  std::string result = value_text(ctx, exception);
  JSValue stack = JS_GetPropertyStr(ctx, exception, "stack");
  if (!JS_IsUndefined(stack) && !JS_IsNull(stack)) {
    const auto stack_text = value_text(ctx, stack);
    if (!stack_text.empty() && stack_text != result) result += "\n" + stack_text;
  }
  JS_FreeValue(ctx, stack);
  JS_FreeValue(ctx, exception);
  return result;
}

void eval_checked(JSContext* ctx, const char* source, std::size_t size, const char* filename) {
  JSValue value = JS_Eval(ctx, source, size, filename, JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    const auto error = exception_text(ctx);
    JS_FreeValue(ctx, value);
    throw std::runtime_error(std::string("JavaScript frontend failed while loading ") + filename + ": " + error);
  }
  JS_FreeValue(ctx, value);
}

void set_global_string(JSContext* ctx, const char* name, const std::string& value) {
  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, name, JS_NewStringLen(ctx, value.data(), value.size()));
  JS_FreeValue(ctx, global);
}

void set_global_bool(JSContext* ctx, const char* name, bool value) {
  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, name, JS_NewBool(ctx, value));
  JS_FreeValue(ctx, global);
}

std::uint32_t get_u32(JSContext* ctx, JSValueConst object, const char* name) {
  JSValue value = JS_GetPropertyStr(ctx, object, name);
  std::uint32_t out = 0;
  if (JS_ToUint32(ctx, &out, value) < 0) {
    JS_FreeValue(ctx, value);
    throw std::runtime_error(std::string("JavaScript frontend returned an invalid ") + name);
  }
  JS_FreeValue(ctx, value);
  return out;
}

std::string get_string(JSContext* ctx, JSValueConst object, const char* name) {
  JSValue value = JS_GetPropertyStr(ctx, object, name);
  const auto out = value_text(ctx, value);
  JS_FreeValue(ctx, value);
  return out;
}

bool get_bool(JSContext* ctx, JSValueConst object, const char* name) {
  JSValue value = JS_GetPropertyStr(ctx, object, name);
  const bool out = JS_ToBool(ctx, value) > 0;
  JS_FreeValue(ctx, value);
  return out;
}

struct FrontendError {
  std::string message;
  std::size_t line = 0;
  std::size_t column = 0;
};

std::string line_text(const std::string& source, std::size_t line) {
  if (line == 0) return {};
  std::size_t begin = 0;
  for (std::size_t current = 1; current < line; ++current) {
    begin = source.find('\n', begin);
    if (begin == std::string::npos) return {};
    ++begin;
  }
  const auto end = source.find('\n', begin);
  return source.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
}

FrontendError get_frontend_error(JSContext* ctx, JSValueConst value) {
  FrontendError out;
  if (JS_IsObject(value)) {
    out.message = get_string(ctx, value, "message");
    out.line = get_u32(ctx, value, "line");
    out.column = get_u32(ctx, value, "col");
  } else {
    out.message = value_text(ctx, value);
  }
  return out;
}

std::vector<std::string> get_string_array(JSContext* ctx, JSValueConst object, const char* name) {
  JSValue array = JS_GetPropertyStr(ctx, object, name);
  const auto length = get_u32(ctx, array, "length");
  std::vector<std::string> out;
  out.reserve(length);
  for (std::uint32_t i = 0; i < length; ++i) {
    JSValue item = JS_GetPropertyUint32(ctx, array, i);
    out.push_back(value_text(ctx, item));
    JS_FreeValue(ctx, item);
  }
  JS_FreeValue(ctx, array);
  return out;
}

const char kBootstrap[] = R"JS(
globalThis.self = globalThis;
globalThis.window = globalThis;
globalThis.__venom_frontend_result = undefined;
globalThis.__venom_frontend_error = undefined;
)JS";

const char kRunFrontend[] = R"JS(
(() => {
  try {
    const source = globalThis.__venom_frontend_source;
    const filename = globalThis.__venom_frontend_filename;
    const isModule = !!globalThis.__venom_frontend_module;
    const ast = globalThis.Terser.__venom_parse(source, { module: isModule, filename });
    ast.figure_out_scope({ module: isModule });
    const refs = [];
    const moduleImports = [];
    const exportBindings = [];
    const exportedFunctions = [];
    const unsupportedExports = [];
    const moduleFeatures = [];
    const topLevelFunctions = [];
    let nodes = 0, functions = 0, declarations = 0, imports = 0, exports = 0, lexicalScopes = 0;
    const tokenEnd = token => {
      if (!token || typeof token.pos !== 'number') return 0;
      const value = String(token.value == null ? '' : token.value);
      return token.pos + (token.type === 'string' ? value.length + 2 : Math.max(1, value.length));
    };
    const location = node => ({
      line: ((node.start && node.start.line) || 1),
      col: ((node.start && node.start.col) || 0)
    });
    const importClause = node => {
      const parts = [];
      if (node.imported_name && typeof node.imported_name.name === 'string') parts.push(node.imported_name.name);
      const mappings = Array.isArray(node.imported_names) ? node.imported_names : [];
      if (mappings.length) {
        const namespace = mappings.find(mapping => mapping.foreign_name && mapping.foreign_name.name === '*');
        if (namespace) {
          parts.push('* as ' + namespace.name.name);
        } else {
          parts.push('{' + mappings.map(mapping => {
            const foreignName = mapping.foreign_name && mapping.foreign_name.name;
            const localName = mapping.name && mapping.name.name;
            return foreignName === localName ? foreignName : foreignName + ' as ' + localName;
          }).join(',') + '}');
        }
      }
      return parts.join(', ');
    };
    ast.walk(new globalThis.Terser.__venom_tree_walker(function(node) {
      nodes++;
      const type = String(node.TYPE || '');
      if (node.variables instanceof Map && Array.isArray(node.enclosed)) lexicalScopes++;
      if (type === 'Defun' || type === 'Function' || type === 'Arrow' || type === 'Accessor') functions++;
      if (type === 'Defun' || type === 'DefClass' || type === 'Var' || type === 'Let' || type === 'Const') declarations++;
      if (type === 'Import') {
        imports++;
        if (node.module_name && typeof node.module_name.value === 'string') {
          const loc = location(node);
          refs.push({specifier: node.module_name.value, dynamic: false, line: loc.line, col: loc.col});
          const bindings = [];
          if (node.imported_name && typeof node.imported_name.name === 'string') {
            bindings.push({ localName: node.imported_name.name, importedName: 'default', namespaceImport: false });
          }
          for (const mapping of (Array.isArray(node.imported_names) ? node.imported_names : [])) {
            const localName = mapping.name && mapping.name.name;
            const importedName = mapping.foreign_name && mapping.foreign_name.name;
            if (localName && importedName) bindings.push({ localName, importedName, namespaceImport: importedName === '*' });
          }
          moduleImports.push({
            specifier: node.module_name.value,
            clause: importClause(node),
            sideEffectOnly: !node.imported_name && (!node.imported_names || node.imported_names.length === 0),
            hasDefault: !!node.imported_name,
            bindings,
            begin: node.start.pos,
            end: tokenEnd(node.end),
            line: loc.line,
            col: loc.col
          });
        }
      } else if (type === 'Export') {
        exports++;
        const loc = location(node);
        const specifier = node.module_name && typeof node.module_name.value === 'string' ? node.module_name.value : '';
        if (specifier) refs.push({specifier, dynamic: false, line: loc.line, col: loc.col});
        const definition = node.exported_definition;
        const definitionType = String(definition && definition.TYPE || '');
        if (definition && definition.name && definition.name.name) {
          const localName = String(definition.name.name);
          exportBindings.push({ exportedName: node.is_default ? 'default' : localName, localName,
            specifier: '', exportAll: false, namespaceExport: false, defaultExport: !!node.is_default,
            line: loc.line, col: loc.col });
        }
        if (node.is_default && node.exported_value && String(node.exported_value.TYPE || '') === 'SymbolRef') {
          exportBindings.push({ exportedName: 'default', localName: String(node.exported_value.name || ''),
            specifier: '', exportAll: false, namespaceExport: false, defaultExport: true,
            line: loc.line, col: loc.col });
        }
        for (const mapping of (Array.isArray(node.exported_names) ? node.exported_names : [])) {
          const localName = mapping.name && mapping.name.name ? String(mapping.name.name) : '';
          const exportedName = mapping.foreign_name && mapping.foreign_name.name ? String(mapping.foreign_name.name) : localName;
          const exportAll = localName === '*' && exportedName === '*';
          const namespaceExport = localName === '*' && exportedName !== '*';
          exportBindings.push({ exportedName, localName, specifier, exportAll, namespaceExport,
            defaultExport: exportedName === 'default', line: loc.line, col: loc.col });
        }
        if (definitionType === 'Defun' && definition.name && definition.name.name) {
          exportedFunctions.push({
            name: definition.name.name,
            parameters: (definition.argnames || []).map(arg => arg.print_to_string()).join(','),
            async: !!definition.async,
            exportBegin: node.start.pos,
            declarationBegin: definition.start.pos,
            line: loc.line,
            col: loc.col
          });
        } else if (!definition && (!node.exported_names || node.exported_names.length === 0) &&
                   !(node.is_default && node.exported_value && String(node.exported_value.TYPE || '') === 'SymbolRef')) {
          let kind = node.is_default ? 'default-expression' : (node.module_name ? 're-export' : 'unknown');
          unsupportedExports.push({kind, line: loc.line, col: loc.col});
        }
      } else if (type === 'DynamicImport' || (type === 'Call' && node.expression && String(node.expression.TYPE || '') === 'SymbolRef' && node.expression.name === 'import')) {
        const arg = node.args && node.args[0];
        const loc = location(node);
        if (arg && String(arg.TYPE || '') === 'String') {
          refs.push({specifier: arg.value, dynamic: true, line: loc.line, col: loc.col});
          moduleFeatures.push('literal dynamic import: ' + arg.value);
        } else {
          moduleFeatures.push('non-literal dynamic import');
        }
      } else if (type === 'Call' && node.expression && String(node.expression.TYPE || '') === 'SymbolRef' && node.expression.name === 'require') {
        const arg = node.args && node.args[0];
        if (arg && String(arg.TYPE || '') === 'String') moduleFeatures.push('CommonJS require: ' + arg.value);
        else moduleFeatures.push('dynamic CommonJS require');
      } else if (type === 'Assign') {
        const path = (() => {
          const parts = []; let current = node.left;
          while (current) {
            const currentType = String(current.TYPE || '');
            if (currentType === 'Dot') { parts.unshift(String(current.property || '')); current = current.expression; continue; }
            if (currentType === 'Sub' && current.property && String(current.property.TYPE || '') === 'String') { parts.unshift(String(current.property.value)); current = current.expression; continue; }
            if (currentType === 'SymbolRef') { parts.unshift(String(current.name || '')); break; }
            return [];
          }
          return parts;
        })();
        if ((path[0] === 'module' && path[1] === 'exports') || path[0] === 'exports') moduleFeatures.push('CommonJS export assignment');
      }
    }));
    let topLevelDeclarations = 0;
    for (const statement of (ast.body || [])) {
      const type = String(statement.TYPE || '');
      const exported = type === 'Export';
      const definition = exported ? statement.exported_definition : statement;
      const definitionType = String(definition && definition.TYPE || '');
      if (definitionType === 'Defun' || definitionType === 'DefClass' || definitionType === 'Var' ||
          definitionType === 'Let' || definitionType === 'Const') topLevelDeclarations++;
      if (definitionType === 'Defun' && definition.name && definition.name.name) {
        topLevelFunctions.push({
          name: definition.name.name, syntaxKind: definition.async ? 'async-function-declaration' : 'function-declaration',
          begin: definition.start.pos, end: tokenEnd(definition.end), line: definition.start.line || 1, col: definition.start.col || 0,
          async: !!definition.async, generator: !!definition.is_generator, exported
        });
      } else if (definitionType === 'Const' || definitionType === 'Let' || definitionType === 'Var') {
        for (const item of (definition.definitions || [])) {
          const valueType = String(item.value && item.value.TYPE || '');
          if (!item.name || !item.name.name || (valueType !== 'Arrow' && valueType !== 'Function')) continue;
          topLevelFunctions.push({
            name: item.name.name, syntaxKind: valueType === 'Arrow' ? (item.value.async ? 'async-arrow-function' : 'arrow-function') : (item.value.async ? 'async-function-expression' : 'function-expression'),
            begin: definition.start.pos, end: tokenEnd(definition.end), line: definition.start.line || 1, col: definition.start.col || 0,
            async: !!item.value.async, generator: !!item.value.is_generator, exported
          });
        }
      }
    }
    globalThis.__venom_frontend_result = {
      refs, moduleImports, exportBindings, exportedFunctions, unsupportedExports, moduleFeatures, topLevelFunctions,
      nodes, functions, declarations, imports, exports, topLevelDeclarations, lexicalScopes,
      globalReferences: ast.globals instanceof Map ? ast.globals.size : 0
    };
  } catch (error) {
    globalThis.__venom_frontend_error = { message: String(error && error.message ? error.message : error), line: Number(error && (error.line || error.lineNumber) || 0), col: Number(error && (error.col || error.column || error.columnNumber) || 0) };
  }
})();
)JS";


const char kRunProtectedUnitLowering[] = R"JS(
(() => {
  try {
    const source = globalThis.__venom_frontend_source;
    const filename = globalThis.__venom_frontend_filename;
    const isModule = !!globalThis.__venom_frontend_module;
    const targetName = globalThis.__venom_frontend_target;
    const ast = globalThis.Terser.__venom_parse(source, { module: isModule, filename });
    ast.figure_out_scope({ module: isModule });
    const tokenEnd = token => {
      if (!token || typeof token.pos !== 'number') return 0;
      const value = String(token.value == null ? '' : token.value);
      return token.pos + (token.type === 'string' ? value.length + 2 : Math.max(1, value.length));
    };
    const location = node => ({ line: ((node.start && node.start.line) || 1), col: ((node.start && node.start.col) || 0) });
    const params = node => (node.argnames || []).map(arg => arg.print_to_string()).join(',');
    let result = { found: false };
    for (const statement of (ast.body || [])) {
      const statementType = String(statement.TYPE || '');
      const exported = statementType === 'Export';
      const node = exported ? statement.exported_definition : statement;
      if (!node) continue;
      const type = String(node.TYPE || '');
      const replacementNode = exported ? statement : node;
      const loc = location(node);
      if (type === 'Defun' && node.name && node.name.name === targetName) {
        result = {
          found: true,
          name: targetName,
          parameters: params(node),
          declaration: source.slice(replacementNode.start.pos, tokenEnd(replacementNode.end)),
          callableExpression: node.print_to_string(),
          syntaxKind: node.async ? 'async-function-declaration' : 'function-declaration',
          begin: replacementNode.start.pos,
          end: tokenEnd(replacementNode.end),
          line: loc.line,
          col: loc.col,
          async: !!node.async,
          generator: !!node.is_generator,
          exported
        };
        break;
      }
      if (type === 'Const' || type === 'Let' || type === 'Var') {
        const definitions = Array.isArray(node.definitions) ? node.definitions : [];
        for (const definition of definitions) {
          const name = definition.name && definition.name.name;
          const value = definition.value;
          const valueType = String(value && value.TYPE || '');
          if (name !== targetName || (valueType !== 'Arrow' && valueType !== 'Function')) continue;
          if (definitions.length !== 1) throw new Error('VENOM-E2304 protected variable declaration must contain exactly one binding at ' + loc.line + ':' + (loc.col + 1));
          result = {
            found: true,
            name: targetName,
            parameters: params(value),
            declaration: source.slice(replacementNode.start.pos, tokenEnd(replacementNode.end)),
            callableExpression: value.print_to_string(),
            syntaxKind: valueType === 'Arrow' ? (value.async ? 'async-arrow-function' : 'arrow-function') : (value.async ? 'async-function-expression' : 'function-expression'),
            begin: replacementNode.start.pos,
            end: tokenEnd(replacementNode.end),
            line: loc.line,
            col: loc.col,
            async: !!value.async,
            generator: !!value.is_generator,
            exported
          };
          break;
        }
        if (result.found) break;
      }
    }
    globalThis.__venom_frontend_result = result;
  } catch (error) {
    globalThis.__venom_frontend_error = { message: String(error && error.message ? error.message : error), line: Number(error && (error.line || error.lineNumber) || 0), col: Number(error && (error.col || error.column || error.columnNumber) || 0) };
  }
})();
)JS";

const char kRunScopeAnalysis[] = R"JS(
(() => {
  try {
    const source = globalThis.__venom_frontend_source;
    const filename = globalThis.__venom_frontend_filename;
    const isModule = !!globalThis.__venom_frontend_module;
    const targetName = globalThis.__venom_frontend_target;
    const ast = globalThis.Terser.__venom_parse(source, { module: isModule, filename });
    ast.figure_out_scope({ module: isModule });
    const tokenEnd = token => {
      if (!token || typeof token.pos !== 'number') return 0;
      const value = String(token.value == null ? '' : token.value);
      return token.pos + (token.type === 'string' ? value.length + 2 : Math.max(1, value.length));
    };
    const location = node => ({ line: ((node.start && node.start.line) || 1), col: ((node.start && node.start.col) || 0) });
    const uniq = values => Array.from(new Set(values.filter(Boolean)));
    const ownedByScope = (def, scope) => {
      for (let current = def && def.scope; current; current = current.parent_scope) if (current === scope) return true;
      return false;
    };
    const enclosedNames = node => uniq((node && node.enclosed || [])
      .filter(def => !ownedByScope(def, node))
      .map(def => def && def.name));
    const unsafeFeatures = node => {
      const features = [];
      if (!node) return features;
      node.walk(new globalThis.Terser.__venom_tree_walker(function(child) {
        const type = String(child.TYPE || '');
        if (type === 'With') features.push('with statement');
        if (type === 'Super') features.push('super binding');
        if (type === 'NewTarget') features.push('new.target binding');
        if (type === 'Yield') features.push('generator semantics');
        if (type === 'ImportMeta') features.push('module metadata');
        if (type === 'This') features.push('dynamic this binding');
        if (type === 'Call' && child.expression && String(child.expression.TYPE || '') === 'SymbolRef' && child.expression.name === 'eval') features.push('dynamic eval');
      }));
      return uniq(features);
    };
    const propertyPath = node => {
      const parts = [];
      let current = node;
      while (current) {
        const type = String(current.TYPE || '');
        if (type === 'Dot') { parts.unshift(String(current.property || '')); current = current.expression; continue; }
        if (type === 'Sub' && current.property && String(current.property.TYPE || '') === 'String') { parts.unshift(String(current.property.value)); current = current.expression; continue; }
        if (type === 'SymbolRef') { parts.unshift(String(current.name || '')); return parts; }
        return [];
      }
      return parts;
    };
    const hostReferences = node => {
      const references = [];
      if (!node) return references;
      const browserRoots = new Set(['window', 'self', 'globalThis']);
      const browserProperties = new Set(['document','navigator','location','history','localStorage','sessionStorage','customElements','screen','fetch','alert','confirm','prompt']);
      node.walk(new globalThis.Terser.__venom_tree_walker(function(child) {
        const type = String(child.TYPE || '');
        if (type !== 'Dot' && type !== 'Sub') return;
        const path = propertyPath(child);
        if (path.length >= 2 && browserRoots.has(path[0]) && browserProperties.has(path[1])) references.push(path.slice(0, 3).join('.'));
      }));
      return uniq(references);
    };
    const observableEffects = node => {
      const effects = [];
      if (!node) return effects;
      const externalSymbol = symbol => {
        if (!symbol || String(symbol.TYPE || '') !== 'SymbolRef') return '';
        const def = typeof symbol.definition === 'function' ? symbol.definition() : null;
        return def && !ownedByScope(def, node) ? String(symbol.name || '') : '';
      };
      const externalRoot = expression => {
        let current = expression;
        while (current && (String(current.TYPE || '') === 'Dot' || String(current.TYPE || '') === 'Sub')) current = current.expression;
        return externalSymbol(current);
      };
      const mutatingMethods = new Set(['add','clear','copyWithin','delete','fill','pop','push','reverse','set','shift','sort','splice','unshift']);
      node.walk(new globalThis.Terser.__venom_tree_walker(function(child) {
        const type = String(child.TYPE || '');
        if (type === 'Assign') {
          const name = externalSymbol(child.left);
          if (name) effects.push('writes captured binding: ' + name);
          const root = externalRoot(child.left);
          if (!name && root) effects.push('writes property on captured binding: ' + root);
        } else if (type === 'UnaryPostfix' || type === 'UnaryPrefix') {
          if (child.operator === 'delete') {
            const root = externalRoot(child.expression);
            if (root) effects.push('deletes property on captured binding: ' + root);
            return;
          }
          if (child.operator !== '++' && child.operator !== '--') return;
          const name = externalSymbol(child.expression);
          if (name) effects.push('updates captured binding: ' + name);
          const root = externalRoot(child.expression);
          if (!name && root) effects.push('updates property on captured binding: ' + root);
        } else if (type === 'Call') {
          const calleeType = String(child.expression && child.expression.TYPE || '');
          if (calleeType !== 'Dot' && calleeType !== 'Sub') return;
          const path = propertyPath(child.expression);
          const method = path.length ? path[path.length - 1] : '';
          const root = externalRoot(child.expression);
          if (root && mutatingMethods.has(method)) effects.push('calls mutating method on captured binding: ' + root + '.' + method);
        }
      }));
      return uniq(effects);
    };
    const symbolsInPattern = pattern => {
      const names = [];
      if (!pattern) return names;
      pattern.walk(new globalThis.Terser.__venom_tree_walker(function(node) {
        const type = String(node.TYPE || '');
        if (type.startsWith('Symbol') && typeof node.name === 'string') names.push(node.name);
      }));
      return uniq(names);
    };
    const primitiveValue = value => {
      if (!value) return null;
      const type = String(value.TYPE || '');
      if (type === 'String' || type === 'Number' || type === 'True' || type === 'False' || type === 'Null') return value.print_to_string();
      if (type === 'UnaryPrefix' && (value.operator === '+' || value.operator === '-') && value.expression && String(value.expression.TYPE || '') === 'Number') return value.print_to_string();
      return null;
    };
    const declarations = [];
    let targetNode = null;
    for (const statement of (ast.body || [])) {
      const node = String(statement.TYPE || '') === 'Export' ? statement.exported_definition : statement;
      if (!node) continue;
      const type = String(node.TYPE || '');
      const loc = location(node);
      if (type === 'Defun' && node.name && node.name.name) {
        const name = node.name.name;
        declarations.push({
          kind: 'function', name,
          declaration: source.slice(node.start.pos, tokenEnd(node.end)),
          parameters: (node.argnames || []).map(arg => arg.print_to_string()).join(','),
          free: enclosedNames(node), unsafe: unsafeFeatures(node), line: loc.line, col: loc.col
        });
        if (name === targetName) targetNode = node;
      } else if (type === 'DefClass' && node.name && node.name.name) {
        declarations.push({kind:'class', name:node.name.name, declaration:source.slice(node.start.pos, tokenEnd(node.end)), parameters:'', free:enclosedNames(node), unsafe:unsafeFeatures(node), line:loc.line, col:loc.col});
      } else if (type === 'Const' || type === 'Let' || type === 'Var') {
        const definitions = Array.isArray(node.definitions) ? node.definitions : [];
        for (const definition of definitions) {
          const names = symbolsInPattern(definition.name);
          for (const name of names) {
            const valueType = String(definition.value && definition.value.TYPE || '');
            let kind = type === 'Const' ? 'unsupported-constant' : 'mutable';
            let declaration = source.slice(node.start.pos, tokenEnd(node.end));
            if (type === 'Const' && (valueType === 'Function' || valueType === 'Arrow')) {
              kind = 'function';
            } else if (type === 'Const' && definitions.length === 1 && names.length === 1) {
              const primitive = primitiveValue(definition.value);
              if (primitive !== null) {
                kind = 'primitive-constant';
                declaration = 'const ' + name + '=' + primitive + ';';
              }
            }
            declarations.push({kind, name, declaration, parameters: '', free: [], unsafe: [], line: loc.line, col: loc.col});
            if (name === targetName && definition.value && ['Function','Arrow'].includes(String(definition.value.TYPE || ''))) targetNode = definition.value;
          }
        }
      } else if (type === 'Import') {
        if (node.imported_name && node.imported_name.name) {
          declarations.push({kind:'import', name:node.imported_name.name, declaration:source.slice(node.start.pos, tokenEnd(node.end)), parameters:'', free:[], unsafe:[], line:loc.line, col:loc.col});
        }
        for (const mapping of (node.imported_names || [])) {
          if (mapping.name && mapping.name.name) declarations.push({kind:'import', name:mapping.name.name, declaration:source.slice(node.start.pos, tokenEnd(node.end)), parameters:'', free:[], unsafe:[], line:loc.line, col:loc.col});
        }
      }
    }
    const directCalls = [];
    const indirectCalls = [];
    if (targetNode) targetNode.walk(new globalThis.Terser.__venom_tree_walker(function(child) {
      const type = String(child.TYPE || '');
      if (type !== 'Call' && type !== 'New') return;
      const expression = child.expression;
      if (!expression) return;
      const expressionType = String(expression.TYPE || '');
      if (expressionType === 'SymbolRef') {
        const def = typeof expression.definition === 'function' ? expression.definition() : null;
        if (def && !ownedByScope(def, targetNode)) directCalls.push(String(expression.name || ''));
        return;
      }
      if (['Conditional','Sequence','Call','New','Function','Arrow'].includes(expressionType)) {
        indirectCalls.push(expressionType.toLowerCase() + ' callee');
      }
    }));
    const targetFree = enclosedNames(targetNode);
    const declarationKind = new Map(declarations.map(item => [item.name, item.kind]));
    const capturesOfKind = kind => targetFree.filter(name => declarationKind.get(name) === kind);
    globalThis.__venom_frontend_result = {
      declarations,
      targetFound: !!targetNode,
      targetFree,
      targetUnsafe: unsafeFeatures(targetNode),
      targetHost: hostReferences(targetNode),
      targetEffects: observableEffects(targetNode),
      targetMutableCaptures: capturesOfKind('mutable'),
      targetUnsupportedCaptures: capturesOfKind('unsupported-constant'),
      targetFunctionCaptures: capturesOfKind('function'),
      targetImportCaptures: capturesOfKind('import'),
      targetConstantCaptures: capturesOfKind('primitive-constant'),
      targetClassCaptures: capturesOfKind('class'),
      targetDirectCalls: uniq(directCalls),
      targetIndirectCalls: uniq(indirectCalls)
    };
  } catch (error) {
    globalThis.__venom_frontend_error = { message: String(error && error.message ? error.message : error), line: Number(error && (error.line || error.lineNumber) || 0), col: Number(error && (error.col || error.column || error.columnNumber) || 0) };
  }
})();
)JS";

} // namespace

ParseSummary parse(const std::string& source, const std::string& filename, bool module) {
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) throw std::runtime_error("JavaScript frontend could not create QuickJS runtime");
  JS_SetMemoryLimit(runtime, 256ull * 1024ull * 1024ull);
  JS_SetMaxStackSize(runtime, 8ull * 1024ull * 1024ull);
  JSContext* context = JS_NewContext(runtime);
  if (!context) {
    JS_FreeRuntime(runtime);
    throw std::runtime_error("JavaScript frontend could not create QuickJS context");
  }
  try {
    eval_checked(context, kBootstrap, sizeof(kBootstrap) - 1, "<venom-frontend-bootstrap>");
    eval_checked(context, kEmbeddedTerserBundle, sizeof(kEmbeddedTerserBundle) - 1, "<embedded-terser-frontend>");
    set_global_string(context, "__venom_frontend_source", source);
    set_global_string(context, "__venom_frontend_filename", filename);
    set_global_bool(context, "__venom_frontend_module", module);
    eval_checked(context, kRunFrontend, sizeof(kRunFrontend) - 1, "<venom-frontend-run>");

    JSValue global = JS_GetGlobalObject(context);
    JSValue error = JS_GetPropertyStr(context, global, "__venom_frontend_error");
    if (!JS_IsUndefined(error) && !JS_IsNull(error)) {
      const auto frontend_error = get_frontend_error(context, error);
      JS_FreeValue(context, error);
      JS_FreeValue(context, global);
      raise_diagnostic("VENOM-E2101", "JavaScript parse failed",
                       {filename, frontend_error.line, frontend_error.column + 1u, line_text(source, frontend_error.line)},
                       frontend_error.message,
                       "Correct the JavaScript syntax at the reported location before rebuilding.",
                       "docs/reference/diagnostics.md#venom-e2101");
    }
    JS_FreeValue(context, error);
    JSValue result = JS_GetPropertyStr(context, global, "__venom_frontend_result");
    if (!JS_IsObject(result)) {
      JS_FreeValue(context, result);
      JS_FreeValue(context, global);
      throw std::runtime_error("JavaScript frontend produced no parse result for " + filename);
    }

    ParseSummary summary;
    summary.module = module;
    summary.node_count = get_u32(context, result, "nodes");
    summary.function_count = get_u32(context, result, "functions");
    summary.declaration_count = get_u32(context, result, "declarations");
    summary.import_count = get_u32(context, result, "imports");
    summary.export_count = get_u32(context, result, "exports");
    summary.top_level_declaration_count = get_u32(context, result, "topLevelDeclarations");
    summary.lexical_scope_count = get_u32(context, result, "lexicalScopes");
    summary.global_reference_count = get_u32(context, result, "globalReferences");
    JSValue refs = JS_GetPropertyStr(context, result, "refs");
    std::uint32_t length = get_u32(context, refs, "length");
    summary.module_references.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
      JSValue item = JS_GetPropertyUint32(context, refs, i);
      ModuleReference ref;
      ref.specifier = get_string(context, item, "specifier");
      JSValue dynamic = JS_GetPropertyStr(context, item, "dynamic");
      ref.dynamic = JS_ToBool(context, dynamic) > 0;
      JS_FreeValue(context, dynamic);
      ref.line = get_u32(context, item, "line");
      ref.column = get_u32(context, item, "col");
      summary.module_references.push_back(std::move(ref));
      JS_FreeValue(context, item);
    }
    JS_FreeValue(context, refs);

    JSValue module_imports = JS_GetPropertyStr(context, result, "moduleImports");
    length = get_u32(context, module_imports, "length");
    summary.module_imports.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
      JSValue item = JS_GetPropertyUint32(context, module_imports, i);
      ModuleImport imported;
      imported.specifier = get_string(context, item, "specifier");
      imported.clause = get_string(context, item, "clause");
      imported.side_effect_only = get_bool(context, item, "sideEffectOnly");
      imported.has_default = get_bool(context, item, "hasDefault");
      JSValue bindings = JS_GetPropertyStr(context, item, "bindings");
      const auto binding_length = get_u32(context, bindings, "length");
      imported.bindings.reserve(binding_length);
      for (std::uint32_t binding_index = 0; binding_index < binding_length; ++binding_index) {
        JSValue binding_item = JS_GetPropertyUint32(context, bindings, binding_index);
        ImportBinding binding;
        binding.local_name = get_string(context, binding_item, "localName");
        binding.imported_name = get_string(context, binding_item, "importedName");
        binding.namespace_import = get_bool(context, binding_item, "namespaceImport");
        imported.bindings.push_back(std::move(binding));
        JS_FreeValue(context, binding_item);
      }
      JS_FreeValue(context, bindings);
      imported.begin = get_u32(context, item, "begin");
      imported.end = get_u32(context, item, "end");
      imported.line = get_u32(context, item, "line");
      imported.column = get_u32(context, item, "col");
      summary.module_imports.push_back(std::move(imported));
      JS_FreeValue(context, item);
    }
    JS_FreeValue(context, module_imports);

    JSValue export_bindings = JS_GetPropertyStr(context, result, "exportBindings");
    length = get_u32(context, export_bindings, "length");
    summary.export_bindings.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
      JSValue item = JS_GetPropertyUint32(context, export_bindings, i);
      ExportBinding binding;
      binding.exported_name = get_string(context, item, "exportedName");
      binding.local_name = get_string(context, item, "localName");
      binding.specifier = get_string(context, item, "specifier");
      binding.export_all = get_bool(context, item, "exportAll");
      binding.namespace_export = get_bool(context, item, "namespaceExport");
      binding.default_export = get_bool(context, item, "defaultExport");
      binding.line = get_u32(context, item, "line");
      binding.column = get_u32(context, item, "col");
      summary.export_bindings.push_back(std::move(binding));
      JS_FreeValue(context, item);
    }
    JS_FreeValue(context, export_bindings);

    JSValue exported_functions = JS_GetPropertyStr(context, result, "exportedFunctions");
    length = get_u32(context, exported_functions, "length");
    summary.exported_functions.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
      JSValue item = JS_GetPropertyUint32(context, exported_functions, i);
      ExportedFunction exported;
      exported.name = get_string(context, item, "name");
      exported.parameters = get_string(context, item, "parameters");
      exported.async = get_bool(context, item, "async");
      exported.export_begin = get_u32(context, item, "exportBegin");
      exported.declaration_begin = get_u32(context, item, "declarationBegin");
      exported.line = get_u32(context, item, "line");
      exported.column = get_u32(context, item, "col");
      summary.exported_functions.push_back(std::move(exported));
      JS_FreeValue(context, item);
    }
    JS_FreeValue(context, exported_functions);

    JSValue unsupported_exports = JS_GetPropertyStr(context, result, "unsupportedExports");
    length = get_u32(context, unsupported_exports, "length");
    summary.unsupported_exports.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
      JSValue item = JS_GetPropertyUint32(context, unsupported_exports, i);
      UnsupportedExport exported;
      exported.kind = get_string(context, item, "kind");
      exported.line = get_u32(context, item, "line");
      exported.column = get_u32(context, item, "col");
      summary.unsupported_exports.push_back(std::move(exported));
      JS_FreeValue(context, item);
    }
    JS_FreeValue(context, unsupported_exports);

    JSValue module_features = JS_GetPropertyStr(context, result, "moduleFeatures");
    length = get_u32(context, module_features, "length");
    summary.module_features.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
      JSValue item = JS_GetPropertyUint32(context, module_features, i);
      summary.module_features.push_back(value_text(context, item));
      JS_FreeValue(context, item);
    }
    JS_FreeValue(context, module_features);

    JSValue top_level_functions = JS_GetPropertyStr(context, result, "topLevelFunctions");
    length = get_u32(context, top_level_functions, "length");
    summary.top_level_functions.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
      JSValue item = JS_GetPropertyUint32(context, top_level_functions, i);
      ParsedFunction function;
      function.name = get_string(context, item, "name");
      function.syntax_kind = get_string(context, item, "syntaxKind");
      function.begin = get_u32(context, item, "begin");
      function.end = get_u32(context, item, "end");
      function.line = get_u32(context, item, "line");
      function.column = get_u32(context, item, "col");
      function.async = get_bool(context, item, "async");
      function.generator = get_bool(context, item, "generator");
      function.exported = get_bool(context, item, "exported");
      summary.top_level_functions.push_back(std::move(function));
      JS_FreeValue(context, item);
    }
    JS_FreeValue(context, top_level_functions);

    JS_FreeValue(context, result);
    JS_FreeValue(context, global);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    return summary;
  } catch (...) {
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    throw;
  }
}



ProtectedUnit lower_protected_unit(const std::string& source, const std::string& filename,
                                   const std::string& target_name, bool module) {
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) throw std::runtime_error("JavaScript frontend could not create QuickJS runtime");
  JS_SetMemoryLimit(runtime, 256ull * 1024ull * 1024ull);
  JS_SetMaxStackSize(runtime, 8ull * 1024ull * 1024ull);
  JSContext* context = JS_NewContext(runtime);
  if (!context) {
    JS_FreeRuntime(runtime);
    throw std::runtime_error("JavaScript frontend could not create QuickJS context");
  }
  try {
    eval_checked(context, kBootstrap, sizeof(kBootstrap) - 1, "<venom-frontend-bootstrap>");
    eval_checked(context, kEmbeddedTerserBundle, sizeof(kEmbeddedTerserBundle) - 1, "<embedded-terser-frontend>");
    set_global_string(context, "__venom_frontend_source", source);
    set_global_string(context, "__venom_frontend_filename", filename);
    set_global_string(context, "__venom_frontend_target", target_name);
    set_global_bool(context, "__venom_frontend_module", module);
    eval_checked(context, kRunProtectedUnitLowering, sizeof(kRunProtectedUnitLowering) - 1, "<venom-protected-unit-lowering>");

    JSValue global = JS_GetGlobalObject(context);
    JSValue error = JS_GetPropertyStr(context, global, "__venom_frontend_error");
    if (!JS_IsUndefined(error) && !JS_IsNull(error)) {
      const auto frontend_error = get_frontend_error(context, error);
      JS_FreeValue(context, error);
      JS_FreeValue(context, global);
      std::string code = "VENOM-E2301";
      std::string message = frontend_error.message;
      if (message.rfind("VENOM-E2304", 0) == 0) { code = "VENOM-E2304"; message = message.substr(12); }
      raise_diagnostic(code, "Protected-function lowering failed",
                       {filename, frontend_error.line, frontend_error.column + 1u, line_text(source, frontend_error.line)},
                       message,
                       "Use a supported named function, function expression, or single-binding arrow declaration.",
                       "docs/reference/diagnostics.md#" + code);
    }
    JS_FreeValue(context, error);
    JSValue result = JS_GetPropertyStr(context, global, "__venom_frontend_result");
    if (!JS_IsObject(result)) {
      JS_FreeValue(context, result);
      JS_FreeValue(context, global);
      throw std::runtime_error("JavaScript frontend produced no protected-unit result for " + filename);
    }
    ProtectedUnit unit;
    unit.found = get_bool(context, result, "found");
    if (unit.found) {
      unit.name = get_string(context, result, "name");
      unit.parameters = get_string(context, result, "parameters");
      unit.declaration = get_string(context, result, "declaration");
      unit.callable_expression = get_string(context, result, "callableExpression");
      unit.syntax_kind = get_string(context, result, "syntaxKind");
      unit.begin = get_u32(context, result, "begin");
      unit.end = get_u32(context, result, "end");
      unit.line = get_u32(context, result, "line");
      unit.column = get_u32(context, result, "col");
      unit.async = get_bool(context, result, "async");
      unit.generator = get_bool(context, result, "generator");
      unit.exported = get_bool(context, result, "exported");
    }
    JS_FreeValue(context, result);
    JS_FreeValue(context, global);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    return unit;
  } catch (...) {
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    throw;
  }
}

FunctionScopeAnalysis analyze_function_scope(const std::string& source, const std::string& filename,
                                             const std::string& target_name, bool module) {
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) throw std::runtime_error("JavaScript frontend could not create QuickJS runtime");
  JS_SetMemoryLimit(runtime, 256ull * 1024ull * 1024ull);
  JS_SetMaxStackSize(runtime, 8ull * 1024ull * 1024ull);
  JSContext* context = JS_NewContext(runtime);
  if (!context) {
    JS_FreeRuntime(runtime);
    throw std::runtime_error("JavaScript frontend could not create QuickJS context");
  }
  try {
    eval_checked(context, kBootstrap, sizeof(kBootstrap) - 1, "<venom-frontend-bootstrap>");
    eval_checked(context, kEmbeddedTerserBundle, sizeof(kEmbeddedTerserBundle) - 1, "<embedded-terser-frontend>");
    set_global_string(context, "__venom_frontend_source", source);
    set_global_string(context, "__venom_frontend_filename", filename);
    set_global_string(context, "__venom_frontend_target", target_name);
    set_global_bool(context, "__venom_frontend_module", module);
    eval_checked(context, kRunScopeAnalysis, sizeof(kRunScopeAnalysis) - 1, "<venom-scope-analysis>");

    JSValue global = JS_GetGlobalObject(context);
    JSValue error = JS_GetPropertyStr(context, global, "__venom_frontend_error");
    if (!JS_IsUndefined(error) && !JS_IsNull(error)) {
      const auto frontend_error = get_frontend_error(context, error);
      JS_FreeValue(context, error);
      JS_FreeValue(context, global);
      raise_diagnostic("VENOM-E2102", "JavaScript scope analysis failed",
                       {filename, frontend_error.line, frontend_error.column + 1u, line_text(source, frontend_error.line)},
                       frontend_error.message,
                       "Simplify the affected declaration or move runtime-bound behavior behind an explicit capability.",
                       "docs/reference/diagnostics.md#venom-e2102");
    }
    JS_FreeValue(context, error);
    JSValue result = JS_GetPropertyStr(context, global, "__venom_frontend_result");
    if (!JS_IsObject(result)) {
      JS_FreeValue(context, result);
      JS_FreeValue(context, global);
      throw std::runtime_error("JavaScript frontend produced no scope result for " + filename);
    }
    FunctionScopeAnalysis analysis;
    analysis.target_found = get_bool(context, result, "targetFound");
    analysis.target_free_identifiers = get_string_array(context, result, "targetFree");
    analysis.target_unsafe_features = get_string_array(context, result, "targetUnsafe");
    analysis.target_host_references = get_string_array(context, result, "targetHost");
    analysis.target_effects = get_string_array(context, result, "targetEffects");
    analysis.target_mutable_captures = get_string_array(context, result, "targetMutableCaptures");
    analysis.target_unsupported_captures = get_string_array(context, result, "targetUnsupportedCaptures");
    analysis.target_function_captures = get_string_array(context, result, "targetFunctionCaptures");
    analysis.target_import_captures = get_string_array(context, result, "targetImportCaptures");
    analysis.target_constant_captures = get_string_array(context, result, "targetConstantCaptures");
    analysis.target_class_captures = get_string_array(context, result, "targetClassCaptures");
    analysis.target_direct_calls = get_string_array(context, result, "targetDirectCalls");
    analysis.target_indirect_calls = get_string_array(context, result, "targetIndirectCalls");
    JSValue declarations = JS_GetPropertyStr(context, result, "declarations");
    const auto length = get_u32(context, declarations, "length");
    analysis.declarations.reserve(length);
    for (std::uint32_t i = 0; i < length; ++i) {
      JSValue item = JS_GetPropertyUint32(context, declarations, i);
      ScopeDeclaration declaration;
      const auto kind = get_string(context, item, "kind");
      if (kind == "function") declaration.kind = ScopeDeclaration::Kind::Function;
      else if (kind == "primitive-constant") declaration.kind = ScopeDeclaration::Kind::PrimitiveConstant;
      else if (kind == "mutable") declaration.kind = ScopeDeclaration::Kind::MutableBinding;
      else if (kind == "import") declaration.kind = ScopeDeclaration::Kind::ImportBinding;
      else declaration.kind = ScopeDeclaration::Kind::UnsupportedConstant;
      declaration.name = get_string(context, item, "name");
      declaration.declaration = get_string(context, item, "declaration");
      declaration.parameters = get_string(context, item, "parameters");
      declaration.free_identifiers = get_string_array(context, item, "free");
      declaration.unsafe_features = get_string_array(context, item, "unsafe");
      declaration.line = get_u32(context, item, "line");
      declaration.column = get_u32(context, item, "col");
      analysis.declarations.push_back(std::move(declaration));
      JS_FreeValue(context, item);
    }
    JS_FreeValue(context, declarations);
    JS_FreeValue(context, result);
    JS_FreeValue(context, global);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    return analysis;
  } catch (...) {
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    throw;
  }
}

const char* parser_name() { return "Terser AST frontend"; }
const char* parser_version() { return "5.49.0"; }

} // namespace venom::compiler::frontend
