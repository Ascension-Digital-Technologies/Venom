#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace venom::compiler::frontend {

struct ModuleReference {
  std::string specifier;
  bool dynamic = false;
  std::size_t line = 0;
  std::size_t column = 0;
};

struct ImportBinding {
  std::string local_name;
  std::string imported_name;
  bool namespace_import = false;
};

struct ModuleImport {
  std::string specifier;
  std::string clause;
  bool side_effect_only = false;
  bool has_default = false;
  std::vector<ImportBinding> bindings;
  std::size_t begin = 0;
  std::size_t end = 0;
  std::size_t line = 0;
  std::size_t column = 0;
};

struct ExportBinding {
  std::string exported_name;
  std::string local_name;
  std::string specifier;
  bool export_all = false;
  bool namespace_export = false;
  bool default_export = false;
  std::size_t line = 0;
  std::size_t column = 0;
};

struct ExportedFunction {
  std::string name;
  std::string parameters;
  bool async = false;
  std::size_t export_begin = 0;
  std::size_t declaration_begin = 0;
  std::size_t line = 0;
  std::size_t column = 0;
};

struct UnsupportedExport {
  std::string kind;
  std::size_t line = 0;
  std::size_t column = 0;
};


struct ScopeDeclaration {
  enum class Kind { Function, PrimitiveConstant, MutableBinding, UnsupportedConstant, ImportBinding };
  Kind kind = Kind::Function;
  std::string name;
  std::string declaration;
  std::string parameters;
  std::vector<std::string> free_identifiers;
  std::vector<std::string> unsafe_features;
  std::size_t line = 0;
  std::size_t column = 0;
};


struct ProtectedUnit {
  std::string name;
  std::string parameters;
  std::string declaration;
  std::string callable_expression;
  std::string syntax_kind;
  std::size_t begin = 0;
  std::size_t end = 0;
  std::size_t line = 0;
  std::size_t column = 0;
  bool async = false;
  bool generator = false;
  bool exported = false;
  bool found = false;
};

struct FunctionScopeAnalysis {
  std::vector<ScopeDeclaration> declarations;
  std::vector<std::string> target_free_identifiers;
  std::vector<std::string> target_unsafe_features;
  std::vector<std::string> target_host_references;
  std::vector<std::string> target_effects;
  std::vector<std::string> target_mutable_captures;
  std::vector<std::string> target_unsupported_captures;
  std::vector<std::string> target_function_captures;
  std::vector<std::string> target_import_captures;
  std::vector<std::string> target_constant_captures;
  std::vector<std::string> target_class_captures;
  std::vector<std::string> target_direct_calls;
  std::vector<std::string> target_indirect_calls;
  bool target_found = false;
};


struct ParsedFunction {
  std::string name;
  std::string syntax_kind;
  std::size_t begin = 0;
  std::size_t end = 0;
  std::size_t line = 0;
  std::size_t column = 0;
  bool async = false;
  bool generator = false;
  bool exported = false;
};

struct ParseSummary {
  std::vector<ModuleReference> module_references;
  std::vector<ModuleImport> module_imports;
  std::vector<ExportBinding> export_bindings;
  std::vector<ExportedFunction> exported_functions;
  std::vector<UnsupportedExport> unsupported_exports;
  std::vector<std::string> module_features;
  std::vector<ParsedFunction> top_level_functions;
  std::size_t node_count = 0;
  std::size_t function_count = 0;
  std::size_t declaration_count = 0;
  std::size_t import_count = 0;
  std::size_t export_count = 0;
  std::size_t top_level_declaration_count = 0;
  std::size_t lexical_scope_count = 0;
  std::size_t global_reference_count = 0;
  bool module = false;
};

ParseSummary parse(const std::string& source, const std::string& filename, bool module);
ProtectedUnit lower_protected_unit(const std::string& source, const std::string& filename,
                                    const std::string& target_name, bool module);
FunctionScopeAnalysis analyze_function_scope(const std::string& source, const std::string& filename,
                                             const std::string& target_name, bool module);
const char* parser_name();
const char* parser_version();

} // namespace venom::compiler::frontend
