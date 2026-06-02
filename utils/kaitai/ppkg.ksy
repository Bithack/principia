meta:
  id: ppkg
  file-extension: ppkg
  endian: le
seq:
  - id: version
    type: u1
  - id: community_id
    type: u4
    if: version >= 2
  - id: name
    type: str
    size: 0xFF
    encoding: UTF-8
  - id: levels_unlocked
    type: u1
    doc: The amount of incomplete levels that are available at a time. Default is 2.
    if: version >= 3
  - id: first_is_menu
    type: u1
    doc: When 1, Principia automatically plays the first level in the package. On level completion, you go back to this first level. Can be used for a custom level selection level.
    if: version >= 1
  - id: return_on_finish
    type: u1
    doc: Return to level select on level finish
    if: version >= 1
  - id: levels
    type: u1
  - id: level_id
    type: u4
    repeat: expr
    repeat-expr: levels
