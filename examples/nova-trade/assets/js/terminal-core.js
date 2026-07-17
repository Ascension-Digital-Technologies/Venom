// @venom: browser
// src/js/terminal-core.js
// ===================== TERMINAL CORE HELPERS V4 =====================
// Shared helpers used by panels, order entry, charting, and simulator.
function getSelectedMarket() {
  return state.watchlist.find(w => w.sym === state.selectedSymbol) || state.watchlist[0];
}

function getBaseAsset(symbol = state.selectedSymbol) {
  return String(symbol || state.selectedSymbol || 'BTC/USDT').split('/')[0] || 'BTC';
}

function getQuoteAsset(symbol = state.selectedSymbol) {
  return String(symbol || state.selectedSymbol || 'BTC/USDT').split('/')[1] || 'USDT';
}

function getActiveOrderType() {
  return document.querySelector('.oe-tab.active')?.dataset.type || 'limit';
}

function applySelectedSymbol(symbol) {
  if (!state.watchlist.some(w => w.sym === symbol)) return;
  state.selectedSymbol = symbol;
  const selected = getSelectedMarket();
  state.prevPrice = selected.last;
  state.currentPrice = selected.last;
  safeStorage.write({ selectedSymbol: symbol });
  resetChartForSymbol(symbol);
  renderWatchlist?.();
  renderOrderBook?.();
  renderTape?.();
  syncOrderEntry?.();
}

function resetChartForSymbol(symbol = state.selectedSymbol) {
  const selected = state.watchlist.find(w => w.sym === symbol) || getSelectedMarket();
  if (!selected) return;
  state.selectedSymbol = selected.sym;
  state.prevPrice = selected.last;
  state.currentPrice = selected.last;
  const nextCandles = generateCandles(selected.last);
  candles.splice(0, candles.length, ...nextCandles);
  ema20 = calcEMA(candles, 20);
  ema50 = calcEMA(candles, 50);
  ema200 = calcEMA(candles, 200);
  if (typeof chartView !== 'undefined') { chartView.pan = 0; chartView.zoom = 1; }
  const pair = document.querySelector('.ticker-pair'); if (pair) pair.textContent = selected.sym;
  const overlayPair = document.querySelector('#chartOverlay .highlight'); if (overlayPair) overlayPair.textContent = selected.sym;
  updateTickerHeader(selected.last >= state.prevPrice ? 1 : -1);
  updateOhlcOverlay();
  if (typeof resizeCanvas === 'function') resizeCanvas();
}

function pushLiveCandle(price) {
  const last = candles[candles.length - 1];
  const now = Date.now();
  if (!last || state.tick % 10 === 0) {
    const previousClose = last?.close || price;
    candles.push({ open: previousClose, high: Math.max(previousClose, price), low: Math.min(previousClose, price), close: price, volume: 350 + Math.random() * 1300, time: now });
    if (candles.length > 220) candles.shift();
  } else {
    last.close = price;
    last.high = Math.max(last.high, price);
    last.low = Math.min(last.low, price);
    last.volume += 25 + Math.random() * 160;
    last.time = now;
  }
  ema20 = calcEMA(candles, 20);
  ema50 = calcEMA(candles, 50);
  ema200 = calcEMA(candles, 200);
}

function updateOhlcOverlay() {
  const c = candles[candles.length - 1];
  if (!c) return;
  const set = (id, value) => { const el = document.getElementById(id); if (el) el.textContent = formatPrice(value); };
  set('ohlcO', c.open);
  set('ohlcH', c.high);
  set('ohlcL', c.low);
  set('ohlcC', c.close);
  const first = candles[Math.max(0, candles.length - 24)] || candles[0] || c;
  const change = c.close - first.open;
  const pct = first.open ? (change / first.open) * 100 : 0;
  const ohlcChange = document.getElementById('ohlcChange');
  if (ohlcChange) {
    ohlcChange.textContent = `${change >= 0 ? '+' : ''}${formatPrice(change)} (${pct >= 0 ? '+' : ''}${pct.toFixed(2)}%)`;
    ohlcChange.style.color = change >= 0 ? 'var(--green)' : 'var(--red)';
  }
}

