import fs from 'node:fs';
import { chromium, firefox, webkit } from 'playwright';

const browserName = process.argv[2] || 'chromium';
const baseUrl = process.argv[3];
const manifestPath = process.argv[4];
if (!baseUrl || !manifestPath) {
  console.error('usage: browser-e2e-runner.mjs <chromium|firefox|webkit> <url> <manifest.json>');
  process.exit(2);
}
const launcher = { chromium, firefox, webkit }[browserName];
if (!launcher) process.exit(2);
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
const result = { browser: browserName, fixture: manifest.id || 'unnamed', url: baseUrl, passed: false, checks: [], console_errors: [], page_errors: [], duration_ms: 0, performance: { browser_launch_ms: 0, context_create_ms: 0, scenarios: [] } };
const started = performance.now();
function add(id, passed, detail='') { result.checks.push({id, passed:Boolean(passed), detail:String(detail ?? '')}); if (!passed) throw new Error(`${id}: ${detail}`); }
async function read(page, check) {
  const selector = check.selector;
  if (check.type === 'exists') return (await page.locator(selector).count()) > 0;
  if (check.type === 'text') return (await page.textContent(selector) || '') === String(check.equals ?? '');
  if (check.type === 'contains-text') return (await page.textContent(selector) || '').includes(String(check.contains ?? ''));
  if (check.type === 'attribute') { const value = await page.getAttribute(selector, check.name); if (check.exists === true) return value !== null; if (check.exists === false) return value === null; return value === String(check.equals ?? ''); }
  if (check.type === 'class') return (await page.getAttribute(selector, 'class') || '').split(/\s+/).includes(String(check.contains ?? ''));
  if (check.type === 'url-path') return new URL(page.url()).pathname.endsWith(String(check.equals ?? ''));
  throw new Error(`unsupported check type: ${check.type}`);
}
let browser;
try {
  const launchStarted = performance.now();
  browser = await launcher.launch({headless:true});
  result.performance.browser_launch_ms = Math.round((performance.now() - launchStarted) * 1000) / 1000;
  const contextStarted = performance.now();
  const context = await browser.newContext({serviceWorkers:'block'});
  result.performance.context_create_ms = Math.round((performance.now() - contextStarted) * 1000) / 1000;
  const page = await context.newPage();
  page.on('console', m => { if (m.type() === 'error') result.console_errors.push(m.text()); });
  page.on('pageerror', e => result.page_errors.push(String(e?.stack || e)));
  for (const scenario of manifest.scenarios || []) {
    const scenarioStarted = performance.now();
    const scenarioPerf = { id: scenario.id, navigation_wall_ms: 0, fixture_ready_ms: 0, action_ms: [], total_ms: 0, navigation_timing: null };
    const target = new URL(scenario.path || '/', baseUrl).toString();
    const navigationStarted = performance.now();
    const response = await page.goto(target, {waitUntil:'networkidle', timeout: scenario.timeout_ms || 30000});
    scenarioPerf.navigation_wall_ms = Math.round((performance.now() - navigationStarted) * 1000) / 1000;
    add(`${scenario.id}.navigation`, response && response.ok(), response ? `${response.status()} ${response.statusText()}` : 'no response');
    const readyStarted = performance.now();
    if (scenario.wait_for?.selector) await page.waitForSelector(scenario.wait_for.selector, {timeout: scenario.timeout_ms || 20000});
    if (scenario.wait_for?.attribute) {
      const w = scenario.wait_for;
      await page.waitForFunction(({selector,name,equals}) => document.querySelector(selector)?.getAttribute(name) === equals, w, {timeout: scenario.timeout_ms || 20000});
    }
    scenarioPerf.fixture_ready_ms = Math.round((performance.now() - readyStarted) * 1000) / 1000;
    scenarioPerf.navigation_timing = await page.evaluate(() => { const n = performance.getEntriesByType('navigation')[0]; return n ? { dom_content_loaded_ms: n.domContentLoadedEventEnd, load_event_ms: n.loadEventEnd, response_end_ms: n.responseEnd } : null; });
    for (const action of scenario.actions || []) {
      const actionStarted = performance.now();
      if (action.type === 'click') await page.click(action.selector);
      else if (action.type === 'fill') await page.fill(action.selector, String(action.value ?? ''));
      else throw new Error(`unsupported action type: ${action.type}`);
      if (action.wait_for_path) await page.waitForFunction(path => location.pathname.endsWith(path), action.wait_for_path, {timeout: scenario.timeout_ms || 20000});
      scenarioPerf.action_ms.push(Math.round((performance.now() - actionStarted) * 1000) / 1000);
    }
    for (const check of scenario.checks || []) add(`${scenario.id}.${check.id}`, await read(page, check), JSON.stringify(check));
    scenarioPerf.total_ms = Math.round((performance.now() - scenarioStarted) * 1000) / 1000;
    result.performance.scenarios.push(scenarioPerf);
  }
  add('runtime.page-errors', result.page_errors.length === 0, result.page_errors.join('\n'));
  add('runtime.console-errors', result.console_errors.length === 0, result.console_errors.join('\n'));
  result.passed = true;
} catch (e) { result.error = String(e?.stack || e); }
finally { result.duration_ms = Math.round((performance.now()-started) * 1000) / 1000; if (browser) await browser.close(); }
console.log(JSON.stringify(result));
process.exit(result.passed ? 0 : 1);
