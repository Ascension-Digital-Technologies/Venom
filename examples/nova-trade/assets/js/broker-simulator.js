// @venom: browser
// src/js/broker-simulator.js
// ===================== BROKER SIMULATOR V4 =====================
// Builds on the v3 paper engine with realistic partial fills, maker/taker fees, and reserved balances.
state.broker = state.broker || { fillSeq: 1 };

function syncReservedBalances() {
  const quoteReservedByAsset = {};
  for (const order of state.orders.filter(o => o.status === 'Open')) {
    const quote = getQuoteAsset(order.symbol);
    const base = getBaseAsset(order.symbol);
    const remaining = Math.max(0, order.amount - (order.filled || 0));
    if (order.side === 'Buy') {
      quoteReservedByAsset[quote] = (quoteReservedByAsset[quote] || 0) + (order.price || symbolPrice(order.symbol)) * remaining;
    } else {
      quoteReservedByAsset[base] = (quoteReservedByAsset[base] || 0) + remaining;
    }
  }
  for (const balance of state.balances) {
    balance.reserved = quoteReservedByAsset[balance.asset] || 0;
    balance.total = Number(balance.available || 0) + Number(balance.reserved || 0);
  }
}

function markOrderEvent(order, status, reason) {
  const snapshot = { ...order, status, reason, ts: Date.now() };
  state.orderHistory.unshift(snapshot);
  if (state.orderHistory.length > 250) state.orderHistory.length = 250;
}

function simulatedFillRatio(order, price) {
  if (order.type === 'Market') return 1;
  const distance = Math.abs((order.price - price) / Math.max(price, 1));
  if (distance < 0.00005) return 0.25 + Math.random() * 0.35;
  if (distance < 0.00015) return 0.45 + Math.random() * 0.35;
  return 0.75 + Math.random() * 0.25;
}

function applyBrokerFill(order, fillAmount, fillPrice, reason = 'Filled') {
  fillAmount = Math.min(fillAmount, Math.max(0, order.amount - (order.filled || 0)));
  if (fillAmount <= 0) return null;
  const fillNotional = fillAmount * fillPrice;
  const liquidity = order.type === 'Limit' && order.postOnly ? 'maker' : 'taker';
  const fee = estimateFees(fillNotional, liquidity);
  const realized = applyPositionFill(order.side, order.symbol, fillAmount, fillPrice, { reduceOnly: order.reduceOnly });

  order.filled = Number(((order.filled || 0) + fillAmount).toFixed(8));
  order.avgFillPrice = order.avgFillPrice
    ? ((order.avgFillPrice * (order.filled - fillAmount)) + fillPrice * fillAmount) / order.filled
    : fillPrice;
  order.fees = Number(((order.fees || 0) + fee).toFixed(8));
  order.lastFillTs = Date.now();
  order.status = order.filled + 1e-8 >= order.amount ? 'Filled' : 'Partial';

  const trade = {
    id: nextOrderId(), orderId: order.id, ts: Date.now(), symbol: order.symbol, side: order.side,
    type: order.type, price: order.price, fillPrice, amount: fillAmount, status: order.status,
    realizedPnl: realized, fee, liquidity, reason
  };
  state.tradeHistory.unshift(trade);
  if (state.tradeHistory.length > 250) state.tradeHistory.length = 250;
  markOrderEvent(order, order.status, reason);
  return trade;
}

