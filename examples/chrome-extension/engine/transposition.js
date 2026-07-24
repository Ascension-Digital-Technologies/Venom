export const TT_EXACT = 0;
export const TT_ALPHA = 1;
export const TT_BETA = 2;

export class TranspositionTable {
  constructor(bits = 20) {
    this.size = 1 << bits;
    this.mask = this.size - 1;
    this.keyLo = new Uint32Array(this.size);
    this.keyHi = new Uint32Array(this.size);
    this.move = new Uint32Array(this.size);
    this.score = new Int16Array(this.size);
    this.depth = new Int8Array(this.size);
    this.flag = new Uint8Array(this.size);
    this.age = new Uint8Array(this.size);
    this.generation = 1;
  }

  clear() {
    this.keyLo.fill(0); this.keyHi.fill(0); this.move.fill(0);
    this.score.fill(0); this.depth.fill(0); this.flag.fill(0); this.age.fill(0);
  }

  nextGeneration() { this.generation = (this.generation + 1) & 255 || 1; }

  probe(lo, hi) {
    const index = lo & this.mask;
    return this.keyLo[index] === lo && this.keyHi[index] === hi ? index : -1;
  }

  store(lo, hi, depth, score, flag, move) {
    const index = lo & this.mask;
    const replace = this.keyLo[index] !== lo || this.keyHi[index] !== hi || depth >= this.depth[index] || this.age[index] !== this.generation;
    if (!replace) return;
    this.keyLo[index] = lo;
    this.keyHi[index] = hi;
    this.depth[index] = depth;
    this.score[index] = score;
    this.flag[index] = flag;
    this.move[index] = move >>> 0;
    this.age[index] = this.generation;
  }
}
