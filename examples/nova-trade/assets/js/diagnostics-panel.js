// @venom: browser
// src/js/diagnostics-panel.js

// ===================== V5 DIAGNOSTICS PANEL =====================
state.diagnostics = state.diagnostics || { ticks: 0, lastTick: Date.now(), renderCount: 0, warnings: [] };

function estimateStorageBytes() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY) || '';
    return new Blob([raw]).size;
  } catch { return 0; }
}

function renderDiagnosticsPanel() {
  const lastAge = Date.now() - (state.diagnostics.lastTick || Date.now());
  const storageKb = estimateStorageBytes() / 1024;
  const open = state.orders.filter(o => o.status === 'Open' || o.status === 'Partial').length;
  const conditional = state.conditionalOrders?.length || 0;
  const status = lastAge < 2500 ? 'Online' : 'Lagging';
  const statusClass = lastAge < 2500 ? 'diag-ok' : 'diag-warn';
  return `<div class="metric-grid">
    <div class="metric-card"><div class="label">Market Simulator</div><div class="value ${statusClass}">${status}</div><div class="sub">Last tick ${lastAge}ms ago</div></div>
    <div class="metric-card"><div class="label">Tick Count</div><div class="value">${state.diagnostics.ticks}</div><div class="sub">Interval ${state.simIntervalMs || 800}ms</div></div>
    <div class="metric-card"><div class="label">State Storage</div><div class="value">${storageKb.toFixed(1)} KB</div><div class="sub">localStorage snapshot</div></div>
    <div class="metric-card"><div class="label">Orders Watched</div><div class="value">${open + conditional}</div><div class="sub">Open ${open} · Conditional ${conditional}</div></div>
    <div class="metric-card"><div class="label">Provider Layer</div><div class="value diag-ok">Ready</div><div class="sub">Market/Paper/Portfolio adapters loaded</div></div>
    <div class="metric-card"><div class="label">Risk Engine</div><div class="value ${window.NovaRisk ? 'diag-ok' : 'diag-bad'}">${window.NovaRisk ? 'Loaded' : 'Missing'}</div><div class="sub">Pre-trade checks active</div></div>
    <div class="metric-card"><div class="label">Workspace</div><div class="value">${state.activeWorkspace || 'default'}</div><div class="sub">Saved presets: ${Object.keys(state.workspaces || {}).length}</div></div>
    <div class="metric-card"><div class="label">Audit Events</div><div class="value">${state.auditLog?.length || 0}</div><div class="sub">Session trace enabled</div></div>
  </div>`;
}

const v5UpdateLiveDataDiagBase = updateLiveData;
updateLiveData = function updateLiveDataV5Diagnostics() {
  state.diagnostics.ticks++;
  state.diagnostics.lastTick = Date.now();
  v5UpdateLiveDataDiagBase();
  if (activeBottomTab && activeBottomTab() === 'diagnostics') renderBottomPanel('diagnostics');
};

window.NovaDiagnostics = { render: renderDiagnosticsPanel, estimateStorageBytes };
