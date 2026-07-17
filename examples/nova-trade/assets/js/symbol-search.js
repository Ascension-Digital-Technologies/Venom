// @venom: browser
// src/js/symbol-search.js
// ===================== SYMBOL SEARCH V4 =====================
let symbolSearchIndex = 0;
function openSymbolSearch() {
  const overlay = document.getElementById('symbolSearchOverlay');
  const input = document.getElementById('symbolSearchInput');
  if (!overlay || !input) return;
  overlay.classList.add('active');
  overlay.setAttribute('aria-hidden', 'false');
  input.value = '';
  renderSymbolResults('');
  setTimeout(() => input.focus(), 0);
}
function closeSymbolSearch() {
  const overlay = document.getElementById('symbolSearchOverlay');
  if (!overlay) return;
  overlay.classList.remove('active');
  overlay.setAttribute('aria-hidden', 'true');
}
function renderSymbolResults(query = '') {
  const q = query.trim().toLowerCase();
  const rows = state.watchlist.filter(w => !q || w.sym.toLowerCase().includes(q) || w.sym.split('/')[0].toLowerCase().includes(q));
  const box = document.getElementById('symbolSearchResults');
  if (!box) return;
  if (!rows.length) { box.innerHTML = `<div class="bottom-empty-state">No matching symbols.</div>`; return; }
  symbolSearchIndex = Math.min(symbolSearchIndex, rows.length - 1);
  box.innerHTML = rows.map((w, i) => {
    const up = w.chg >= 0;
    return `<div class="symbol-result ${i === symbolSearchIndex ? 'active' : ''}" data-symbol="${w.sym}"><div><div class="name">${w.sym}</div><div style="color:var(--text-muted);font-size:10px;">Crypto · Simulated feed</div></div><div class="price">${formatPrice(w.last)}</div><div class="chg ${up ? 'up' : 'down'}">${up ? '+' : ''}${w.chg.toFixed(2)}%</div></div>`;
  }).join('');
  box.querySelectorAll('.symbol-result').forEach(row => row.addEventListener('click', () => selectSymbolFromSearch(row.dataset.symbol)));
}
function selectSymbolFromSearch(symbol) {
  const row = document.querySelector(`.watchlist-row[data-sym="${symbol}"]`);
  if (row) row.click();
  else {
    state.selectedSymbol = symbol;
    safeStorage.write({ selectedSymbol: symbol });
    resetChartForSymbol(symbol);
    renderWatchlist();
  }
  closeSymbolSearch();
  showToast('success', 'Symbol Selected', symbol);
}

document.addEventListener('DOMContentLoaded', () => {
  document.getElementById('searchBox')?.addEventListener('click', openSymbolSearch);
  document.getElementById('symbolSearchClose')?.addEventListener('click', closeSymbolSearch);
  document.getElementById('symbolSearchOverlay')?.addEventListener('click', e => { if (e.target.id === 'symbolSearchOverlay') closeSymbolSearch(); });
  const input = document.getElementById('symbolSearchInput');
  if (input) {
    input.addEventListener('input', () => { symbolSearchIndex = 0; renderSymbolResults(input.value); });
    input.addEventListener('keydown', (e) => {
      const rows = [...document.querySelectorAll('.symbol-result')];
      if (e.key === 'ArrowDown') { e.preventDefault(); symbolSearchIndex = Math.min(rows.length - 1, symbolSearchIndex + 1); renderSymbolResults(input.value); }
      if (e.key === 'ArrowUp') { e.preventDefault(); symbolSearchIndex = Math.max(0, symbolSearchIndex - 1); renderSymbolResults(input.value); }
      if (e.key === 'Enter' && rows[symbolSearchIndex]) selectSymbolFromSearch(rows[symbolSearchIndex].dataset.symbol);
      if (e.key === 'Escape') closeSymbolSearch();
    });
  }
});

// Extend keyboard shortcuts without replacing the v3 handler.
document.addEventListener('keydown', (e) => {
  const typing = ['INPUT', 'TEXTAREA', 'SELECT'].includes(e.target.tagName) || e.target.isContentEditable;
  if (!typing && e.key === '/') { e.preventDefault(); openSymbolSearch(); }
  if (!typing && e.altKey && e.key.toLowerCase() === 'a') { e.preventDefault(); createPriceAlert(state.selectedSymbol, 'above', symbolPrice(state.selectedSymbol) * 1.0025); }
  if (!typing && e.key.toLowerCase() === 'l') setChartTool('trend-line');
  if (!typing && e.key.toLowerCase() === 'm') setChartTool('measure');
  if (!typing && e.key.toLowerCase() === 'r') resetChartView();
  if (e.ctrlKey && e.key.toLowerCase() === 's') { e.preventDefault(); exportChartScreenshot(); }
  if (e.key === 'Escape') { closeSymbolSearch(); closeSettingsDrawer?.(); }
});
window.NovaSymbolSearch = { openSymbolSearch, closeSymbolSearch, renderSymbolResults };
