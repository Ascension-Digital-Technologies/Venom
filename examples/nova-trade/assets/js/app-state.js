// @venom: browser
// src/js/app-state.js
// ===================== DATA STATE =====================
const state = {
  currentPrice: 66247.5,
  prevPrice: 66247.5,
  equity: 100000.00,
  positions: [
    { symbol: 'BTC/USDT', side: 'Long', size: 0.250, avgEntry: 65000.00, markPrice: 66247.50, margin: 1625.00, risk: 23.41 },
    { symbol: 'ETH/USDT', side: 'Long', size: 4.000, avgEntry: 3400.00, markPrice: 3512.18, margin: 2720.00, risk: 18.75 }
  ],
  watchlist: [
    { sym: 'BTC/USDT', last: 66247.5, chg: 2.21 },
    { sym: 'ETH/USDT', last: 3512.18, chg: 1.35 },
    { sym: 'SOL/USDT', last: 152.35, chg: 3.44 },
    { sym: 'BNB/USDT', last: 592.61, chg: 0.91 },
    { sym: 'XRP/USDT', last: 0.5364, chg: -0.23 },
    { sym: 'ADA/USDT', last: 0.4641, chg: -0.72 },
    { sym: 'DOGE/USDT', last: 0.15172, chg: 1.45 },
    { sym: 'AVAX/USDT', last: 36.25, chg: 2.01 },
    { sym: 'MATIC/USDT', last: 0.6843, chg: -0.18 },
    { sym: 'LINK/USDT', last: 16.28, chg: 0.63 }
  ],
  selectedSymbol: 'BTC/USDT',
  tick: 0,
  marketMode: 'normal',
  volatility: 1,
  baseDrift: 0.000015
};

const STORAGE_KEY = 'novaTrade.layout.v2';
const safeStorage = {
  read() {
    try { return JSON.parse(localStorage.getItem(STORAGE_KEY) || '{}'); }
    catch { return {}; }
  },
  write(patch) {
    try {
      const current = safeStorage.read();
      localStorage.setItem(STORAGE_KEY, JSON.stringify({ ...current, ...patch }));
    } catch {}
  }
};

function parseNumber(value) {
  return Number(String(value || '').replace(/,/g, '').trim());
}

function decimalsForPrice(value) {
  if (value < 1) return 5;
  if (value < 10) return 3;
  if (value < 1000) return 2;
  return 1;
}

function formatPrice(value) {
  const digits = decimalsForPrice(Math.abs(value));
  return Number(value).toLocaleString('en-US', { minimumFractionDigits: digits, maximumFractionDigits: digits });
}

function formatAmount(value) {
  return Number(value).toLocaleString('en-US', { minimumFractionDigits: 3, maximumFractionDigits: 4 });
}

function randomNormal() {
  let u = 0, v = 0;
  while (u === 0) u = Math.random();
  while (v === 0) v = Math.random();
  return Math.sqrt(-2.0 * Math.log(u)) * Math.cos(2.0 * Math.PI * v);
}

// Generate candlestick data around a seed price so every selected market has believable scale.
function generateCandles(seedPrice = 61000) {
  const generated = [];
  let price = seedPrice * (0.94 + Math.random() * 0.04);
  const volatility = Math.max(seedPrice * 0.0025, 0.004);
  for (let i = 0; i < 120; i++) {
    const drift = (seedPrice - price) * 0.012;
    const change = drift + (Math.random() - 0.46) * volatility;
    const open = price;
    const close = Math.max(seedPrice * 0.05, price + change);
    const wick = Math.max(Math.abs(change) * 0.55, volatility * 0.22);
    const high = Math.max(open, close) + Math.random() * wick;
    const low = Math.max(0.00001, Math.min(open, close) - Math.random() * wick);
    const volume = 500 + Math.random() * 1500 + Math.abs(change / Math.max(seedPrice, 1)) * 180000;
    generated.push({ open, high, low, close, volume, time: Date.now() - (120 - i) * 60_000 });
    price = close;
  }
  return generated;
}

const candles = generateCandles();
let ema20 = [], ema50 = [], ema200 = [];

function calcEMA(data, period) {
  const ema = [];
  const multiplier = 2 / (period + 1);
  ema[0] = data[0].close;
  for (let i = 1; i < data.length; i++) {
    ema[i] = (data[i].close - ema[i-1]) * multiplier + ema[i-1];
  }
  return ema;
}

ema20 = calcEMA(candles, 20);
ema50 = calcEMA(candles, 50);
ema200 = calcEMA(candles, 200);
