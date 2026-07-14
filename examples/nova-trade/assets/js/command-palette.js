// @venom: browser
// src/js/command-palette.js
// ===================== COMMAND PALETTE V3 =====================
const novaCommands = [
  { title: 'Reset Layout', subtitle: 'Restore default panel sizes', icon: 'R', shortcut: 'reset', run: () => resetLayoutPreset() },
  { title: 'Layout: Default', subtitle: 'Balanced trading terminal', icon: 'D', shortcut: 'layout', run: () => applyLayoutPreset('default') },
  { title: 'Layout: Chart Focus', subtitle: 'Maximize chart room', icon: 'C', shortcut: 'layout', run: () => applyLayoutPreset('chart') },
  { title: 'Layout: Scalping', subtitle: 'Prioritize order book and execution', icon: 'S', shortcut: 'layout', run: () => applyLayoutPreset('scalping') },
  { title: 'Layout: Portfolio', subtitle: 'Larger positions and account view', icon: 'P', shortcut: 'layout', run: () => applyLayoutPreset('portfolio') },
  ...['BTC/USDT','ETH/USDT','SOL/USDT','BNB/USDT','AVAX/USDT'].map(sym => ({ title: `Switch Symbol: ${sym}`, subtitle: 'Change active market', icon: sym[0], shortcut: 'symbol', run: () => applySelectedSymbol(sym) })),
  ...['1m','5m','15m','1h','4h','D','W'].map(tf => ({ title: `Timeframe: ${tf}`, subtitle: 'Change chart timeframe', icon: 'T', shortcut: 'timeframe', run: () => document.querySelector(`.tf-btn[data-tf="${tf}"]`)?.click() })),
  { title: 'Show Open Orders', subtitle: 'Open bottom tab', icon: 'O', shortcut: 'orders', run: () => document.querySelector('.bottom-tab[data-tab="openorders"]')?.click() },
  { title: 'Show Balances', subtitle: 'Open account balances', icon: 'B', shortcut: 'balances', run: () => document.querySelector('.bottom-tab[data-tab="balances"]')?.click() },
  { title: 'Show Performance', subtitle: 'Open trading performance', icon: 'P', shortcut: 'performance', run: () => document.querySelector('.bottom-tab[data-tab="performance"]')?.click() }
];
let activeCommandIndex = 0;

function openCommandPalette() {
  const overlay = document.getElementById('commandPalette');
  const input = document.getElementById('commandInput');
  overlay.classList.add('active'); overlay.setAttribute('aria-hidden', 'false');
  input.value = ''; activeCommandIndex = 0; renderCommandResults();
  setTimeout(() => input.focus(), 0);
}
function closeCommandPalette() {
  const overlay = document.getElementById('commandPalette');
  overlay.classList.remove('active'); overlay.setAttribute('aria-hidden', 'true');
}
function filteredCommands() {
  const q = document.getElementById('commandInput')?.value?.toLowerCase().trim() || '';
  if (!q) return novaCommands.slice(0, 12);
  return novaCommands.filter(c => `${c.title} ${c.subtitle} ${c.shortcut}`.toLowerCase().includes(q)).slice(0, 12);
}
function renderCommandResults() {
  const results = document.getElementById('commandResults');
  if (!results) return;
  const commands = filteredCommands();
  if (activeCommandIndex >= commands.length) activeCommandIndex = Math.max(0, commands.length - 1);
  results.innerHTML = commands.length ? commands.map((cmd, idx) => `<div class="command-item ${idx === activeCommandIndex ? 'active' : ''}" data-command-index="${idx}"><div class="command-icon">${cmd.icon}</div><div><div class="command-title">${cmd.title}</div><div class="command-subtitle">${cmd.subtitle}</div></div><div class="command-shortcut">${cmd.shortcut}</div></div>`).join('') : '<div class="bottom-empty-state">No matching commands.</div>';
  results.querySelectorAll('.command-item').forEach(item => {
    item.addEventListener('click', () => runCommand(Number(item.dataset.commandIndex)));
  });
}
function runCommand(index = activeCommandIndex) {
  const command = filteredCommands()[index];
  if (!command) return;
  closeCommandPalette();
  command.run();
}

document.addEventListener('keydown', (e) => {
  if (e.ctrlKey && e.key.toLowerCase() === 'k') {
    e.preventDefault(); e.stopImmediatePropagation(); openCommandPalette(); return;
  }
  const overlay = document.getElementById('commandPalette');
  if (!overlay?.classList.contains('active')) return;
  if (e.key === 'Escape') { e.preventDefault(); closeCommandPalette(); }
  if (e.key === 'ArrowDown') { e.preventDefault(); activeCommandIndex++; renderCommandResults(); }
  if (e.key === 'ArrowUp') { e.preventDefault(); activeCommandIndex = Math.max(0, activeCommandIndex - 1); renderCommandResults(); }
  if (e.key === 'Enter') { e.preventDefault(); runCommand(); }
}, true);

document.addEventListener('DOMContentLoaded', () => {
  document.getElementById('commandInput')?.addEventListener('input', () => { activeCommandIndex = 0; renderCommandResults(); });
  document.getElementById('commandPalette')?.addEventListener('click', (e) => { if (e.target.id === 'commandPalette') closeCommandPalette(); });
});
