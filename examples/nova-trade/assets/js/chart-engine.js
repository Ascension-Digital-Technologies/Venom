// @venom: browser
// src/js/chart-engine.js
// ===================== CHART ENGINE =====================
const canvas = document.getElementById('tradingChart');
const ctx = canvas.getContext('2d');
const container = document.getElementById('chartContainer');
let chartW, chartH, dpr = window.devicePixelRatio || 1;
let hoveredCandle = -1;

function resizeCanvas() {
  chartW = container.clientWidth;
  chartH = container.clientHeight;
  if (!chartW || !chartH) return;
  canvas.width = chartW * dpr;
  canvas.height = chartH * dpr;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  drawChart();
}

function drawChart() {
  if (!chartW || !chartH) return;
  const pad = { top: 10, right: 60, bottom: 10, left: 10 };
  const chartAreaH = chartH * 0.68;
  const volH = chartH * 0.18;
  const rsiH = chartH * 0.14;

  ctx.clearRect(0, 0, chartW, chartH);

  const visibleCount = Math.min(candles.length, Math.floor((chartW - pad.left - pad.right) / 8));
  const startIdx = Math.max(0, candles.length - visibleCount);
  const visible = candles.slice(startIdx);
  const vEma20 = ema20.slice(startIdx);
  const vEma50 = ema50.slice(startIdx);
  const vEma200 = ema200.slice(startIdx);

  const prices = visible.map(c => [c.high, c.low]).flat();
  const minPrice = Math.min(...prices) - 200;
  const maxPrice = Math.max(...prices) + 200;
  const priceRange = maxPrice - minPrice;

  const gap = (chartW - pad.left - pad.right) / visible.length;
  const candleW = gap * 0.65;

  // Grid lines
  ctx.strokeStyle = '#21262d';
  ctx.lineWidth = 1;
  for (let i = 0; i <= 5; i++) {
    const y = pad.top + (chartAreaH / 5) * i;
    ctx.beginPath(); ctx.moveTo(pad.left, y); ctx.lineTo(chartW - pad.right, y); ctx.stroke();
    const price = maxPrice - (priceRange / 5) * i;
    ctx.fillStyle = '#6e7681'; ctx.font = '10px sans-serif';
    ctx.textAlign = 'left';
    ctx.fillText(price.toFixed(1), chartW - pad.right + 4, y + 3);
  }

  // Candlesticks
  visible.forEach((c, i) => {
    const x = pad.left + i * gap + gap / 2;
    const yOpen = pad.top + chartAreaH - ((c.open - minPrice) / priceRange) * chartAreaH;
    const yClose = pad.top + chartAreaH - ((c.close - minPrice) / priceRange) * chartAreaH;
    const yHigh = pad.top + chartAreaH - ((c.high - minPrice) / priceRange) * chartAreaH;
    const yLow = pad.top + chartAreaH - ((c.low - minPrice) / priceRange) * chartAreaH;

    const isGreen = c.close >= c.open;
    const color = isGreen ? '#3fb950' : '#f85149';
    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.lineWidth = 1;

    ctx.beginPath(); ctx.moveTo(x, yHigh); ctx.lineTo(x, yLow); ctx.stroke();
    const bodyTop = Math.min(yOpen, yClose);
    const bodyH = Math.max(Math.abs(yOpen - yClose), 1);
    ctx.fillRect(x - candleW / 2, bodyTop, candleW, bodyH);

    // Highlight hovered candle
    if (i === hoveredCandle - startIdx) {
      ctx.strokeStyle = 'rgba(88,166,255,0.5)';
      ctx.lineWidth = 1;
      ctx.strokeRect(x - candleW / 2 - 2, Math.min(yHigh, bodyTop) - 2, candleW + 4, Math.abs(yHigh - yLow) + 4);
    }
  });

  // EMA Lines
  function drawEMA(emaData, color, width) {
    ctx.strokeStyle = color; ctx.lineWidth = width; ctx.beginPath();
    emaData.forEach((v, i) => {
      const x = pad.left + i * gap + gap / 2;
      const y = pad.top + chartAreaH - ((v - minPrice) / priceRange) * chartAreaH;
      if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  }

  drawEMA(vEma20, '#d29922', 1.5);
  drawEMA(vEma50, '#58a6ff', 1.5);
  drawEMA(vEma200, '#a371f7', 1.5);

  // Volume bars
  const maxVol = Math.max(...visible.map(c => c.volume));
  visible.forEach((c, i) => {
    const x = pad.left + i * gap + gap / 2;
    const vH = (c.volume / maxVol) * volH;
    const y = pad.top + chartAreaH + 10 + volH - vH;
    ctx.fillStyle = c.close >= c.open ? 'rgba(63,185,80,0.35)' : 'rgba(248,81,73,0.35)';
    ctx.fillRect(x - candleW / 2, y, candleW, vH);
  });

  // RSI
  function calcRSI(data, period) {
    let gains = 0, losses = 0;
    for (let i = 1; i <= period; i++) {
      const change = data[i].close - data[i-1].close;
      if (change > 0) gains += change; else losses -= change;
    }
    let avgGain = gains / period, avgLoss = losses / period;
    const rsi = [100 - (100 / (1 + avgGain / avgLoss))];
    for (let i = period + 1; i < data.length; i++) {
      const change = data[i].close - data[i-1].close;
      const gain = change > 0 ? change : 0;
      const loss = change < 0 ? -change : 0;
      avgGain = (avgGain * (period - 1) + gain) / period;
      avgLoss = (avgLoss * (period - 1) + loss) / period;
      rsi.push(100 - (100 / (1 + avgGain / avgLoss)));
    }
    return rsi;
  }

  const rsiValues = calcRSI(visible, 14);
  const rsiTop = pad.top + chartAreaH + 10 + volH + 5;
  ctx.strokeStyle = '#21262d'; ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(pad.left, rsiTop); ctx.lineTo(chartW - pad.right, rsiTop); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(pad.left, rsiTop + rsiH); ctx.lineTo(chartW - pad.right, rsiTop + rsiH); ctx.stroke();
  ctx.strokeStyle = '#6e7681'; ctx.setLineDash([2, 2]);
  ctx.beginPath(); ctx.moveTo(pad.left, rsiTop + rsiH * 0.5); ctx.lineTo(chartW - pad.right, rsiTop + rsiH * 0.5); ctx.stroke();
  ctx.setLineDash([]);

  ctx.strokeStyle = '#a371f7'; ctx.lineWidth = 1.5; ctx.beginPath();
  rsiValues.forEach((v, i) => {
    const x = pad.left + (i + 14) * gap + gap / 2;
    const y = rsiTop + rsiH - (v / 100) * rsiH;
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();

  ctx.fillStyle = '#6e7681'; ctx.font = '10px sans-serif';
  ctx.fillText('RSI 14  61.23', pad.left + 4, rsiTop + rsiH - 4);
  ctx.fillText('75.00', chartW - pad.right + 4, rsiTop + 8);
  ctx.fillText('50.00', chartW - pad.right + 4, rsiTop + rsiH * 0.5 + 3);
  ctx.fillText('25.00', chartW - pad.right + 4, rsiTop + rsiH - 2);
}


// ===================== CROSSHAIR =====================
const crosshair = document.getElementById('crosshair');
const crosshairV = document.getElementById('crosshairV');
const crosshairH = document.getElementById('crosshairH');
const crosshairPriceLabel = document.getElementById('crosshairPriceLabel');
const crosshairTimeLabel = document.getElementById('crosshairTimeLabel');
const crosshairTooltip = document.getElementById('crosshairTooltip');

container.addEventListener('mousemove', (e) => {
  if (!chartW || !chartH || candles.length < 2) return;
  const rect = container.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const y = e.clientY - rect.top;

  crosshair.style.display = 'block';
  crosshairV.style.left = x + 'px';
  crosshairH.style.top = y + 'px';

  const pad = { top: 10, right: 60, bottom: 10, left: 10 };
  const chartAreaH = chartH * 0.68;
  const visibleCount = Math.min(candles.length, Math.floor((chartW - pad.left - pad.right) / 8));
  const startIdx = Math.max(0, candles.length - visibleCount);
  const visible = candles.slice(startIdx);
  const prices = visible.map(c => [c.high, c.low]).flat();
  const minPrice = Math.min(...prices) - 200;
  const maxPrice = Math.max(...prices) + 200;
  const priceRange = maxPrice - minPrice;
  const gap = (chartW - pad.left - pad.right) / visible.length;

  const price = maxPrice - ((y - pad.top) / chartAreaH) * priceRange;
  crosshairPriceLabel.textContent = price.toFixed(1);
  crosshairPriceLabel.style.top = (y - 8) + 'px';

  const candleIdx = Math.floor((x - pad.left) / gap);
  if (candleIdx >= 0 && candleIdx < visible.length) {
    const c = visible[candleIdx];
    hoveredCandle = startIdx + candleIdx;
    drawChart();

    crosshairTimeLabel.textContent = `Bar ${candleIdx + 1}`;
    crosshairTimeLabel.style.left = (x - 20) + 'px';

    crosshairTooltip.style.display = 'block';
    crosshairTooltip.style.left = Math.min(x + 20, chartW - 160) + 'px';
    crosshairTooltip.style.top = Math.min(y + 20, chartH - 180) + 'px';

    document.getElementById('tipOpen').textContent = c.open.toFixed(1);
    document.getElementById('tipHigh').textContent = c.high.toFixed(1);
    document.getElementById('tipLow').textContent = c.low.toFixed(1);
    document.getElementById('tipClose').textContent = c.close.toFixed(1);
    document.getElementById('tipVol').textContent = c.volume.toFixed(0);
    const chg = ((c.close - c.open) / c.open * 100).toFixed(2);
    const tipChg = document.getElementById('tipChange');
    tipChg.textContent = (chg >= 0 ? '+' : '') + chg + '%';
    tipChg.className = 'value ' + (chg >= 0 ? 'up' : 'down');
  }
});

container.addEventListener('mouseleave', () => {
  crosshair.style.display = 'none';
  crosshairTooltip.style.display = 'none';
  hoveredCandle = -1;
  drawChart();
});

// Right-click context menu on chart
container.addEventListener('contextmenu', (e) => {
  e.preventDefault();
  const menu = document.getElementById('contextMenu');
  menu.style.display = 'block';
  menu.style.left = Math.min(e.clientX, window.innerWidth - 180) + 'px';
  menu.style.top = Math.min(e.clientY, window.innerHeight - 200) + 'px';
});

document.addEventListener('click', () => {
  document.getElementById('contextMenu').style.display = 'none';
});


// ===================== DEPTH CHART =====================
function drawDepth() {
  const dCanvas = document.getElementById('depthChart');
  const dCtx = dCanvas.getContext('2d');
  const dContainer = dCanvas.parentElement;
  if (!dContainer.clientWidth || !dContainer.clientHeight) return;
  dpr = window.devicePixelRatio || 1;
  dCanvas.width = dContainer.clientWidth * dpr;
  dCanvas.height = dContainer.clientHeight * dpr;
  dCtx.setTransform(dpr, 0, 0, dpr, 0, 0);

  const w = dContainer.clientWidth;
  const h = dContainer.clientHeight;
  dCtx.clearRect(0, 0, w, h);

  dCtx.fillStyle = 'rgba(63,185,80,0.25)';
  dCtx.beginPath(); dCtx.moveTo(0, h);
  const bidPoints = [[0, h*0.25], [w*0.12, h*0.4], [w*0.28, h*0.55], [w*0.42, h*0.78], [w*0.5, h]];
  bidPoints.forEach(p => dCtx.lineTo(p[0], p[1]));
  dCtx.closePath(); dCtx.fill();

  dCtx.strokeStyle = '#3fb950'; dCtx.lineWidth = 2; dCtx.beginPath();
  bidPoints.forEach((p, i) => { if (i === 0) dCtx.moveTo(p[0], p[1]); else dCtx.lineTo(p[0], p[1]); });
  dCtx.stroke();

  dCtx.fillStyle = 'rgba(248,81,73,0.25)';
  dCtx.beginPath(); dCtx.moveTo(w*0.5, h);
  const askPoints = [[w*0.5, h], [w*0.55, h*0.78], [w*0.7, h*0.55], [w*0.85, h*0.4], [w, h*0.25]];
  askPoints.forEach(p => dCtx.lineTo(p[0], p[1]));
  dCtx.closePath(); dCtx.fill();

  dCtx.strokeStyle = '#f85149'; dCtx.lineWidth = 2; dCtx.beginPath();
  askPoints.forEach((p, i) => { if (i === 0) dCtx.moveTo(p[0], p[1]); else dCtx.lineTo(p[0], p[1]); });
  dCtx.stroke();

  dCtx.fillStyle = '#6e7681'; dCtx.font = '9px sans-serif';
  dCtx.fillText('1.5K', 4, 12); dCtx.fillText('1.0K', 4, h*0.33+4); dCtx.fillText('500', 4, h*0.66+4);
  dCtx.fillText('0', 4, h-4);
  dCtx.fillText('66,000', 4, h-2);
  dCtx.fillText('66,200', w*0.25-15, h-2);
  dCtx.fillText('66,400', w*0.5-15, h-2);
  dCtx.fillText('66,600', w*0.75-15, h-2);
}
