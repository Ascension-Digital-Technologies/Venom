// @venom: browser
// src/js/app-state-extensions.js
// ===================== V3 CENTRAL STATE EXTENSIONS =====================
const savedTradingState = safeStorage.read();

state.orders = Array.isArray(savedTradingState.orders) ? savedTradingState.orders : [
  { id: 'NOVA-1001', ts: Date.now() - 360000, symbol: 'BTC/USDT', side: 'Buy', type: 'Limit', price: 65880.0, amount: 0.075, filled: 0, status: 'Open', reduceOnly: false, postOnly: true },
  { id: 'NOVA-1002', ts: Date.now() - 180000, symbol: 'ETH/USDT', side: 'Sell', type: 'Limit', price: 3560.0, amount: 0.500, filled: 0, status: 'Open', reduceOnly: true, postOnly: false }
];
state.orderHistory = Array.isArray(savedTradingState.orderHistory) ? savedTradingState.orderHistory : [];
state.tradeHistory = Array.isArray(savedTradingState.tradeHistory) ? savedTradingState.tradeHistory : [];
state.realizedPnl = Number.isFinite(savedTradingState.realizedPnl) ? savedTradingState.realizedPnl : 0;
state.orderSeq = Number.isFinite(savedTradingState.orderSeq) ? savedTradingState.orderSeq : 1003;
state.layoutPreset = savedTradingState.layoutPreset || 'default';
state.balances = Array.isArray(savedTradingState.balances) ? savedTradingState.balances : [
  { asset: 'USDT', available: 100000.00, reserved: 0.00, total: 100000.00 },
  { asset: 'BTC', available: 0.250, reserved: 0.000, total: 0.250 },
  { asset: 'ETH', available: 4.000, reserved: 0.000, total: 4.000 },
  { asset: 'SOL', available: 0.000, reserved: 0.000, total: 0.000 }
];
if (Array.isArray(savedTradingState.positions) && savedTradingState.positions.length) {
  state.positions = savedTradingState.positions;
}

function persistTradingState() {
  safeStorage.write({
    orders: state.orders.slice(-100),
    orderHistory: state.orderHistory.slice(-250),
    tradeHistory: state.tradeHistory.slice(-250),
    positions: state.positions,
    balances: state.balances,
    realizedPnl: state.realizedPnl,
    orderSeq: state.orderSeq,
    layoutPreset: state.layoutPreset
  });
}

function nextOrderId() {
  const id = `NOVA-${state.orderSeq++}`;
  persistTradingState();
  return id;
}

function activeBottomTab() {
  return document.querySelector('.bottom-tab.active')?.dataset.tab || 'positions';
}

function sideDirection(side) { return side === 'Long' ? 1 : -1; }
function orderSideToPositionSide(side) { return side === 'Buy' ? 'Long' : 'Short'; }
function oppositeSide(side) { return side === 'Long' ? 'Short' : 'Long'; }
function formatMoney(value) { return `${value < 0 ? '-' : ''}${Math.abs(value).toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`; }
function formatTimestamp(ts) { return new Date(ts).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }); }
