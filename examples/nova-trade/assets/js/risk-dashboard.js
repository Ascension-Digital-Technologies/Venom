// @venom: browser
// src/js/risk-dashboard.js

// ===================== V5 RISK DASHBOARD =====================
function accountRiskSnapshot() {
  const unrealized = state.positions.reduce((sum, p) => sum + positionPnl(p), 0);
  const grossExposure = state.positions.reduce((sum, p) => sum + Math.abs(p.markPrice * p.size), 0);
  const marginUsed = state.positions.reduce((sum, p) => sum + (p.margin || Math.abs(p.avgEntry * p.size) * 0.1), 0);
  const equityNow = state.equity + unrealized + (state.realizedPnl || 0);
  const openNotional = state.orders.filter(o => o.status === 'Open' || o.status === 'Partial').reduce((sum, o) => sum + (o.price || symbolPrice(o.symbol)) * Math.max(0, o.amount - (o.filled || 0)), 0);
  const rawHealth = computeAccountHealth();
  const leveragePenalty = Math.min(45, Math.max(0, rawHealth.leverage - 1) * 18);
  const marginPenalty = rawHealth.level === 'critical' ? 45 : rawHealth.level === 'warning' ? 22 : 0;
  const orderPenalty = openNotional > equityNow * 0.75 ? 15 : openNotional > equityNow * 0.4 ? 7 : 0;
  const score = Math.max(0, Math.min(100, 94 - leveragePenalty - marginPenalty - orderPenalty));
  const health = { ...rawHealth, score, level: rawHealth.level };
  return { unrealized, grossExposure, marginUsed, equityNow, openNotional, health };
}

function stressPnl(movePct) {
  return state.positions.reduce((sum, p) => {
    const stressed = p.markPrice * (1 + movePct / 100);
    return sum + (stressed - p.avgEntry) * p.size * sideDirection(p.side);
  }, 0);
}

function renderRiskDashboard() {
  const r = accountRiskSnapshot();
  const cond = state.conditionalOrders?.length || 0;
  const score = Math.max(0, Math.min(100, r.health.score));
  return `<div class="metric-grid">
    <div class="metric-card"><div class="label">Account Health</div><div class="value ${score > 70 ? 'diag-ok' : score > 45 ? 'diag-warn' : 'diag-bad'}">${score.toFixed(0)}/100</div><div class="health-bar"><span style="width:${score}%"></span></div><div class="sub">${r.health.level}</div></div>
    <div class="metric-card"><div class="label">Gross Exposure</div><div class="value">${formatMoney(r.grossExposure)}</div><div class="sub">Open notional: ${formatMoney(r.openNotional)}</div></div>
    <div class="metric-card"><div class="label">Margin Used</div><div class="value">${formatMoney(r.marginUsed)}</div><div class="sub">Conditional exits: ${cond}</div></div>
    <div class="metric-card"><div class="label">Stress -3%</div><div class="value ${stressPnl(-3) >= 0 ? 'diag-ok' : 'diag-bad'}">${stressPnl(-3) >= 0 ? '+' : '-'}${Math.abs(stressPnl(-3)).toFixed(2)}</div><div class="sub">Stress +3%: ${stressPnl(3) >= 0 ? '+' : '-'}${Math.abs(stressPnl(3)).toFixed(2)}</div></div>
    <div class="metric-card"><div class="label">Realized PnL</div><div class="value ${(state.realizedPnl || 0) >= 0 ? 'diag-ok' : 'diag-bad'}">${(state.realizedPnl || 0) >= 0 ? '+' : '-'}${Math.abs(state.realizedPnl || 0).toFixed(2)}</div><div class="sub">Trade count: ${state.tradeHistory.length}</div></div>
    <div class="metric-card"><div class="label">Unrealized PnL</div><div class="value ${r.unrealized >= 0 ? 'diag-ok' : 'diag-bad'}">${r.unrealized >= 0 ? '+' : '-'}${Math.abs(r.unrealized).toFixed(2)}</div><div class="sub">Positions: ${state.positions.length}</div></div>
    <div class="metric-card"><div class="label">Open Orders</div><div class="value">${state.orders.filter(o => o.status === 'Open' || o.status === 'Partial').length}</div><div class="sub">Reserved balances synced</div></div>
    <div class="metric-card"><div class="label">Risk Guard</div><div class="value ${r.health.score > 45 ? 'diag-ok' : 'diag-warn'}">${r.health.score > 45 ? 'Ready' : 'Caution'}</div><div class="sub">Blocks over-limit orders</div></div>
  </div>`;
}

window.NovaRiskDashboard = { render: renderRiskDashboard, snapshot: accountRiskSnapshot };
