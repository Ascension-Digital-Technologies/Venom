// @venom: browser
'use strict';

var START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
var STACK_SIZE = 100;
var board = null;
var $board = $('#myBoard');
var currentState = { fen: START_FEN, turn: 'w', gameOver: false, inCheck: false, history: [], repetition: [], evaluation: 0 };
var undoStack = [];
var redoStack = [];
var engineEvaluation = 0;
var squareClass = 'square-55d63';
var timer = null;
var engineBusy = false;
var autoPlayGeneration = 0;
var autoPlayRunning = false;
var evaluationHistory = [0];
var protectedRuntimeReady = false;
var benchmarkGeneration = 0;
var benchmarkRunning = false;

var config = {
  draggable: true,
  position: 'start',
  onDragStart: onDragStart,
  onDrop: onDrop,
  onMouseoutSquare: removeGreySquares,
  onMouseoverSquare: onMouseoverSquare,
  onSnapEnd: function () { board.position(currentState.fen); }
};
board = Chessboard('myBoard', config);

function getVenomApi() {
  var api = globalThis.venom;
  if (!api || typeof api.ready !== 'function' || !api.exports) throw new Error('Venom protected bridge is unavailable');
  return api;
}

async function callChessEngine(request, options) {
  var api = getVenomApi();
  await api.ready();
  if (typeof api.exports.runChessEngine !== 'function') throw new Error('Protected export runChessEngine is unavailable');
  return api.exports.runChessEngine(request, options || { timeout: 35000 });
}

async function ensureProtectedRuntime() {
  if (protectedRuntimeReady) return getVenomApi();
  var api = getVenomApi();
  await api.ready();
  if (typeof api.exports.runChessEngine !== 'function') throw new Error('Protected export runChessEngine is unavailable');
  protectedRuntimeReady = true;
  return api;
}

function setStatus(message, isError) {
  $('#status').text(message).toggleClass('error-text', Boolean(isError));
}

function setEngineBusy(busy, label) {
  engineBusy = busy;
  $('#engine-busy').toggleClass('active', busy).text(label || (busy ? 'Searching…' : 'Idle'));
}

function updateTurn() {
  $('#turn-badge').text(currentState.turn === 'w' ? 'White to move' : 'Black to move');
}

function statusMessage() {
  if (currentState.checkmate) return 'Checkmate — ' + (currentState.turn === 'w' ? 'White' : 'Black') + ' lost.';
  if (currentState.insufficientMaterial) return 'Draw by insufficient material.';
  if (currentState.threefoldRepetition) return 'Draw by threefold repetition.';
  if (currentState.stalemate) return 'Draw by stalemate.';
  if (currentState.draw) return 'Draw by the 50-move rule.';
  if (currentState.inCheck) return (currentState.turn === 'w' ? 'White' : 'Black') + ' is in check.';
  return autoPlayRunning ? 'Protected engines are playing.' : 'Protected engine ready.';
}

