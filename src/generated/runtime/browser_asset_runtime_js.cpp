#include "compiler/pipeline/js.hpp"

namespace venom::compiler {

std::string make_browser_asset_runtime_js() {
  return R"JS(function normalizeAssetSource(value) {
  let out = String(value || '').replace(/\\/g, '/');
  while (out.startsWith('./')) out = out.slice(2);
  while (out.startsWith('/')) out = out.slice(1);
  const parts = [];
  for (const part of out.split('/')) {
    if (!part || part === '.') continue;
    if (part === '..') parts.pop();
    else parts.push(part);
  }
  return parts.join('/');
}

function dirname(path) {
  const normalized = normalizeAssetSource(path);
  const idx = normalized.lastIndexOf('/');
  return idx < 0 ? '' : normalized.slice(0, idx + 1);
}

function resolveAssetValue(route, value, assetManifest, assetBaseUrl) {
  if (!value || assetManifest.size === 0) return value;
  const raw = String(value);
  const lower = raw.toLowerCase();
  if (lower.startsWith('http:') || lower.startsWith('https:') || lower.startsWith('//') ||
      lower.startsWith('data:') || lower.startsWith('blob:') || lower.startsWith('#') ||
      lower.startsWith('mailto:') || lower.startsWith('tel:') || lower.startsWith('javascript:')) {
    return value;
  }
  const withoutQuery = raw.split(/[?#]/, 1)[0];
  const suffix = raw.slice(withoutQuery.length);
  const candidates = [normalizeAssetSource(withoutQuery)];
  if (!withoutQuery.startsWith('/')) {
    candidates.push(normalizeAssetSource(dirname(route.sourcePath || '') + withoutQuery));
  }
  for (const candidate of candidates) {
    const record = assetManifest.get(candidate);
    if (record) return new URL(record.outputName + suffix, assetBaseUrl || document.baseURI).href;
  }
  return value;
}

let activeBrowserAssetRoute = null;
function setActiveBrowserAssetRoute(route) { activeBrowserAssetRoute = route || null; }

function installBrowserAssetResolver(initialRoute, assetManifest, assetBaseUrl) {
  setActiveBrowserAssetRoute(initialRoute);
  if (!assetManifest || assetManifest.size === 0 || globalThis.__venomBrowserAssetResolverInstalled === true) return;
  const activeRoute = () => activeBrowserAssetRoute || initialRoute || {};
  const resolve = (value) => resolveAssetValue(activeRoute(), value, assetManifest, assetBaseUrl);
  const resolveSrcsetValue = (value) => resolveSrcset(activeRoute(), value, assetManifest, assetBaseUrl);
  Object.defineProperty(globalThis, '__venomResolveAsset', {
    value: resolve, enumerable: false, configurable: false, writable: false,
  });
  const patchProperty = (ctorName, property, resolver) => {
    const ctor = globalThis[ctorName];
    if (!ctor || !ctor.prototype) return;
    const descriptor = Object.getOwnPropertyDescriptor(ctor.prototype, property);
    if (!descriptor || typeof descriptor.set !== 'function' || typeof descriptor.get !== 'function' || descriptor.configurable === false) return;
    Object.defineProperty(ctor.prototype, property, {
      configurable: descriptor.configurable,
      enumerable: descriptor.enumerable,
      get: descriptor.get,
      set(value) { descriptor.set.call(this, resolver(value)); },
    });
  };
  for (const item of [
    ['HTMLImageElement', 'src', resolve],
    ['HTMLImageElement', 'srcset', resolveSrcsetValue],
    ['HTMLSourceElement', 'src', resolve],
    ['HTMLSourceElement', 'srcset', resolveSrcsetValue],
    ['HTMLVideoElement', 'src', resolve],
    ['HTMLVideoElement', 'poster', resolve],
    ['HTMLAudioElement', 'src', resolve],
    ['HTMLInputElement', 'src', resolve],
    ['HTMLObjectElement', 'data', resolve],
    ['HTMLEmbedElement', 'src', resolve],
  ]) patchProperty(...item);
  const elementProto = globalThis.Element && globalThis.Element.prototype;
  if (elementProto && typeof elementProto.setAttribute === 'function') {
    const originalSetAttribute = elementProto.setAttribute;
    Object.defineProperty(elementProto, 'setAttribute', {
      configurable: true,
      enumerable: false,
      writable: true,
      value(name, value) {
        const lowerName = String(name || '').toLowerCase();
        if (lowerName === 'srcset') value = resolveSrcsetValue(value);
        else if (shouldRemapAttribute(lowerName)) value = resolve(value);
        return originalSetAttribute.call(this, name, value);
      },
    });
  }
  Object.defineProperty(globalThis, '__venomBrowserAssetResolverInstalled', {
    value: true, enumerable: false, configurable: false, writable: false,
  });
}

function shouldRemapAttribute(name) {
  return name === 'src' || name === 'href' || name === 'poster' || name === 'data' || name === 'srcset';
}

function resolveSrcset(route, value, assetManifest, assetBaseUrl) {
  return String(value).split(',').map((part) => {
    const trimmed = part.trim();
    if (!trimmed) return trimmed;
    const pieces = trimmed.split(/\s+/);
    pieces[0] = resolveAssetValue(route, pieces[0], assetManifest, assetBaseUrl);
    return pieces.join(' ');
  }).join(', ');
}
)JS";
}

} // namespace venom::compiler
