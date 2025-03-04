/**
 * C++ Bindings for TMS
 **/

#pragma once

extern "C" {
    #include <tms/core/glob.h>
    #include <tms/math/glob.h>
}

namespace tms
{

    extern "C" {
        extern tms_screen_spec _oopassthrough;
        void _oopassthrough_entity_update(struct tms_entity *e);
    }


    namespace math
    {
        class vec2 : public tvec2
        {
            float operator * (vec2 b) {
                return this->x * b.x + this->y * b.y;
            }

            vec2 operator * (float b) {
                vec2 t;
                t.x = this->x * b;
                t.y = this->y * b;
                return t;
            }

            vec2 operator + (vec2 b) {
                vec2 t;
                t.x = this->x + b.x;
                t.y = this->y + b.y;
                return t;
            }

            vec2 operator - (vec2 b) {
                vec2 t;
                t.x = this->x - b.x;
                t.y = this->y - b.y;
                return t;
            }
        };

        class vec3 : public tvec3
        {
            vec3 operator * (float b) {
                vec3 t;
                t.x = this->x * b;
                t.y = this->y * b;
                t.z = this->z * b;
                return t;
            }

            vec3 operator + (vec3 b) {
                vec3 t;
                t.x = this->x + b.x;
                t.y = this->y + b.y;
                t.z = this->z + b.z;
                return t;
            }

            vec3 operator - (vec3 b) {
                vec3 t;
                t.x = this->x - b.x;
                t.y = this->y - b.y;
                t.z = this->z - b.z;
                return t;
            }
        };
    };

    typedef int status;
    typedef struct tms_event event;

    class screen;
    class entity;

    class texture : public tms_texture
    {
      public:
          texture()
          {
              tms_texture_init(this);
          }

          ~texture()
          {
              this->free_buffer();

              if (this->is_uploaded) {
                  glDeleteTextures(1, &this->gl_texture);
              }
          }

          inline int load(const char *filename)
          {
              return tms_texture_load(this, filename);
          }

          inline int load_mem2(const char *buf, size_t size, int freesrc)
          {
              return tms_texture_load_mem2(this, buf, size, freesrc);
          }

          inline int upload()
          {
              return tms_texture_upload(this);
          }

          inline int download()
          {
              return tms_texture_download(this);
          }

          inline int bind()
          {
              return tms_texture_bind(this);
          }

          inline int free_buffer()
          {
              return tms_texture_free_buffer(this);
          }

          inline int flip_x()
          {
              return tms_texture_flip_x(this);
          }

          inline int flip_y()
          {
              return tms_texture_flip_y(this);
          }

          inline int add_alpha(float a)
          {
              return tms_texture_add_alpha(this, a);
          }

          inline void render()
          {
              tms_texture_render(this);
          }

          inline unsigned char *alloc_buffer(int width, int height, int num_channels)
          {
              return tms_texture_alloc_buffer(this, width, height, num_channels);
          }

          inline int clear_buffer(unsigned char clear_value)
          {
              return tms_texture_clear_buffer(this, clear_value);
          }

          inline int get_width()
          {
              return tms_texture_get_width(this);
          }

          inline int get_height()
          {
              return tms_texture_get_height(this);
          }

          inline int get_num_channels()
          {
              return tms_texture_get_num_channels(this);
          }

          inline unsigned char *get_buffer()
          {
              return tms_texture_get_buffer(this);
          }
    };

    class ddraw : public tms_ddraw
    {
      public:
        ddraw()
        {
            tms_ddraw_init(this);
        }

        inline int square_textured(float x, float y, float width, float height, tms::texture *tex)
        {
            return tms_ddraw_square_textured(this, x, y, width, height, tex);
        }

        inline int square(float x, float y, float width, float height)
        {
            return tms_ddraw_square(this, x, y, width, height);
        }

        inline int circle(float x, float y, float width, float height)
        {
            return tms_ddraw_circle(this, x, y, width, height);
        }

        inline int line(float x1, float y1, float x2, float y2)
        {
            return tms_ddraw_line(this, x1, y1, x2, y2);
        }

        inline int line3d(float x1, float y1, float z1, float x2, float y2, float z2)
        {
            return tms_ddraw_line3d(this, x1, y1, z1, x2, y2, z2);
        }

        inline void set_matrices(float *mv, float *p)
        {
            tms_ddraw_set_matrices(this, mv, p);
        }

        inline void set_color(float r, float g, float b, float a)
        {
            tms_ddraw_set_color(this, r, g, b, a);
        }
    };

    class camera : public tms_camera
    {
      public:
        camera()
        {
            tms_camera_init(this);
        }

        inline float *get_combined_matrix()
        {
            return this->combined;
        }

        inline float *get_projection_matrix()
        {
            return this->projection;
        }

        inline float *get_view_matrix()
        {
            return this->view;
        }

        inline void set_position(float x, float y, float z)
        {
            tms_camera_set_position(this, x,y,z);
        }

        inline void translate(float x, float y, float z)
        {
            tms_camera_translate(this, x,y,z);
        }

