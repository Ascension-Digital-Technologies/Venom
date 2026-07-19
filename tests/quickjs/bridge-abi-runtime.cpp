#include "venom/quickjs/bytecode.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
std::uint32_t venom_qjs_context_alloc(void);
std::uint32_t venom_qjs_context_free(std::uint32_t);
std::uint32_t venom_qjs_context_set_limits(std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_context_set_execution_limits(std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_test_load_bytes(std::uint32_t, const unsigned char*, std::uint32_t);
std::uint32_t venom_qjs_script_buffer_set_expected_hash(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_execute_bytecode(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_bridge_abi(void);
std::uint32_t venom_qjs_test_bridge_load_bytes(std::uint32_t, const unsigned char*, std::uint32_t);
std::uint32_t venom_qjs_bridge_invoke(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_bridge_result_size(void);
std::uint32_t venom_qjs_test_bridge_result_byte(std::uint32_t);
std::uint32_t venom_qjs_bridge_release(std::uint32_t);
const char* venom_qjs_test_exception_message(void);
}

static std::uint32_t fnv1a32(const std::vector<unsigned char>& bytes) {
  std::uint32_t h = 2166136261u;
  for (const auto byte : bytes) { h ^= byte; h *= 16777619u; }
  return h;
}

int main() {
  const auto ctx = venom_qjs_context_alloc();
  if (!ctx || venom_qjs_bridge_abi() != 1u) return 1;
  venom_qjs_context_set_limits(ctx, 8u * 1024u * 1024u, 256u * 1024u);
  venom_qjs_context_set_execution_limits(ctx, 1000000u, 1024u);
  const std::string registry_source = R"JS(
globalThis.__venomBridgeRevive=function(v){
  if(v&&typeof v==='object'&&!Array.isArray(v)&&typeof v.__venomBinaryValue==='string'&&Array.isArray(v.bytes)){
    const b=Uint8Array.from(v.bytes),t=v.__venomBinaryValue;
    if(t==='ArrayBuffer')return b.buffer;
    const C=globalThis[t];
    if(typeof C!=='function'||!['Uint8Array','Uint8ClampedArray','Int8Array','Uint16Array','Int16Array','Uint32Array','Int32Array','Float32Array','Float64Array'].includes(t)||b.byteLength%C.BYTES_PER_ELEMENT)throw new TypeError('invalid protected binary value');
    return new C(b.buffer);
  }
  if(Array.isArray(v))return v.map(globalThis.__venomBridgeRevive);
  if(v&&typeof v==='object')for(const k of Object.keys(v))v[k]=globalThis.__venomBridgeRevive(v[k]);
  return v;
};
globalThis.__venomBridgeEncode=function(v){
  let t='';
  if(v instanceof ArrayBuffer)t='ArrayBuffer';
  else if(ArrayBuffer.isView(v)&&!(v instanceof DataView))t=v.constructor&&v.constructor.name||'';
  if(t){
    if(!['ArrayBuffer','Uint8Array','Uint8ClampedArray','Int8Array','Uint16Array','Int16Array','Uint32Array','Int32Array','Float32Array','Float64Array'].includes(t))throw new TypeError('unsupported protected binary result');
    const b=v instanceof ArrayBuffer?new Uint8Array(v):new Uint8Array(v.buffer,v.byteOffset,v.byteLength);
    return {__venomBinaryValue:t,bytes:Array.from(b)};
  }
  if(Array.isArray(v))return v.map(globalThis.__venomBridgeEncode);
  if(v&&typeof v==='object'){const o=Object.create(null);for(const k of Object.keys(v))o[k]=globalThis.__venomBridgeEncode(v[k]);return o;}
  return v;
};
globalThis.__venomProtectedBridge={
  "__venomPlaygroundExecute":async(input)=>{ const consoleCapture=[]; const normalize=(value)=>{ if(value===undefined)return {type:"undefined"}; return value; }; const console={log:(...values)=>consoleCapture.push({level:"log",values})}; const source='console.log("values"); const values=[3,8,13,21]; ({total:values.reduce((a,b)=>a+b,0),count:values.length});'; const result=await eval(source); return {ok:true,result:normalize(result),consoleCapture}; },
  "math.js::add":(a,b)=>({sum:a+b}),
  "math.js::asyncAdd":async(a,b)=>({sum:a+b,async:true}),
  "binary.js::increment":(value)=>{
    const input=globalThis.__venomBridgeRevive(value);
    if(!(input instanceof Uint16Array))throw new TypeError('expected Uint16Array');
    const output=new Uint16Array(input.length);
    for(let i=0;i<input.length;i++)output[i]=input[i]+1;
    return globalThis.__venomBridgeEncode(output);
  }
};
)JS";
  const std::vector<venom::quickjs::ModuleSourceRecord> modules = {{"bridge-registry.js", "bridge-registry.js", registry_source}};
  const auto registry = venom::quickjs::compile_native_quickjs_module_bundle("bridge-registry.js", modules, false);
  if (!venom_qjs_test_load_bytes(ctx, registry.data(), static_cast<std::uint32_t>(registry.size()))) return 2;
  venom_qjs_script_buffer_set_expected_hash(ctx, fnv1a32(registry));
  if (!venom_qjs_execute_bytecode(ctx, static_cast<std::uint32_t>(registry.size()))) {
    std::cerr << venom_qjs_test_exception_message() << '\n';
    return 3;
  }
  const std::string request = "{\"candidate\":\"math.js::add\",\"args\":[7,9]}";
  if (!venom_qjs_test_bridge_load_bytes(ctx, reinterpret_cast<const unsigned char*>(request.data()), static_cast<std::uint32_t>(request.size()))) return 4;
  if (!venom_qjs_bridge_invoke(ctx, static_cast<std::uint32_t>(request.size()))) {
    std::cerr << venom_qjs_test_exception_message() << '\n';
    return 5;
  }
  std::string result;
  for (std::uint32_t i = 0; i < venom_qjs_bridge_result_size(); ++i) result.push_back(static_cast<char>(venom_qjs_test_bridge_result_byte(i)));
  if (result != "{\"sum\":16}") { std::cerr << result << '\n'; return 6; }
  if (!venom_qjs_bridge_release(ctx)) return 7;
  const std::string async_request = "{\"candidate\":\"math.js::asyncAdd\",\"args\":[4,5]}";
  if (!venom_qjs_test_bridge_load_bytes(ctx, reinterpret_cast<const unsigned char*>(async_request.data()), static_cast<std::uint32_t>(async_request.size()))) return 11;
  if (!venom_qjs_bridge_invoke(ctx, static_cast<std::uint32_t>(async_request.size()))) {
    std::cerr << venom_qjs_test_exception_message() << '\n';
    return 12;
  }
  result.clear();
  for (std::uint32_t i = 0; i < venom_qjs_bridge_result_size(); ++i) result.push_back(static_cast<char>(venom_qjs_test_bridge_result_byte(i)));
  if (result != "{\"sum\":9,\"async\":true}") { std::cerr << result << '\n'; return 13; }
  if (!venom_qjs_bridge_release(ctx)) return 14;
  const std::string binary_request = "{\"candidate\":\"binary.js::increment\",\"args\":[{\"__venomBinaryValue\":\"Uint16Array\",\"bytes\":[1,0,255,0]}]}";
  if (!venom_qjs_test_bridge_load_bytes(ctx, reinterpret_cast<const unsigned char*>(binary_request.data()), static_cast<std::uint32_t>(binary_request.size()))) return 15;
  if (!venom_qjs_bridge_invoke(ctx, static_cast<std::uint32_t>(binary_request.size()))) {
    std::cerr << venom_qjs_test_exception_message() << '\n';
    return 16;
  }
  result.clear();
  for (std::uint32_t i = 0; i < venom_qjs_bridge_result_size(); ++i) result.push_back(static_cast<char>(venom_qjs_test_bridge_result_byte(i)));
  if (result != "{\"__venomBinaryValue\":\"Uint16Array\",\"bytes\":[2,0,0,1]}") { std::cerr << result << '\n'; return 17; }
  if (!venom_qjs_bridge_release(ctx)) return 18;
  const std::string playground_request = "{\"candidate\":\"__venomPlaygroundExecute\",\"args\":[{}]}";
  if (!venom_qjs_test_bridge_load_bytes(ctx, reinterpret_cast<const unsigned char*>(playground_request.data()), static_cast<std::uint32_t>(playground_request.size()))) return 31;
  if (!venom_qjs_bridge_invoke(ctx, static_cast<std::uint32_t>(playground_request.size()))) { std::cerr << venom_qjs_test_exception_message() << '\n'; return 32; }
  result.clear();
  for (std::uint32_t i = 0; i < venom_qjs_bridge_result_size(); ++i) result.push_back(static_cast<char>(venom_qjs_test_bridge_result_byte(i)));
  if (result.find("\"total\":45") == std::string::npos) return 33;
  if (!venom_qjs_bridge_release(ctx)) return 34;
  const std::string missing = "{\"candidate\":\"math.js::missing\",\"args\":[]}";
  if (!venom_qjs_test_bridge_load_bytes(ctx, reinterpret_cast<const unsigned char*>(missing.data()), static_cast<std::uint32_t>(missing.size()))) return 8;
  if (venom_qjs_bridge_invoke(ctx, static_cast<std::uint32_t>(missing.size())) != 0u) return 9;
  const std::string error = venom_qjs_test_exception_message();
  if (error.find("not registered") == std::string::npos) return 10;
  venom_qjs_bridge_release(ctx);
  venom_qjs_context_free(ctx);
  std::cout << "[venom] QuickJS bridge ABI runtime: PASS\n";
  return 0;
}
