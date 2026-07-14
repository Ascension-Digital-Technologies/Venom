// @venom: browser
// src/js/paper-trading.js
// ===================== FUNCTIONAL PAPER TRADING V3 =====================
function positionPnl(position) {
  const dir = sideDirection(position.side);
  return (position.markPrice - position.avgEntry) * position.size * dir;
}

function updateAccountSummary() {
  const unrealized = state.positions.reduce((sum, p) => sum + positionPnl(p), 0);
  const marginUsed = state.positions.reduce((sum, p) => sum + (p.margin || Math.abs(p.avgEntry * p.size) * 0.1), 0);
  const freeMargin = Math.max(0, state.equity + unrealized - marginUsed);
  const marginLevel = marginUsed > 0 ? ((state.equity + unrealized) / marginUsed) * 100 : 9999;
  const pct = state.equity ? (unrealized / state.equity) * 100 : 0;

  const totalPnl = document.getElementById('totalPnl');
  if (totalPnl) {
    totalPnl.textContent = `${unrealized >= 0 ? '+' : '-'}${Math.abs(unrealized).toFixed(2)} USDT (${pct >= 0 ? '+' : ''}${pct.toFixed(2)}%)`;
    totalPnl.className = `value ${unrealized >= 0 ? 'pos' : 'neg'}`;
  }
  const uPnl = document.getElementById('unrealizedPnl');
  if (uPnl) {
    uPnl.textContent = `${unrealized >= 0 ? '+' : '-'}$${Math.abs(unrealized).toFixed(2)} (${pct.toFixed(2)}%)`;
    uPnl.style.color = unrealized >= 0 ? 'var(--green)' : 'var(--red)';
  }
  const marginEl = document.getElementById('marginUsed'); if (marginEl) marginEl.textContent = `${formatMoney(marginUsed)} USDT`;
  const freeEl = document.getElementById('freeMargin'); if (freeEl) freeEl.textContent = `${formatMoney(freeMargin)} USDT`;
  const levelEl = document.getElementById('marginLevel'); if (levelEl) levelEl.textContent = `${marginLevel.toFixed(2)}%`;
  const posBadge = document.getElementById('posBadge'); if (posBadge) posBadge.textContent = `(${state.positions.length})`;
  const openBadge = document.querySelector('.bottom-tab[data-tab="openorders"] .badge');
  if (openBadge) openBadge.textContent = `(${state.orders.filter(o => o.status === 'Open' || o.status === 'Partial').length})`;
}

function ensurePositionsTable() {
  const bottom = document.getElementById('bottomContent');
  if (!bottom || document.getElementById('positionsBody')) return;
  bottom.innerHTML = `<table class="positions-table" id="positionsTable">
    <thead><tr><th>Symbol</th><th>Side</th><th>Size</th><th>Avg Entry</th><th>Mark Price</th><th>Unrealized PnL</th><th>Unrealized PnL %</th><th>Margin</th><th>Risk</th><th>Actions</th></tr></thead>
    <tbody id="positionsBody"></tbody>
  </table>`;
}

function renderPositions() {
  updateAccountSummary();
  if (activeBottomTab() !== 'positions') return;
  ensurePositionsTable();
  const tbody = document.getElementById('positionsBody');
  if (!tbody) return;
  if (!state.positions.length) {
    tbody.innerHTML = `<tr><td colspan="10"><div class="bottom-empty-state">No open positions yet. Submit a paper order to start tracking PnL.</div></td></tr>`;
    return;
  }
  tbody.innerHTML = state.positions.map((p, idx) => {
    const pnl = positionPnl(p);
    const notional = Math.max(1, Math.abs(p.avgEntry * p.size));
    const pnlPct = (pnl / notional) * 100;
    const base = p.symbol.split('/')[0];
    const cls = pnl > 0 ? 'pnl-pos' : pnl < 0 ? 'pnl-neg' : 'pnl-zero';
    return `<tr data-idx="${idx}">
      <td>${p.symbol}</td>
      <td class="side-${p.side.toLowerCase()}">${p.side}</td>
      <td>${p.size.toFixed(4)} ${base}</td>
      <td>${formatPrice(p.avgEntry)}</td>
      <td class="mark-price" data-idx="${idx}">${formatPrice(p.markPrice)}</td>
      <td class="${cls}" data-pnl="${idx}">${pnl >= 0 ? '+' : '-'}${Math.abs(pnl).toFixed(2)} USDT</td>
      <td class="${cls}" data-pnlpct="${idx}">${pnlPct >= 0 ? '+' : ''}${pnlPct.toFixed(2)}%</td>
      <td>${formatMoney(p.margin || 0)} USDT</td>
      <td>${(p.risk || 0).toFixed(2)}%</td>
      <td><button class="action-btn" onclick="openTPSL(${idx})">TP/SL</button><button class="action-btn danger" onclick="openCloseModal(${idx})">Close</button></td>
    </tr>`;
  }).join('');
}

