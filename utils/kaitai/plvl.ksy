meta:
  id: plvl
  file-extension: plvl
  endian: le
seq:
  - id: version
    type: u1
  - id: type
    type: u1
    enum: level_type
  - id: community_id
    type: u4
  - id: autosave_id
    type: u4
    if: version >= 28
  - id: revision
    type: u4
  - id: parent_id
    type: u4
    doc: Level ID of parent level (for derivatives)
  - id: name_size
    type: u1
  - id: descr_size
    type: u2
  - id: allow_derivatives
    type: u1
  - id: visibility
    type: u1
    if: version >= 3
  - id: parent_revision
    type: u4
    if: version >= 7
  - id: pause_on_finish
    type: u1
    if: version >= 7
  - id: show_score
    type: u1
    if: version >= 7

  - id: bg
    type: u1
  - id: bg_color
    type: u4
    doc: Assumedly RGBA, needs testing though
    if: version >= 28

  - id: size_x
    type: u2
  - id: size_y
    type: u2
  - id: size_x_2
    type: u2
    if: version >= 12
  - id: size_y_2
    type: u2
    if: version >= 12

  - id: velocity_iterations
    type: u1
  - id: position_iterations
    type: u1
  - id: final_score
    type: u4

  - id: sandbox_cam_x
    type: f4
  - id: sandbox_cam_y
    type: f4
  - id: sandbox_cam_zoom
    type: f4

  - id: gravity_x
    type: f4
    if: version >= 3
  - id: gravity_y
    type: f4
    if: version >= 3

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

  - id: flags
    type: u8
    if: version >= 9
    doc: TODO

  - id: prismatic_tolerance
    type: f4
    if: version >= 26
  - id: pivot_tolerance
    type: f4
    if: version >= 26

  - id: seed
    type: u8
    if: version >= 28
  - id: adventure_id
    type: u4
    if: version >= 28

  - id: linear_damping
    type: f4
    if: version >= 28
  - id: angular_damping
    type: f4
    if: version >= 28
  - id: joint_friction
    type: f4
    if: version >= 28
  - id: body_absorb_time
    type: f4
    if: version >= 28
  - id: respawn_cooldown
    type: f4
    if: version >= 28
  - id: compression_buf_size
    type: u8
    if: version >= 28

  - id: name
    type: str
    size: name_size
    encoding: UTF-8

  - id: level_thumbnail
    size: 128*128
    if: version >= 6
    doc: >
      A 128x128 8-bit grayscale bitmap, which is a screenshot of the level where it last was saved.
      It is used for package thumbnails.

  - id: descr
    type: str
    size: descr_size
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
