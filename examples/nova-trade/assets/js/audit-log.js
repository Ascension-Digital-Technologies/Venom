// @venom: browser
// src/js/audit-log.js

// ===================== V5 AUDIT LOG =====================
state.auditLog = Array.isArray(savedTradingState.auditLog) ? savedTradingState.auditLog : [];

function auditEvent(kind, message, data = {}) {
  const item = { ts: Date.now(), kind, message, data };
  state.auditLog.unshift(item);
  if (state.auditLog.length > 500) state.auditLog.length = 500;
  try { safeStorage.write({ auditLog: state.auditLog.slice(0, 500) }); } catch {}
  return item;
}

function renderAuditLog() {
  if (!state.auditLog.length) return `<div class="bottom-empty-state">No audit events yet. Orders, workspace changes, risk events, and exports will appear here.</div>`;
  return `<div class="audit-toolbar"><button class="panel-action-btn" onclick="NovaAudit.export()">Export Audit JSON</button><button class="panel-action-btn" onclick="NovaAudit.clear()">Clear</button></div>
    <div>${state.auditLog.slice(0, 200).map(e => `<div class="audit-line"><span class="time">${formatTimestamp(e.ts)}</span><span class="kind">${e.kind}</span><span>${e.message}</span></div>`).join('')}</div>`;
}

function exportAuditLog() {
  const blob = new Blob([JSON.stringify(state.auditLog, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `nova-trade-audit-${new Date().toISOString().slice(0,19).replace(/[:T]/g,'-')}.json`;
  a.click();
  URL.revokeObjectURL(url);
  auditEvent('EXPORT', 'Audit log exported');
}

function clearAuditLog() {
  state.auditLog = [];
  safeStorage.write({ auditLog: [] });
  renderBottomPanel(activeBottomTab());
}

window.NovaAudit = { event: auditEvent, render: renderAuditLog, export: exportAuditLog, clear: clearAuditLog };

document.addEventListener('DOMContentLoaded', () => {
  auditEvent('BOOT', 'Terminal session initialized');
});
