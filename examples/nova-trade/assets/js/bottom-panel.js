// @venom: browser
// src/js/bottom-panel.js
// ===================== BOTTOM PANEL VIEWS =====================
function statusClass(status) {
  return status === 'Open' ? 'status-open' : status === 'Partial' ? 'status-partial' : status === 'Filled' ? 'status-filled' : status === 'Cancelled' ? 'status-cancelled' : status === 'Expired' ? 'status-expired' : 'status-rejected';
}

function renderOrdersTable(rows, mode = 'open') {
  if (!rows.length) return `<div class="bottom-empty-state">${mode === 'open' ? 'No open orders. Resting limit orders will appear here.' : 'No history yet.'}</div>`;
  return `<table class="positions-table"><thead><tr><th>ID</th><th>Time</th><th>Symbol</th><th>Side</th><th>Type</th><th>Price</th><th>Amount</th><th>Status</th><th>Filled</th><th>Fees</th><th>Actions</th></tr></thead><tbody>${rows.map(o => `
    <tr><td>${o.id}</td><td>${formatTimestamp(o.ts)}</td><td>${o.symbol}</td><td class="side-${o.side === 'Buy' ? 'long' : 'short'}">${o.side}</td><td>${o.type}</td><td>${formatPrice(o.fillPrice || o.price)}</td><td>${formatAmount(o.amount)}</td><td><span class="status-pill ${statusClass(o.status)}">${o.status}</span></td><td>${formatAmount(o.filled || 0)} / ${formatAmount(o.amount)}</td><td>${formatMoney(o.fees || 0)}</td><td>${o.status === 'Open' || o.status === 'Partial' ? `<button class="action-btn danger" onclick="${o.isConditional ? 'cancelConditionalOrder' : 'cancelPaperOrder'}('${o.id}')">Cancel</button>` : ''}</td></tr>`).join('')}</tbody></table>`;
}

function renderTradeHistory() {
  const rows = state.tradeHistory.slice(0, 100);
  if (!rows.length) return `<div class="bottom-empty-state">No executed trades yet.</div>`;
  return `<table class="positions-table"><thead><tr><th>Time</th><th>Symbol</th><th>Side</th><th>Fill Price</th><th>Amount</th><th>Realized PnL</th><th>Fee</th><th>Liquidity</th><th>Reason</th></tr></thead><tbody>${rows.map(t => {
    const cls = t.realizedPnl > 0 ? 'pnl-pos' : t.realizedPnl < 0 ? 'pnl-neg' : 'pnl-zero';
    return `<tr><td>${formatTimestamp(t.ts)}</td><td>${t.symbol}</td><td class="side-${t.side === 'Buy' ? 'long' : 'short'}">${t.side}</td><td>${formatPrice(t.fillPrice || t.price)}</td><td>${formatAmount(t.amount)}</td><td class="${cls}">${t.realizedPnl >= 0 ? '+' : '-'}${Math.abs(t.realizedPnl || 0).toFixed(2)} USDT</td><td>${formatMoney(t.fee || 0)} USDT</td><td>${t.liquidity || '-'}</td><td>${t.reason || 'Filled'}</td></tr>`;
  }).join('')}</tbody></table>`;
}

function renderBalances() {
  return `<table class="positions-table"><thead><tr><th>Asset</th><th>Available</th><th>Reserved</th><th>Total</th><th>Value</th></tr></thead><tbody>${state.balances.map(b => {
    const price = b.asset === 'USDT' ? 1 : (state.watchlist.find(w => w.sym.startsWith(`${b.asset}/`))?.last || 0);
    const value = b.total * price;
    return `<tr><td>${b.asset}</td><td>${Number(b.available).toLocaleString()}</td><td>${Number(b.reserved).toLocaleString()}</td><td>${Number(b.total).toLocaleString()}</td><td>${formatMoney(value)} USDT</td></tr>`;
  }).join('')}</tbody></table>`;
}

function renderPerformance() {
  const unrealized = state.positions.reduce((sum, p) => sum + positionPnl(p), 0);
  const closed = state.realizedPnl || 0;
  const open = state.orders.filter(o => o.status === 'Open' || o.status === 'Partial').length;
  return `<div style="display:grid;grid-template-columns:repeat(4,minmax(160px,1fr));gap:10px;min-width:720px;">
    <div class="oe-chip">Realized PnL <strong style="color:${closed >= 0 ? 'var(--green)' : 'var(--red)'}">${closed >= 0 ? '+' : '-'}${Math.abs(closed).toFixed(2)} USDT</strong></div>
    <div class="oe-chip">Unrealized <strong style="color:${unrealized >= 0 ? 'var(--green)' : 'var(--red)'}">${unrealized >= 0 ? '+' : '-'}${Math.abs(unrealized).toFixed(2)} USDT</strong></div>
    <div class="oe-chip">Open Orders <strong>${open}</strong></div>
    <div class="oe-chip">Trades <strong>${state.tradeHistory.length}</strong></div>
  </div>`;
}

function renderBottomPanel(tab = activeBottomTab()) {
  const bottom = document.getElementById('bottomContent');
  if (!bottom) return;
  updateAccountSummary();
  if (tab === 'positions') {
    ensurePositionsTable();
    renderPositions();
    return;
  }
  if (tab === 'openorders') bottom.innerHTML = renderOrdersTable([...state.orders.filter(o => o.status === 'Open' || o.status === 'Partial'), ...(state.conditionalOrders || [])], 'open');
  else if (tab === 'orderhistory') bottom.innerHTML = renderOrdersTable(state.orderHistory.slice(0, 100), 'history');
  else if (tab === 'tradehistory') bottom.innerHTML = renderTradeHistory();
  else if (tab === 'balances') bottom.innerHTML = renderBalances();
  else if (tab === 'performance') bottom.innerHTML = renderPerformance();
  else if (tab === 'risk') bottom.innerHTML = (window.NovaRiskDashboard ? NovaRiskDashboard.render() : `<div class="bottom-empty-state">Risk dashboard not loaded.</div>`);
  else if (tab === 'brackets') bottom.innerHTML = (window.NovaAdvancedOrders ? NovaAdvancedOrders.renderBracketOrders() : `<div class="bottom-empty-state">Advanced orders not loaded.</div>`);
  else if (tab === 'diagnostics') bottom.innerHTML = (window.NovaDiagnostics ? NovaDiagnostics.render() : `<div class="bottom-empty-state">Diagnostics not loaded.</div>`);
  else if (tab === 'audit') bottom.innerHTML = (window.NovaAudit ? NovaAudit.render() : `<div class="bottom-empty-state">Audit log not loaded.</div>`);
  else if (tab === 'alerts') bottom.innerHTML = (window.NovaAlerts ? NovaAlerts.renderAlertsTable() : `<div class="bottom-empty-state">No active alerts. Right-click the chart and choose Add Alert.</div>`);
  else if (tab === 'logs') bottom.innerHTML = `<div class="bottom-empty-state">System logs are clean. Market simulator and paper engine are online.</div>`;
}

document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('.bottom-tab').forEach(tab => {
    tab.addEventListener('click', () => setTimeout(() => renderBottomPanel(tab.dataset.tab), 0));
  });
  renderBottomPanel(activeBottomTab());
});
