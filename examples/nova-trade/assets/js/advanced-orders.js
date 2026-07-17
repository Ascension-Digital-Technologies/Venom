// @venom: browser
// src/js/advanced-orders.js

// ===================== V5 ADVANCED / BRACKET ORDERS =====================
state.conditionalOrders = Array.isArray(savedTradingState.conditionalOrders) ? savedTradingState.conditionalOrders : [];
state.tifDefaults = savedTradingState.tifDefaults || { tif: 'GTC', tpPct: 1.5, slPct: 0.8 };

function readBracketConfig(side, price) {
  const tpOn = Boolean(document.querySelector('#tpCheck .oe-check-box.checked'));
  const slOn = Boolean(document.querySelector('#slCheck .oe-check-box.checked'));
  const tpPct = Math.max(0.1, Number(document.getElementById('tpPct')?.value || state.tifDefaults.tpPct || 1.5));
  const slPct = Math.max(0.1, Number(document.getElementById('slPct')?.value || state.tifDefaults.slPct || 0.8));
  const tif = document.getElementById('orderTif')?.value || 'GTC';
  safeStorage.write({ tifDefaults: { tif, tpPct, slPct } });
  return { enabled: tpOn || slOn, tpOn, slOn, tpPct, slPct, tif, entrySide: side, entryPrice: price };
}

function exitSideForEntry(side) { return side === 'Buy' ? 'Sell' : 'Buy'; }
function targetPriceFor(kind, cfg, entryPrice) {
  const pct = (kind === 'TP' ? cfg.tpPct : cfg.slPct) / 100;
  if (cfg.entrySide === 'Buy') return kind === 'TP' ? entryPrice * (1 + pct) : entryPrice * (1 - pct);
  return kind === 'TP' ? entryPrice * (1 - pct) : entryPrice * (1 + pct);
}

function createBracketOrders(parentOrder, fillPrice) {
  const cfg = parentOrder.bracketConfig;
  if (!cfg || !cfg.enabled || parentOrder.bracketsCreated) return [];
  const remaining = Number(parentOrder.amount || 0);
  if (!(remaining > 0)) return [];
  const ocoGroup = `OCO-${parentOrder.id}`;
  const created = [];
  const add = (kind) => {
    const price = targetPriceFor(kind, cfg, fillPrice);
    const child = {
      id: nextOrderId(), parentId: parentOrder.id, ocoGroup, ts: Date.now(), symbol: parentOrder.symbol,
      side: exitSideForEntry(parentOrder.side), type: kind === 'TP' ? 'Take Profit' : 'Stop Loss', kind,
      price, triggerPrice: price, amount: remaining, filled: 0, status: 'Open', reduceOnly: true,
      tif: cfg.tif, isConditional: true, fees: 0
    };
    state.conditionalOrders.unshift(child);
    created.push(child);
  };
  if (cfg.tpOn) add('TP');
  if (cfg.slOn) add('SL');
  parentOrder.bracketsCreated = true;
  if (created.length) {
    auditEvent('BRACKET', `Created ${created.length} bracket order(s) for ${parentOrder.id}`);
    showToast('info', 'Bracket Orders Created', `${created.length} linked reduce-only exits are now monitoring price`);
  }
  persistTradingState();
  return created;
}

function shouldTriggerConditional(order, price) {
  if (order.kind === 'TP') return order.side === 'Sell' ? price >= order.triggerPrice : price <= order.triggerPrice;
  if (order.kind === 'SL') return order.side === 'Sell' ? price <= order.triggerPrice : price >= order.triggerPrice;
  return false;
}

function cancelOcoSiblings(filledOrder) {
  if (!filledOrder.ocoGroup) return;
  for (const sibling of state.conditionalOrders.filter(o => o.ocoGroup === filledOrder.ocoGroup && o.id !== filledOrder.id)) {
    sibling.status = 'Cancelled';
    markOrderEvent(sibling, 'Cancelled', `OCO sibling cancelled by ${filledOrder.id}`);
    auditEvent('OCO', `${sibling.id} cancelled after ${filledOrder.id} triggered`);
  }
  state.conditionalOrders = state.conditionalOrders.filter(o => o.status === 'Open' || o.status === 'Partial');
}

function processConditionalOrders() {
  for (const order of state.conditionalOrders.filter(o => o.status === 'Open' || o.status === 'Partial')) {
    const price = symbolPrice(order.symbol);
    const ageMs = Date.now() - order.ts;
    if (order.tif === 'DAY' && ageMs > 1000 * 60 * 60 * 24) {
      order.status = 'Expired';
      markOrderEvent(order, 'Expired', 'DAY conditional expired');
      auditEvent('ORDER', `${order.id} expired`);
      continue;
    }
    if (shouldTriggerConditional(order, price)) {
      const remaining = Math.max(0, order.amount - (order.filled || 0));
      const trade = applyBrokerFill(order, remaining, price, `${order.type} trigger`);
      order.status = 'Filled';
      cancelOcoSiblings(order);
      auditEvent('FILL', `${order.id} ${order.type} triggered at ${formatPrice(price)}`);
      showToast(order.side === 'Sell' ? 'error' : 'success', `${order.type} Triggered`, `${order.side} ${formatAmount(remaining)} at ${formatPrice(price)}`);
    }
  }
  state.conditionalOrders = state.conditionalOrders.filter(o => o.status === 'Open' || o.status === 'Partial');
  syncReservedBalances();
  persistTradingState();
}

