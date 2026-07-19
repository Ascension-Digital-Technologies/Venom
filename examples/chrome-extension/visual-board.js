(() => {
  'use strict';
  if (globalThis.VelocityVisualBoardV045) return;

  const FILES = 'abcdefgh';
  const START_PLACEMENT = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR';
  const UNICODE_PIECES = Object.freeze({
    '♙': 'P', '♘': 'N', '♗': 'B', '♖': 'R', '♕': 'Q', '♔': 'K',
    '♟': 'p', '♞': 'n', '♝': 'b', '♜': 'r', '♛': 'q', '♚': 'k'
  });
  const TYPE_LETTER = Object.freeze({ pawn: 'p', knight: 'n', bishop: 'b', rook: 'r', queen: 'q', king: 'k' });
  const PIECE_NAMES = Object.keys(TYPE_LETTER);
  const ROOT_SELECTORS = [
    '#myBoard',
    '[data-velocity-chess-board]',
    '[data-chess-board]',
    '[data-board-type="chess"]',
    'chess-board',
    'wc-chess-board',
    'cg-board',
    '.board-b72b1',
    '.chessboard',
    '.chess-board',
    '[class*="chessboard"]',
    '[class*="chess-board"]',
    '[role="grid"][aria-label*="chess" i]'
  ];

  function colorOf(piece) {
    if (!piece) return '';
    return piece === piece.toUpperCase() ? 'w' : 'b';
  }

  function opposite(color) {
    return color === 'b' ? 'w' : 'b';
  }

  function cleanSignal(value) {
    return String(value || '')
      .replace(/%20/gi, ' ')
      .replace(/[\\/_:.]+/g, ' ')
      .replace(/([a-z])([A-Z])/g, '$1 $2')
      .toLowerCase();
  }

  function pieceFromRaw(value) {
    const direct = String(value || '').trim();
    if (!direct) return null;
    if (UNICODE_PIECES[direct]) return UNICODE_PIECES[direct];
    if (/^[prnbqkPRNBQK]$/.test(direct)) return direct;

    const compact = cleanSignal(direct).replace(/[^a-z0-9]+/g, ' ').trim();
    if (!compact) return null;
    const tokens = compact.split(/\s+/);

    for (const token of tokens) {
      const short = token.match(/^(white|black|w|b)(pawn|knight|bishop|rook|queen|king|p|n|b|r|q|k)$/);
      if (short) {
        const color = short[1] === 'white' || short[1] === 'w' ? 'w' : 'b';
        const rawType = short[2];
        const type = TYPE_LETTER[rawType] || rawType;
        if (/^[pnbrqk]$/.test(type)) return color === 'w' ? type.toUpperCase() : type;
      }
      const reversed = token.match(/^(pawn|knight|bishop|rook|queen|king)(white|black)$/);
      if (reversed) {
        const type = TYPE_LETTER[reversed[1]];
        return reversed[2] === 'white' ? type.toUpperCase() : type;
      }
      if (/^[wb][pnbrqk]$/.test(token)) {
        return token[0] === 'w' ? token[1].toUpperCase() : token[1];
      }
    }

    let color = null;
    if (tokens.includes('white') || tokens.includes('light') || tokens.includes('w')) color = 'w';
    if (tokens.includes('black') || tokens.includes('dark') || (tokens.includes('b') && !tokens.includes('w'))) color = 'b';
    if (!color) {
      if (/(?:^|\W)white(?:\W|$)/.test(compact)) color = 'w';
      else if (/(?:^|\W)black(?:\W|$)/.test(compact)) color = 'b';
    }

    let type = null;
    for (const token of tokens) {
      if (/^[pnbrqk]$/.test(token) && token !== color) { type = token; break; }
      if (color === 'b' && token === 'b' && tokens.filter((entry) => entry === 'b').length >= 2) { type = 'b'; break; }
    }
    for (const name of PIECE_NAMES) {
      if (type) break;
      if (tokens.includes(name) || new RegExp(`(?:^|\\W)${name}(?:\\W|$)`).test(compact)) {
        type = TYPE_LETTER[name];
        break;
      }
    }
    if (color && type) return color === 'w' ? type.toUpperCase() : type;
    return null;
  }

  function elementSignals(element) {
    if (!element) return [];
    const signals = [];
    const attrs = [
      'data-piece', 'data-chess-piece', 'data-role', 'data-type', 'data-color',
      'data-side', 'aria-label', 'alt', 'title', 'src', 'href'
    ];
    for (const name of attrs) {
      const value = element.getAttribute?.(name);
      if (value) signals.push(value);
    }
    const color = element.getAttribute?.('data-color') || element.getAttribute?.('data-side');
    const type = element.getAttribute?.('data-type') || element.getAttribute?.('data-role');
    if (color && type) signals.push(`${color} ${type}`);
    if (typeof element.className === 'string') signals.push(element.className);
    else if (element.className?.baseVal) signals.push(element.className.baseVal);
    if (element.style?.backgroundImage) signals.push(element.style.backgroundImage);
    const text = String(element.textContent || '').trim();
    if (text.length <= 3) signals.push(text);
    return signals;
  }

  function pieceFromElement(element) {
    let best = null;
    let score = -1;
    for (const signal of elementSignals(element)) {
      const piece = pieceFromRaw(signal);
      if (!piece) continue;
      let candidateScore = 1;
      if (element.getAttribute?.('data-piece')) candidateScore += 10;
      if (element.getAttribute?.('data-chess-piece')) candidateScore += 9;
      if (element.matches?.('.piece, piece, [class*="piece-"]')) candidateScore += 5;
      if (element.tagName === 'IMG' || element.getAttribute?.('role') === 'img') candidateScore += 3;
      if (UNICODE_PIECES[String(element.textContent || '').trim()]) candidateScore += 4;
      if (candidateScore > score) {
        best = piece;
        score = candidateScore;
      }
    }
    return best ? { piece: best, score } : null;
  }

  function normalizeSquare(value) {
    const text = String(value || '').toLowerCase();
    const algebraic = text.match(/(?:^|[^a-h1-8])([a-h][1-8])(?:$|[^a-h1-8])/);
    if (algebraic) return algebraic[1];
    const exact = text.match(/^([a-h][1-8])$/);
    if (exact) return exact[1];
    const numeric = text.match(/(?:square[-_ ]?|^)([1-8])([1-8])(?:$|[^0-9])/);
    if (numeric) return FILES[Number(numeric[1]) - 1] + numeric[2];
    return null;
  }

  function explicitSquareFromElement(element, root) {
    let current = element;
    for (let depth = 0; current && depth < 5; depth++, current = current.parentElement) {
      const attrs = ['data-square', 'data-cell', 'data-coord', 'square', 'id', 'aria-label'];
      for (const name of attrs) {
        const square = normalizeSquare(current.getAttribute?.(name));
        if (square) return square;
      }
      const classText = typeof current.className === 'string' ? current.className : current.className?.baseVal;
      const square = normalizeSquare(classText);
      if (square) return square;
      if (current === root) break;
    }
    return null;
  }

  function visibleRect(element) {
    const rect = element?.getBoundingClientRect?.();
    if (!rect || rect.width < 3 || rect.height < 3) return null;
    if (rect.bottom <= 0 || rect.right <= 0 || rect.top >= innerHeight || rect.left >= innerWidth) return null;
    const style = getComputedStyle(element);
    if (style.display === 'none' || style.visibility === 'hidden' || Number(style.opacity) === 0) return null;
    return rect;
  }

  function orientationFromRoot(root) {
    const values = [
      root?.getAttribute?.('data-orientation'),
      root?.getAttribute?.('orientation'),
      root?.getAttribute?.('data-perspective'),
      typeof root?.className === 'string' ? root.className : root?.className?.baseVal
    ].join(' ').toLowerCase();
    if (/\b(black|flipped|reverse|rotated)\b/.test(values)) return 'black';
    if (/\b(white|normal)\b/.test(values)) return 'white';

    const a1 = root?.querySelector?.('.square-a1, [data-square="a1"]');
    const h1 = root?.querySelector?.('.square-h1, [data-square="h1"]');
    if (a1 && h1) {
      const ar = a1.getBoundingClientRect();
      const hr = h1.getBoundingClientRect();
      return ar.left < hr.left ? 'white' : 'black';
    }
    return null;
  }

  function pointToSquare(point, rect, orientation) {
    if (!rect || rect.width <= 0 || rect.height <= 0) return null;
    let column = Math.floor(((point.x - rect.left) / rect.width) * 8);
    let row = Math.floor(((point.y - rect.top) / rect.height) * 8);
    column = Math.max(0, Math.min(7, column));
    row = Math.max(0, Math.min(7, row));
    if (orientation === 'black') return FILES[7 - column] + String(row + 1);
    return FILES[column] + String(8 - row);
  }

  function pointForSquare(rect, square, orientation) {
    const file = FILES.indexOf(String(square)[0]);
    const rank = Number(String(square)[1]);
    if (file < 0 || rank < 1 || rank > 8) return null;
    const column = orientation === 'black' ? 7 - file : file;
    const row = orientation === 'black' ? rank - 1 : 8 - rank;
    return {
      x: rect.left + (column + 0.5) * rect.width / 8,
      y: rect.top + (row + 0.5) * rect.height / 8
    };
  }

  function collectPieceCandidates(root) {
    const selectors = [
      '[data-piece]', '[data-chess-piece]', '[data-color][data-type]',
      'piece', '.piece', '[class*="piece-"]', '[class~="wp"]', '[class~="wn"]',
      '[class~="wb"]', '[class~="wr"]', '[class~="wq"]', '[class~="wk"]',
      '[class~="bp"]', '[class~="bn"]', '[class~="bb"]', '[class~="br"]',
      '[class~="bq"]', '[class~="bk"]', 'img[alt]', 'img[src]', '[role="img"]'
    ];
    const found = new Set();
    for (const element of root.querySelectorAll(selectors.join(','))) found.add(element);
    if (found.size < 2 && root.children.length <= 250) {
      for (const element of root.querySelectorAll('*')) {
        if (pieceFromElement(element)) found.add(element);
      }
    }
    return Array.from(found);
  }

  function scoreRoot(root) {
    const rect = visibleRect(root);
    if (!rect) return -Infinity;
    const ratio = rect.width / rect.height;
    if (ratio < 0.65 || ratio > 1.45 || Math.min(rect.width, rect.height) < 160) return -Infinity;
    const explicit = root.querySelectorAll('[data-square], [class*="square-"]').length;
    let pieces = 0;
    let kings = 0;
    for (const element of collectPieceCandidates(root).slice(0, 100)) {
      const identified = pieceFromElement(element);
      if (!identified) continue;
      pieces++;
      if (identified.piece.toLowerCase() === 'k') kings++;
    }
    if (explicit < 8 && (pieces < 2 || kings < 1)) return -Infinity;
    return explicit * 0.4 + pieces * 4 + kings * 12 - Math.abs(1 - ratio) * 20;
  }

  function findBoardRoots(doc = document) {
    const candidates = new Set();
    for (const selector of ROOT_SELECTORS) {
      for (const element of doc.querySelectorAll(selector)) candidates.add(element);
    }
    for (const piece of doc.querySelectorAll('[data-piece], .piece, piece, [class*="piece-"]')) {
      let parent = piece.parentElement;
      for (let depth = 0; parent && depth < 6; depth++, parent = parent.parentElement) {
        const rect = parent.getBoundingClientRect?.();
        if (rect && Math.min(rect.width, rect.height) >= 160 && Math.max(rect.width, rect.height) / Math.min(rect.width, rect.height) < 1.5) {
          candidates.add(parent);
        }
      }
    }
    return Array.from(candidates)
      .map((root) => ({ root, score: scoreRoot(root) }))
      .filter((entry) => Number.isFinite(entry.score))
      .sort((a, b) => b.score - a.score)
      .map((entry) => entry.root);
  }

  function findBoardRoot(doc = document) {
    return findBoardRoots(doc)[0] || null;
  }

  function inferOrientation(root, candidates, rootRect) {
    const explicit = orientationFromRoot(root);
    if (explicit) return explicit;
    let whiteY = 0;
    let blackY = 0;
    let whiteCount = 0;
    let blackCount = 0;
    for (const element of candidates) {
      const info = pieceFromElement(element);
      const rect = visibleRect(element);
      if (!info || !rect) continue;
      const center = rect.top + rect.height / 2;
      if (colorOf(info.piece) === 'w') { whiteY += center; whiteCount++; }
      else { blackY += center; blackCount++; }
    }
    if (whiteCount >= 2 && blackCount >= 2) {
      return whiteY / whiteCount > blackY / blackCount ? 'white' : 'black';
    }
    const bodyText = String(document.body?.className || '').toLowerCase();
    if (/orientation-black|board-flipped/.test(bodyText)) return 'black';
    return 'white';
  }

  function placementFromBoard(board) {
    const ranks = [];
    for (let rank = 8; rank >= 1; rank--) {
      let row = '';
      let empty = 0;
      for (const file of FILES) {
        const piece = board[file + rank] || '';
        if (!piece) empty++;
        else {
          if (empty) row += String(empty);
          empty = 0;
          row += piece;
        }
      }
      if (empty) row += String(empty);
      ranks.push(row);
    }
    return ranks.join('/');
  }

  function boardKey(board) {
    let key = '';
    for (let rank = 8; rank >= 1; rank--) {
      for (const file of FILES) key += board[file + rank] || '.';
    }
    return key;
  }

  const START_BOARD = (() => {
    const board = Object.create(null);
    const ranks = START_PLACEMENT.split('/');
    for (let row = 0; row < 8; row++) {
      let file = 0;
      for (const token of ranks[row]) {
        if (/\d/.test(token)) file += Number(token);
        else {
          board[FILES[file] + String(8 - row)] = token;
          file++;
        }
      }
    }
    return Object.freeze(board);
  })();

  function openingActivity(board) {
    const activity = { w: 0, b: 0 };
    for (let rank = 1; rank <= 8; rank++) {
      for (const file of FILES) {
        const square = file + rank;
        const started = START_BOARD[square] || '';
        const current = board[square] || '';
        if (started && current !== started) activity[colorOf(started)]++;
        if (current && current !== started) activity[colorOf(current)]++;
      }
    }
    return activity;
  }

  function inferInitialTurn(snapshot, hints = {}) {
    if (hints.turn === 'w' || hints.turn === 'b') {
      return { turn: hints.turn, source: hints.turnSource || 'page-turn-hint', confidence: 1 };
    }
    if (snapshot.placement === START_PLACEMENT) {
      return { turn: 'w', source: 'initial-position', confidence: 1 };
    }
    if (hints.lastMover === 'w' || hints.lastMover === 'b') {
      return { turn: opposite(hints.lastMover), source: hints.lastMoveSource || 'last-move-highlight', confidence: 0.95 };
    }

    // For games started during the opening, moved/captured home pieces provide a
    // useful parity signal. The color with more board activity normally made
    // the most recent move, so the other color is next.
    const activity = openingActivity(snapshot.board);
    const totalActivity = activity.w + activity.b;
    const openingLike = snapshot.pieceCount >= 20 && totalActivity <= 24;
    if (openingLike && activity.w !== activity.b) {
      return {
        turn: activity.w > activity.b ? 'b' : 'w',
        source: 'opening-activity-parity',
        confidence: Math.abs(activity.w - activity.b) <= 2 ? 0.86 : 0.72
      };
    }

    // Auto-side detection already tells us which pieces belong to the local
    // player. On a non-initial stable board, falling back to that side avoids
    // deadlocking when the website exposes no turn label at all. The first
    // observed visual move will immediately replace this with exact delta-based
    // tracking.
    if (hints.playerSide === 'w' || hints.playerSide === 'b') {
      return { turn: hints.playerSide, source: 'player-side-bootstrap', confidence: 0.62 };
    }

    // A visual board always has a perspective, even when the page exposes no
    // turn or player metadata. On a restart, the bottom side is the safest
    // recoverable bootstrap. This is deliberately low confidence and is
    // replaced as soon as a page hint or a visual move delta is observed.
    if (snapshot.orientation === 'black') {
      return { turn: 'b', source: 'orientation-bootstrap', confidence: 0.42 };
    }
    return { turn: 'w', source: 'orientation-bootstrap', confidence: 0.42 };
  }

  function makeSnapshot(boardInput, extras = {}) {
    const board = Object.create(null);
    if (boardInput instanceof Map) {
      for (const [square, piece] of boardInput) if (/^[a-h][1-8]$/.test(square) && /^[prnbqkPRNBQK]$/.test(piece)) board[square] = piece;
    } else {
      for (const [square, piece] of Object.entries(boardInput || {})) if (/^[a-h][1-8]$/.test(square) && /^[prnbqkPRNBQK]$/.test(piece)) board[square] = piece;
    }
    const pieces = Object.keys(board).length;
    return {
      board,
      placement: placementFromBoard(board),
      key: boardKey(board),
      pieceCount: pieces,
      orientation: extras.orientation || 'white',
      confidence: Number(extras.confidence) || 1,
      root: extras.root || null,
      rootRect: extras.rootRect || null,
      elements: extras.elements || new Map(),
      explicitSquares: Number(extras.explicitSquares) || 0
    };
  }

  function scanRoot(root) {
    const rootRect = visibleRect(root);
    if (!rootRect) throw new Error('The chessboard is not currently visible');
    const candidates = collectPieceCandidates(root);
    const orientation = inferOrientation(root, candidates, rootRect);
    const choices = new Map();
    let explicitSquares = 0;

    for (const element of candidates) {
      const info = pieceFromElement(element);
      const rect = visibleRect(element);
      if (!info || !rect) continue;
      const center = { x: rect.left + rect.width / 2, y: rect.top + rect.height / 2 };
      if (center.x < rootRect.left - 2 || center.x > rootRect.right + 2 || center.y < rootRect.top - 2 || center.y > rootRect.bottom + 2) continue;

      // A real rendered chess piece occupies a meaningful fraction of one board
      // square. Tiny SVG descendants, labels, captured-piece icons, and full-board
      // transition layers are ignored before they can corrupt king counts.
      const squareArea = (rootRect.width * rootRect.height) / 64;
      const areaRatio = (rect.width * rect.height) / squareArea;
      if (areaRatio < 0.08 || areaRatio > 2.35) continue;

      const explicit = explicitSquareFromElement(element, root);
      const square = explicit || pointToSquare(center, rootRect, orientation);
      if (!square) continue;
      if (explicit) explicitSquares++;
      const centerPoint = pointForSquare(rootRect, square, orientation);
      const distance = centerPoint ? Math.hypot(center.x - centerPoint.x, center.y - centerPoint.y) : 0;
      const maxDistance = Math.max(rootRect.width, rootRect.height) / 8;
      if (!explicit && distance > maxDistance * 0.72) continue;
      const geometry = areaRatio >= 0.18 && areaRatio <= 1.75 ? 5 : 1;
      const score = info.score + geometry + (explicit ? 10 : 0) - Math.min(4, distance / Math.max(1, maxDistance));
      const existing = choices.get(square);
      if (!existing || score > existing.score) choices.set(square, { piece: info.piece, element, score });
    }

    const board = Object.create(null);
    const elements = new Map();
    for (const [square, choice] of choices) {
      board[square] = choice.piece;
      elements.set(square, choice.element);
    }
    const snapshot = makeSnapshot(board, {
      orientation,
      root,
      rootRect,
      elements,
      explicitSquares,
      confidence: Math.min(1, 0.4 + Object.keys(board).length / 38 + Math.min(0.3, explicitSquares / 48))
    });
    const whiteKing = Object.values(board).filter((piece) => piece === 'K').length;
    const blackKing = Object.values(board).filter((piece) => piece === 'k').length;
    if (snapshot.pieceCount < 2 || snapshot.pieceCount > 32 || whiteKing !== 1 || blackKing !== 1) {
      const error = new Error(`Visual board scan is incomplete (${snapshot.pieceCount} pieces, kings ${whiteKing}/${blackKing})`);
      error.scanDetails = { pieceCount: snapshot.pieceCount, whiteKing, blackKing, explicitSquares };
      throw error;
    }
    return snapshot;
  }

  function scan(doc = document) {
    const roots = findBoardRoots(doc);
    if (!roots.length) throw new Error('No visible chessboard was found');
    let bestError = null;
    for (const root of roots.slice(0, 12)) {
      try {
        return scanRoot(root);
      } catch (error) {
        if (!bestError) bestError = error;
        const current = error?.scanDetails;
        const previous = bestError?.scanDetails;
        const currentScore = current ? (current.whiteKing === 1 ? 12 : 0) + (current.blackKing === 1 ? 12 : 0) + Math.min(32, current.pieceCount) + Math.min(16, current.explicitSquares || 0) : -1;
        const previousScore = previous ? (previous.whiteKing === 1 ? 12 : 0) + (previous.blackKing === 1 ? 12 : 0) + Math.min(32, previous.pieceCount) + Math.min(16, previous.explicitSquares || 0) : -1;
        if (currentScore > previousScore) bestError = error;
      }
    }
    throw bestError || new Error('The visual chessboard could not be reconstructed');
  }

  function turnFromText(value) {
    const text = cleanSignal(value);
    if (!text) return null;
    if (/\bblack\s*(to move|turn|playing|active)\b|\bblack'?s turn\b/.test(text)) return 'b';
    if (/\bwhite\s*(to move|turn|playing|active)\b|\bwhite'?s turn\b/.test(text)) return 'w';
    if (/^(b|black)$/.test(text.trim())) return 'b';
    if (/^(w|white)$/.test(text.trim())) return 'w';
    return null;
  }

  function playerSideFromText(value, allowBare = false) {
    const text = cleanSignal(value).replace(/[’']/g, ' ').replace(/\s+/g, ' ').trim();
    if (!text) return null;
    if (/\b(you are|you play|you re playing|playing as|your colou?r is|your side is|my colou?r is|my side is|assigned)\s+(white|w)\b/.test(text)) return 'w';
    if (/\b(you are|you play|you re playing|playing as|your colou?r is|your side is|my colou?r is|my side is|assigned)\s+(black|b)\b/.test(text)) return 'b';
    if (/\b(white|w)\s+(player|side)\s*\(?(you|me|self)\)?\b/.test(text)) return 'w';
    if (/\b(black|b)\s+(player|side)\s*\(?(you|me|self)\)?\b/.test(text)) return 'b';
    if (allowBare && /^(w|white|light)$/.test(text)) return 'w';
    if (allowBare && /^(b|black|dark)$/.test(text)) return 'b';
    return null;
  }

  function sideFromElement(element, allowBare = true) {
    if (!element) return null;
    const attributes = [
      'data-player-color', 'data-player-side', 'data-user-color', 'data-user-side',
      'data-my-color', 'data-my-side', 'data-perspective', 'data-orientation',
      'data-color', 'data-side', 'aria-label', 'title'
    ];
    for (const name of attributes) {
      const side = playerSideFromText(element.getAttribute?.(name), allowBare);
      if (side) return side;
    }
    const className = typeof element.className === 'string' ? element.className : element.className?.baseVal;
    const context = `${className || ''} ${element.textContent || ''}`;
    return playerSideFromText(context, false);
  }

  function readPlayerSide(doc = document, snapshot = null) {
    const explicitSelectors = [
      '[data-player-color]', '[data-player-side]', '[data-user-color]', '[data-user-side]',
      '[data-my-color]', '[data-my-side]', '[data-perspective]',
      '[data-is-you="true"]', '[data-you="true"]', '.player.you', '.player.self',
      '.player.me', '.player--you', '.player--self', '.bottom-player', '.player-bottom',
      '.current-user', '.local-player'
    ];
    for (const selector of explicitSelectors) {
      for (const element of doc.querySelectorAll?.(selector) || []) {
        const side = sideFromElement(element, true);
        if (side) return { side, source: selector, confidence: 1 };
      }
    }

    const textSelectors = ['.player-label', '.player-name', '.game-status', '.status', '[aria-label]'];
    for (const selector of textSelectors) {
      for (const element of doc.querySelectorAll?.(selector) || []) {
        const values = [element.getAttribute?.('aria-label'), element.textContent];
        for (const value of values) {
          const side = playerSideFromText(value, false);
          if (side) return { side, source: `${selector}-text`, confidence: 0.98 };
        }
      }
    }

    const root = snapshot?.root;
    if (root) {
      const side = sideFromElement(root, true);
      if (side) return { side, source: 'board-attributes', confidence: 0.9 };
    }

    if (snapshot?.elements) {
      const interactive = { w: 0, b: 0 };
      for (const [square, element] of snapshot.elements) {
        const piece = snapshot.board?.[square];
        if (!piece) continue;
        const enabled = element.draggable === true
          || element.getAttribute?.('draggable') === 'true'
          || element.getAttribute?.('data-draggable') === 'true'
          || element.getAttribute?.('aria-grabbed') === 'false';
        if (enabled) interactive[colorOf(piece)]++;
      }
      if (interactive.w >= 2 && interactive.b === 0) return { side: 'w', source: 'interactive-white-pieces', confidence: 0.93 };
      if (interactive.b >= 2 && interactive.w === 0) return { side: 'b', source: 'interactive-black-pieces', confidence: 0.93 };
    }

    if (snapshot?.orientation === 'black') return { side: 'b', source: 'board-orientation', confidence: 0.82 };
    if (snapshot?.orientation === 'white') return { side: 'w', source: 'board-orientation', confidence: 0.82 };
    return { side: null, source: '', confidence: 0 };
  }

  function rgbTuple(value) {
    const match = String(value || '').match(/rgba?\(\s*(\d+)\s*[, ]\s*(\d+)\s*[, ]\s*(\d+)/i);
    return match ? [Number(match[1]), Number(match[2]), Number(match[3])] : null;
  }

  function colorDistance(a, b) {
    if (!a || !b) return 0;
    return Math.hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
  }

  function highlightedSquareInfo(doc = document, snapshot = null) {
    const root = snapshot?.root;
    if (!root) return { squares: [], source: '' };
    const knownSelectors = [
      '[data-last-move]', '[data-lastmove]', '[data-move-from]', '[data-move-to]',
      '[class*="last-move"]', '[class*="lastmove"]', '[class*="move-from"]',
      '[class*="move-to"]', '[class*="highlight1"]', '[class*="highlight2"]'
    ];
    const known = new Map();
    for (const selector of knownSelectors) {
      for (const element of root.querySelectorAll?.(selector) || []) {
        const rect = visibleRect(element);
        const square = explicitSquareFromElement(element, root)
          || (rect ? pointToSquare({ x: rect.left + rect.width / 2, y: rect.top + rect.height / 2 }, snapshot.rootRect, snapshot.orientation) : null);
        if (square) known.set(square, element);
      }
    }
    if (known.size === 2) return { squares: Array.from(known.keys()), source: 'last-move-highlight-class' };

    // Generic fallback for boards that paint the two last-move squares with a
    // third background color but expose no semantic class names.
    const squareElements = new Map();
    for (const element of root.querySelectorAll?.('[data-square], [data-cell], [data-coord], [class*="square-"]') || []) {
      const square = explicitSquareFromElement(element, root);
      if (square && !squareElements.has(square)) squareElements.set(square, element);
    }
    if (squareElements.size < 48) return { squares: [], source: '' };
    const groups = new Map();
    for (const [square, element] of squareElements) {
      const color = getComputedStyle(element).backgroundColor;
      if (!rgbTuple(color)) continue;
      if (!groups.has(color)) groups.set(color, []);
      groups.get(color).push(square);
    }
    const ranked = Array.from(groups.entries()).sort((a, b) => b[1].length - a[1].length);
    const baseColors = ranked.slice(0, 2).map(([color]) => rgbTuple(color)).filter(Boolean);
    const rarePairs = ranked.filter(([, squares]) => squares.length === 2).map(([color, squares]) => ({
      color: rgbTuple(color), squares,
      distance: Math.min(...baseColors.map((base) => colorDistance(rgbTuple(color), base)))
    })).filter((entry) => entry.color && entry.distance >= 28).sort((a, b) => b.distance - a.distance);
    if (rarePairs.length) return { squares: rarePairs[0].squares, source: 'last-move-highlight-color' };
    return { squares: [], source: '' };
  }

  function lastMoverFromHighlights(doc = document, snapshot = null) {
    const highlighted = highlightedSquareInfo(doc, snapshot);
    if (highlighted.squares.length !== 2) return { lastMover: null, lastMoveSource: '' };
    const occupied = highlighted.squares
      .map((square) => ({ square, piece: snapshot?.board?.[square] || '' }))
      .filter((entry) => entry.piece);
    if (occupied.length === 1) {
      return { lastMover: colorOf(occupied[0].piece), lastMoveSource: highlighted.source, lastMoveSquares: highlighted.squares };
    }
    // Some boards retain a translucent source-piece clone during the highlight.
    // Prefer the square explicitly marked as a destination when available.
    for (const square of highlighted.squares) {
      const element = snapshot?.root?.querySelector?.(`.square-${square}, [data-square="${square}"]`);
      const signal = `${element?.className || ''} ${element?.getAttribute?.('data-move-to') || ''}`.toLowerCase();
      if (/highlight2|move-to|destination|last-to/.test(signal) && snapshot?.board?.[square]) {
        return { lastMover: colorOf(snapshot.board[square]), lastMoveSource: highlighted.source, lastMoveSquares: highlighted.squares };
      }
    }
    return { lastMover: null, lastMoveSource: highlighted.source, lastMoveSquares: highlighted.squares };
  }

  function readHints(doc = document, snapshot = null) {
    const lastMove = lastMoverFromHighlights(doc, snapshot);
    const directSelectors = [
      '[data-side-to-move]', '[data-turn]', '[data-active-color]', '[data-current-player]',
      '#turn-badge', '.turn-indicator', '.side-to-move', '[aria-live="polite"]'
    ];
    for (const selector of directSelectors) {
      for (const element of doc.querySelectorAll(selector)) {
        const values = [
          element.getAttribute('data-side-to-move'), element.getAttribute('data-turn'),
          element.getAttribute('data-active-color'), element.getAttribute('data-current-player'),
          element.getAttribute('aria-label'), element.textContent
        ];
        for (const value of values) {
          const turn = turnFromText(value);
          if (turn) return { turn, turnSource: selector, gameOver: readGameOver(doc), ...lastMove };
        }
      }
    }

    const activeSelectors = ['.clock.active', '.clock.is-active', '[data-active="true"]', '.player-turn', '.active-clock'];
    for (const selector of activeSelectors) {
      for (const element of doc.querySelectorAll(selector)) {
        const context = `${element.getAttribute('data-color') || ''} ${element.getAttribute('aria-label') || ''} ${element.className || ''} ${element.parentElement?.textContent || ''}`;
        const turn = turnFromText(context);
        if (turn) return { turn, turnSource: selector, gameOver: readGameOver(doc), ...lastMove };
      }
    }

    if (snapshot?.elements) {
      const draggable = { w: 0, b: 0 };
      for (const [square, element] of snapshot.elements) {
        const piece = snapshot.board[square];
        if (!piece) continue;
        const enabled = element.draggable === true || element.getAttribute?.('draggable') === 'true' || element.getAttribute?.('aria-grabbed') === 'false';
        if (enabled) draggable[colorOf(piece)]++;
      }
      if (draggable.w > 0 && draggable.b === 0) return { turn: 'w', turnSource: 'draggable-pieces', gameOver: readGameOver(doc), ...lastMove };
      if (draggable.b > 0 && draggable.w === 0) return { turn: 'b', turnSource: 'draggable-pieces', gameOver: readGameOver(doc), ...lastMove };
    }
    return { turn: null, turnSource: '', gameOver: readGameOver(doc), ...lastMove };
  }

  function readGameOver(doc = document) {
    const selectors = ['[data-game-over="true"]', '.game-over', '.checkmate', '.stalemate', '.result', '[role="dialog"]'];
    for (const selector of selectors) {
      for (const element of doc.querySelectorAll(selector)) {
        const text = cleanSignal(`${element.getAttribute('data-result') || ''} ${element.getAttribute('aria-label') || ''} ${element.textContent || ''}`);
        if (/checkmate|stalemate|game over|draw|wins|resigned|time out|timeout|1 0|0 1|1 2/.test(text)) return true;
      }
    }
    return false;
  }

  function inferMover(previous, current) {
    const changed = [];
    for (let rank = 1; rank <= 8; rank++) {
      for (const file of FILES) {
        const square = file + rank;
        const before = previous[square] || '';
        const after = current[square] || '';
        if (before !== after) changed.push({ square, before, after });
      }
    }
    const candidates = [];
    for (const color of ['w', 'b']) {
      const removed = changed.filter((entry) => entry.before && colorOf(entry.before) === color && entry.after !== entry.before);
      const added = changed.filter((entry) => entry.after && colorOf(entry.after) === color && entry.after !== entry.before);
      if (removed.length && added.length) candidates.push({ color, removed, added, changed });
    }
    if (candidates.length !== 1) return null;
    const candidate = candidates[0];
    let from = null;
    let to = null;
    for (const removed of candidate.removed) {
      for (const added of candidate.added) {
        const sameType = removed.before.toLowerCase() === added.after.toLowerCase();
        const promotion = removed.before.toLowerCase() === 'p' && added.after.toLowerCase() !== 'k';
        if (sameType || promotion) {
          from = removed.square;
          to = added.square;
          if (sameType) break;
        }
      }
      if (from) break;
    }
    return { ...candidate, from, to };
  }

  function removeRight(rights, flags) {
    return Array.from(rights).filter((flag) => !flags.includes(flag)).join('');
  }

  function initialRights(snapshot) {
    if (snapshot.placement === START_PLACEMENT) return 'KQkq';
    return '';
  }

  function updateRights(rights, previous, current) {
    let next = rights;
    if (previous.e1 === 'K' && current.e1 !== 'K') next = removeRight(next, 'KQ');
    if (previous.h1 === 'R' && current.h1 !== 'R') next = removeRight(next, 'K');
    if (previous.a1 === 'R' && current.a1 !== 'R') next = removeRight(next, 'Q');
    if (previous.e8 === 'k' && current.e8 !== 'k') next = removeRight(next, 'kq');
    if (previous.h8 === 'r' && current.h8 !== 'r') next = removeRight(next, 'k');
    if (previous.a8 === 'r' && current.a8 !== 'r') next = removeRight(next, 'q');
    return next;
  }

  function conservativeRights(snapshot) {
    if (snapshot.placement === START_PLACEMENT) return 'KQkq';
    let rights = '';
    // Rights cannot be proven from a single frame. Preserve only rights that
    // remain structurally possible, and only when explicitly requested by a
    // page hint. Restart recovery otherwise prefers '-' over inventing a castle.
    if (snapshot.board.e1 === 'K' && snapshot.board.h1 === 'R') rights += 'K';
    if (snapshot.board.e1 === 'K' && snapshot.board.a1 === 'R') rights += 'Q';
    if (snapshot.board.e8 === 'k' && snapshot.board.h8 === 'r') rights += 'k';
    if (snapshot.board.e8 === 'k' && snapshot.board.a8 === 'r') rights += 'q';
    return rights;
  }

  function createTracker() {
    let previous = null;
    let turn = null;
    let turnSource = '';
    let turnConfidence = 0;
    let rights = '';
    let enPassant = '-';
    let halfmove = 0;
    let fullmove = 1;
    let revision = 0;

    function buildState(snapshot, hints = {}, source = 'visual-dom') {
      const fen = `${snapshot.placement} ${turn} ${rights || '-'} ${enPassant} ${halfmove} ${fullmove}`;
      return {
        fen,
        turn,
        castling: rights || '-',
        enPassant,
        halfmove,
        fullmove,
        gameOver: Boolean(hints.gameOver),
        source,
        positionKey: `${snapshot.key}:${turn}:${rights}:${enPassant}:${revision}`,
        revision,
        pieceCount: snapshot.pieceCount,
        orientation: snapshot.orientation,
        confidence: snapshot.confidence,
        turnSource,
        turnConfidence
      };
    }

    function resync(snapshot, hints = {}, reason = 'live-board-resync') {
      if (!snapshot?.board || !snapshot.placement) throw new Error('Invalid visual board snapshot');
      const inferred = inferInitialTurn(snapshot, hints);
      turn = inferred.turn;
      turnSource = reason;
      turnConfidence = Math.min(inferred.confidence || 0.5, 0.68);
      rights = snapshot.placement === START_PLACEMENT ? 'KQkq' : '';
      enPassant = '-';
      halfmove = 0;
      fullmove = Math.max(1, Number(hints.fullmove) || 1);
      revision++;
      previous = snapshot;
      return buildState(snapshot, hints, 'visual-dom-resync');
    }

    return {
      update(snapshot, hints = {}) {
        if (!snapshot?.board || !snapshot.placement) throw new Error('Invalid visual board snapshot');
        let transition = null;
        if (!previous) {
          rights = initialRights(snapshot);
          const inferred = inferInitialTurn(snapshot, hints);
          turn = inferred.turn;
          turnSource = inferred.source;
          turnConfidence = inferred.confidence;
          if (!turn) {
            turn = snapshot.orientation === 'black' ? 'b' : 'w';
            turnSource = 'orientation-bootstrap';
            turnConfidence = 0.35;
          }
        } else if (snapshot.key !== previous.key) {
          if (snapshot.placement === START_PLACEMENT && previous.placement !== START_PLACEMENT) {
            rights = 'KQkq';
            enPassant = '-';
            halfmove = 0;
            fullmove = 1;
            turn = hints.turn || 'w';
            turnSource = hints.turn ? (hints.turnSource || 'page-turn-hint') : 'new-game-reset';
            turnConfidence = hints.turn ? 1 : 1;
            revision++;
            previous = snapshot;
            const fen = `${snapshot.placement} ${turn} ${rights} - 0 1`;
            return {
              fen,
              turn,
              castling: rights,
              enPassant: '-',
              halfmove: 0,
              fullmove: 1,
              gameOver: Boolean(hints.gameOver),
              source: 'visual-dom',
              positionKey: `${snapshot.key}:${turn}:${rights}:-:${revision}`,
              revision,
              pieceCount: snapshot.pieceCount,
              orientation: snapshot.orientation,
              confidence: snapshot.confidence,
              turnSource,
              turnConfidence
            };
          }
          transition = inferMover(previous.board, snapshot.board);
          if (!transition) throw new Error('The visual board changed in a way that could not be interpreted as one chess move');
          const mover = transition.color;
          rights = updateRights(rights, previous.board, snapshot.board);
          enPassant = '-';
          if (transition.from && transition.to) {
            const movedPiece = previous.board[transition.from];
            const fromRank = Number(transition.from[1]);
            const toRank = Number(transition.to[1]);
            if (movedPiece?.toLowerCase() === 'p' && transition.from[0] === transition.to[0] && Math.abs(toRank - fromRank) === 2) {
              enPassant = transition.from[0] + String((fromRank + toRank) / 2);
            }
            const capture = Boolean(previous.board[transition.to]) || transition.changed.length >= 3;
            halfmove = movedPiece?.toLowerCase() === 'p' || capture ? 0 : halfmove + 1;
          } else {
            halfmove++;
          }
          if (mover === 'b') fullmove++;
          turn = opposite(mover);
          turnSource = 'visual-move-delta';
          turnConfidence = 1;
          revision++;
        } else if ((hints.turn === 'w' || hints.turn === 'b') && turnSource === 'player-side-bootstrap') {
          // A late page hint may safely refine the low-confidence bootstrap only
          // while the board itself has not changed.
          turn = hints.turn;
          turnSource = hints.turnSource || 'page-turn-hint';
          turnConfidence = 1;
        }
        previous = snapshot;
        return buildState(snapshot, hints);
      },
      resync,
      reset() {
        previous = null;
        turn = null;
        turnSource = '';
        turnConfidence = 0;
        rights = '';
        enPassant = '-';
        halfmove = 0;
        fullmove = 1;
        revision = 0;
      }
    };
  }

  globalThis.VelocityVisualBoardV045 = Object.freeze({
    START_PLACEMENT,
    colorOf,
    pieceFromRaw,
    normalizeSquare,
    placementFromBoard,
    boardKey,
    openingActivity,
    inferInitialTurn,
    makeSnapshot,
    scan,
    readHints,
    highlightedSquareInfo,
    lastMoverFromHighlights,
    readPlayerSide,
    playerSideFromText,
    createTracker,
    pointForSquare,
    pointToSquare,
    explicitSquareFromElement,
    findBoardRoot,
    findBoardRoots,
    scanRoot
  });
})();
