(() => {
  const form = document.getElementById('profile-form');
  const name = document.getElementById('name');
  const enabled = document.getElementById('enabled');
  const mode = document.getElementById('mode');
  const parent = document.getElementById('parent');
  const child = document.getElementById('child');
  const status = document.getElementById('status');
  let bubbled = 0;
  parent.addEventListener('click', (event) => {
    if (event.target === child && event.currentTarget === parent) bubbled++;
  });
  child.addEventListener('click', () => { status.dataset.clicked = 'yes'; });
  form.addEventListener('submit', (event) => {
    event.preventDefault();
    const data = new FormData(form);
    status.textContent = [data.get('name'), enabled.checked ? 'on' : 'off', mode.value, bubbled].join(':');
  });
  const observer = new MutationObserver((records) => {
    status.dataset.mutations = String(records.length);
  });
  observer.observe(status, { attributes: true, childList: true, subtree: true });
  requestAnimationFrame(() => { status.dataset.raf = 'ready'; });
})();
