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

## Names

A generated Ironstone deposit uses one of the engine's native **named landmark
instances**, with the name `Ironstone`. This is separate from the 16 native
landmark type bits; there are up to 64 native named instances per map.
The customized `find_landmark` lookup prioritizes a named Ironstone deposit
when the queried tile is itself Ironstone.

Older SAV and MP files receive missing names at successful load, without
regenerating terrain or altering `THMIN001` tile identifiers. Existing names
are reused rather than duplicated. If the original 64-name array is full,
the mineral bonus still works but the new name cannot be registered.

The name is passed through the **native named-landmark lookup**. The tile
status window is a separate path: native `#NATURAL` short labels are normally
shown only when a tile has an LM_* flag. The new `mod_StatusWin_draw_status`
adapter supplies a temporary `LM_NEXUS` marker and short label while drawing,
then restores both the tile flags and the native label pointers. Neither
the map/save data nor resource computations are persistently changed.

Future minor terrain types only need a new branch in
`minor_terrain_square_label()` in `src/gui.cpp` to acquire a status label.

**In-game verification pending**: direct call-site discovery, positioning,
major-landmark coexistence, fog handling and save/reload require a rebuilt
Develop DLL and inspection in the game. This code is not validated by the
static source review alone.

## In-game tests

1. Pull `master`, rebuild **Develop**, and install the new `thinker.dll`.
   The diagnostic `minor_terrain.pcx` does not need to change.
2. Load an existing save with Ironstone. Verify the existing Ironstone tiles
   are still present. Hover/select a known Ironstone tile and inspect the
   terrain/landmark text for `Ironstone`. Test the centre and edge of a patch.
3. Compare a flat non-Ironstone tile with a matching unimproved Ironstone
   tile: expect +1 mineral, unless a normal mineral cap intervenes.
4. Found a base on an Ironstone tile and compare the base-centre mineral
   output to an equivalent ordinary base site. Base-centre yields have no
   ordinary tile cap.
5. Try a mine/road and an Ironstone tile with 2+ minerals; verify Thinker's
   normal mine bonus and mineral-cap logic is maintained.
6. Save/reload and ensure the mineral yield and name persist. Loading older
   saves will backfill names in memory and saving will persist them.

This change requires no new custom PCX or extra native landmark type.
