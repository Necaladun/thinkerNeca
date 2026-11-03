# Ironstone minerals and native map labels

## Behaviour

- Every `MINOR_IRONSTONE` tile grants **+1 mineral** through Thinker's
  `mod_mine_yield` implementation.
- The bonus uses the same **landmark mineral** calculation path as Garland
  Crater, volcanoes, Fossil Field Ridge and canyon bonuses. It applies to
  base-centre mineral production as well as worked tiles.
- This is **not a special mineral resource**: early tile output caps still
  apply where applicable, and mining improvements interact with landmark
  bonuses under Thinker's existing formula.
- The independent `item_yield` estimator counts +1 mineral for Ironstone
  so AI evaluation is aligned with the new feature.
- Existing jungle, uranium and major landmarks have not been changed.

## Minor-terrain identity and display

Ironstone is a **minor terrain feature**, not a native named landmark.
Its identity is the independent `MINOR_IRONSTONE` tile ID in `THMIN001`.
Generation does **not** register an `Ironstone` name with `new_landmark()`;
the native map's landmark-name table and its 64-entry limit are reserved for
actual named landmarks.

Jungle and Uranium are also treated as *minor* terrain conceptually. They
currently use the original `LM_JUNGLE` and `LM_URANIUM` bits for their
working engine effects, including native tile descriptions. This patch leaves
both unchanged.

**Tile-inspector limitation:** the native status renderer does not yet show
`Ironstone` as a terrain descriptor. The previous experimental wrapper of
`StatusWin::draw_status` caused crashes when selecting Ironstone tiles and
has been removed. The correct follow-up is a verified, narrow status-text
integration that reads the independent minor-terrain ID without spoofing
`LM_*` flags or temporarily rewriting the global `#NATURAL` table.
Do not reintroduce unverified binary call-site scanning for this purpose.

The only Ironstone-specific UI at present is the map sprite; the mineral
yield works independently of tile-status text.

## In-game tests

1. Pull `master`, rebuild **Develop**, and install the new `thinker.dll`.
2. Start a fresh world with Ironstone. Click and hover Ironstone tiles and
   confirm that the game no longer crashes.
3. Confirm that Ironstone does not appear as a named map landmark. It should
   remain visible via its custom sprite.
4. Confirm Jungle and Uranium native status descriptions, resources, and map
   graphics behave as before.
5. Compare a flat non-Ironstone tile with a matching unimproved Ironstone
   tile: expect +1 mineral, unless a normal mineral cap intervenes.
6. Found a base on an Ironstone tile and compare the base-centre mineral
   output to an equivalent ordinary base site.
7. Save/reload a fresh game and verify Ironstone graphics and yield persist
   without any new native landmark-name entries.

Ironstone is *not yet* expected to appear in the tile-inspector terrain
description; that is a separate unresolved UI integration.
