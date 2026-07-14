// @venom: browser
// src/js/chart-upgrade.js
// ===================== CHART ENGINE V3: ZOOM / PAN / BETTER AXES =====================
const chartView = { zoom: 1, pan: 0, dragging: false, dragStartX: 0, startPan: 0 };

function drawChart() {
  if (!chartW || !chartH || !candles.length) return;
  const pad = { top: 12, right: 66, bottom: 28, left: 12 };
  const chartAreaH = Math.max(160, chartH * 0.66);
  const volTop = pad.top + chartAreaH + 18;
  const volH = Math.max(44, chartH * 0.17);
  const rsiTop = volTop + volH + 10;
  const rsiH = Math.max(36, chartH - rsiTop - 8);

  ctx.clearRect(0, 0, chartW, chartH);

  const baseCount = Math.max(28, Math.floor((chartW - pad.left - pad.right) / 8));
  const visibleCount = Math.max(24, Math.min(candles.length, Math.round(baseCount / chartView.zoom)));
  chartView.pan = Math.max(0, Math.min(chartView.pan, Math.max(0, candles.length - visibleCount)));
  const endIdx = candles.length - chartView.pan;
  const startIdx = Math.max(0, endIdx - visibleCount);
  const visible = candles.slice(startIdx, endIdx);
  const vEma20 = ema20.slice(startIdx, endIdx);
  const vEma50 = ema50.slice(startIdx, endIdx);
  const vEma200 = ema200.slice(startIdx, endIdx);
  if (!visible.length) return;

  const prices = visible.flatMap(c => [c.high, c.low]);
  const minRaw = Math.min(...prices);
  const maxRaw = Math.max(...prices);
  const padding = Math.max((maxRaw - minRaw) * 0.08, Math.abs(maxRaw) * 0.001);
  const minPrice = minRaw - padding;
  const maxPrice = maxRaw + padding;
  const priceRange = Math.max(maxPrice - minPrice, 0.000001);

  const gap = (chartW - pad.left - pad.right) / visible.length;
  const candleW = Math.max(2, Math.min(18, gap * 0.64));
  const toY = (price) => pad.top + chartAreaH - ((price - minPrice) / priceRange) * chartAreaH;

  ctx.strokeStyle = '#21262d';
  ctx.lineWidth = 1;
  ctx.font = '10px sans-serif';
  for (let i = 0; i <= 6; i++) {
    const y = pad.top + (chartAreaH / 6) * i;
    const price = maxPrice - (priceRange / 6) * i;
    ctx.beginPath(); ctx.moveTo(pad.left, y); ctx.lineTo(chartW - pad.right, y); ctx.stroke();
    ctx.fillStyle = '#6e7681'; ctx.textAlign = 'left';
    ctx.fillText(formatPrice(price), chartW - pad.right + 5, y + 3);
  }

  visible.forEach((c, i) => {
    const x = pad.left + i * gap + gap / 2;
    const yOpen = toY(c.open), yClose = toY(c.close), yHigh = toY(c.high), yLow = toY(c.low);
    const isGreen = c.close >= c.open;
    ctx.strokeStyle = isGreen ? '#3fb950' : '#f85149';
    ctx.fillStyle = ctx.strokeStyle;
    ctx.beginPath(); ctx.moveTo(x, yHigh); ctx.lineTo(x, yLow); ctx.stroke();
    ctx.fillRect(x - candleW / 2, Math.min(yOpen, yClose), candleW, Math.max(Math.abs(yOpen - yClose), 1));
    if (i === hoveredCandle - startIdx) {
      ctx.strokeStyle = 'rgba(88,166,255,.55)';
      ctx.strokeRect(x - candleW / 2 - 2, yHigh - 3, candleW + 4, Math.max(6, yLow - yHigh + 6));
    }
  });

  function drawEMA(emaD, color, width = 1.4) {
    ctx.strokeStyle = color; ctx.lineWidth = width; ctx.beginPath();
    let started = false;
    emaD.forEach((v, i) => {
      if (v == null) return;
      const x = pad.left + i * gap + gap / 2;
      const y = toY(v);
      if (!started) { ctx.moveTo(x, y); started = true; }
      else ctx.lineTo(x, y);
    });
    ctx.stroke();
  }
  if (state.chartTools?.showEma20 !== false) drawEMA(vEma20, '#d29922');
  if (state.chartTools?.showEma50 !== false) drawEMA(vEma50, '#58a6ff');
  if (state.chartTools?.showEma200 !== false) drawEMA(vEma200, '#a371f7');

  if (state.chartTools?.showVolume !== false) {
    const maxVol = Math.max(...visible.map(c => c.volume), 1);
    visible.forEach((c, i) => {
      const x = pad.left + i * gap + gap / 2;
      const h = Math.max(1, (c.volume / maxVol) * volH);
      ctx.fillStyle = c.close >= c.open ? 'rgba(63,185,80,.32)' : 'rgba(248,81,73,.32)';
      ctx.fillRect(x - candleW / 2, volTop + volH - h, candleW, h);
    });
    ctx.fillStyle = '#6e7681'; ctx.fillText('Volume', pad.left + 4, volTop + 12);
  }

  if (state.chartTools?.showRsi !== false) {
    ctx.strokeStyle = '#21262d';
    ctx.beginPath(); ctx.moveTo(pad.left, rsiTop); ctx.lineTo(chartW - pad.right, rsiTop); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(pad.left, rsiTop + rsiH); ctx.lineTo(chartW - pad.right, rsiTop + rsiH); ctx.stroke();
    ctx.setLineDash([2,2]);
    ctx.beginPath(); ctx.moveTo(pad.left, rsiTop + rsiH * .5); ctx.lineTo(chartW - pad.right, rsiTop + rsiH * .5); ctx.stroke();
    ctx.setLineDash([]);
    ctx.strokeStyle = '#a371f7'; ctx.beginPath();
    visible.forEach((c, i) => {
      const rsi = 50 + Math.sin((startIdx + i) / 5) * 18 + randomNormal() * 2;
      const x = pad.left + i * gap + gap / 2;
      const y = rsiTop + rsiH - (Math.max(10, Math.min(90, rsi)) / 100) * rsiH;
      if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
    ctx.fillStyle = '#6e7681'; ctx.fillText(`RSI 14  ${state.marketMode.toUpperCase()}`, pad.left + 4, rsiTop + rsiH - 4);
  }

  const labelEvery = Math.max(1, Math.floor(visible.length / 6));
  ctx.fillStyle = '#6e7681'; ctx.textAlign = 'center';
  visible.forEach((c, i) => {
    if (i % labelEvery !== 0 && i !== visible.length - 1) return;
    const x = pad.left + i * gap + gap / 2;
    const t = new Date(c.time).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    ctx.fillText(t, x, chartH - 8);
  });

  const hint = document.getElementById('chartHint') || (() => {
    const el = document.createElement('div'); el.id = 'chartHint'; el.className = 'chart-hint';
    document.getElementById('chartContainer').appendChild(el); return el;
  })();
  hint.textContent = `Wheel zoom ${chartView.zoom.toFixed(1)}x · drag pan`;
}

(function initChartInteractions() {
  const el = document.getElementById('chartContainer');
  if (!el) return;
  el.addEventListener('wheel', (e) => {
    e.preventDefault();
    const delta = e.deltaY < 0 ? 0.12 : -0.12;
    chartView.zoom = Math.max(0.75, Math.min(4.5, chartView.zoom + delta));
    drawChart();
  }, { passive: false });
  el.addEventListener('mousedown', (e) => {
    if (e.button !== 0) return;
    chartView.dragging = true; chartView.dragStartX = e.clientX; chartView.startPan = chartView.pan;
    el.classList.add('grabbing');
  });
  window.addEventListener('mousemove', (e) => {
    if (!chartView.dragging) return;
    const baseCount = Math.max(28, Math.floor((chartW - 78) / 8));
    const visibleCount = Math.max(24, Math.min(candles.length, Math.round(baseCount / chartView.zoom)));
    const pxPerCandle = Math.max(4, (chartW - 78) / visibleCount);
    chartView.pan = Math.max(0, Math.min(candles.length - visibleCount, Math.round(chartView.startPan + (e.clientX - chartView.dragStartX) / pxPerCandle)));
    drawChart();
  });
  window.addEventListener('mouseup', () => {
    chartView.dragging = false; el.classList.remove('grabbing');
  });
})();
