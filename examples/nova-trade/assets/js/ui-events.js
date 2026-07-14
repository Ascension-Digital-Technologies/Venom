// @venom: browser
// src/js/ui-events.js
// ===================== POSITIONS =====================
function renderPositions() {
  const tbody = document.getElementById('positionsBody');
  let totalPnl = 0;

  tbody.innerHTML = state.positions.map((p, idx) => {
    const pnl = (p.markPrice - p.avgEntry) * p.size;
    const pnlPct = (pnl / (p.avgEntry * p.size)) * 100;
    totalPnl += pnl;

    return `<tr data-idx="${idx}">
      <td>${p.symbol}</td>
      <td class="side-${p.side.toLowerCase()}">${p.side}</td>
      <td>${p.size.toFixed(3)} ${p.symbol.split('/')[0]}</td>
      <td>${p.avgEntry.toFixed(2)}</td>
      <td class="mark-price" data-idx="${idx}">${p.markPrice.toFixed(2)}</td>
      <td class="pnl-pos" data-pnl="${idx}">+${pnl.toFixed(2)} USDT</td>
      <td class="pnl-pos" data-pnlpct="${idx}">+${pnlPct.toFixed(2)}%</td>
      <td>${p.margin.toFixed(2)} USDT</td>
      <td>${p.risk.toFixed(2)}%</td>
      <td>
        <button class="action-btn" onclick="openTPSL(${idx})">TP/SL</button>
        <button class="action-btn" onclick="openCloseModal(${idx})">Close</button>
      </td>
    </tr>`;
  }).join('');

  // Update summary
  const totalPnlPct = (totalPnl / state.equity) * 100;
  document.getElementById('totalPnl').textContent = `+${totalPnl.toFixed(2)} USDT (+${totalPnlPct.toFixed(2)}%)`;
  document.getElementById('unrealizedPnl').textContent = `+$${totalPnl.toFixed(2)} (${totalPnlPct.toFixed(2)}%)`;
}


// ===================== TOAST NOTIFICATIONS =====================
function showToast(type, title, message) {
  const container = document.getElementById('toastContainer');
  const toast = document.createElement('div');
  toast.className = 'toast';
  const iconClass = type === 'success' ? 'success' : type === 'error' ? 'error' : 'info';
  const iconSvg = type === 'success' ? '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><path d="M20 6 9 17l-5-5"/></svg>' :
                  type === 'error' ? '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><path d="M18 6 6 18"/><path d="m6 6 12 12"/></svg>' :
                  '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M12 16v-4"/><path d="M12 8h.01"/></svg>';

  toast.innerHTML = `
    <div class="toast-icon ${iconClass}">${iconSvg}</div>
    <div class="toast-content">
      <div class="toast-title">${title}</div>
      <div class="toast-message">${message}</div>
    </div>
    <span class="toast-close" onclick="this.parentElement.remove()">&times;</span>
  `;
  container.appendChild(toast);
  setTimeout(() => toast.remove(), 5000);
}


// ===================== MODAL =====================
let closePositionIdx = -1;

function openCloseModal(idx) {
  closePositionIdx = idx;
  const p = state.positions[idx];
  const pnl = (p.markPrice - p.avgEntry) * p.size;
  document.getElementById('modalSymbol').textContent = p.symbol;
  document.getElementById('modalSide').textContent = p.side;
  document.getElementById('modalSide').style.color = p.side === 'Long' ? 'var(--green)' : 'var(--red)';
  document.getElementById('modalSize').textContent = p.size.toFixed(3) + ' ' + p.symbol.split('/')[0];
  document.getElementById('modalPnl').textContent = (pnl >= 0 ? '+' : '') + pnl.toFixed(2) + ' USDT';
  document.getElementById('modalPnl').style.color = pnl >= 0 ? 'var(--green)' : 'var(--red)';
  document.getElementById('closeModal').classList.add('active');
}

function openTPSL(idx) {
  showToast('info', 'TP/SL Editor', 'Take Profit / Stop Loss editor would open here');
}