const v5PersistTradingStateBase = persistTradingState;
persistTradingState = function persistTradingStateV5() {
  v5PersistTradingStateBase();
  safeStorage.write({
    conditionalOrders: state.conditionalOrders.slice(0, 100),
    tifDefaults: state.tifDefaults,
    auditLog: state.auditLog ? state.auditLog.slice(0, 500) : []
  });
};

const v5SubmitPaperOrderBase = submitPaperOrder;
submitPaperOrder = function submitPaperOrderV5(side, override = null) {
  const price = override?.price || (getActiveOrderType && getActiveOrderType() === 'market' ? state.currentPrice : parseNumber(document.getElementById('oePrice')?.value));
  const cfg = readBracketConfig(side, price || state.currentPrice);
  const order = v5SubmitPaperOrderBase(side, override);
  if (order && cfg.enabled && !order.reduceOnly) {
    order.bracketConfig = cfg;
    if (order.status === 'Filled') createBracketOrders(order, order.avgFillPrice || order.price || symbolPrice(order.symbol));
    auditEvent('ORDER', `${order.id} submitted with ${cfg.tpOn ? 'TP ' : ''}${cfg.slOn ? 'SL ' : ''}${cfg.tif}`.trim());
    persistTradingState();
  } else if (order) {
    auditEvent('ORDER', `${order.id} ${order.side} ${order.type} submitted`);
  }
  return order;
};

const v5ApplyBrokerFillBase = applyBrokerFill;
applyBrokerFill = function applyBrokerFillV5(order, fillAmount, fillPrice, reason = 'Filled') {
  const trade = v5ApplyBrokerFillBase(order, fillAmount, fillPrice, reason);
  if (trade && order.bracketConfig && order.status === 'Filled') createBracketOrders(order, fillPrice);
  if (trade) auditEvent('FILL', `${order.id} filled ${formatAmount(fillAmount)} at ${formatPrice(fillPrice)}`);
  return trade;
};

const v5ProcessOpenOrdersBase = processOpenOrders;
processOpenOrders = function processOpenOrdersV5() {
  v5ProcessOpenOrdersBase();
  processConditionalOrders();
};

function cancelConditionalOrder(orderId) {
  const order = state.conditionalOrders.find(o => o.id === orderId);
  if (!order) return false;
  order.status = 'Cancelled';
  markOrderEvent(order, 'Cancelled', 'User cancel');
  state.conditionalOrders = state.conditionalOrders.filter(o => o.id !== orderId);
  auditEvent('ORDER', `${order.id} conditional cancelled`);
  persistTradingState();
  renderBottomPanel(activeBottomTab());
  return true;
}

function renderBracketOrders() {
  const rows = state.conditionalOrders || [];
  if (!rows.length) return `<div class="bottom-empty-state">No active bracket orders. Enable Take Profit and/or Stop Loss in Order Entry before submitting.</div>`;
  return `<table class="positions-table"><thead><tr><th>ID</th><th>Parent</th><th>Symbol</th><th>Kind</th><th>Side</th><th>Trigger</th><th>Amount</th><th>TIF</th><th>Status</th><th>Actions</th></tr></thead><tbody>${rows.map(o => `<tr><td>${o.id}</td><td>${o.parentId || '-'}</td><td>${o.symbol}</td><td><span class="bracket-tag">${o.kind}</span></td><td class="side-${o.side === 'Buy' ? 'long' : 'short'}">${o.side}</td><td>${formatPrice(o.triggerPrice || o.price)}</td><td>${formatAmount(o.amount)}</td><td>${o.tif || 'GTC'}</td><td><span class="status-pill ${statusClass(o.status)}">${o.status}</span></td><td><button class="action-btn danger" onclick="cancelConditionalOrder('${o.id}')">Cancel</button></td></tr>`).join('')}</tbody></table>`;
}

window.NovaAdvancedOrders = { processConditionalOrders, renderBracketOrders, cancelConditionalOrder };
window.cancelConditionalOrder = cancelConditionalOrder;

document.addEventListener('DOMContentLoaded', () => {
  const saved = safeStorage.read().tifDefaults || state.tifDefaults;
  if (document.getElementById('tpPct')) document.getElementById('tpPct').value = saved.tpPct || 1.5;
  if (document.getElementById('slPct')) document.getElementById('slPct').value = saved.slPct || 0.8;
  if (document.getElementById('orderTif')) document.getElementById('orderTif').value = saved.tif || 'GTC';
  ['tpPct','slPct','orderTif'].forEach(id => document.getElementById(id)?.addEventListener('change', () => {
    state.tifDefaults = {
      tpPct: Number(document.getElementById('tpPct')?.value || 1.5),
      slPct: Number(document.getElementById('slPct')?.value || 0.8),
      tif: document.getElementById('orderTif')?.value || 'GTC'
    };
    safeStorage.write({ tifDefaults: state.tifDefaults });
  }));
});
