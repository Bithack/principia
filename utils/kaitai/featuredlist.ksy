meta:
  id: featuredlist
  endian: le
seq:
  - id: featured_level_count
    type: u4
  - id: featured_levels
    type: featured_level
    repeat: expr
    repeat-expr: featured_level_count
  - id: contest_stuff
    type: u4
    doc: Intended for telling the count of contests to show in the create dialog, but this is unused and mostly not usable, just keep it at zero.
  - id: gettingstarted_list_count
    type: u4
  - id: gettingstarted_list
    type: gettingstarted
    repeat: expr
    repeat-expr: gettingstarted_list_count
types:
  featured_level:
    seq:
      - id: id
        type: u4
      - id: name_size
        type: u4
      - id: name
        type: str
        size: name_size
        encoding: UTF-8
      - id: author_size
        type: u4
      - id: author
        type: str
        size: author_size
        encoding: UTF-8
      - id: jpegstream_size
        type: u4
      - id: jpegstream
        size: jpegstream_size
  gettingstarted:
    seq:
      - id: name_size
        type: u4
      - id: name
        type: str
        size: name_size
        encoding: UTF-8
      - id: link_size
        type: u4
      - id: link
        type: str
        size: link_size
        encoding: UTF-8