document.getElementById('closeModalX').addEventListener('click', () => {
  document.getElementById('closeModal').classList.remove('active');
});

document.getElementById('modalCancel').addEventListener('click', () => {
  document.getElementById('closeModal').classList.remove('active');
});

document.getElementById('modalConfirm').addEventListener('click', () => {
  if (closePositionIdx >= 0) {
    const p = state.positions[closePositionIdx];
    showToast('success', 'Position Closed', `Closed ${p.size} ${p.symbol.split('/')[0]} ${p.side} at ${p.markPrice.toFixed(2)}`);
    state.positions.splice(closePositionIdx, 1);
    document.getElementById('posBadge').textContent = `(${state.positions.length})`;
    renderPositions();
    closePositionIdx = -1;
  }
  document.getElementById('closeModal').classList.remove('active');
});


// ===================== DRAG RESIZE / LAYOUT PERSISTENCE =====================
function applyStoredLayout() {
  const saved = safeStorage.read();
  const sizes = saved.panelSizes || {};
  Object.entries(sizes).forEach(([id, size]) => {
    const el = document.getElementById(id);
    const value = Number(size);
    if (!el || !Number.isFinite(value)) return;
    if (id === 'bottomPanel') {
      el.style.height = value + 'px';
      el.style.flexBasis = value + 'px';
    } else {
      el.style.width = value + 'px';
      el.style.flexBasis = value + 'px';
    }
  });
  if (saved.selectedSymbol && state.watchlist.some(w => w.sym === saved.selectedSymbol)) {
    state.selectedSymbol = saved.selectedSymbol;
    const selected = getSelectedMarket();
    state.currentPrice = selected.last;
    state.prevPrice = selected.last;
  }
  if (saved.timeframe) {
    document.querySelectorAll('.tf-btn').forEach(btn => btn.classList.toggle('active', btn.dataset.tf === saved.timeframe));
    document.getElementById('chartTf').textContent = saved.timeframe;
  }
}

function savePanelSize(target) {
  const saved = safeStorage.read();
  const panelSizes = { ...(saved.panelSizes || {}) };
  panelSizes[target.id] = target.id === 'bottomPanel' ? target.offsetHeight : target.offsetWidth;
  safeStorage.write({ panelSizes });
}

function initResize(handle, target, direction, minSize, maxSize) {
  if (!handle || !target) return;
  let isDragging = false;
  let startPos, startSize;

  handle.addEventListener('mousedown', (e) => {
    isDragging = true;
    handle.classList.add('dragging');
    document.body.style.cursor = direction === 'horizontal' ? 'col-resize' : 'row-resize';
    document.body.style.userSelect = 'none';
    startPos = direction === 'horizontal' ? e.clientX : e.clientY;
    startSize = direction === 'horizontal' ? target.offsetWidth : target.offsetHeight;
    e.preventDefault();
  });

  document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;
    const delta = direction === 'horizontal'
      ? (handle.classList.contains('right') ? startPos - e.clientX : e.clientX - startPos)
      : startPos - e.clientY;
    const viewportGuard = direction === 'vertical'
      ? Math.max(minSize, window.innerHeight - 260)
      : maxSize;
    const effectiveMax = Math.min(maxSize, viewportGuard);
    const newSize = Math.max(minSize, Math.min(effectiveMax, startSize + delta));
    if (direction === 'horizontal') {
      target.style.width = newSize + 'px';
      target.style.flexBasis = newSize + 'px';
    } else {
      target.style.height = newSize + 'px';
      target.style.flexBasis = newSize + 'px';
    }
    resizeCanvas();
    drawDepth();
  });

  document.addEventListener('mouseup', () => {
    if (!isDragging) return;
    isDragging = false;
    handle.classList.remove('dragging');
    document.body.style.cursor = '';
    document.body.style.userSelect = '';
    savePanelSize(target);
    showToast('info', 'Layout Saved', 'Panel size saved for this browser');
  });
}


