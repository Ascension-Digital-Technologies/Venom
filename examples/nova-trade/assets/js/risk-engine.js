// @venom: browser
// src/js/risk-engine.js
// ===================== RISK ENGINE V4 =====================
state.riskConfig = {
  makerFee: 0.0002,
  takerFee: 0.0006,
  initialMarginRate: 0.10,
  maintenanceMarginRate: 0.035,
  maxOrderNotionalPct: 0.45,
  maxSymbolExposurePct: 0.65,
  maxPortfolioExposurePct: 2.5
};

function symbolPrice(symbol = state.selectedSymbol) {
  return state.watchlist.find(w => w.sym === symbol)?.last || state.currentPrice || 0;
}

function getBaseAsset(symbol = state.selectedSymbol) {
  return String(symbol || state.selectedSymbol).split('/')[0] || 'BTC';
}

function getQuoteAsset(symbol = state.selectedSymbol) {
  return String(symbol || state.selectedSymbol).split('/')[1] || 'USDT';
}

function accountEquityMark() {
  const unrealized = state.positions.reduce((sum, p) => sum + positionPnl(p), 0);
  return state.equity + unrealized;
}

function portfolioNotional() {
  return state.positions.reduce((sum, p) => sum + Math.abs(p.markPrice * p.size), 0);
}

function symbolExposure(symbol) {
  return state.positions
    .filter(p => p.symbol === symbol)
    .reduce((sum, p) => sum + Math.abs(p.markPrice * p.size), 0);
}

function reservedQuote() {
  return state.orders.filter(o => o.status === 'Open').reduce((sum, o) => {
    if (o.side !== 'Buy') return sum;
    return sum + Math.abs((o.price || symbolPrice(o.symbol)) * o.amount) * (1 + state.riskConfig.makerFee);
  }, 0);
}

function freeCollateral() {
  const eq = accountEquityMark();
  const marginUsed = state.positions.reduce((sum, p) => sum + Math.abs(p.markPrice * p.size) * state.riskConfig.initialMarginRate, 0);
  return Math.max(0, eq - marginUsed - reservedQuote());
}

function estimateFees(notional, liquidity = 'taker') {
  return Math.abs(notional) * (liquidity === 'maker' ? state.riskConfig.makerFee : state.riskConfig.takerFee);
}

function evaluateRisk(order) {
  const price = Number(order.price || symbolPrice(order.symbol));
  const amount = Number(order.amount || 0);
  const notional = Math.abs(price * amount);
  const eq = Math.max(accountEquityMark(), 1);
  const maker = order.type === 'Limit' && order.postOnly;
  const fee = estimateFees(notional, maker ? 'maker' : 'taker');
  const requiredMargin = notional * state.riskConfig.initialMarginRate + fee;
  const available = freeCollateral();
  const warnings = [];
  const blockers = [];

  if (!Number.isFinite(price) || price <= 0) blockers.push('Price must be greater than zero');
  if (!Number.isFinite(amount) || amount <= 0) blockers.push('Amount must be greater than zero');
  if (notional > eq * state.riskConfig.maxOrderNotionalPct) warnings.push('Large order relative to account equity');
  if (symbolExposure(order.symbol) + notional > eq * state.riskConfig.maxSymbolExposurePct) warnings.push('High single-symbol exposure');
  if (portfolioNotional() + notional > eq * state.riskConfig.maxPortfolioExposurePct) warnings.push('High total portfolio leverage');
  if (!order.reduceOnly && requiredMargin > available) blockers.push('Insufficient free margin for estimated requirement');

  const health = computeAccountHealth();
  const score = blockers.length ? 'blocked' : warnings.length || health.level !== 'healthy' ? 'warn' : 'ok';
  return { valid: blockers.length === 0, score, warnings, blockers, notional, fee, requiredMargin, available, health };
}

function computeAccountHealth() {
  const eq = accountEquityMark();
  const maintenance = state.positions.reduce((sum, p) => sum + Math.abs(p.markPrice * p.size) * state.riskConfig.maintenanceMarginRate, 0);
  const marginUsed = state.positions.reduce((sum, p) => sum + Math.abs(p.markPrice * p.size) * state.riskConfig.initialMarginRate, 0);
  const ratio = maintenance > 0 ? eq / maintenance : 999;
  const leverage = eq > 0 ? portfolioNotional() / eq : 0;
  const level = ratio < 1.25 ? 'critical' : ratio < 2.0 || leverage > 3.0 ? 'warning' : 'healthy';
  return { equity: eq, maintenance, marginUsed, ratio, leverage, level, freeCollateral: freeCollateral() };
}

function riskLabelFromScore(score) {
  if (score === 'blocked') return 'Blocked';
  if (score === 'warn') return 'Medium';
  return 'Low';
}

window.NovaRisk = { evaluateRisk, computeAccountHealth, estimateFees, freeCollateral, accountEquityMark };
