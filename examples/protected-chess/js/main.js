// @venom: browser
'use strict';

var STACK_SIZE = 100;
var board = null;
var $board = $('#myBoard');
var game = new Chess();
var globalSum = 0;
var squareClass = 'square-55d63';
var timer = null;
var engineBusy = false;
var autoPlayGeneration = 0;
var autoPlayRunning = false;
var undoStack = [];
var evaluationHistory = [0];
var protectedRuntimeReady = false;

var config = {
  draggable: true,
  position: 'start',
  onDragStart: onDragStart,
  onDrop: onDrop,
  onMouseoutSquare: removeGreySquares,
  onMouseoverSquare: onMouseoverSquare,
  onSnapEnd: function () { board.position(game.fen()); }
};
board = Chessboard('myBoard', config);


function getVenomApi() {
  var api = globalThis.venom;
  if (!api || typeof api.ready !== 'function' || !api.exports) {
    throw new Error('Venom protected bridge is unavailable');
  }
  return api;
}

async function callChessEngine(request, options) {
  var api = getVenomApi();
  await api.ready();
  if (typeof api.exports.runChessEngine !== 'function') {
    throw new Error('Protected export runChessEngine is unavailable');
  }
  return api.exports.runChessEngine(request, options || { timeout: 30000 });
}

async function ensureProtectedRuntime() {
  if (protectedRuntimeReady) return getVenomApi();
  var api = getVenomApi();
  await api.ready();
  if (typeof api.exports.runChessEngine !== 'function') {
    throw new Error('Protected export runChessEngine is unavailable');
  }
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
  $('#turn-badge').text(game.turn() === 'w' ? 'White to move' : 'Black to move');
}

function gameOverMessage(color) {
  if (game.in_checkmate()) return 'Checkmate — ' + color + ' lost.';
  if (game.insufficient_material()) return 'Draw by insufficient material.';
  if (game.in_threefold_repetition()) return 'Draw by threefold repetition.';
  if (game.in_stalemate()) return 'Draw by stalemate.';
  if (game.in_draw()) return 'Draw by the 50-move rule.';
  return null;
}

function checkStatus(color) {
  var terminal = gameOverMessage(color);
  if (terminal) {
    setStatus(terminal, false);
    return true;
  }
  if (game.in_check()) setStatus(color + ' is in check.', false);
  else setStatus(autoPlayRunning ? 'Protected engines are playing.' : 'Protected engine ready.', false);
  updateTurn();
  return false;
}

function clamp(value, min, max) { return Math.max(min, Math.min(max, value)); }

