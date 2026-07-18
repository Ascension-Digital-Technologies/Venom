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
  $('#position-count').attr('title', 'Quiescence nodes: ' + Number(result.qnodes || 0).toLocaleString() + ' · TT hits: ' + Number(result.ttHits || 0).toLocaleString());
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
    $('#runtime-name').text(identity.runtime || 'QuickJS / WASM');
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
$('#startBtn').on('click', function () { void loadOpening(null); });
$('#ruyLopezBtn').on('click', function () { void loadOpening('r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1'); });
$('#italianGameBtn').on('click', function () { void loadOpening('r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1'); });
$('#sicilianDefenseBtn').on('click', function () { void loadOpening('rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1'); });
$(window).on('resize', drawEvaluationChart);

updateMoveLog();
updateTurn();
drawEvaluationChart();
void verifyProtectedRuntime();