function updateTickerHeader(direction = 1) {
  const selected = getSelectedMarket();
  if (!selected) return;
  const priceEl = document.getElementById('tickerPrice');
  if (priceEl) {
    priceEl.textContent = formatPrice(selected.last);
    priceEl.classList.remove('flash-up', 'flash-down');
    void priceEl.offsetWidth;
    priceEl.classList.add(direction >= 0 ? 'flash-up' : 'flash-down');
  }
  const change = document.getElementById('tickerChange');
  if (change) {
    const abs = selected.last * selected.chg / 100;
    change.textContent = `${selected.chg >= 0 ? '+' : ''}${formatPrice(abs)} (${selected.chg >= 0 ? '+' : ''}${selected.chg.toFixed(2)}%)`;
    change.style.color = selected.chg >= 0 ? 'var(--green)' : 'var(--red)';
  }
  const high = document.getElementById('tickerHigh'); if (high) high.textContent = formatPrice(selected.last * 1.006);
  const low = document.getElementById('tickerLow'); if (low) low.textContent = formatPrice(selected.last * 0.992);
  const vol = document.getElementById('tickerVol'); if (vol) vol.textContent = `${(12000 + Math.random() * 9000).toLocaleString('en-US', { maximumFractionDigits: 2 })} ${getBaseAsset(selected.sym)}`;
  const equity = document.getElementById('equityValue'); if (equity) equity.textContent = `$${formatMoney((typeof accountEquityMark === 'function' ? accountEquityMark() : state.equity) || state.equity)}`;
}

function validateOrderEntry() {
  const type = getActiveOrderType();
  const priceInput = document.getElementById('oePrice');
  const amountInput = document.getElementById('oeAmount');
  const totalInput = document.getElementById('oeTotal');
  const validation = document.getElementById('oeValidation');
  const price = type === 'market' ? state.currentPrice : parseNumber(priceInput?.value);
  const amount = parseNumber(amountInput?.value);
  const total = price * amount;
  const validBase = Number.isFinite(price) && price > 0 && Number.isFinite(amount) && amount > 0;
  if (totalInput && document.activeElement !== totalInput && Number.isFinite(total)) {
    totalInput.value = total.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
  }
  if (priceInput) {
    priceInput.disabled = type === 'market';
    if (type === 'market') priceInput.value = 'Market';
    else if (priceInput.value === 'Market') priceInput.value = formatPrice(state.currentPrice);
  }
  if (validation) {
    validation.classList.remove('good', 'warn', 'bad');
    validation.classList.add(validBase ? 'good' : 'bad');
    validation.textContent = validBase ? 'Order ready for risk checks.' : 'Enter a valid price and amount.';
  }
  const buy = document.getElementById('btnBuy'); if (buy) buy.disabled = !validBase;
  const sell = document.getElementById('btnSell'); if (sell) sell.disabled = !validBase;
  return { valid: validBase, type, price, amount, total };
}

function syncOrderEntry() {
  const type = getActiveOrderType();
  const priceInput = document.getElementById('oePrice');
  const amountInput = document.getElementById('oeAmount');
  const totalInput = document.getElementById('oeTotal');
  const base = getBaseAsset();
  const amountSuffix = amountInput?.parentElement?.querySelector('.suffix'); if (amountSuffix) amountSuffix.textContent = base;
  if (priceInput && type !== 'market') priceInput.value = formatPrice(state.currentPrice);
  const price = type === 'market' ? state.currentPrice : parseNumber(priceInput?.value);
  const amount = parseNumber(amountInput?.value) || 0;
  if (totalInput && Number.isFinite(price)) totalInput.value = (price * amount).toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
  const availableCollateral = typeof freeCollateral === 'function' ? freeCollateral() : state.equity;
  const maxAmount = Math.max(0, (availableCollateral || state.equity) / Math.max(state.currentPrice, 1) / Math.max(state.riskConfig?.initialMarginRate || 0.1, 0.01));
  const maxBuy = document.getElementById('maxBuy'); if (maxBuy) maxBuy.textContent = `${formatAmount(maxAmount)} ${base}`;
  const maxSell = document.getElementById('maxSell'); if (maxSell) maxSell.textContent = `${formatAmount(maxAmount)} ${base}`;
  const spread = document.getElementById('oeSpread'); if (spread) spread.textContent = formatPrice(Math.max(state.currentPrice * 0.000012, 0.0001));
  validateOrderEntry();
}