// ===================== TABS =====================
function initTabs() {
  const saved = safeStorage.read();
  const makePersistentTabs = (selector, storageName, onClick) => {
    const tabs = document.querySelectorAll(selector);
    tabs.forEach(tab => {
      const key = tab.dataset.tf || tab.dataset.type || tab.dataset.tab || tab.textContent.trim();
      if (saved[storageName] && saved[storageName] === key) {
        tabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');
      }
      tab.addEventListener('click', () => {
        tabs.forEach(t => t.classList.remove('active'));
        tab.classList.add('active');
        safeStorage.write({ [storageName]: key });
        onClick?.(tab, key);
      });
    });
  };

  makePersistentTabs('.watchlist-tab', 'watchlistTab');
  makePersistentTabs('.tf-btn', 'timeframe', (btn, key) => {
    document.getElementById('chartTf').textContent = key || '1h';
    showToast('info', 'Timeframe', `Switched to ${key || 'custom'}`);
  });
  makePersistentTabs('.oe-tab', 'orderType', () => syncOrderEntry());
  makePersistentTabs('.bottom-tab', 'bottomTab');
  makePersistentTabs('.chart-tab', 'chartTab');
}


// ===================== KEYBOARD SHORTCUTS =====================
document.addEventListener('keydown', (e) => {
  const typing = ['INPUT', 'TEXTAREA', 'SELECT'].includes(e.target.tagName) || e.target.isContentEditable;
  if (e.ctrlKey && e.key.toLowerCase() === 'k') {
    e.preventDefault();
    document.getElementById('searchBox').focus();
    showToast('info', 'Search', 'Search box activated');
    return;
  }
  if (!typing && e.key.toLowerCase() === 'b') {
    document.getElementById('btnBuy').click();
  }
  if (!typing && e.key.toLowerCase() === 's') {
    document.getElementById('btnSell').click();
  }
  if (e.key === 'Escape') {
    document.getElementById('closeModal').classList.remove('active');
    document.getElementById('contextMenu').style.display = 'none';
  }
});


// ===================== CONTEXT MENU ACTIONS =====================
document.getElementById('ctxAddAlert').addEventListener('click', () => {
  showToast('info', 'Alert Added', 'Price alert set for BTC/USDT');
});
document.getElementById('ctxDrawLine').addEventListener('click', () => {
  showToast('info', 'Drawing Tool', 'Trend line tool activated');
});
document.getElementById('ctxMeasure').addEventListener('click', () => {
  showToast('info', 'Measure', 'Measure tool activated');
});
document.getElementById('ctxRefresh').addEventListener('click', () => {
  showToast('success', 'Refreshed', 'Data refreshed successfully');
});
document.getElementById('ctxScreenshot').addEventListener('click', () => {
  showToast('success', 'Screenshot', 'Chart screenshot saved');
});


// ===================== INIT =====================
window.addEventListener('resize', () => { resizeCanvas(); drawDepth(); });
document.addEventListener('DOMContentLoaded', () => {
  applyStoredLayout();
  initTabs();
  initOrderOptionChecks();
  resetChartForSymbol(state.selectedSymbol);
  renderWatchlist();
  renderScanner();
  renderOrderBook();
  renderTape();
  renderPositions();
  syncOrderEntry();
  resizeCanvas();
  drawDepth();

  initResize(document.getElementById('resizeWatchlist'), document.getElementById('watchlistPanel'), 'horizontal', 180, 350);
  initResize(document.getElementById('resizeRight'), document.getElementById('rightPanels'), 'horizontal', 390, 600);
  initResize(document.getElementById('bottomResize'), document.getElementById('bottomPanel'), 'vertical', 120, 400);

  // Start live simulation
  window.__novaSimTimer = setInterval(updateLiveData, state.simIntervalMs || 800);

  // Welcome toast
  setTimeout(() => {
    showToast('success', 'Connected', 'WebSocket connected to Binance');
  }, 500);
});