        inline void confine(float x, float y, float z, float factor_x, float factor_y, float factor_z)
        {
            tms_camera_confine(this, x,y,z, factor_x, factor_y, factor_z);
        }

        inline void set_lookat(float x, float y, float z)
        {
            tms_camera_set_lookat(this, x, y, z);
        }

        inline void enable(int flag)
        {
            tms_camera_enable(this, flag);
        }

        inline void disable(int flag)
        {
            tms_camera_disable(this, flag);
        }

        inline void set_direction(float x, float y, float z)
        {
            tms_camera_set_direction(this, x, y, z);
        }

        inline void calculate()
        {
            tms_camera_calculate(this);
        }
    };

    class fb : public tms_fb
    {
      public:
          fb(unsigned width, unsigned height, int double_buf)
          {
              this->width = width;
              this->height = width;
              this->double_buffering = double_buf;
              tms_fb_init(this);
          }

          inline int bind()
          {
              return tms_fb_bind(this);
          }

          inline int unbind()
          {
              return tms_fb_unbind(this);
          }
    };

    class material : public tms_material
    {
      public:
        material()
        {
            tms_material_init(this);
        }
    };

    class surface : public tms_surface
    {
      public:
        surface()
        {
            tms_surface_init(this);
        }

        inline int remove_widget(tms_wdg *w)
        {
            return tms_surface_remove_widget(
                    static_cast<tms_surface *>(this),
                    w
                );
        }

        inline int add_widget(tms_wdg *w)
        {
            return tms_surface_add_widget(
                    static_cast<tms_surface *>(this),
                    w
                );
        }
    };

    class gbuffer : public tms_gbuffer
    {
      public:
        gbuffer(size_t size)
        {
            tms_gbuffer_init(static_cast<tms_gbuffer*>(this), size);
        };

        gbuffer(void *data, size_t size)
        {
            tms_gbuffer_init(static_cast<tms_gbuffer*>(this), size);
            memcpy(this->buf, data, size);
        };

        inline void *get_buffer()
        {
            return tms_gbuffer_get_buffer(static_cast<tms_gbuffer *>(this));
        }

        inline int upload()
        {
            return tms_gbuffer_upload(static_cast<tms_gbuffer *>(this));
        }

        inline int upload_partial(size_t size)
        {
            return tms_gbuffer_upload_partial(static_cast<tms_gbuffer *>(this), size);
        }
    };

    class varray : public tms_varray
    {
      public:
        varray(size_t num_attributes)
        {
            tms_varray_init(static_cast<tms_varray*>(this), num_attributes);
        }

        inline gbuffer* get_gbuffer(int index)
        {
            return static_cast<gbuffer*>(static_cast<tms_varray*>(this)->gbufs[0].gbuf);
        }

        inline int set_buffer_stride(gbuffer *gbuf, size_t offset)
        {
            return tms_varray_set_buffer_stride(static_cast<tms_varray*>(this), gbuf, offset);
        }

        inline int map_attribute(const char *name, int num_components,
                                         GLenum component_type, gbuffer *gbuf)
        {
            return tms_varray_map_attribute(static_cast<tms_varray*>(this), name, num_components, component_type, static_cast<tms_gbuffer*>(gbuf));
        }

        inline int bind_attributes(int *locations)
        {
            return tms_varray_bind_attributes(static_cast<tms_varray*>(this), locations);
        }

        inline int unbind_attributes(int *locations)
        {
            return tms_varray_unbind_attributes(static_cast<tms_varray*>(this), locations);
        }

        inline void upload_all()
        {
            tms_varray_upload_all(static_cast<tms_varray*>(this));
        }
    };

    class program : public tms_program
    {
    };

    class shader : public tms_shader
    {
      public:
          shader()
          {
              tms_shader_init(static_cast<struct tms_shader *>(this));
          }

          shader(const char *_name)
          {
              tms_shader_init(static_cast<struct tms_shader *>(this));

              this->name = const_cast<char*>(_name);
          }

          ~shader()
          {
              tms_shader_uninit(static_cast<struct tms_shader *>(this));
          }

          inline int compile(GLenum type, const char *src)
          {
              return tms_shader_compile(this, type, src);
          }

          inline tms::program *get_program(int pipeline)
          {
              return reinterpret_cast<tms::program*>(tms_shader_get_program(static_cast<tms_shader*>(this), pipeline));
          }
    };

    class mesh : public tms_mesh
    {
      public:
        mesh(varray *va, gbuffer *indices)
        {
            tms_mesh_init(static_cast<struct tms_mesh *>(this), va, indices);
        }

        inline int render(program *shader)
        {
            return tms_mesh_render(static_cast<tms_mesh*>(this), static_cast<tms_program*>(shader));
        }

        inline gbuffer * get_index_buffer()
        {
            return reinterpret_cast<tms::gbuffer*>(static_cast<tms_mesh*>(this)->indices);
        }