const v3SubmitPaperOrder = submitPaperOrder;
submitPaperOrder = function submitPaperOrderV4(side, override = null) {
  const result = override || validateOrderEntry();
  if (!result.valid && !override) {
    showToast('error', 'Order Blocked', document.getElementById('oeValidation').textContent || 'Check order fields');
    return null;
  }
  const type = override?.type || (result.type === 'market' ? 'Market' : result.type === 'stop' ? 'Stop Limit' : 'Limit');
  const symbol = override?.symbol || state.selectedSymbol;
  const price = type === 'Market' ? symbolPrice(symbol) : result.price;
  const amount = Number(override?.amount || result.amount);
  const reduceOnly = Boolean(document.querySelector('#reduceOnlyCheck .oe-check-box.checked')) || Boolean(override?.reduceOnly);
  const postOnly = Boolean(document.querySelector('#postOnlyCheck .oe-check-box.checked')) || Boolean(override?.postOnly);
  const order = { id: nextOrderId(), ts: Date.now(), symbol, side, type, price, amount, filled: 0, status: 'Open', reduceOnly, postOnly, fees: 0 };
  const risk = evaluateRisk(order);

  if (!risk.valid) {
    showToast('error', 'Risk Rejected', risk.blockers[0] || 'Risk checks failed');
    return null;
  }
  if (postOnly && shouldFillLimit(order, symbolPrice(symbol))) {
    order.status = 'Rejected';
    markOrderEvent(order, 'Rejected', 'Post-only would cross');
    showToast('error', 'Post Only Rejected', 'Order would immediately cross the simulated book');
    persistTradingState();
    renderBottomPanel(activeBottomTab());
    return null;
  }

  const marketPrice = symbolPrice(symbol);
  if (type === 'Market' || shouldFillLimit(order, marketPrice)) {
    applyBrokerFill(order, amount, marketPrice, type === 'Market' ? 'Market taker fill' : 'Immediate limit fill');
    order.status = 'Filled';
    showToast(side === 'Buy' ? 'success' : 'error', `${type} Filled`, `${side} ${formatAmount(amount)} ${getBaseAsset(symbol)} at ${formatPrice(marketPrice)}`);
  } else {
    state.orders.unshift(order);
    markOrderEvent(order, 'Open', 'Resting on simulated book');
    showToast('info', `${type} Order Open`, `${side} ${formatAmount(amount)} ${getBaseAsset(symbol)} at ${formatPrice(price)}`);
  }
  syncReservedBalances();
  persistTradingState();
  renderBottomPanel(activeBottomTab());
  renderPositions();
  syncOrderEntry();
  return order;
};

const v3ProcessOpenOrders = processOpenOrders;
processOpenOrders = function processOpenOrdersV4() {
  const openOrders = state.orders.filter(o => o.status === 'Open' || o.status === 'Partial');
  for (const order of openOrders) {
    const price = symbolPrice(order.symbol);
    const ageMs = Date.now() - order.ts;
    if (ageMs > 1000 * 60 * 60 * 6 && order.type !== 'Stop Limit') {
      order.status = 'Expired';
      markOrderEvent(order, 'Expired', 'Session expiration');
      continue;
    }
    if (shouldFillLimit(order, price)) {
      const remaining = Math.max(0, order.amount - (order.filled || 0));
      const ratio = simulatedFillRatio(order, price);
      const fillAmount = Math.min(remaining, Math.max(remaining * ratio, remaining < 0.0001 ? remaining : 0));
      if (fillAmount > 0) {
        applyBrokerFill(order, fillAmount, price, order.status === 'Partial' ? 'Additional partial fill' : 'Book crossed');
        if (order.status === 'Partial') {
          showToast('info', 'Partial Fill', `${order.id} filled ${formatAmount(order.filled)} / ${formatAmount(order.amount)}`);
        } else {
          showToast(order.side === 'Buy' ? 'success' : 'error', 'Order Filled', `${order.id} filled at ${formatPrice(price)}`);
        }
      }
    }
  }
  state.orders = state.orders.filter(o => o.status === 'Open' || o.status === 'Partial');
  syncReservedBalances();
  persistTradingState();
};

const v3CancelPaperOrder = cancelPaperOrder;
cancelPaperOrder = function cancelPaperOrderV4(orderId) {
  const order = state.orders.find(o => o.id === orderId);
  if (!order) return false;
  order.status = 'Cancelled';
  markOrderEvent(order, 'Cancelled', 'User cancel');
  state.orders = state.orders.filter(o => o.id !== orderId);
  syncReservedBalances();
  persistTradingState();
  showToast('info', 'Order Cancelled', `${order.id} removed from book`);
  renderBottomPanel(activeBottomTab());
  updateAccountSummary();
  return true;
};

document.addEventListener('DOMContentLoaded', () => syncReservedBalances());
window.NovaBroker = { syncReservedBalances, applyBrokerFill, simulatedFillRatio };
