// @venom: browser
// src/js/workspace-manager.js

// ===================== V5 WORKSPACE MANAGER =====================
state.workspaces = savedTradingState.workspaces || {
  default: { name: 'Default', symbol: state.selectedSymbol, timeframe: '1h', layoutPreset: state.layoutPreset || 'default', createdAt: Date.now(), updatedAt: Date.now() }
};
state.activeWorkspace = savedTradingState.activeWorkspace || 'default';

function captureWorkspace(name = 'Workspace') {
  const saved = safeStorage.read();
  return {
    name,
    symbol: state.selectedSymbol,
    timeframe: document.querySelector('.tf-btn.active')?.dataset.tf || '1h',
    layoutPreset: state.layoutPreset || 'default',
    panelSizes: saved.panelSizes || {},
    watchlistTab: saved.watchlistTab || 'favorites',
    orderType: saved.orderType || 'limit',
    bottomTab: activeBottomTab(),
    chartTab: saved.chartTab || 'Chart',
    settings: saved.settings || {},
    createdAt: Date.now(), updatedAt: Date.now()
  };
}

function persistWorkspaces() {
  safeStorage.write({ workspaces: state.workspaces, activeWorkspace: state.activeWorkspace });
}

function updateWorkspaceLabel() {
  const el = document.getElementById('workspaceName');
  if (el) el.textContent = state.workspaces[state.activeWorkspace]?.name || 'Default';
}

function saveActiveWorkspace() {
  const current = state.workspaces[state.activeWorkspace] || { name: 'Default' };
  state.workspaces[state.activeWorkspace] = { ...captureWorkspace(current.name), createdAt: current.createdAt || Date.now() };
  persistWorkspaces();
  auditEvent('WORKSPACE', `Saved workspace ${current.name}`);
  showToast('success', 'Workspace Saved', `${current.name} layout and state saved`);
  renderWorkspaceDrawer();
  updateWorkspaceLabel();
}

function createWorkspace() {
  const name = prompt('Workspace name', `Workspace ${Object.keys(state.workspaces).length + 1}`);
  if (!name) return;
  const id = name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/(^-|-$)/g, '') || `workspace-${Date.now()}`;
  state.workspaces[id] = captureWorkspace(name);
  state.activeWorkspace = id;
  persistWorkspaces();
  updateWorkspaceLabel();
  auditEvent('WORKSPACE', `Created workspace ${name}`);
  renderWorkspaceDrawer();
}

function loadWorkspace(id) {
  const ws = state.workspaces[id];
  if (!ws) return;
  state.activeWorkspace = id;
  safeStorage.write({
    selectedSymbol: ws.symbol,
    timeframe: ws.timeframe,
    layoutPreset: ws.layoutPreset,
    panelSizes: ws.panelSizes || {},
    watchlistTab: ws.watchlistTab,
    orderType: ws.orderType,
    bottomTab: ws.bottomTab,
    chartTab: ws.chartTab
  });
  if (ws.symbol) switchSymbol(ws.symbol);
  if (ws.layoutPreset && window.NovaLayouts) applyLayoutPreset(ws.layoutPreset);
  document.querySelectorAll('.tf-btn').forEach(btn => btn.classList.toggle('active', btn.dataset.tf === ws.timeframe));
  document.querySelectorAll('.bottom-tab').forEach(tab => tab.classList.toggle('active', tab.dataset.tab === (ws.bottomTab || 'positions')));
  applyStoredLayout();
  renderBottomPanel(ws.bottomTab || 'positions');
  persistWorkspaces();
  updateWorkspaceLabel();
  auditEvent('WORKSPACE', `Loaded workspace ${ws.name}`);
  showToast('info', 'Workspace Loaded', ws.name);
}

function deleteWorkspace(id) {
  if (id === 'default') { showToast('error', 'Default Workspace', 'Default workspace cannot be deleted'); return; }
  const name = state.workspaces[id]?.name || id;
  delete state.workspaces[id];
  if (state.activeWorkspace === id) state.activeWorkspace = 'default';
  persistWorkspaces();
  updateWorkspaceLabel();
  auditEvent('WORKSPACE', `Deleted workspace ${name}`);
  renderWorkspaceDrawer();
}

function exportWorkspace() {
  const payload = { exportedAt: new Date().toISOString(), activeWorkspace: state.activeWorkspace, workspaces: state.workspaces };
  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'nova-trade-workspaces.json';
  a.click();
  URL.revokeObjectURL(url);
  auditEvent('EXPORT', 'Workspace package exported');
}

function ensureWorkspaceDrawer() {
  if (document.getElementById('workspaceDrawer')) return;
  const drawer = document.createElement('div');
  drawer.className = 'workspace-drawer';
  drawer.id = 'workspaceDrawer';
  drawer.innerHTML = `<div class="workspace-drawer-header"><strong>Workspace Manager</strong><button id="workspaceCloseBtn">Close</button></div><div class="workspace-drawer-body" id="workspaceDrawerBody"></div>`;
  document.body.appendChild(drawer);
  document.getElementById('workspaceCloseBtn').addEventListener('click', () => drawer.classList.remove('active'));
}

function renderWorkspaceDrawer() {
  ensureWorkspaceDrawer();
  const body = document.getElementById('workspaceDrawerBody');
  const rows = Object.entries(state.workspaces).map(([id, ws]) => `<div class="workspace-row"><div class="meta"><strong>${ws.name}${id === state.activeWorkspace ? ' · active' : ''}</strong><span>${ws.symbol || 'BTC/USDT'} · ${ws.timeframe || '1h'} · ${ws.layoutPreset || 'default'}</span></div><div class="actions"><button onclick="NovaWorkspaces.load('${id}')">Load</button><button onclick="NovaWorkspaces.delete('${id}')">Delete</button></div></div>`).join('');
  body.innerHTML = `<div style="display:flex;gap:8px;"><button onclick="NovaWorkspaces.create()">New</button><button onclick="NovaWorkspaces.save()">Save Current</button><button onclick="NovaWorkspaces.export()">Export</button></div>${rows}`;
}

function toggleWorkspaceDrawer() {
  ensureWorkspaceDrawer();
  renderWorkspaceDrawer();
  document.getElementById('workspaceDrawer').classList.toggle('active');
}

const v5PersistTradingStateWorkspaceBase = persistTradingState;
persistTradingState = function persistTradingStateV5Workspace() {
  v5PersistTradingStateWorkspaceBase();
  persistWorkspaces();
};

window.NovaWorkspaces = { save: saveActiveWorkspace, create: createWorkspace, load: loadWorkspace, delete: deleteWorkspace, export: exportWorkspace, toggle: toggleWorkspaceDrawer };

document.addEventListener('DOMContentLoaded', () => {
  updateWorkspaceLabel();
  document.getElementById('workspaceSaveBtn')?.addEventListener('click', saveActiveWorkspace);
  document.getElementById('workspaceManageBtn')?.addEventListener('click', toggleWorkspaceDrawer);
  document.getElementById('workspaceSwitcher')?.addEventListener('dblclick', createWorkspace);
  document.addEventListener('keydown', (e) => {
    const typing = ['INPUT', 'TEXTAREA', 'SELECT'].includes(e.target.tagName) || e.target.isContentEditable;
    if (!typing && e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 's') { e.preventDefault(); saveActiveWorkspace(); }
    if (!typing && e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'w') { e.preventDefault(); toggleWorkspaceDrawer(); }
  });
});
