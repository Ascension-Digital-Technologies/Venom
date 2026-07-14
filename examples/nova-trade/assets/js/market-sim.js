// @venom: browser
// src/js/market-sim.js
// ===================== ORDER BOOK =====================
function generateOrderBook(midPrice, side) {
  const rows = [];
  let price = midPrice;
  let total = 0;
  for (let i = 0; i < 7; i++) {
    const offset = (i + 1) * (side === 'ask' ? 0.5 + Math.random() * 2 : 0.5 + Math.random() * 2);
    price = side === 'ask' ? midPrice + offset : midPrice - offset;
    const amount = 0.05 + Math.random() * 0.5;
    total += amount;
    rows.push({ price: Math.round(price * 10) / 10, amount: Math.round(amount * 1000) / 1000, total: Math.round(total * 1000) / 1000 });
  }
  return side === 'ask' ? rows.reverse() : rows;
}

function renderOrderBook() {
  const askContainer = document.getElementById('askRows');
  const bidContainer = document.getElementById('bidRows');
  const asks = generateOrderBook(state.currentPrice, 'ask');
  const bids = generateOrderBook(state.currentPrice, 'bid');
  const maxTotal = Math.max(...asks.map(d => d.total), ...bids.map(d => d.total));
  const digits = decimalsForPrice(state.currentPrice);

  askContainer.innerHTML = asks.map(d => {
    const barWidth = (d.total / maxTotal) * 100;
    return `<div class="ob-row ask">
      <span class="price">${d.price.toFixed(digits)}</span>
      <span class="amount">${d.amount.toFixed(3)}</span>
      <span class="total">${d.total.toFixed(3)}</span>
      <div class="bar" style="width:${barWidth}%"></div>
    </div>`;
  }).join('');

  bidContainer.innerHTML = bids.map(d => {
    const barWidth = (d.total / maxTotal) * 100;
    return `<div class="ob-row bid">
      <span class="price">${d.price.toFixed(digits)}</span>
      <span class="amount">${d.amount.toFixed(3)}</span>
      <span class="total">${d.total.toFixed(3)}</span>
      <div class="bar" style="width:${barWidth}%"></div>
    </div>`;
  }).join('');

  const direction = state.currentPrice >= state.prevPrice ? '&#9650;' : '&#9660;';
  const color = state.currentPrice >= state.prevPrice ? 'var(--green)' : 'var(--red)';
  const spread = Math.max(state.currentPrice * 0.000012, 0.0001);
  document.getElementById('obSpread').style.color = color;
  document.getElementById('obSpread').innerHTML = `${formatPrice(state.currentPrice)} <span class="arrow">${direction}</span><div style="font-size:10px;color:var(--text-muted);font-weight:500;margin-top:2px;">Spread ${formatPrice(spread)}</div>`;
}


// ===================== TRADE TAPE =====================
let tapeData = [];
for (let i = 0; i < 20; i++) {
  const price = state.currentPrice + (Math.random() - 0.5) * 5;
  tapeData.push({
    price: Math.round(price * 10) / 10,
    amount: Math.round((0.001 + Math.random() * 0.15) * 1000) / 1000,
    time: new Date(Date.now() - i * 2000).toTimeString().slice(0, 8),
    up: Math.random() > 0.45
  });
}

function renderTape() {
  const digits = decimalsForPrice(state.currentPrice);
  document.getElementById('tapeRows').innerHTML = tapeData.map((d, i) => `
    <div class="tape-row ${i === 0 ? 'new' : ''}">
      <span class="price ${d.up ? 'up' : 'down'}">${Number(d.price).toFixed(digits)}</span>
      <span class="amount">${d.amount.toFixed(3)}</span>
      <span class="time">${d.time}</span>
    </div>
  `).join('');
}

function addTapeEntry() {
  const price = state.currentPrice + randomNormal() * Math.max(state.currentPrice * 0.000018, 0.0001);
  tapeData.unshift({
    price: Math.max(price, 0.00001),
    amount: Math.round((0.001 + Math.random() * 0.18) * 1000) / 1000,
    time: new Date().toTimeString().slice(0, 8),
    up: price >= state.prevPrice
  });
  if (tapeData.length > 28) tapeData.pop();
  renderTape();
}


// ===================== WATCHLIST =====================
function renderWatchlist() {
  const rows = document.getElementById('watchlistRows');
  rows.innerHTML = state.watchlist.map(w => {
    const isUp = w.chg >= 0;
    const isSelected = w.sym === state.selectedSymbol;
    return `<div class="watchlist-row ${isSelected ? 'selected' : ''}" data-sym="${w.sym}">
      <span class="sym">${w.sym}</span>
      <span class="last" data-sym="${w.sym}">${w.last.toLocaleString('en-US', {minimumFractionDigits: w.last < 1 ? 4 : 1, maximumFractionDigits: w.last < 1 ? 5 : 2})}</span>
      <span class="chg ${isUp ? 'up' : 'down'}">${isUp ? '+' : ''}${w.chg.toFixed(2)}%</span>
    </div>`;
  }).join('');

  // Click handlers
  rows.querySelectorAll('.watchlist-row').forEach(row => {
    row.addEventListener('click', () => {
      applySelectedSymbol(row.dataset.sym);
      showToast('info', 'Symbol Changed', `Switched to ${state.selectedSymbol}`);
    });
  });
}