function updateAdvantage() {
  var normalized = clamp(globalSum, -2000, 2000);
  var percent = ((normalized + 2000) / 4000) * 100;
  $('#advantageBar').css('left', percent + '%');
  if (globalSum > 0) $('#evaluation-text').text('Black +' + Math.abs(globalSum));
  else if (globalSum < 0) $('#evaluation-text').text('White +' + Math.abs(globalSum));
  else $('#evaluation-text').text('Equal position');
  evaluationHistory.push(globalSum);
  if (evaluationHistory.length > 60) evaluationHistory.shift();
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
  ctx.beginPath(); ctx.moveTo(0, height / 2); ctx.lineTo(width, height / 2); ctx.stroke();
  if (evaluationHistory.length < 2) return;
  ctx.strokeStyle = '#22d3ee';
  ctx.lineWidth = 2;
  ctx.beginPath();
  evaluationHistory.forEach(function (score, index) {
    var x = (index / (evaluationHistory.length - 1)) * width;
    var y = height / 2 - (clamp(score, -1200, 1200) / 1200) * (height * .42);
    if (index === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();
}

function updateTelemetry(result) {
  $('#metric-depth').text(result.depth || '—');
  $('#position-count').text(Number(result.positions || 0).toLocaleString());
  $('#time').text(((result.elapsedMs || 0) / 1000).toFixed(3));
  $('#positions-per-s').text(Number(result.positionsPerSecond || 0).toLocaleString());
}

function updateMoveLog() {
  var history = game.history();
  $('#move-count').text(history.length + (history.length === 1 ? ' move' : ' moves'));
  var $log = $('#move-log').empty();
  if (!history.length) {
    $log.append('<div class="empty-state">Moves will appear here.</div>');
    return;
  }
  for (var i = 0; i < history.length; i += 2) {
    var number = Math.floor(i / 2) + 1;
    var row = $('<div class="move-row"></div>');
    row.append($('<span></span>').text(number + '.'));
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

async function requestBestMove(color) {
  var selector = color === 'b' ? '#search-depth' : '#search-depth-white';
  var depth = parseInt($(selector).val(), 10);
  var result = await callChessEngine({
    action: 'search',
    fen: String(game.fen()),
    depth: Number.isFinite(depth) ? depth : 3,
    color: color === 'w' ? 'w' : 'b',
    sum: Number.isFinite(globalSum) ? (color === 'b' ? globalSum : -globalSum) : 0
  });
  updateTelemetry(result);
  return result;
}

async function makeBestMove(color) {
  if (engineBusy) return false;
  setEngineBusy(true, 'Protected search…');
  try {
    var result = await requestBestMove(color);
    if (!result.move) return false;
    globalSum = color === 'b' ? result.value : -result.value;
    var applied = game.move(result.move);
    if (!applied) throw new Error('protected engine returned an illegal move');
    board.position(game.fen());
    highlightMove(result.move, color === 'b' ? 'black' : 'white');
    updateAdvantage();
    updateMoveLog();
    checkStatus(color === 'b' ? 'black' : 'white');
    return true;
  } finally {
    setEngineBusy(false);
  }
}

function stopAutoPlay() {
  autoPlayGeneration += 1;
  autoPlayRunning = false;
  if (timer !== null) { window.clearTimeout(timer); timer = null; }
  $('#compVsCompBtn').prop('disabled', false).text('Start Game');
  $('#match-title').text('Human vs. Protected AI');
}

function scheduleAutoPlay(color, generation) {
  if (!autoPlayRunning || generation !== autoPlayGeneration) return;
  if (checkStatus(color === 'w' ? 'white' : 'black')) { stopAutoPlay(); return; }
  var delay = parseInt($('#move-delay').val(), 10) || 250;
  timer = window.setTimeout(async function () {
    timer = null;
    if (!autoPlayRunning || generation !== autoPlayGeneration) return;
    try {
      var moved = await makeBestMove(color);
      if (!moved) { stopAutoPlay(); return; }
      scheduleAutoPlay(color === 'w' ? 'b' : 'w', generation);
    } catch (error) {
      console.error('[chess-ai] automatic game failed', error);
      setStatus('Engine error: ' + String(error && error.message ? error.message : error), true);
      stopAutoPlay();
    }
  }, delay);
}

async function startAutoPlay() {
  var $button = $('#compVsCompBtn');
  $button.prop('disabled', true).text('Starting…');
  setStatus('Starting protected engine…', false);
  try {
    await ensureProtectedRuntime();
    reset();
    autoPlayRunning = true;
    var generation = autoPlayGeneration;
    $button.prop('disabled', true).text('Game Running…');
    $('#match-title').text('Protected AI vs. Protected AI');
    setStatus('Protected engines are playing.', false);
    scheduleAutoPlay('w', generation);
  } catch (error) {
    console.error('[chess-ai] failed to start protected game', error);
    protectedRuntimeReady = false;
    $button.prop('disabled', false).text('Start Game');
    setStatus('Unable to start protected engine: ' + String(error && error.message ? error.message : error), true);
  }
}

function reset() {
  stopAutoPlay();
  game.reset();
  globalSum = 0;
  undoStack = [];
  evaluationHistory = [0];
  $board.find('.' + squareClass).removeClass('highlight-white highlight-black highlight-hint');
  board.position(game.fen());
  updateAdvantage();
  updateMoveLog();
  updateTurn();
  setStatus('Protected engine ready.', false);
}

function loadOpening(fen) {
  reset();
  if (fen) game.load(fen);
  board.position(game.fen());
  updateMoveLog();
  updateTurn();
}

function undo() {
  var move = game.undo();
  if (!move) return;
  undoStack.push(move);
  if (undoStack.length > STACK_SIZE) undoStack.shift();
  board.position(game.fen());
  updateMoveLog();
  updateTurn();
}

function redo() {
  var move = undoStack.pop();
  if (!move) return;
  game.move(move);
  board.position(game.fen());
  updateMoveLog();
  updateTurn();
}

async function showHint() {
  $board.find('.' + squareClass).removeClass('highlight-hint');
  if (!document.getElementById('showHint').checked || game.game_over()) return;
  try {
    var result = await requestBestMove('w');
    if (result.move) {
      $board.find('.square-' + result.move.from).addClass('highlight-hint');
      $board.find('.square-' + result.move.to).addClass('highlight-hint');
    }
  } catch (error) { setStatus('Hint failed: ' + error.message, true); }
}

function removeGreySquares() { $('#myBoard .square-55d63').css('background', ''); }
function greySquare(square) {
  var $square = $('#myBoard .square-' + square);
  $square.css('background', $square.hasClass('black-3c85d') ? '#4a5568' : '#94a3b8');
}
function onMouseoverSquare(square) {
  var moves = game.moves({ square: square, verbose: true });
  if (!moves.length) return;
  greySquare(square);
  moves.forEach(function (move) { greySquare(move.to); });
}
function onDragStart(source, piece) {
  if (autoPlayRunning || engineBusy || game.game_over()) return false;
  if ((game.turn() === 'w' && /^b/.test(piece)) || (game.turn() === 'b' && /^w/.test(piece))) return false;
}

async function onDrop(source, target) {
  undoStack = [];
  removeGreySquares();
  var fenBeforeMove = game.fen();
  var move = game.move({ from: source, to: target, promotion: 'q' });
  if (move === null) return 'snapback';
  highlightMove(move, 'white');
  updateMoveLog();
  updateTurn();
  try {
    var evaluation = await callChessEngine({
      action: 'evaluate', fen: String(fenBeforeMove),
      move: { from: String(move.from), to: String(move.to), promotion: String(move.promotion || 'q') },
      sum: Number.isFinite(globalSum) ? globalSum : 0, color: 'b'
    });
    globalSum = evaluation.value;
    updateAdvantage();
    if (!checkStatus('black')) await makeBestMove('b');
    await showHint();
  } catch (error) {
    console.error('[chess-ai] evaluation failed', error);
    setStatus('Engine error: ' + error.message, true);
  }
}

async function verifyProtectedRuntime() {
  var $button = $('#compVsCompBtn');
  $button.prop('disabled', true).text('Loading Engine…');
  try {
    await ensureProtectedRuntime();
    var identity = await callChessEngine({ action: 'identity' });
    $('#runtime-state').text(identity.protected ? 'Protected engine verified' : 'Runtime unavailable');
    $('#runtime-name').text(identity.runtime || 'QuickJS / WASM');
    $button.prop('disabled', false).text('Start Game');
    setStatus('Protected engine verified and ready.', false);
  } catch (error) {
    protectedRuntimeReady = false;
    $('#runtime-state').text('Protection verification failed');
    $button.prop('disabled', false).text('Retry Start');
    setStatus('Protected engine failed to initialize: ' + String(error && error.message ? error.message : error), true);
  }
}

$('#compVsCompBtn').on('click', function () { void startAutoPlay(); });
$('#resetBtn').on('click', reset);
$('#undoBtn').on('click', function () { stopAutoPlay(); undo(); });
$('#redoBtn').on('click', function () { stopAutoPlay(); redo(); });
$('#showHint').on('change', showHint);
$('#analyzeBtn').on('click', async function () {
  try { await makeBestMove(game.turn()); } catch (error) { setStatus('Analysis failed: ' + error.message, true); }
});
$('#startBtn').on('click', function () { loadOpening(null); });
$('#ruyLopezBtn').on('click', function () { loadOpening('r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1'); });
$('#italianGameBtn').on('click', function () { loadOpening('r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 1'); });
$('#sicilianDefenseBtn').on('click', function () { loadOpening('rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1'); });
$(window).on('resize', drawEvaluationChart);

updateMoveLog();
updateTurn();
drawEvaluationChart();
void verifyProtectedRuntime();
