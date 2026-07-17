import fs from 'node:fs';
import { chromium, firefox, webkit } from 'playwright';

const browserName = process.argv[2] || 'chromium';
const sourceBaseUrl = process.argv[3];
const distBaseUrl = process.argv[4];
const manifestPath = process.argv[5];
if (!sourceBaseUrl || !distBaseUrl || !manifestPath) {
  console.error('usage: browser-equivalence-runner.mjs <chromium|firefox|webkit> <source-url> <dist-url> <manifest.json>');
  process.exit(2);
}
const launcher = { chromium, firefox, webkit }[browserName];
if (!launcher) process.exit(2);
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
const result = {
  schema_version: 1,
  browser: browserName,
  fixture: manifest.id || 'unnamed',
  passed: false,
  scenarios: [],
  source_console_errors: [],
  dist_console_errors: [],
  source_page_errors: [],
  dist_page_errors: [],
  duration_ms: 0,
};
const started = performance.now();
const normalize = value => value == null ? null : typeof value === 'string' ? value.replace(/\s+/g, ' ').trim() : value;

async function observe(page, check) {
  const selector = check.selector;
  if (check.type === 'exists') return (await page.locator(selector).count()) > 0;
  if (check.type === 'text' || check.type === 'contains-text' || check.type === 'not-text') return normalize(await page.textContent(selector));
  if (check.type === 'attribute') return normalize(await page.getAttribute(selector, check.name));
  if (check.type === 'class') return normalize(await page.getAttribute(selector, 'class'));
  if (check.type === 'url-path') return new URL(page.url()).pathname;
  if (check.type === 'number-min') {
    const raw = (await page.textContent(selector) || '').replace(/[^0-9.-]/g, '');
    const value = Number(raw);
    return Number.isFinite(value) ? value : null;
  }
  if (check.type === 'evaluate') return normalize(await page.evaluate(check.expression));
  throw new Error(`unsupported check type: ${check.type}`);
}

function expectedPass(value, check) {
  if (check.type === 'exists') return Boolean(value) === true;
  if (check.type === 'text') return value === normalize(String(check.equals ?? ''));
  if (check.type === 'contains-text') return String(value ?? '').includes(String(check.contains ?? ''));
  if (check.type === 'not-text') return value !== normalize(String(check.equals ?? ''));
  if (check.type === 'attribute') {
    if (check.exists === true) return value !== null;
    if (check.exists === false) return value === null;
    return value === normalize(String(check.equals ?? ''));
  }
  if (check.type === 'class') return String(value ?? '').split(/\s+/).includes(String(check.contains ?? ''));
  if (check.type === 'url-path') return String(value ?? '').endsWith(String(check.equals ?? ''));
  if (check.type === 'number-min') return typeof value === 'number' && value >= Number(check.min ?? 0);
  if (check.type === 'evaluate') return Boolean(value);
  return false;
}

function equivalent(source, dist, check) {
  if (check.equivalence === 'ignore') return true;
  if (check.type === 'number-min' && check.equivalence !== 'exact') {
    return expectedPass(source, check) && expectedPass(dist, check);
  }
  return JSON.stringify(source) === JSON.stringify(dist);
}

async function runAction(page, action, timeout) {
  if (action.type === 'click') await page.click(action.selector);
  else if (action.type === 'fill') await page.fill(action.selector, String(action.value ?? ''));
  else if (action.type === 'select') await page.selectOption(action.selector, String(action.value ?? ''));
  else if (action.type === 'wait') await page.waitForTimeout(Number(action.ms ?? 0));
  else if (action.type === 'wait-for-function') await page.waitForFunction(action.expression, null, { timeout: action.timeout_ms || timeout });
  else if (action.type === 'evaluate') await page.evaluate(action.expression);
  else throw new Error(`unsupported action type: ${action.type}`);
  if (action.wait_for_path) await page.waitForFunction(path => location.pathname.endsWith(path), action.wait_for_path, { timeout });
}

async function waitReady(page, scenario) {
  const timeout = scenario.timeout_ms || 30000;
  if (scenario.wait_for?.selector) await page.waitForSelector(scenario.wait_for.selector, { timeout });
  if (scenario.wait_for?.attribute) {
    const w = scenario.wait_for;
    await page.waitForFunction(({selector, name, equals}) => document.querySelector(selector)?.getAttribute(name) === equals, w, { timeout });
  }
}

async function captureScenario(context, baseUrl, scenario, errors, pageErrors) {
  const page = await context.newPage();
  page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
  page.on('pageerror', error => pageErrors.push(String(error?.stack || error)));
  const timeout = scenario.timeout_ms || 30000;
  const response = await page.goto(new URL(scenario.path || '/', baseUrl).toString(), { waitUntil: 'networkidle', timeout });
  if (!response || !response.ok()) throw new Error(`navigation failed: ${response ? response.status() : 'no response'}`);
  await waitReady(page, scenario);
  for (const action of scenario.actions || []) await runAction(page, action, timeout);
  const checks = [...(scenario.checks || []), ...(scenario.post_action_checks || [])];
  const observations = {};
  for (const check of checks) observations[check.id] = await observe(page, check);
  const snapshot = await page.evaluate(() => ({
    title: document.title,
    path: location.pathname,
    body_text: (document.body?.innerText || '').replace(/\s+/g, ' ').trim(),
    element_count: document.querySelectorAll('*').length,
  }));
  await page.close();
  return { observations, snapshot };
}

let browser;
try {
  browser = await launcher.launch({ headless: true });
  const sourceContext = await browser.newContext({ serviceWorkers: 'block' });
  const distContext = await browser.newContext({ serviceWorkers: 'block' });
  for (const scenario of manifest.scenarios || []) {
    const source = await captureScenario(sourceContext, sourceBaseUrl, scenario, result.source_console_errors, result.source_page_errors);
    const dist = await captureScenario(distContext, distBaseUrl, scenario, result.dist_console_errors, result.dist_page_errors);
    const comparisons = [];
    const checks = [...(scenario.checks || []), ...(scenario.post_action_checks || [])];
    for (const check of checks) {
      const sourceValue = source.observations[check.id];
      const distValue = dist.observations[check.id];
      comparisons.push({
        id: check.id,
        source: sourceValue,
        dist: distValue,
        source_expected: expectedPass(sourceValue, check),
        dist_expected: expectedPass(distValue, check),
        equivalent: equivalent(sourceValue, distValue, check),
      });
    }
    const snapshotEquivalent = scenario.compare_snapshot === true
      ? JSON.stringify(source.snapshot) === JSON.stringify(dist.snapshot)
      : true;
    const passed = comparisons.every(c => c.source_expected && c.dist_expected && c.equivalent) && snapshotEquivalent;
    result.scenarios.push({ id: scenario.id, passed, source, dist, snapshot_equivalent: snapshotEquivalent, comparisons });
  }
  result.passed = result.scenarios.every(s => s.passed)
    && result.source_console_errors.length === 0 && result.dist_console_errors.length === 0
    && result.source_page_errors.length === 0 && result.dist_page_errors.length === 0;
} catch (error) {
  result.error = String(error?.stack || error);
} finally {
  result.duration_ms = Math.round((performance.now() - started) * 1000) / 1000;
  if (browser) await browser.close();
}
console.log(JSON.stringify(result));
process.exit(result.passed ? 0 : 1);
