const KNIGHT_DF = new Int8Array([-2,-1,1,2,2,1,-1,-2]);
const KNIGHT_DR = new Int8Array([1,2,2,1,-1,-2,-2,-1]);
const KING_DF = new Int8Array([-1,0,1,-1,1,-1,0,1]);
const KING_DR = new Int8Array([-1,-1,-1,0,0,1,1,1]);

export const KNIGHT_TARGETS = new Int8Array(64 * 8);
export const KNIGHT_COUNTS = new Uint8Array(64);
export const KING_TARGETS = new Int8Array(64 * 8);
export const KING_COUNTS = new Uint8Array(64);

function initLeapers(targets, counts, dfs, drs) {
  for (let square = 0; square < 64; square++) {
    const file = square & 7, rank = square >>> 3;
    let count = 0;
    for (let i = 0; i < dfs.length; i++) {
      const nf = file + dfs[i], nr = rank + drs[i];
      if ((nf & ~7) !== 0 || (nr & ~7) !== 0) continue;
      targets[square * 8 + count++] = nr * 8 + nf;
    }
    counts[square] = count;
  }
}

initLeapers(KNIGHT_TARGETS, KNIGHT_COUNTS, KNIGHT_DF, KNIGHT_DR);
initLeapers(KING_TARGETS, KING_COUNTS, KING_DF, KING_DR);

export const DIR_DF = new Int8Array([1,-1,1,-1,1,-1,0,0]);
export const DIR_DR = new Int8Array([1,1,-1,-1,0,0,1,-1]);

// Dense ray table: [square][direction][step]. A count array avoids sentinels.
export const RAY_TARGETS = new Int8Array(64 * 8 * 7);
export const RAY_COUNTS = new Uint8Array(64 * 8);
for (let square = 0; square < 64; square++) {
  const file = square & 7, rank = square >>> 3;
  for (let d = 0; d < 8; d++) {
    let f = file + DIR_DF[d], r = rank + DIR_DR[d], n = 0;
    const base = (square * 8 + d) * 7;
    while ((f & ~7) === 0 && (r & ~7) === 0) {
      RAY_TARGETS[base + n++] = r * 8 + f;
      f += DIR_DF[d]; r += DIR_DR[d];
    }
    RAY_COUNTS[square * 8 + d] = n;
  }
}