const scannerData = [
  { sym: 'ARKM/USDT', last: 2.121, chg: 18.73 },
  { sym: 'STX/USDT', last: 2.156, chg: 15.21 },
  { sym: 'TIA/USDT', last: 11.23, chg: 13.67 },
  { sym: 'CFX/USDT', last: 0.2468, chg: 12.31 },
  { sym: 'FET/USDT', last: 2.195, chg: 11.14 },
];

function renderScanner() {
  document.getElementById('scannerRows').innerHTML = `
    <div class="scanner-row" style="color:var(--text-muted);font-size:10px;"><span>Symbol</span><span>Last</span><span>Chg%</span></div>
    ${scannerData.map(d => `<div class="scanner-row"><span class="sym">${d.sym}</span><span class="last">${d.last}</span><span class="chg">+${d.chg.toFixed(2)}%</span></div>`).join('')}
  `;
}


// ===================== LIVE DATA SIMULATION =====================
function updateLiveData() {
  state.tick += 1;
  if (state.tick % 24 === 0) {
    state.marketMode = Math.random() > 0.72 ? 'volatile' : Math.random() > 0.55 ? 'trend' : 'normal';
    state.volatility = state.marketMode === 'volatile' ? 2.2 : state.marketMode === 'trend' ? 1.35 : 1;
    state.baseDrift = state.marketMode === 'trend' ? (Math.random() > 0.5 ? 0.000035 : -0.000025) : 0.000006;
  }

  const selectedBefore = getSelectedMarket();
  state.prevPrice = selectedBefore?.last || state.currentPrice;
  const marketPulse = randomNormal() * 0.00013 * state.volatility + state.baseDrift;
  const spike = Math.random() > 0.985 ? randomNormal() * 0.0012 : 0;

  state.watchlist.forEach((w) => {
    const beta = w.sym.startsWith('BTC') ? 1 : w.sym.startsWith('ETH') ? 1.12 : w.sym.startsWith('SOL') || w.sym.startsWith('AVAX') ? 1.45 : 1.25;
    const localNoise = randomNormal() * 0.00007 * state.volatility;
    const pctMove = (marketPulse + spike) * beta + localNoise;
    const before = w.last;
    w.last = Math.max(0.00001, w.last * (1 + pctMove));
    w.chg = Math.max(-24, Math.min(24, w.chg + pctMove * 35 + randomNormal() * 0.006));
    w.direction = w.last >= before ? 1 : -1;
  });

  const selected = getSelectedMarket();
  state.currentPrice = selected.last;
  const direction = state.currentPrice >= state.prevPrice ? 1 : -1;
  updateTickerHeader(direction);

  state.positions.forEach(p => {
    const market = state.watchlist.find(w => w.sym === p.symbol);
    if (market) p.markPrice = market.last;
    else p.markPrice = Math.max(0.00001, p.markPrice * (1 + marketPulse + randomNormal() * 0.00008));
  });

  const selectedRowPrice = document.querySelector(`[data-sym="${state.selectedSymbol}"].last`);
  if (selectedRowPrice) {
    selectedRowPrice.classList.remove('flash-up', 'flash-down');
    void selectedRowPrice.offsetWidth;
    selectedRowPrice.classList.add(direction > 0 ? 'flash-up' : 'flash-down');
  }
  renderWatchlist();

  pushLiveCandle(state.currentPrice);
  document.getElementById('ohlcC').textContent = formatPrice(state.currentPrice);
  const first = candles[Math.max(0, candles.length - 24)] || candles[0];
  const change24h = state.currentPrice - first.open;
  const change24hPct = (change24h / first.open) * 100;
  const ohlcChange = document.getElementById('ohlcChange');
  ohlcChange.textContent = `${change24h >= 0 ? '+' : ''}${formatPrice(change24h)} (${change24hPct >= 0 ? '+' : ''}${change24hPct.toFixed(2)}%)`;
  ohlcChange.style.color = change24h >= 0 ? 'var(--green)' : 'var(--red)';
  document.getElementById('ohlcO').textContent = formatPrice(candles[candles.length - 1].open);
  document.getElementById('ohlcH').textContent = formatPrice(candles[candles.length - 1].high);
  document.getElementById('ohlcL').textContent = formatPrice(candles[candles.length - 1].low);
  document.getElementById('chartTime').textContent = new Date().toTimeString().slice(0, 8) + ' (UTC-4)';

  renderOrderBook();
  renderPositions();
  validateOrderEntry();
  drawChart();
  if (Math.random() > 0.18) addTapeEntry();
}
