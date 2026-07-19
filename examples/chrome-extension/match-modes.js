(() => {
  'use strict';
  if (globalThis.VelocityMatchModes) return;

  const PRESETS = Object.freeze({
    '1m': Object.freeze({
      id: '1m',
      label: '1 minute · Bullet',
      description: 'Very fast search and compact mouse timing for one-minute games.',
      maxDepth: 4,
      moveTimeMs: 180,
      minDelayMs: 120,
      maxDelayMs: 650,
      dragMinMs: 140,
      dragMaxMs: 360
    }),
    '3m': Object.freeze({
      id: '3m',
      label: '3 minutes · Blitz',
      description: 'Fast blitz pacing with short but visibly varied decisions.',
      maxDepth: 5,
      moveTimeMs: 450,
      minDelayMs: 220,
      maxDelayMs: 1100,
      dragMinMs: 190,
      dragMaxMs: 500
    }),
    '5m': Object.freeze({
      id: '5m',
      label: '5 minutes · Blitz',
      description: 'Balanced search strength and natural movement for five-minute games.',
      maxDepth: 5,
      moveTimeMs: 800,
      minDelayMs: 320,
      maxDelayMs: 1600,
      dragMinMs: 230,
      dragMaxMs: 620
    }),
    '10m': Object.freeze({
      id: '10m',
      label: '10 minutes · Rapid',
      description: 'Longer thought windows and calmer cursor pacing for rapid games.',
      maxDepth: 6,
      moveTimeMs: 1300,
      minDelayMs: 450,
      maxDelayMs: 2400,
      dragMinMs: 270,
      dragMaxMs: 780
    })
  });

  function getPreset(id) {
    const preset = PRESETS[id];
    return preset ? { ...preset } : null;
  }

  globalThis.VelocityMatchModes = Object.freeze({ PRESETS, getPreset });
})();