function checkStatus() {
  setStatus(statusMessage(), false);
  updateTurn();
  return Boolean(currentState.gameOver);
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function formatEvaluation(value) {
  if (value >= 990000) return 'White has mate';
  if (value <= -990000) return 'Black has mate';
  if (Math.abs(value) < 10) return 'Equal position';
  return (value > 0 ? 'White +' : 'Black +') + (Math.abs(value) / 100).toFixed(2);
}

function updateAdvantage(pushHistory) {
  var normalized = clamp(engineEvaluation, -2000, 2000);
  $('#advantageBar').css('left', (((2000 - normalized) / 4000) * 100) + '%');
  $('#evaluation-text').text(formatEvaluation(engineEvaluation));
  if (pushHistory !== false) {
    evaluationHistory.push(engineEvaluation);
    if (evaluationHistory.length > 60) evaluationHistory.shift();
  }
  drawEvaluationChart();
}

function drawEvaluationChart() {
  var canvas = document.getElementById('evaluation-chart');
  if (!canvas || !canvas.getContext) return;
  var ratio = window.devicePixelRatio || 1;
  var width = canvas.clientWidth || 900;
  var height = canvas.clientHeight || 120;
  if (canvas.width !== Math.round(width * ratio) || canvas.height !== Math.round(height * ratio)) {
    canvas.width = Math.round(width * ratio);
    canvas.height = Math.round(height * ratio);
  }
  var ctx = canvas.getContext('2d');
  ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
  ctx.clearRect(0, 0, width, height);
  ctx.strokeStyle = 'rgba(148,163,184,.22)';
  ctx.beginPath();
  ctx.moveTo(0, height / 2);
  ctx.lineTo(width, height / 2);
  ctx.stroke();
  if (evaluationHistory.length < 2) return;
  ctx.strokeStyle = '#22d3ee';
  ctx.lineWidth = 2;
  ctx.beginPath();
  evaluationHistory.forEach(function (score, index) {
    var x = (index / (evaluationHistory.length - 1)) * width;
    var y = height / 2 - (clamp(score, -1200, 1200) / 1200) * (height * 0.42);
    if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();
}

function updateTelemetry(result) {
  var completed = Number(result.depth || 0);
  var requested = Number(result.requestedDepth || completed);
  $('#metric-depth').text(completed ? (completed + (requested > completed ? ' / ' + requested : '')) : '—');
  $('#position-count').text(Number(result.positions || 0).toLocaleString());
  $('#time').text(((result.elapsedMs || 0) / 1000).toFixed(3));
  $('#positions-per-s').text(Number(result.positionsPerSecond || 0).toLocaleString());
  var positions = Number(result.positions || 0);
  var qnodes = Number(result.qnodes || 0);
  var ttHits = Number(result.ttHits || 0);
  var ttStores = Number(result.ttStores || 0);
  var ttRate = positions > 0 ? (ttHits * 100 / positions) : 0;
  $('#metric-qnodes').text(qnodes.toLocaleString());
  $('#metric-tt-rate').text(ttRate.toFixed(1) + '%');
  $('#metric-cutoffs').text(Number(result.cutoffs || 0).toLocaleString());
  $('#metric-tt-stores').text(ttStores.toLocaleString());
  $('#position-count').attr('title', 'Quiescence nodes: ' + qnodes.toLocaleString() + ' · TT hits: ' + ttHits.toLocaleString());
}

function updateMoveLog() {
  var history = currentState.history || [];
  $('#move-count').text(history.length + (history.length === 1 ? ' move' : ' moves'));
  var $log = $('#move-log').empty();
  if (!history.length) {
    $log.append('<div class="empty-state">Moves will appear here.</div>');
    return;
  }
  for (var i = 0; i < history.length; i += 2) {
    var row = $('<div class="move-row"></div>');
    row.append($('<span></span>').text((Math.floor(i / 2) + 1) + '.'));
    row.append($('<span></span>').text(history[i] || ''));
    row.append($('<span></span>').text(history[i + 1] || ''));
    $log.append(row);
  }
  $log.scrollTop($log[0].scrollHeight);
}

function highlightMove(move, color) {
  $board.find('.' + squareClass).removeClass('highlight-white highlight-black');
  $board.find('.square-' + move.from).addClass('highlight-' + color);
  $board.find('.square-' + move.to).addClass('highlight-' + color);
}

function applySnapshot(snapshot, move, pushUndo) {
  if (pushUndo) {
    undoStack.push(JSON.parse(JSON.stringify(currentState)));
    if (undoStack.length > STACK_SIZE) undoStack.shift();
    redoStack = [];
  }
  currentState = snapshot;
  engineEvaluation = Number(currentState.evaluation) || 0;
  board.position(currentState.fen);
  if (move) highlightMove(move, currentState.turn === 'w' ? 'black' : 'white');
  updateMoveLog();
  updateTurn();
  updateAdvantage();
  checkStatus();
}

async function protectedMove(move, pushUndo) {
  var result = await callChessEngine({
    action: 'move',
    fen: currentState.fen,
    move: move,
    history: currentState.history || [],
    repetition: currentState.repetition || []
  });
  if (!result || !result.state) throw new Error('protected move returned no state');
  applySnapshot(result.state, result.move, pushUndo);
  return result;
}

function selectedSearchLimit(color) {
  var depth = parseInt($(color === 'b' ? '#search-depth' : '#search-depth-white').val(), 10);
  var timeMs = parseInt($('#search-time').val(), 10);
  return {
    maxDepth: Number.isFinite(depth) ? depth : 4,
    timeMs: Number.isFinite(timeMs) ? timeMs : 1500
  };
}

var ENGINE_PRESETS = {
  fast: { depth: 3, timeMs: 250, delay: 80 },
  balanced: { depth: 4, timeMs: 1500, delay: 250 },
  strong: { depth: 6, timeMs: 3000, delay: 250 },
  maximum: { depth: 8, timeMs: 5000, delay: 700 }
};

function applyEnginePreset(name) {
  var preset = ENGINE_PRESETS[name] || ENGINE_PRESETS.balanced;
  $('#search-depth-white').val(String(preset.depth));
  $('#search-depth').val(String(preset.depth));
  $('#search-time').val(String(preset.timeMs));
  $('#move-delay').val(String(preset.delay));
  setStatus('Applied ' + name + ' engine preset.', false);
}

var BENCHMARK_POSITIONS = {
  start: START_FEN,
  tactical: 'r1bq1rk1/ppp2ppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 4 8',
  middlegame: 'r2q1rk1/pp1nbppp/2p1pn2/3p4/3P4/2NBPN2/PPQ2PPP/2KR1B1R w - - 5 10',
  endgame: '8/5pk1/6p1/3p4/3P1P2/4K1P1/8/8 w - - 0 1'
};

var BENCHMARK_PRESETS = {
  quick: [
    { name: 'Move generation · start', type: 'perft', fen: BENCHMARK_POSITIONS.start, depth: 3, expected: 8902 },
    { name: 'Search · tactical', type: 'search', fen: BENCHMARK_POSITIONS.tactical, depth: 4, timeMs: 350 },
    { name: 'Search · endgame', type: 'search', fen: BENCHMARK_POSITIONS.endgame, depth: 5, timeMs: 350 }
  ],
  standard: [
    { name: 'Move generation · start', type: 'perft', fen: BENCHMARK_POSITIONS.start, depth: 4, expected: 197281 },
    { name: 'Move generation · tactical', type: 'perft', fen: BENCHMARK_POSITIONS.tactical, depth: 3 },
    { name: 'Search · opening', type: 'search', fen: BENCHMARK_POSITIONS.start, depth: 5, timeMs: 900 },
    { name: 'Search · tactical', type: 'search', fen: BENCHMARK_POSITIONS.tactical, depth: 6, timeMs: 1200 },
    { name: 'Search · middlegame', type: 'search', fen: BENCHMARK_POSITIONS.middlegame, depth: 6, timeMs: 1200 },
    { name: 'Search · endgame', type: 'search', fen: BENCHMARK_POSITIONS.endgame, depth: 7, timeMs: 900 }
  ],
  endurance: [
    { name: 'Move generation · start d5', type: 'perft', fen: BENCHMARK_POSITIONS.start, depth: 5, expected: 4865609 },
    { name: 'Move generation · tactical d4', type: 'perft', fen: BENCHMARK_POSITIONS.tactical, depth: 4 },
    { name: 'Search · opening 3s', type: 'search', fen: BENCHMARK_POSITIONS.start, depth: 9, timeMs: 3000 },
    { name: 'Search · tactical 4s', type: 'search', fen: BENCHMARK_POSITIONS.tactical, depth: 10, timeMs: 4000 },
    { name: 'Search · middlegame 4s', type: 'search', fen: BENCHMARK_POSITIONS.middlegame, depth: 10, timeMs: 4000 },
    { name: 'Search · endgame 3s', type: 'search', fen: BENCHMARK_POSITIONS.endgame, depth: 11, timeMs: 3000 }
  ]
};

function benchmarkNumber(value) {
  return Number(value || 0).toLocaleString();
}

function benchmarkResultValue(value) {
  var current = value;
  for (var depth = 0; depth < 4; depth += 1) {
    if (typeof current === 'string') {
      try { current = JSON.parse(current); } catch (_) { break; }
      continue;
    }
    if (!current || typeof current !== 'object') break;
    if (current.result && typeof current.result === 'object') { current = current.result; continue; }
    if (current.value && typeof current.value === 'object' && !('nodes' in current) && !('positions' in current)) { current = current.value; continue; }
    if (current.data && typeof current.data === 'object' && !('nodes' in current) && !('positions' in current)) { current = current.data; continue; }
    break;
  }
  return current && typeof current === 'object' ? current : {};
}

function benchmarkCell(text, className) {
  var cell = document.createElement('td');
  cell.textContent = text;
  if (className) cell.className = className;
  return cell;
}

function renderBenchmarkRows(tests) {
  var body = document.getElementById('benchmark-results');
  if (!body) return;
  body.replaceChildren();
  tests.forEach(function (test, index) {
    var row = document.createElement('tr');
    row.id = 'benchmark-row-' + index;
    row.appendChild(benchmarkCell(test.name));
    row.appendChild(benchmarkCell('Queued', 'benchmark-result-running'));
    row.appendChild(benchmarkCell('—'));
    row.appendChild(benchmarkCell('—'));
    row.appendChild(benchmarkCell('—'));
    body.appendChild(row);
  });
}

function updateBenchmarkRow(index, status, rawResult, error) {
  var row = document.getElementById('benchmark-row-' + index);
  if (!row || row.cells.length < 5) return;
  var result = benchmarkResultValue(rawResult);
  var elapsed = Number(result.elapsedMs || result.durationMs || result.timeMs || 0);
  var nodes = Number(result.nodes || result.positions || result.nodeCount || 0);
  var reportedNps = Number(result.positionsPerSecond || result.nps || 0);
  var nps = reportedNps || (elapsed > 0 ? Math.round(nodes * 1000 / elapsed) : 0);
  var label = error ? String(error) : status;
  var cls = status === 'Passed' ? 'benchmark-result-pass' : status === 'Running' ? 'benchmark-result-running' : 'benchmark-result-fail';
  row.cells[1].className = cls;
  row.cells[1].textContent = label;
  row.cells[2].textContent = elapsed > 0 ? (elapsed / 1000).toFixed(3) + 's' : '—';
  row.cells[3].textContent = nodes > 0 ? benchmarkNumber(nodes) : '—';
  row.cells[4].textContent = nps > 0 ? benchmarkNumber(nps) : '—';
}

function setBenchmarkSummary(totalElapsed, totalNodes, completed, total, failed) {
  $('#benchmark-total-time').text((totalElapsed / 1000).toFixed(2) + 's');
  $('#benchmark-total-nodes').text(benchmarkNumber(totalNodes));
  $('#benchmark-average-nps').text(totalElapsed > 0 ? benchmarkNumber(Math.round(totalNodes * 1000 / totalElapsed)) : '0');
  $('#benchmark-completed').text(completed + ' / ' + total);
  var summary = document.getElementById('benchmark-final-summary');
  if (summary) {
    summary.hidden = completed === 0;
    summary.className = failed ? 'benchmark-final-summary benchmark-final-fail' : 'benchmark-final-summary benchmark-final-pass';
    summary.textContent = completed === total
      ? (failed ? 'Benchmark completed with one or more failed workloads. Results are shown below.' : 'Benchmark complete. All protected TurboJS workloads passed.')
      : 'Benchmark progress: ' + completed + ' of ' + total + ' workloads completed.';
  }
}

function stopBenchmark(cancelled) {
  benchmarkGeneration += 1;
  benchmarkRunning = false;
  $('#benchmarkBtn').prop('disabled', false).text('Run Benchmark');
  $('#benchmarkCancelBtn').prop('disabled', true);
  $('#benchmark-state').removeClass('active').text(cancelled ? 'Cancelled' : 'Ready');
  var summary = document.getElementById('benchmark-final-summary');
  if (summary && cancelled) {
    summary.hidden = false;
    summary.className = 'benchmark-final-summary benchmark-final-fail';
    summary.textContent = 'Benchmark cancelled. Completed workload results remain visible below.';
  }
}

function benchmarkClockMs() {
  if (typeof performance !== 'undefined' && performance && typeof performance.now === 'function') {
    return performance.now();
  }
  return Date.now();
}

async function executeBenchmarkTest(test) {
  var response;
  var started = benchmarkClockMs();
  if (test.type === 'perft') {
    response = await callChessEngine({ action: 'perft', fen: test.fen, depth: test.depth }, { timeout: 120000 });
  } else {
    response = await callChessEngine({ action: 'search', fen: test.fen, maxDepth: test.depth, timeMs: test.timeMs, play: false }, { timeout: Math.max(35000, test.timeMs + 15000) });
  }
  var finished = benchmarkClockMs();
  var result = benchmarkResultValue(response);
  var engineElapsed = Number(result.elapsedMs || result.durationMs || result.timeMs || 0);
  var wallElapsed = Math.max(0.001, finished - started);
  result.engineElapsedMs = engineElapsed;
  result.wallElapsedMs = wallElapsed;
  // Benchmark the complete protected TurboJS/WASM call. This remains reliable even
  // when the sandboxed engine clock has millisecond resolution or is unavailable.
  result.elapsedMs = wallElapsed;
  var nodes = Number(result.nodes || result.positions || result.nodeCount || 0);
  result.positionsPerSecond = nodes > 0 ? Math.round(nodes * 1000 / wallElapsed) : 0;
  return result;
}

async function runBenchmark() {
  if (benchmarkRunning || engineBusy || autoPlayRunning) return;
  await ensureProtectedRuntime();
  var presetName = String($('#benchmark-preset').val() || 'standard');
  var tests = BENCHMARK_PRESETS[presetName] || BENCHMARK_PRESETS.standard;
  var generation = ++benchmarkGeneration;
  benchmarkRunning = true;
  $('#benchmarkBtn').prop('disabled', true).text('Benchmark Running…');
  $('#benchmarkCancelBtn').prop('disabled', false);
  $('#benchmark-state').addClass('active').text('Running');
  $('#benchmark-progress-bar').css('width', '0%');
  renderBenchmarkRows(tests);
  var summary = document.getElementById('benchmark-final-summary');
  if (summary) summary.hidden = true;
  setBenchmarkSummary(0, 0, 0, tests.length, false);

  var totalElapsed = 0;
  var totalNodes = 0;
  var completed = 0;
  var failed = false;
  for (var index = 0; index < tests.length; index += 1) {
    if (!benchmarkRunning || generation !== benchmarkGeneration) return;
    var test = tests[index];
    updateBenchmarkRow(index, 'Running');
    try {
      var result = await executeBenchmarkTest(test);
      if (!benchmarkRunning || generation !== benchmarkGeneration) return;
      var nodes = Number(result.nodes || result.positions || result.nodeCount || 0);
      var elapsed = Number(result.elapsedMs || result.durationMs || result.timeMs || 0);
      if (typeof test.expected === 'number' && nodes !== test.expected) throw new Error('Expected ' + benchmarkNumber(test.expected) + ', received ' + benchmarkNumber(nodes));
      updateBenchmarkRow(index, 'Passed', result);
      totalElapsed += elapsed;
      totalNodes += nodes;
      completed += 1;
      if (test.type === 'search') updateTelemetry(result);
    } catch (error) {
      failed = true;
      updateBenchmarkRow(index, 'Failed', null, error && error.message ? error.message : String(error));
      completed += 1;
    }
    var progress = Math.round(completed * 100 / tests.length);
    $('#benchmark-progress-bar').css('width', progress + '%');
    setBenchmarkSummary(totalElapsed, totalNodes, completed, tests.length, failed);
    await new Promise(function (resolve) { window.setTimeout(resolve, 30); });
  }
  benchmarkRunning = false;
  $('#benchmarkBtn').prop('disabled', false).text('Run Benchmark');
  $('#benchmarkCancelBtn').prop('disabled', true);
  $('#benchmark-state').removeClass('active').text(failed ? 'Completed with errors' : 'Complete');
  setBenchmarkSummary(totalElapsed, totalNodes, completed, tests.length, failed);
  setStatus('Protected benchmark ' + (failed ? 'completed with one or more failures.' : 'completed successfully.'), failed);
}

async function requestBestMove(color, playMove) {
  var limits = selectedSearchLimit(color);
  var result = await callChessEngine({
    action: 'search',
    fen: currentState.fen,
    history: currentState.history || [],
    repetition: currentState.repetition || [],
    maxDepth: limits.maxDepth,
    timeMs: limits.timeMs,
    play: Boolean(playMove)
  });
  updateTelemetry(result);
  return result;
}

async function makeBestMove(color) {
  if (engineBusy) return false;
  if (currentState.turn !== color) color = currentState.turn;
  setEngineBusy(true, 'Protected search…');
  try {
    var result = await requestBestMove(color, true);
    if (!result.move || !result.state) return false;
    result.state.evaluation = Number(result.value) || 0;
    applySnapshot(result.state, result.move, true);
    return true;
  } finally {
    setEngineBusy(false);
  }
}

async function analyzePosition() {
  if (engineBusy || currentState.gameOver) return;
  setEngineBusy(true, 'Analyzing…');
  try {
    var result = await requestBestMove(currentState.turn, false);
    if (typeof result.value === 'number') {
      engineEvaluation = result.value;
      updateAdvantage();
    }
    if (result.move) highlightMove(result.move, 'white');
    var line = Array.isArray(result.pv) && result.pv.length ? ' · PV: ' + result.pv.join(' ') : '';
    var timeoutNote = result.timedOut ? ' (time limit reached)' : '';
    setStatus('Analysis: ' + formatEvaluation(result.value || 0) + ' at depth ' + (result.depth || 0) + timeoutNote + line, false);
  } finally {
    setEngineBusy(false);
  }
}

function stopAutoPlay() {
  autoPlayGeneration += 1;
  autoPlayRunning = false;
  if (timer !== null) {
    window.clearTimeout(timer);
    timer = null;
  }
  $('#compVsCompBtn').prop('disabled', false).text('Start Game');
  $('#match-title').text('Human vs. Protected AI');
}

function scheduleAutoPlay(color, generation) {
  if (!autoPlayRunning || generation !== autoPlayGeneration) return;
  if (checkStatus()) {
    stopAutoPlay();
    return;
  }
  var delay = parseInt($('#move-delay').val(), 10) || 250;
  timer = window.setTimeout(async function () {
    timer = null;
    if (!autoPlayRunning || generation !== autoPlayGeneration) return;
    try {
      var moved = await makeBestMove(color);
      if (!moved) {
        stopAutoPlay();
        return;
      }
      scheduleAutoPlay(color === 'w' ? 'b' : 'w', generation);
    } catch (error) {
      setStatus('Engine error: ' + error.message, true);
      stopAutoPlay();
    }
  }, delay);
}

async function startAutoPlay() {
  var $button = $('#compVsCompBtn');
  $button.prop('disabled', true).text('Starting…');
  try {
    await ensureProtectedRuntime();
    reset();
    autoPlayRunning = true;
    var generation = autoPlayGeneration;
    $button.text('Game Running…');
    $('#match-title').text('Protected AI vs. Protected AI');
    scheduleAutoPlay('w', generation);
  } catch (error) {
    protectedRuntimeReady = false;
    $button.prop('disabled', false).text('Start Game');
    setStatus('Unable to start protected engine: ' + error.message, true);
  }
}

function reset() {
  stopAutoPlay();
  currentState = { fen: START_FEN, turn: 'w', gameOver: false, inCheck: false, history: [], repetition: [], evaluation: 0 };
  engineEvaluation = 0;
  undoStack = [];
  redoStack = [];
  evaluationHistory = [0];
  $board.find('.' + squareClass).removeClass('highlight-white highlight-black highlight-hint');
  board.position(currentState.fen);
  updateAdvantage(false);
  updateMoveLog();
  updateTurn();
  setStatus('Protected engine ready.', false);
}

async function loadOpening(fen) {
  reset();
  try {
    var result = await callChessEngine({ action: 'state', fen: fen || START_FEN, history: [] });
    if (!result || !result.state) throw new Error('protected state request returned no state');
    currentState = result.state;
    engineEvaluation = Number(currentState.evaluation) || 0;
    board.position(currentState.fen);
    updateTurn();
    updateAdvantage();
    checkStatus();
  } catch (error) {
    setStatus('Unable to load opening: ' + error.message, true);
  }
}

function undo() {
  if (!undoStack.length) return;
  redoStack.push(JSON.parse(JSON.stringify(currentState)));
  currentState = undoStack.pop();
  engineEvaluation = Number(currentState.evaluation) || 0;
  board.position(currentState.fen);
  updateMoveLog();
  updateTurn();
  updateAdvantage();
  checkStatus();
}

function redo() {
  if (!redoStack.length) return;
  undoStack.push(JSON.parse(JSON.stringify(currentState)));
  currentState = redoStack.pop();
  engineEvaluation = Number(currentState.evaluation) || 0;
  board.position(currentState.fen);
  updateMoveLog();
  updateTurn();
  updateAdvantage();
  checkStatus();
}

async function showHint() {
  $board.find('.' + squareClass).removeClass('highlight-hint');
  if (!document.getElementById('showHint').checked || currentState.gameOver || engineBusy) return;
  try {
    var result = await requestBestMove(currentState.turn, false);
    if (result.move) {
      $board.find('.square-' + result.move.from).addClass('highlight-hint');
      $board.find('.square-' + result.move.to).addClass('highlight-hint');
    }
  } catch (error) {
    setStatus('Hint failed: ' + error.message, true);
  }
}

function removeGreySquares() {
  $('#myBoard .square-55d63').css('background', '');
}

function greySquare(square) {
  var $square = $('#myBoard .square-' + square);
  $square.css('background', $square.hasClass('black-3c85d') ? '#4a5568' : '#94a3b8');
}

function onMouseoverSquare(square) {
  void callChessEngine({ action: 'moves', fen: currentState.fen, square: String(square) }).then(function (result) {
    var moves = result.moves || [];
    if (!moves.length) return;
    greySquare(square);
    moves.forEach(function (move) { greySquare(move.to); });
  }).catch(function () {});
}

function onDragStart(source, piece) {
  if (autoPlayRunning || engineBusy || currentState.gameOver) return false;
  if ((currentState.turn === 'w' && /^b/.test(piece)) || (currentState.turn === 'b' && /^w/.test(piece))) return false;
}

async function onDrop(source, target) {
  removeGreySquares();
  try {
    await protectedMove({ from: source, to: target, promotion: 'q' }, true);
    if (!checkStatus()) await makeBestMove('b');
    await showHint();
  } catch (error) {
    setStatus('Illegal move or engine error: ' + error.message, true);
    return 'snapback';
  }
}

async function verifyProtectedRuntime() {
  var $button = $('#compVsCompBtn');
  $button.prop('disabled', true).text('Loading Engine…');
  try {
    await ensureProtectedRuntime();
    var identity = await callChessEngine({ action: 'identity' });
    $('#runtime-state').text(identity.protected ? 'Protected engine v' + (identity.version || '2') + ' verified' : 'Runtime unavailable');
    $('#runtime-name').text(identity.runtime || 'TurboJS / WASM');
    $button.prop('disabled', false).text('Start Game');
    setStatus('Protected engine verified and ready.', false);
  } catch (error) {
    protectedRuntimeReady = false;
    $('#runtime-state').text('Protection verification failed');
    $button.prop('disabled', false).text('Retry Start');
    setStatus('Protected engine failed to initialize: ' + error.message, true);
  }
}

$('#compVsCompBtn').on('click', function () { void startAutoPlay(); });
$('#resetBtn').on('click', reset);
$('#undoBtn').on('click', function () { stopAutoPlay(); undo(); });
$('#redoBtn').on('click', function () { stopAutoPlay(); redo(); });
$('#showHint').on('change', showHint);
$('#analyzeBtn').on('click', function () { void analyzePosition(); });
$('#engine-preset').on('change', function () { applyEnginePreset(String($(this).val())); });
$('#benchmarkBtn').on('click', function () { void runBenchmark().catch(function (error) { stopBenchmark(false); setStatus('Benchmark failed: ' + error.message, true); }); });
$('#benchmarkCancelBtn').on('click', function () { stopBenchmark(true); setStatus('Benchmark cancelled.', false); });
$('#startBtn').on('click', function () { void loadOpening(null); });
$('#ruyLopezBtn').on('click', function () { void loadOpening('r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1'); });
$('#italianGameBtn').on('click', function () { void loadOpening('r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1'); });
$('#sicilianDefenseBtn').on('click', function () { void loadOpening('rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1'); });
$(window).on('resize', drawEvaluationChart);

updateMoveLog();
updateTurn();
drawEvaluationChart();
void verifyProtectedRuntime();
