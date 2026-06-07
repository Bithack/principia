#pragma once

struct tms_vertex_spec {
    int num_components;
    unsigned stride;
    unsigned buf_offset;
    struct tms_gbuffer *buf;
};

struct tms_vertex_array {
    struct tms_vertex_spec *vspecs;
    int num_vspecs;
};
