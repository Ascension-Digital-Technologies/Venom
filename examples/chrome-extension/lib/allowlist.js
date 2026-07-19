const HTTP_URL = /^https?:\/\//i;

export function parseAllowedSites(text) {
  return String(text || '')
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line && !line.startsWith('#'))
    .map(compilePattern);
}

export function compilePattern(pattern) {
  if (!HTTP_URL.test(pattern)) {
    throw new Error(`Only http:// and https:// patterns are accepted: ${pattern}`);
  }
  if (pattern === 'http://*/*' || pattern === 'https://*/*' || pattern.includes('<all_urls>')) {
    throw new Error('Global URL patterns are intentionally forbidden');
  }

  const escaped = pattern
    .replace(/[.+?^${}()|[\]\\]/g, '\\$&')
    .replace(/\*/g, '.*');
  return { source: pattern, regex: new RegExp(`^${escaped}$`, 'i') };
}

export function matchAllowedSite(url, compiledPatterns) {
  let parsed;
  try {
    parsed = new URL(url);
  } catch {
    return false;
  }
  if (parsed.protocol !== 'http:' && parsed.protocol !== 'https:') return false;
  return compiledPatterns.some(({ regex }) => regex.test(parsed.href));
}
