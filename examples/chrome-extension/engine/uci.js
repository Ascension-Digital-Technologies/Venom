import readline from 'node:readline';
import { Position } from './position.js';
import { START_FEN } from './constants.js';
import { generateLegal } from './movegen.js';
import { moveToUci } from './move.js';
import { Search } from './search.js';

let position = new Position(START_FEN);
const search = new Search();

function applyUciMove(text) {
  const moves = generateLegal(position);
  for (let i = 0; i < moves.length; i++) {
    if (moveToUci(moves[i]) === text) { position.makeMove(moves[i]); return true; }
  }
  return false;
}

function setPosition(tokens) {
  let i = 1;
  if (tokens[i] === 'startpos') { position.loadFen(START_FEN); i++; }
  else if (tokens[i] === 'fen') {
    i++;
    position.loadFen(tokens.slice(i, i + 6).join(' '));
    i += 6;
  }
  if (tokens[i] === 'moves') for (i++; i < tokens.length; i++) applyUciMove(tokens[i]);
}

const rl = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
rl.on('line', line => {
  const tokens = line.trim().split(/\s+/), cmd = tokens[0];
  if (cmd === 'uci') { console.log('id name Velocity 0.3'); console.log('id author Mr V'); console.log('option name Hash type spin default 64 min 1 max 1024'); console.log('uciok'); }
  else if (cmd === 'isready') console.log('readyok');
  else if (cmd === 'ucinewgame') { position.loadFen(START_FEN); search.tt.clear(); }
  else if (cmd === 'position') setPosition(tokens);
  else if (cmd === 'go') {
    let depth = 6, movetime = 0;
    for (let i = 1; i < tokens.length; i++) {
      if (tokens[i] === 'depth') depth = Number(tokens[++i]) | 0;
      else if (tokens[i] === 'movetime') movetime = Number(tokens[++i]) | 0;
    }
    const result = search.iterative(position, depth, movetime);
    console.log(`bestmove ${result.move ? moveToUci(result.move) : '0000'}`);
  } else if (cmd === 'stop') search.stop = true;
  else if (cmd === 'quit') process.exit(0);
});
