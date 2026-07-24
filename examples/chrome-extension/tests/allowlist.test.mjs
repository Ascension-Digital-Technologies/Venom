import test from 'node:test';
import assert from 'node:assert/strict';
import { compilePattern, matchAllowedSite, parseAllowedSites } from '../lib/allowlist.js';

test('allowlist supports exact hosts, paths, ports, and subdomain wildcards', () => {
  const list = parseAllowedSites(`
    # comment
    http://localhost:*/*
    https://chess.example.com/play/*
    https://*.example.net/chess/*
  `);
  assert.equal(matchAllowedSite('http://localhost:8080/game', list), true);
  assert.equal(matchAllowedSite('https://chess.example.com/play/42', list), true);
  assert.equal(matchAllowedSite('https://app.example.net/chess/42', list), true);
  assert.equal(matchAllowedSite('https://chess.example.com/admin', list), false);
  assert.equal(matchAllowedSite('https://evil.example.org/play/42', list), false);
});

test('allowlist rejects non-http and global patterns', () => {
  assert.throws(() => compilePattern('<all_urls>'));
  assert.throws(() => compilePattern('file:///*'));
  assert.throws(() => compilePattern('https://*/*'));
});
