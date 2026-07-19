(() => {
  'use strict';
  if (globalThis.__VELOCITY_CHESS_PAGE_BRIDGE_INSTALLED_V045__) return;
  globalThis.__VELOCITY_CHESS_PAGE_BRIDGE_INSTALLED_V045__ = true;

  const REQUEST_EVENT = 'VELOCITY_CHESS_EXTENSION_REQUEST_V045';
  const RESPONSE_EVENT = 'VELOCITY_CHESS_EXTENSION_RESPONSE_V045';
  const visual = globalThis.VelocityVisualBoardV045;
  const humanizer = globalThis.VelocityHumanizer;
  if (!visual) throw new Error('Velocity visual board reader was not loaded');
  if (!humanizer) throw new Error('Velocity humanization module was not loaded');

  const tracker = visual.createTracker();
  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
  let latestSnapshot = null;
  let latestState = null;
  let cursorPoint = null;
  let cursorElement = null;

  function currentChannel() {
    return String(globalThis.__VELOCITY_CHESS_EXTENSION_CHANNEL_V045__ || '');
  }

  function adapterHints() {
    try {
      const adapter = globalThis.VelocityChessAdapter;
      if (!adapter || typeof adapter.getVisualHints !== 'function') return {};
      const hints = adapter.getVisualHints();
      return hints && typeof hints === 'object' ? hints : {};
    } catch (_) {
      return {};
    }
  }

  async function stableSnapshot(timeoutMs = 1400) {
    const deadline = Date.now() + timeoutMs;
    let previous = null;
    let stableCount = 0;
    let lastError = null;
    while (Date.now() < deadline) {
      try {
        const snapshot = visual.scan(document);
        if (previous && snapshot.key === previous.key) stableCount++;
        else stableCount = 1;
        previous = snapshot;
        if (stableCount >= 2) return snapshot;
      } catch (error) {
        lastError = error;
        stableCount = 0;
      }
      await sleep(80);
    }
    if (previous) return previous;
    if (lastError) throw lastError;
    throw new Error('The visual chessboard is still animating');
  }


  function resetPageState() {
    tracker.reset();
    latestSnapshot = null;
    latestState = null;
    cursorPoint = null;
    if (cursorElement?.isConnected) cursorElement.remove();
    cursorElement = null;
    return { reset: true };
  }

  async function readPageState() {
    const snapshot = await stableSnapshot();
    const domHints = visual.readHints(document, snapshot);
    const customHints = adapterHints();
    const customSide = customHints.playerSide === 'w' || customHints.playerSide === 'b'
      ? customHints.playerSide
      : customHints.side === 'w' || customHints.side === 'b' ? customHints.side : null;
    const detectedSide = customSide
      ? { side: customSide, source: 'VelocityChessAdapter.getVisualHints', confidence: 1 }
      : visual.readPlayerSide(document, snapshot);
    latestSnapshot = snapshot;
    const trackerHints = {
      turn: customHints.turn === 'w' || customHints.turn === 'b' ? customHints.turn : domHints.turn,
      turnSource: customHints.turn ? 'VelocityChessAdapter.getVisualHints' : domHints.turnSource,
      playerSide: detectedSide.side,
      playerSideSource: detectedSide.source,
      lastMover: customHints.lastMover === 'w' || customHints.lastMover === 'b' ? customHints.lastMover : domHints.lastMover,
      lastMoveSource: customHints.lastMover ? 'VelocityChessAdapter.getVisualHints' : domHints.lastMoveSource,
      gameOver: Boolean(customHints.gameOver || domHints.gameOver)
    };
    let trackedState;
    try {
      trackedState = tracker.update(snapshot, trackerHints);
    } catch (error) {
      // Complex animations, tab suspension, or a stopped extension can skip one
      // or more visual transitions. Re-seed from the live board rather than
      // permanently failing on an uninterpretable historical delta.
      trackedState = tracker.resync(snapshot, trackerHints, 'automatic-live-board-resync');
      trackedState.resyncReason = error instanceof Error ? error.message : String(error);
    }
    latestState = {
      ...trackedState,
      playerSide: detectedSide.side,
      playerSideSource: detectedSide.source,
      playerSideConfidence: detectedSide.confidence
    };
    return latestState;
  }

  function explicitSquareElement(square) {
    const root = latestSnapshot?.root || visual.findBoardRoot(document) || document;
    const escaped = CSS.escape(square);
    return root.querySelector(`.square-${escaped}, [data-square="${escaped}"], [data-cell="${escaped}"], [data-coord="${escaped}"]`);
  }

  function pointFor(element) {
    const rect = element.getBoundingClientRect();
    return { x: rect.left + rect.width / 2, y: rect.top + rect.height / 2 };
  }

  function squarePoint(square) {
    if (!latestSnapshot) throw new Error('The visual board has not been scanned yet');
    const explicit = explicitSquareElement(square);
    if (explicit) return pointFor(explicit);
    const point = visual.pointForSquare(latestSnapshot.rootRect, square, latestSnapshot.orientation);
    if (!point) throw new Error(`Unable to locate visual square ${square}`);
    return point;
  }

  function squareTarget(square) {
    const explicit = explicitSquareElement(square);
    if (explicit) return explicit;
    const point = squarePoint(square);
    return document.elementFromPoint(point.x, point.y) || latestSnapshot?.root || document.body;
  }

  function sourcePiece(square) {
    if (!latestSnapshot) throw new Error('The visual board has not been scanned yet');
    const element = latestSnapshot.elements.get(square);
    if (element) return element;
    const cell = explicitSquareElement(square);
    return cell?.querySelector('[data-piece], [data-chess-piece], piece, .piece, [class*="piece-"], img') || cell;
  }

  function mouse(type, point, buttons = 0) {
    return new MouseEvent(type, {
      bubbles: true, cancelable: true, composed: true, view: window,
      clientX: point.x, clientY: point.y, screenX: point.x, screenY: point.y,
      button: 0, buttons
    });
  }

  function pointer(type, point, buttons = 0) {
    if (typeof PointerEvent !== 'function') return null;
    return new PointerEvent(type, {
      bubbles: true, cancelable: true, composed: true, view: window,
      pointerId: 8173, pointerType: 'mouse', isPrimary: true,
      clientX: point.x, clientY: point.y, screenX: point.x, screenY: point.y,
      button: 0, buttons
    });
  }

  function emit(element, type, point, buttons) {
    const pointerType = ({ mouseover: 'pointerover', mouseout: 'pointerout', mousedown: 'pointerdown', mousemove: 'pointermove', mouseup: 'pointerup', click: 'pointerup' })[type];
    const pointerEvent = pointerType ? pointer(pointerType, point, buttons) : null;
    if (pointerEvent) element.dispatchEvent(pointerEvent);
    element.dispatchEvent(mouse(type, point, buttons));
  }

  function createTransfer(from) {
    try {
      const transfer = new DataTransfer();
      transfer.effectAllowed = 'move';
      transfer.dropEffect = 'move';
      transfer.setData('text/plain', from);
      return transfer;
    } catch (_) {
      const values = new Map([['text/plain', from]]);
      return {
        effectAllowed: 'move', dropEffect: 'move', files: [], items: [], types: ['text/plain'],
        setData(type, value) { values.set(type, String(value)); },
        getData(type) { return values.get(type) || ''; },
        clearData(type) { if (type) values.delete(type); else values.clear(); }
      };
    }
  }

  function drag(type, point, transfer) {
    try {
      return new DragEvent(type, {
        bubbles: true, cancelable: true, composed: true, view: window,
        clientX: point.x, clientY: point.y, screenX: point.x, screenY: point.y,
        dataTransfer: transfer
      });
    } catch (_) {
      const event = new Event(type, { bubbles: true, cancelable: true, composed: true });
      Object.defineProperties(event, {
        clientX: { value: point.x }, clientY: { value: point.y },
        screenX: { value: point.x }, screenY: { value: point.y },
        dataTransfer: { value: transfer }
      });
      return event;
    }
  }

  function ensureCursor() {
    if (cursorElement?.isConnected) return cursorElement;
    const cursor = document.createElement('div');
    cursor.id = 'velocity-human-cursor-v032';
    Object.assign(cursor.style, {
      position: 'fixed', left: '0', top: '0', width: '15px', height: '20px', zIndex: '2147483647',
      pointerEvents: 'none', opacity: '0', transform: 'translate(-2px, -2px)',
      transition: 'opacity 90ms ease', filter: 'drop-shadow(0 1px 2px rgba(0,0,0,.65))'
    });
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    svg.setAttribute('width', '15');
    svg.setAttribute('height', '20');
    svg.setAttribute('viewBox', '0 0 15 20');
    const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    path.setAttribute('d', 'M1 1L1 16L5.2 12.2L8.1 18.5L11 17.1L8.1 11.2L14 10.7Z');
    path.setAttribute('fill', 'white');
    path.setAttribute('stroke', '#111827');
    path.setAttribute('stroke-width', '1.2');
    path.setAttribute('stroke-linejoin', 'round');
    svg.appendChild(path);
    cursor.appendChild(svg);
    (document.documentElement || document.body).appendChild(cursor);
    cursorElement = cursor;
    return cursor;
  }

  function displayCursor(point, visible) {
    const cursor = ensureCursor();
    cursor.style.left = `${point.x}px`;
    cursor.style.top = `${point.y}px`;
    cursor.style.opacity = visible ? '0.94' : '0';
    cursorPoint = { x: point.x, y: point.y };
  }

  async function moveAlongPath(start, end, behavior, buttons = 0, transfer = null) {
    const durationMs = Math.max(20, Number(behavior.durationMs) || 300);
    const points = humanizer.buildPath(start, end, { ...behavior, durationMs });
    const stepMs = durationMs / Math.max(1, points.length);
    let previousOver = null;
    for (const point of points) {
      const over = document.elementFromPoint(point.x, point.y) || document;
      displayCursor(point, Boolean(behavior.showCursor));
      emit(over, 'mousemove', point, buttons);
      if (transfer) {
        if (over !== previousOver) {
          if (previousOver) previousOver.dispatchEvent(drag('dragleave', point, transfer));
          over.dispatchEvent(drag('dragenter', point, transfer));
          previousOver = over;
        }
        over.dispatchEvent(drag('dragover', point, transfer));
      }
      await sleep(stepMs);
    }
  }

  function initialCursorPoint(target) {
    if (cursorPoint) return cursorPoint;
    const angle = Math.random() * Math.PI * 2;
    const radius = 45 + Math.random() * 95;
    return {
      x: Math.max(4, Math.min(innerWidth - 4, target.x + Math.cos(angle) * radius)),
      y: Math.max(4, Math.min(innerHeight - 4, target.y + Math.sin(angle) * radius))
    };
  }

  async function approachSource(piece, start, behavior) {
    if (!behavior.humanize) return;
    const origin = initialCursorPoint(start);
    displayCursor(origin, Boolean(behavior.showCursor));
    await moveAlongPath(origin, start, {
      ...behavior,
      durationMs: behavior.approachMs,
      bendRatio: Number(behavior.bendRatio) * 0.75,
      pathJitterPx: Number(behavior.pathJitterPx) * 0.8
    });
    emit(piece, 'mouseover', start, 0);
    await sleep(Number(behavior.hoverMs) || 0);
    if (Number(behavior.microCorrections) > 0) {
      const correction = { x: start.x + (Math.random() - 0.5) * 5, y: start.y + (Math.random() - 0.5) * 5 };
      await moveAlongPath(start, correction, { ...behavior, durationMs: 35, bendRatio: 0, pathJitterPx: 0 });
      await moveAlongPath(correction, start, { ...behavior, durationMs: 42, bendRatio: 0, pathJitterPx: 0 });
    }
  }

  async function humanDrag(from, to, behavior = {}) {
    await readPageState();
    const piece = sourcePiece(from);
    if (!piece) throw new Error(`Unable to find the visual piece on ${from}`);
    const start = pointFor(piece);
    const end = squarePoint(to);
    const target = squareTarget(to);
    const transfer = createTransfer(from);

    await approachSource(piece, start, behavior);
    emit(piece, 'mousedown', start, 1);
    piece.dispatchEvent(drag('dragstart', start, transfer));
    await sleep(Math.max(18, Number(behavior.holdMs) || 35));

    let dragEnd = end;
    const overshoot = Number(behavior.overshootPx) || 0;
    if (behavior.humanize && overshoot > 0) {
      const dx = end.x - start.x;
      const dy = end.y - start.y;
      const length = Math.max(1, Math.hypot(dx, dy));
      dragEnd = { x: end.x + dx / length * overshoot, y: end.y + dy / length * overshoot };
    }
    await moveAlongPath(start, dragEnd, behavior, 1, transfer);
    if (dragEnd.x !== end.x || dragEnd.y !== end.y) {
      await moveAlongPath(dragEnd, end, { ...behavior, durationMs: 45 + Math.random() * 45, bendRatio: 0, pathJitterPx: 0 }, 1, transfer);
    }
    await sleep(Math.max(0, Number(behavior.destinationPauseMs) || 0));
    target.dispatchEvent(drag('dragover', end, transfer));
    target.dispatchEvent(drag('drop', end, transfer));
    piece.dispatchEvent(drag('dragend', end, transfer));
    emit(target, 'mouseup', end, 0);
    await sleep(Math.max(25, Number(behavior.releaseMs) || 90));
    if (behavior.showCursor) displayCursor(end, false);
  }

  async function clickMove(from, to, behavior = {}) {
    await readPageState();
    const source = sourcePiece(from) || explicitSquareElement(from) || squareTarget(from);
    const target = explicitSquareElement(to) || squareTarget(to);
    const sourcePoint = squarePoint(from);
    const targetPoint = squarePoint(to);
    await approachSource(source, sourcePoint, behavior);
    emit(source, 'mousedown', sourcePoint, 1);
    await sleep(Math.max(25, Number(behavior.holdMs) || 50));
    emit(source, 'mouseup', sourcePoint, 0);
    source.dispatchEvent(mouse('click', sourcePoint, 0));
    await sleep(Math.max(50, Number(behavior.hoverMs) || 95));
    if (behavior.humanize) await moveAlongPath(sourcePoint, targetPoint, { ...behavior, durationMs: Math.max(120, Number(behavior.durationMs) * 0.75) });
    target.dispatchEvent(mouse('click', targetPoint, 0));
    if (behavior.showCursor) displayCursor(targetPoint, false);
  }

  async function playMove(payload) {
    const uci = String(payload?.move || '').toLowerCase();
    if (!/^[a-h][1-8][a-h][1-8][qrbn]?$/.test(uci)) throw new Error(`Invalid UCI move: ${uci}`);
    const from = uci.slice(0, 2);
    const to = uci.slice(2, 4);
    const promotion = uci[4] || 'q';
    const behavior = payload.behavior || {};

    if (globalThis.VelocityChessAdapter && typeof globalThis.VelocityChessAdapter.playMove === 'function') {
      await globalThis.VelocityChessAdapter.playMove({ from, to, promotion, uci, behavior });
      return { method: 'VelocityChessAdapter' };
    }

    const before = (await readPageState()).positionKey;
    await humanDrag(from, to, behavior);
    await sleep(260);
    let after = '';
    try { after = (await readPageState()).positionKey; } catch (_) {}
    if (after && after !== before) return { method: behavior.humanize ? 'humanized-visual-drag' : 'visual-drag' };

    await clickMove(from, to, behavior);
    await sleep(320);
    try { after = (await readPageState()).positionKey; } catch (_) {}
    if (after && after !== before) return { method: behavior.humanize ? 'humanized-click-click' : 'visual-click-click' };

    if (typeof globalThis.onDrop === 'function') {
      const result = globalThis.onDrop(from, to);
      if (result && typeof result.then === 'function') await result;
      return { method: 'page-drop-handler' };
    }
    throw new Error('The visual chessboard did not accept the simulated move');
  }

  window.addEventListener(REQUEST_EVENT, async (event) => {
    let request;
    try {
      request = JSON.parse(String(event.detail || ''));
      if (!request || request.channel !== currentChannel()) return;
      let result;
      if (request.type === 'RESET_STATE') result = resetPageState();
      else if (request.type === 'GET_STATE') result = await readPageState();
      else if (request.type === 'PLAY_MOVE') result = await playMove(request.payload || {});
      else throw new Error(`Unknown bridge request: ${request.type}`);
      window.dispatchEvent(new CustomEvent(RESPONSE_EVENT, {
        detail: JSON.stringify({ channel: request.channel, id: request.id, ok: true, result })
      }));
    } catch (error) {
      if (!request?.channel || request.channel !== currentChannel()) return;
      window.dispatchEvent(new CustomEvent(RESPONSE_EVENT, {
        detail: JSON.stringify({ channel: request.channel, id: request.id, ok: false, error: error instanceof Error ? error.message : String(error) })
      }));
    }
  });
})();
