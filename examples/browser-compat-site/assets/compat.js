(function venomCompatFixture() {
  const output = document.getElementById('compat-output');
  const button = document.querySelector('#compat-button');
  const form = document.getElementById('compat-form');
  const input = document.querySelector('input[name="bridge-input"]');
  const link = document.querySelector('a[data-route="about"]');
  const add = (left) => (right) => left + right;
  const values = [1, 2, 3, 4];

  globalThis.__venomCompatClosure = add(4)(3);
  globalThis.__venomCompatIterator = values.map((value) => value * 2).reduce((sum, value) => sum + value, 0);

  try {
    throw new Error('compat-exception');
  } catch (error) {
    globalThis.__venomCompatException = error.message;
  }

  if (output) {
    output.textContent = 'dom:' + globalThis.__venomCompatClosure + ':' + globalThis.__venomCompatIterator;
    output.setAttribute('data-sync', 'done');
    output.classList.add('ready');
    output.classList.remove('pending');
    output.classList.toggle('toggled', true);
    output.dataset.bridge = 'dataset-ready';
    output.setAttribute('data-remove-me', 'temporary');
    output.removeAttribute('data-remove-me');
    output.style.setProperty('borderColor', 'green');
    output.style.backgroundColor = 'blue';
    const container = document.getElementById('compat-inner');
    if (container) {
      container.innerHTML = '<span class="bridge-child" data-kind="inner">inner-ready</span>';
      const appended = document.createElement('span');
      appended.id = 'compat-appended';
      appended.setAttribute('id', 'compat-appended');
      appended.textContent = 'append-ready';
      appended.setAttribute('data-created', 'yes');
      container.appendChild(appended);
    }
    globalThis.__venomCompatQueryAll = document.querySelectorAll('.action').length;
    globalThis.__venomCompatClosest = button && button.closest('#compat-root') ? button.closest('#compat-root').id : 'missing';
    globalThis.__venomCompatMatches = output.matches('#compat-output.ready');
  }

  if (button) {
    button.addEventListener('click', () => {
      globalThis.__venomCompatClicks = (globalThis.__venomCompatClicks || 0) + 1;
      if (output) output.setAttribute('data-clicked', 'yes');
    });
  }

  if (form) {
    form.addEventListener('submit', (event) => {
      event.preventDefault();
      globalThis.__venomCompatForm = 'posted:' + (input ? input.value : 'missing');
      if (output) output.setAttribute('data-form', 'submitted');
    });
    if (input) input.value = 'form-value';
    form.dispatchEvent(new Event('submit', { bubbles: true, cancelable: true }));
  }

  Promise.resolve(21).then((value) => {
    globalThis.__venomCompatPromise = value * 2;
    if (output) output.setAttribute('data-promise', 'done');
  });

  Promise.reject(new Error('async-bridge-error')).catch((error) => {
    globalThis.__venomCompatAsyncError = error.message;
  });

  fetch('compat-data.json')
    .then((response) => {
      globalThis.__venomCompatFetchStatus = response.status + ':' + (response.ok ? 'ok' : 'bad');
      globalThis.__venomCompatFetchHeader = response.headers && response.headers.get ? response.headers.get('x-venom-fixture') : 'missing';
      return response.json();
    })
    .then((data) => {
      globalThis.__venomCompatFetch = data.message + ':' + data.version;
      globalThis.__venomCompatFetchDetail = data.detail;
      if (output) {
        output.setAttribute('data-fetch', 'done');
        output.setAttribute('data-fetch-status', globalThis.__venomCompatFetchStatus);
        output.setAttribute('data-fetch-header', globalThis.__venomCompatFetchHeader || 'missing');
      }
    })
    .catch((error) => {
      globalThis.__venomCompatFetchDenied = error.message;
    });

  fetch('blocked-secret.json')
    .then(() => {
      globalThis.__venomCompatDenyByDefault = 'unexpected-allow';
    })
    .catch((error) => {
      globalThis.__venomCompatDenyByDefault = 'blocked-secret.json:denied';
      globalThis.__venomCompatDeniedHostCall = error.message;
    });


  fetch('compat-data.json')
    .then((response) => response.text())
    .then((text) => {
      globalThis.__venomCompatFetchText = text.includes('text-ok') ? 'text-ok' : 'text-missing';
      if (output) output.setAttribute('data-fetch-text', 'done');
    })
    .catch((error) => {
      globalThis.__venomCompatFetchDenied = error.message;
    });

  (async function venomCompatAsyncAwait() {
    const value = await Promise.resolve('await-ok');
    globalThis.__venomCompatAwait = value;
    if (output) output.setAttribute('data-await', 'done');
  }()).catch((error) => {
    globalThis.__venomCompatAwait = error.message;
  });

  (async function venomCompatAsyncThrow() {
    try {
      await Promise.resolve();
      throw new Error('async-throw-captured');
    } catch (error) {
      globalThis.__venomCompatAsyncThrow = error.message;
      if (output) output.setAttribute('data-async-throw', 'captured');
    }
  }());

  if (link) {
    link.addEventListener('click', (event) => {
      event.preventDefault();
      history.pushState({ route: 'about' }, '', link.getAttribute('href'));
      globalThis.__venomCompatRoute = 'about.html';
      globalThis.__venomCompatRouteHydrated = 'about-ready';
    });
  }

  setTimeout(() => {
    globalThis.__venomCompatTimer = 'timer-fired';
    if (output) output.setAttribute('data-timer', 'done');
  }, 0);
}());