function applyPositionFill(side, symbol, amount, price, options = {}) {
  const posSide = orderSideToPositionSide(side);
  const opposite = oppositeSide(posSide);
  let remaining = amount;
  let realized = 0;

  for (const p of state.positions.filter(p => p.symbol === symbol && p.side === opposite)) {
    if (remaining <= 0) break;
    const closeSize = Math.min(p.size, remaining);
    realized += (price - p.avgEntry) * closeSize * sideDirection(p.side);
    p.size -= closeSize;
    remaining -= closeSize;
  }
  state.positions = state.positions.filter(p => p.size > 0.000001);

  if (remaining > 0 && !options.reduceOnly) {
    let p = state.positions.find(p => p.symbol === symbol && p.side === posSide);
    if (p) {
      const nextSize = p.size + remaining;
      p.avgEntry = ((p.avgEntry * p.size) + (price * remaining)) / nextSize;
      p.size = nextSize;
      p.markPrice = price;
      p.margin = Math.abs(p.avgEntry * p.size) * 0.1;
      p.risk = Math.min(99, Math.max(5, (p.margin / state.equity) * 100));
    } else {
      p = { symbol, side: posSide, size: remaining, avgEntry: price, markPrice: price, margin: Math.abs(price * remaining) * 0.1, risk: Math.min(99, Math.max(5, (Math.abs(price * remaining) * 0.1 / state.equity) * 100)) };
      state.positions.push(p);
    }
  }
  state.realizedPnl += realized;
  return realized;
}

function recordFill(order, fillPrice, reason = 'Filled') {
  order.status = 'Filled';
  order.filled = order.amount;
  order.fillPrice = fillPrice;
  order.filledAt = Date.now();
  const realized = applyPositionFill(order.side, order.symbol, order.amount, fillPrice, { reduceOnly: order.reduceOnly });
  state.tradeHistory.unshift({ ...order, realizedPnl: realized, ts: Date.now(), reason });
  state.orderHistory.unshift({ ...order });
  persistTradingState();
  showToast(order.side === 'Buy' ? 'success' : 'error', 'Paper Fill', `${order.side} ${formatAmount(order.amount)} ${order.symbol.split('/')[0]} at ${formatPrice(fillPrice)}`);
  renderPositions();
  if (activeBottomTab() !== 'positions') renderBottomPanel(activeBottomTab());
}

function shouldFillLimit(order, price) {
  return order.side === 'Buy' ? order.price >= price : order.price <= price;
}

function processOpenOrders() {
  const open = state.orders.filter(o => o.status === 'Open');
  open.forEach(order => {
    const market = state.watchlist.find(w => w.sym === order.symbol);
    const price = market?.last || state.currentPrice;
    if (order.type === 'Market' || shouldFillLimit(order, price)) {
      recordFill(order, price, 'Book cross');
    }
  });
  state.orders = state.orders.filter(o => o.status === 'Open');
  persistTradingState();
}

