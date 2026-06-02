meta:
  id: pobj
  file-extension: pobj
  endian: le
seq:
  - id: version
    type: u1
  - id: type
    type: u1
    enum: level_type
  - id: name_size
    type: u1

  - id: bounds_x1
    type: f4
    if: version >= 13
  - id: bounds_y1
    type: f4
    if: version >= 13
  - id: bounds_x2
    type: f4
    if: version >= 13
  - id: bounds_y2
    type: f4
    if: version >= 13

  - id: name
    type: str
    size: name_size
    encoding: UTF-8

  - id: group_count_pre28
    type: u2
    if: version < 28
  - id: entity_count_pre28
    type: u2
    if: version < 28
  - id: connection_count_pre28
    type: u2
    if: version < 28
  - id: cable_count_pre28
    type: u2
    if: version < 28

  - id: group_count
    type: u4
    if: version >= 28
  - id: entity_count
    type: u4
    if: version >= 28
  - id: connection_count
    type: u4
    if: version >= 28
  - id: cable_count
    type: u4
    if: version >= 28
  - id: chunk_count
    type: u4
    if: version >= 28
  - id: state_size
    type: u4
    if: version >= 28
  - id: gencount
    type: u4
    if: version >= 28

  - id: level_buffer
    size-eos: true
    doc: >
      zlib compressed level buffer data.
      In earlier level versions (TODO: what versions?), the level buffer is uncompressed.

enums:
  level_version:
    0: any
    1: version_beta_1
    2: version_beta_2
    3: version_beta_3
    4: version_beta_4
    5: version_beta_5
    6: version_beta_6
    7: version_beta_7
    8: version_beta_8
    9: version_beta_9
    10: version_beta_10
    11: version_beta_11
    12: version_beta_12
    13: version_beta_13
    14: version_beta_14
    15: version_1_0
    16: version_1_1_6
    17: version_1_1_7
    18: version_1_2
    19: version_1_2_1
    20: version_1_2_2
    21: version_1_2_3
    22: version_1_2_4
    23: version_1_3_0_1
    24: version_1_3_0_2
    25: version_1_3_0_3
    26: version_1_4
    27: version_1_4_0_2
    28: version_1_5
    29: version_1_5_1
  level_type:
    0: puzzle
    1: adventure
    2: custom
    100: partial
