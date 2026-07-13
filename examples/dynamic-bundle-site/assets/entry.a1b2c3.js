import { bootLabel } from './shared.4d5e6f.js';
const status = document.getElementById('status');
status.textContent = bootLabel();
document.getElementById('load').addEventListener('click', async () => {
  const feature = await import('./feature.9b1c2d.js');
  status.textContent = feature.dynamicLabel();
  status.setAttribute('data-dynamic', 'ready');
});