function submitPaperOrder(side, override = null) {
  const result = override || validateOrderEntry();
  if (!result.valid && !override) {
    showToast('error', 'Order Blocked', document.getElementById('oeValidation').textContent || 'Check order fields');
    return null;
  }
  const type = override?.type || (result.type === 'market' ? 'Market' : result.type === 'stop' ? 'Stop Limit' : 'Limit');
  const symbol = override?.symbol || state.selectedSymbol;
  const price = type === 'Market' ? (state.watchlist.find(w => w.sym === symbol)?.last || state.currentPrice) : result.price;
  const amount = result.amount;
  const reduceOnly = Boolean(document.querySelector('#reduceOnlyCheck .oe-check-box.checked'));
  const postOnly = Boolean(document.querySelector('#postOnlyCheck .oe-check-box.checked'));
  const order = { id: nextOrderId(), ts: Date.now(), symbol, side, type, price, amount, filled: 0, status: 'Open', reduceOnly, postOnly };

  if (type === 'Market' || (!postOnly && shouldFillLimit(order, state.currentPrice))) {
    recordFill(order, state.currentPrice, type === 'Market' ? 'Market' : 'Immediate cross');
  } else {
    state.orders.unshift(order);
    state.orderHistory.unshift({ ...order });
    persistTradingState();
    showToast('info', `${type} Order Open`, `${side} ${formatAmount(amount)} ${symbol.split('/')[0]} at ${formatPrice(price)}`);
  }
  renderBottomPanel(activeBottomTab());
  renderPositions();
  return order;
}

function cancelPaperOrder(orderId) {
  const order = state.orders.find(o => o.id === orderId);
  if (!order) return false;
  order.status = 'Cancelled';
  state.orderHistory.unshift({ ...order, cancelledAt: Date.now() });
  state.orders = state.orders.filter(o => o.id !== orderId);
  persistTradingState();
  showToast('info', 'Order Cancelled', `${order.id} removed from book`);
  renderBottomPanel(activeBottomTab());
  updateAccountSummary();
  return true;
}

const baseUpdateLiveDataV3 = updateLiveData;
updateLiveData = function updateLiveDataV3() {
  baseUpdateLiveDataV3();
  processOpenOrders();
  const currentTab = activeBottomTab();
  if (currentTab !== 'positions') renderBottomPanel(currentTab);
};

// Upgrade modal close to realize PnL instead of simply removing rows.
openCloseModal = function openCloseModalV3(idx) {
  closePositionIdx = idx;
  const p = state.positions[idx];
  if (!p) return;
  const pnl = positionPnl(p);
  document.getElementById('modalSymbol').textContent = p.symbol;
  document.getElementById('modalSide').textContent = p.side;
  document.getElementById('modalSide').style.color = p.side === 'Long' ? 'var(--green)' : 'var(--red)';
  document.getElementById('modalSize').textContent = `${p.size.toFixed(4)} ${p.symbol.split('/')[0]}`;
  document.getElementById('modalPnl').textContent = `${pnl >= 0 ? '+' : '-'}${Math.abs(pnl).toFixed(2)} USDT`;
  document.getElementById('modalPnl').style.color = pnl >= 0 ? 'var(--green)' : 'var(--red)';
  document.getElementById('closeModal').classList.add('active');
};

document.addEventListener('DOMContentLoaded', () => {
  const confirm = document.getElementById('modalConfirm');
  if (confirm) {
    confirm.addEventListener('click', (event) => {
      event.preventDefault();
      event.stopImmediatePropagation();
      if (closePositionIdx >= 0) {
        const p = state.positions[closePositionIdx];
        if (p) {
          const closingSide = p.side === 'Long' ? 'Sell' : 'Buy';
          const size = p.size;
          const price = p.markPrice;
          const realized = applyPositionFill(closingSide, p.symbol, size, price, { reduceOnly: true });
          state.tradeHistory.unshift({ id: nextOrderId(), ts: Date.now(), symbol: p.symbol, side: closingSide, type: 'Market', price, fillPrice: price, amount: size, status: 'Filled', realizedPnl: realized, reason: 'Manual close' });
          persistTradingState();
          showToast('success', 'Position Closed', `Closed ${formatAmount(size)} ${p.symbol.split('/')[0]} at ${formatPrice(price)}`);
          renderPositions();
          renderBottomPanel(activeBottomTab());
        }
        closePositionIdx = -1;
      }
      document.getElementById('closeModal').classList.remove('active');
    }, true);
  }
  updateAccountSummary();
});
