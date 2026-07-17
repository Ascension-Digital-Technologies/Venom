// @venom: browser
// src/js/layout-presets.js
// ===================== LAYOUT PRESETS V3 =====================
const NOVA_LAYOUT_PRESETS = {
  default: { label: 'Default', watchlist: 220, right: 420, bottom: 220 },
  chart: { label: 'Chart Focus', watchlist: 180, right: 340, bottom: 150 },
  scalping: { label: 'Scalping', watchlist: 210, right: 540, bottom: 180 },
  portfolio: { label: 'Portfolio', watchlist: 230, right: 380, bottom: 320 },
  compact: { label: 'Compact', watchlist: 180, right: 320, bottom: 150 }
};

function setPanelSize(id, size, axis = 'width') {
  const el = document.getElementById(id);
  if (!el) return;
  el.style[axis] = `${size}px`;
  el.style.flexBasis = `${size}px`;
}

function applyLayoutPreset(name = 'default') {
  const preset = NOVA_LAYOUT_PRESETS[name] || NOVA_LAYOUT_PRESETS.default;
  state.layoutPreset = name;
  setPanelSize('watchlistPanel', preset.watchlist, 'width');
  setPanelSize('rightPanels', preset.right, 'width');
  setPanelSize('bottomPanel', preset.bottom, 'height');
  document.querySelectorAll('.layout-preset').forEach(btn => btn.classList.toggle('active', btn.dataset.layoutPreset === name));
  safeStorage.write({ layoutPreset: name, watchlistWidth: preset.watchlist, rightWidth: preset.right, bottomHeight: preset.bottom });
  persistTradingState();
  resizeCanvas();
  drawDepth();
  showToast('info', 'Layout Preset', `${preset.label} layout applied`);
}

function resetLayoutPreset() {
  try { localStorage.removeItem(STORAGE_KEY); } catch {}
  applyLayoutPreset('default');
}

document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('.layout-preset').forEach(btn => {
    btn.addEventListener('click', () => applyLayoutPreset(btn.dataset.layoutPreset));
  });
  document.querySelectorAll('.layout-preset').forEach(btn => btn.classList.toggle('active', btn.dataset.layoutPreset === state.layoutPreset));
});
