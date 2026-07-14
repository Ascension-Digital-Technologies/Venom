// @venom: browser
// src/js/settings-drawer.js
// ===================== SETTINGS DRAWER V4 =====================
const SETTINGS_KEY = 'novaTrade.settings.v4';
function readSettings() {
  try { return JSON.parse(localStorage.getItem(SETTINGS_KEY) || '{}'); } catch { return {}; }
}
function writeSettings(patch) {
  try { localStorage.setItem(SETTINGS_KEY, JSON.stringify({ ...readSettings(), ...patch })); } catch {}
}

state.simIntervalMs = readSettings().simIntervalMs || 800;
state.density = readSettings().density || 'comfortable';
state.chartTools = { ...state.chartTools, ...(readSettings().indicators || {}) };
state.indicators = state.chartTools;

function applyDensity(density) {
  state.density = density;
  document.body.dataset.density = density;
  document.querySelectorAll('#densityControl button').forEach(btn => btn.classList.toggle('active', btn.dataset.density === density));
  writeSettings({ density });
}

function applyVolatilityMode(mode) {
  const map = {
    calm: { volatility: 0.55, baseDrift: 0.000006 },
    normal: { volatility: 1.0, baseDrift: 0.000015 },
    volatile: { volatility: 1.85, baseDrift: 0.00002 },
    event: { volatility: 3.2, baseDrift: 0.000055 }
  };
  const cfg = map[mode] || map.normal;
  state.marketMode = mode;
  state.volatility = cfg.volatility;
  state.baseDrift = cfg.baseDrift;
  writeSettings({ volatilityMode: mode });
  showToast('info', 'Volatility Mode', `${mode} simulator profile applied`);
}

function refreshSettingsMetrics() {
  const box = document.getElementById('settingsAccountMetrics');
  if (!box) return;
  const health = computeAccountHealth();
  box.innerHTML = `
    <div class="settings-metric"><span>Equity</span><strong>${formatMoney(health.equity)} USDT</strong></div>
    <div class="settings-metric"><span>Free Collateral</span><strong>${formatMoney(health.freeCollateral)} USDT</strong></div>
    <div class="settings-metric"><span>Leverage</span><strong>${health.leverage.toFixed(2)}x</strong></div>
    <div class="settings-metric"><span>Risk Health</span><strong>${health.level.toUpperCase()}</strong></div>`;
}

function openSettingsDrawer() {
  refreshSettingsMetrics();
  const overlay = document.getElementById('settingsDrawerOverlay');
  overlay.classList.add('active');
  overlay.setAttribute('aria-hidden', 'false');
}
function closeSettingsDrawer() {
  const overlay = document.getElementById('settingsDrawerOverlay');
  overlay.classList.remove('active');
  overlay.setAttribute('aria-hidden', 'true');
}

function resetPaperAccount() {
  state.orders = [];
  state.orderHistory = [];
  state.tradeHistory = [];
  state.realizedPnl = 0;
  state.positions = [];
  state.equity = 100000;
  state.balances = [{ asset: 'USDT', available: 100000, reserved: 0, total: 100000 }];
  persistTradingState();
  renderBottomPanel(activeBottomTab());
  updateAccountSummary();
  refreshSettingsMetrics();
  showToast('success', 'Paper Account Reset', 'Orders, trades, positions, and balances reset');
}

function exportState() {
  const data = JSON.stringify({ state, layout: safeStorage.read(), settings: readSettings() }, null, 2);
  const blob = new Blob([data], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = `nova-trade-state-${Date.now()}.json`;
  link.click();
  URL.revokeObjectURL(url);
  showToast('success', 'State Exported', 'Paper terminal state downloaded');
}

let simulationTimer = null;
function restartSimulationLoop() {
  if (window.__novaSimTimer) clearInterval(window.__novaSimTimer);
  window.__novaSimTimer = setInterval(updateLiveData, state.simIntervalMs || 800);
}

document.addEventListener('DOMContentLoaded', () => {
  applyDensity(state.density);
  const saved = readSettings();
  const settingsButton = document.getElementById('settingsButton');
  const close = document.getElementById('settingsClose');
  const overlay = document.getElementById('settingsDrawerOverlay');
  if (settingsButton) settingsButton.addEventListener('click', openSettingsDrawer);
  if (close) close.addEventListener('click', closeSettingsDrawer);
  if (overlay) overlay.addEventListener('click', e => { if (e.target === overlay) closeSettingsDrawer(); });

  document.querySelectorAll('#densityControl button').forEach(btn => btn.addEventListener('click', () => applyDensity(btn.dataset.density)));
  const speed = document.getElementById('simSpeed');
  const speedLabel = document.getElementById('simSpeedLabel');
  if (speed) {
    speed.value = state.simIntervalMs;
    speedLabel.textContent = `${state.simIntervalMs} ms`;
    speed.addEventListener('input', () => { state.simIntervalMs = Number(speed.value); speedLabel.textContent = `${state.simIntervalMs} ms`; writeSettings({ simIntervalMs: state.simIntervalMs }); restartSimulationLoop(); });
  }
  const vol = document.getElementById('volatilityMode');
  if (vol) {
    vol.value = saved.volatilityMode || state.marketMode || 'normal';
    vol.addEventListener('change', () => applyVolatilityMode(vol.value));
    applyVolatilityMode(vol.value);
  }
  const indicatorMap = { settingEma20: 'showEma20', settingEma50: 'showEma50', settingEma200: 'showEma200', settingVolume: 'showVolume', settingRsi: 'showRsi' };
  Object.entries(indicatorMap).forEach(([id, key]) => {
    const el = document.getElementById(id);
    if (!el) return;
    el.checked = state.chartTools[key] !== false;
    el.addEventListener('change', () => { state.chartTools[key] = el.checked; writeSettings({ indicators: state.chartTools }); resizeCanvas(); });
  });
  document.getElementById('resetLayoutBtn')?.addEventListener('click', () => { safeStorage.write({ panelSizes: {}, layoutPreset: 'default' }); location.reload(); });
  document.getElementById('resetPaperBtn')?.addEventListener('click', resetPaperAccount);
  document.getElementById('exportStateBtn')?.addEventListener('click', exportState);
  restartSimulationLoop();
});

window.NovaSettings = { openSettingsDrawer, closeSettingsDrawer, resetPaperAccount, exportState, restartSimulationLoop };
