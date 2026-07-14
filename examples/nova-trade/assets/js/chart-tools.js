// @venom: browser
// src/js/chart-tools.js
// ===================== CHART TOOLS V4 =====================
state.chartTools = {
  activeTool: 'crosshair',
  showEma20: true,
  showEma50: true,
  showEma200: true,
  showVolume: true,
  showRsi: true,
  measurements: []
};

function ensureChartHud() {
  const container = document.getElementById('chartContainer');
  if (!container || document.getElementById('chartHud')) return;
  const hud = document.createElement('div');
  hud.id = 'chartHud';
  hud.className = 'chart-hud';
  hud.innerHTML = `<div class="chart-hud-pill">Tool <strong id="chartHudTool">Crosshair</strong></div><div class="chart-hud-pill">Zoom <strong id="chartHudZoom">1.0x</strong></div>`;
  container.appendChild(hud);
}

function setChartTool(tool) {
  state.chartTools.activeTool = tool;
  const label = document.getElementById('chartHudTool');
  if (label) label.textContent = tool.replace(/(^|-)\w/g, m => m.toUpperCase()).replace('-', ' ');
  showToast('info', 'Chart Tool', `${label?.textContent || tool} activated`);
}

function resetChartView() {
  if (typeof chartView !== 'undefined') {
    chartView.zoom = 1;
    chartView.pan = 0;
  }
  resizeCanvas();
  showToast('success', 'Chart Reset', 'Zoom and pan restored');
}

function exportChartScreenshot() {
  try {
    const link = document.createElement('a');
    link.download = `nova-chart-${state.selectedSymbol.replace('/', '-')}-${Date.now()}.png`;
    link.href = document.getElementById('tradingChart').toDataURL('image/png');
    link.click();
    showToast('success', 'Screenshot Saved', 'Chart PNG exported by the browser');
  } catch (err) {
    showToast('error', 'Screenshot Failed', 'Browser blocked chart export');
  }
}

const v4BaseDrawChart = drawChart;
drawChart = function drawChartWithTools() {
  v4BaseDrawChart();
  ensureChartHud();
  const zoom = document.getElementById('chartHudZoom');
  if (zoom && typeof chartView !== 'undefined') zoom.textContent = `${chartView.zoom.toFixed(1)}x`;
};

document.addEventListener('DOMContentLoaded', () => {
  ensureChartHud();
  const draw = document.getElementById('ctxDrawLine');
  const measure = document.getElementById('ctxMeasure');
  const screenshot = document.getElementById('ctxScreenshot');
  const reset = document.getElementById('ctxResetChart');
  const refresh = document.getElementById('ctxRefresh');
  if (draw) draw.onclick = () => { setChartTool('trend-line'); document.getElementById('contextMenu').style.display = 'none'; };
  if (measure) measure.onclick = () => { setChartTool('measure'); document.getElementById('contextMenu').style.display = 'none'; };
  if (screenshot) screenshot.onclick = () => { exportChartScreenshot(); document.getElementById('contextMenu').style.display = 'none'; };
  if (reset) reset.onclick = () => { resetChartView(); document.getElementById('contextMenu').style.display = 'none'; };
  if (refresh) refresh.onclick = () => { updateLiveData(); resizeCanvas(); showToast('success', 'Refreshed', 'Market simulator ticked manually'); document.getElementById('contextMenu').style.display = 'none'; };
});

window.NovaChartTools = { setChartTool, resetChartView, exportChartScreenshot };
