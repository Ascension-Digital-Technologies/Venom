// @venom: browser
// src/js/alerts-engine.js
// ===================== ALERTS ENGINE V4 =====================
const savedAlertsState = safeStorage.read();
state.alerts = Array.isArray(savedAlertsState.alerts) ? savedAlertsState.alerts : [];
state.alertSeq = Number.isFinite(savedAlertsState.alertSeq) ? savedAlertsState.alertSeq : 1;

function persistAlerts() {
  safeStorage.write({ alerts: state.alerts.slice(-100), alertSeq: state.alertSeq });
}

function nextAlertId() {
  return `ALERT-${state.alertSeq++}`;
}

function createPriceAlert(symbol = state.selectedSymbol, direction = 'above', price = symbolPrice(symbol)) {
  const alert = { id: nextAlertId(), symbol, direction, price, status: 'Active', createdAt: Date.now(), triggeredAt: null };
  state.alerts.unshift(alert);
  persistAlerts();
  showToast('success', 'Alert Added', `${symbol} ${direction} ${formatPrice(price)}`);
  renderBottomPanel(activeBottomTab());
  return alert;
}

function removeAlert(alertId) {
  state.alerts = state.alerts.filter(a => a.id !== alertId);
  persistAlerts();
  renderBottomPanel(activeBottomTab());
}

function checkAlerts() {
  for (const alert of state.alerts.filter(a => a.status === 'Active')) {
    const price = symbolPrice(alert.symbol);
    const hit = alert.direction === 'above' ? price >= alert.price : price <= alert.price;
    if (hit) {
      alert.status = 'Triggered';
      alert.triggeredAt = Date.now();
      showToast('success', 'Price Alert Triggered', `${alert.symbol} traded ${alert.direction} ${formatPrice(alert.price)}`);
    }
  }
  persistAlerts();
}

function renderAlertsTable() {
  if (!state.alerts.length) return `<div class="bottom-empty-state">No active alerts. Right-click the chart and choose Add Alert, or press Alt+A.</div>`;
  return `<table class="positions-table"><thead><tr><th>ID</th><th>Symbol</th><th>Condition</th><th>Price</th><th>Status</th><th>Created</th><th>Actions</th></tr></thead><tbody>${state.alerts.map(a => `
    <tr><td>${a.id}</td><td>${a.symbol}</td><td>${a.direction}</td><td>${formatPrice(a.price)}</td><td><span class="alert-pill ${a.status.toLowerCase()}">${a.status}</span></td><td>${formatTimestamp(a.createdAt)}</td><td><button class="action-btn danger" onclick="removeAlert('${a.id}')">Remove</button></td></tr>`).join('')}</tbody></table>`;
}

const v4BaseUpdateLiveDataForAlerts = updateLiveData;
updateLiveData = function updateLiveDataAlertsWrapper() {
  v4BaseUpdateLiveDataForAlerts();
  checkAlerts();
  if (activeBottomTab() === 'alerts') renderBottomPanel('alerts');
};

document.addEventListener('DOMContentLoaded', () => {
  const add = document.getElementById('ctxAddAlert');
  if (add) {
    add.onclick = () => {
      const p = symbolPrice(state.selectedSymbol);
      const dir = state.currentPrice >= state.prevPrice ? 'above' : 'below';
      const offset = dir === 'above' ? 1.0025 : 0.9975;
      createPriceAlert(state.selectedSymbol, dir, p * offset);
      document.getElementById('contextMenu').style.display = 'none';
    };
  }
});

window.NovaAlerts = { createPriceAlert, removeAlert, checkAlerts, renderAlertsTable };