        inline varray * get_vertex_array()
        {
            return reinterpret_cast<varray*>(static_cast<tms_mesh*>(this)->vertex_array);
        }

        inline void set_primitive_type(int type)
        {
            tms_mesh_set_primitive_type(static_cast<tms_mesh*>(this), type);
        }

        inline void set_autofree_buffers(int f)
        {
            tms_mesh_set_autofree_buffers(static_cast<tms_mesh*>(this), f);
        }

    };

    class meshfactory
    {
      public:
          static inline const tms::mesh *get_cube()
          {
              return reinterpret_cast<const mesh*>(tms_meshfactory_get_cube());
          }

          static inline const tms::mesh *get_cylinder()
          {
              return reinterpret_cast<const mesh*>(tms_meshfactory_get_cylinder());
          }
    };

    class entity : public tms_entity
    {
      public:
        entity()
        {
            tms_entity_init(static_cast<tms_entity*>(this));
             static_cast<tms_entity*>(this)->update = _oopassthrough_entity_update;
        }

        virtual ~entity()
        {
            tms_entity_uninit(static_cast<tms_entity*>(this));
        }

        inline int set_mesh(tms::mesh *m)
        {
            return tms_entity_set_mesh(static_cast<tms_entity*>(this), static_cast<tms_mesh*>(m));
        }

        inline int set_mesh(tms_mesh *m)
        {
            return tms_entity_set_mesh(static_cast<tms_entity*>(this), m);
        }

        inline int set_material(tms::material *m)
        {
            return tms_entity_set_material(static_cast<tms_entity*>(this), m);
        }

        inline int add_child(entity *e)
        {
            return tms_entity_add_child(static_cast<tms_entity*>(this), static_cast<tms_entity*>(e));
        }

        inline int remove_child(entity *e)
        {
            return tms_entity_remove_child(static_cast<tms_entity*>(this), static_cast<tms_entity*>(e));
        }

        inline int set_uniform(const char *name, float r, float g, float b, float a)
        {
            return tms_entity_set_uniform4f(static_cast<tms_entity*>(this), name, r, g, b, a);
        }

        inline int set_uniform(const char *name, float r, float g)
        {
            return tms_entity_set_uniform2f(static_cast<tms_entity*>(this), name, r, g);
        }

        inline entity *get_child(int index)
        {
            return static_cast<entity*>(static_cast<tms_entity*>(this)->children[index]);
        }

        inline int get_num_children()
        {
            return static_cast<tms_entity*>(this)->num_children;
        }

        virtual void update()
        {
        }
    };

    class graph : public tms_graph
    {
      public:
        graph(int pipeline)
        {
            memset(this, 0, sizeof(*this));
            tms_graph_init(static_cast<tms_graph*>(this), 0, pipeline);
        }

        inline int render(tms::camera *cam, void *data)
        {
            return tms_graph_render(static_cast<tms_graph*>(this),
                        static_cast<tms_camera *>(cam),
                        data
                    );
        }

        inline int render(tms_camera *cam, void *data)
        {
            return tms_graph_render(static_cast<tms_graph*>(this),
                        cam,
                        data
                    );
        }
    };

    class scene : public tms_scene
    {
      public:
        scene()
        {
            tms_scene_init(this);
        }

        inline tms::graph* create_graph(int pipeline)
        {
            return reinterpret_cast<tms::graph*>(tms_scene_create_graph(static_cast<tms_scene*>(this), pipeline));
        }

        inline int add_entity(entity *e)
        {
            return tms_scene_add_entity(static_cast<tms_scene*>(this), static_cast<tms_entity*>(e));
        }

        inline int remove_entity(entity *e)
        {
            return tms_scene_remove_entity(static_cast<tms_scene*>(this), static_cast<tms_entity*>(e));
        }
    };

    class screen
    {
      public:
        struct tms_screen super;

        screen()
        {
            super.data = reinterpret_cast<void*>(this);
            super.spec = &_oopassthrough;

            super.surface = 0;
            super.scene = 0;
        }

        virtual int handle_input(event *ev, int action){return 0;};
        virtual int pause(){return 0;};
        virtual int resume(){return 0;};
        virtual int render(){return 0;};
        virtual int post_render(){return 0;};
        virtual int begin_frame(){return 0;};
        virtual int end_frame(){return 0;};
        virtual int step(double dt){return 0;};

        inline int set_surface(surface *surf)
        {
            return tms_screen_set_surface(&super, surf);
        }

        inline int set_scene(scene *s)
        {
            return tms_screen_set_scene(&super, s);
        }

        inline scene *get_scene()
        {
            return reinterpret_cast<scene*>(tms_screen_get_scene(&super));
        }

        inline surface *get_surface()
        {
            return (surface*)tms_screen_get_surface(&super);
        }
    };

    /* functions for the singleton object "tms" */

    inline int set_screen(screen *s)
    {
        return tms_set_screen((struct tms_screen *)&s->super);
    }

    inline int get_window_height()
    {
        return _tms.window_height;
    }

    inline int get_window_width()
    {
        return _tms.window_width;
    }
};
