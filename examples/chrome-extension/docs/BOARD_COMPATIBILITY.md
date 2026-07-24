# Board Compatibility

## Best-supported markup

Boards are easiest to detect when each square has an algebraic identifier:

```html
<div class="square square-e4" data-square="e4"></div>
```

Pieces may identify themselves through attributes, classes, images, labels, or text:

```html
<div class="piece black-knight" data-piece="bN" aria-label="Black knight"></div>
```

## Orientation

Orientation can be resolved from:

- page adapter hints;
- board orientation attributes;
- coordinate labels;
- interactive piece ownership;
- physical placement of known starting pieces.

## Turn detection

Turn detection uses:

1. explicit visual/page hints;
2. untouched initial-position rules;
3. opening-like activity parity;
4. detected local-player side as a bootstrap;
5. exact board deltas after observed moves.

## Canvas boards

A single canvas does not expose squares and pieces as DOM nodes. Add accessible overlays or implement the optional `VelocityChessAdapter` to support these boards.
