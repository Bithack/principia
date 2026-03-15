#include "lua_this.hh"
#include "escript.hh"
#include "receiver.hh"

struct lua_vert base[4] = {
    {
        (tvec3){.5f,.5f,0.f},
        (tvec2){.5f, 1.f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    }, {
        (tvec3){-.5f,.5f,0.f},
        (tvec2){0.f, 1.f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    }, {
        (tvec3){-.5f,-.5f,0.f},
        (tvec2){0.f, .5f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    }, {
        (tvec3){.5f,-.5f,0.f},
        (tvec2){.5f, .5f},
        (tvec4){0.f, 0.f, 0.f, 0.f},
    }
};

static uint32_t
upper_power_of_two(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

/* this:write(socket, value) */
static int l_this_write(lua_State *L)
{
	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	int socket = luaL_checkint(L, 2);
	double value = tclampf(luaL_checknumber(L, 3), 0.0, 1.0);
	if (socket < 0) socket = 0;
	if (socket > 3) socket = 3;

	if (!e->s_out[socket].written()) {
		e->s_out[socket].write(value);
	}

	return 0;
}

/* this:read(socket) */
static int l_this_read(lua_State *L)
{
	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	int socket = luaL_checkint(L, 2);
	if (socket < 0) socket = 0;
	if (socket > 3) socket = 3;

	lua_pushnumber(L, e->val[socket]);

	return 1;
}

/* this:has_plug(socket) */
static int l_this_has_plug(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:has_plug", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	int socket = luaL_checkint(L, 2);
	if (socket < 0) socket = 0;
	if (socket > 3) socket = 3;

	lua_pushboolean(L, e->socket_active[socket]);

	return 1;
}

/* this:write_frequency(frequency, value) */
static int l_this_write_frequency(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:write_frequency", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	uint32_t freq = (uint32_t)luaL_checklong(L, 2);
	double value = tclampf(luaL_checknumber(L, 3), 0.0, 1.0);

	std::pair<std::multimap<uint32_t, receiver_base*>::iterator, std::multimap<uint32_t, receiver_base*>::iterator> range = W->receivers.equal_range(freq);
	for (std::multimap<uint32_t, receiver_base*>::iterator
			i = range.first;
			i != range.second && i != W->receivers.end();
			i++) {
		i->second->pending_value = value;
		i->second->no_broadcast = true;
	}

	return 0;
}

/* this:listen_on_frequency(frequency) */
static int l_this_listen_on_frequency(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:listen_on_frequency", "1.4", LEVEL_VERSION_1_4);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	uint32_t freq = (uint32_t)luaL_checklong(L, 2);
	if (e->first_run) {
		receiver_base *rb = new receiver_base();
		std::pair<std::map<uint32_t, receiver_base*>::iterator, bool> ret;
		ret = e->receivers.insert(std::pair<uint32_t, receiver_base*>(freq, rb));
		if (!ret.second) {
			delete rb;
		} else {
			W->add_receiver(freq, rb);
		}
	} else {
		lua_pushstring(L, "You can only start listening to frequency in init().");
		lua_error(L);
	}

	return 0;
}

/* this:read_frequency(frequency) */
static int l_this_read_frequency(lua_State *L)
{
	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	uint32_t freq = luaL_checklong(L, 2);

	std::map<uint32_t, receiver_base*>::iterator ret;
	ret = e->receivers.find(freq);
	float v = 0.f;
	if (ret != e->receivers.end()) {
		v = ret->second->pending_value;
	} else {
		char err[512];
		snprintf(err, 511, "You are not listening to frequency %u.\nUse this:listen_on_frequency() to begin.", freq);
		lua_pushstring(L, err);
		lua_error(L);
	}

	lua_pushnumber(L, v);

	return 1;
}

/* this:first_run() */
static int l_this_first_run(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:first_run", "1.3.0.2", LEVEL_VERSION_1_3_0_2);
	ESCRIPT_FUNCTION_DEPRECATED(L, "this:first_run", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	lua_pushboolean(L, e->first_run);

	return 1;
}

/* x, y = this:get_position() */
static int l_this_get_position(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:get_position", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	b2Vec2 pos = e->get_position();
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);

	return 2;
}

/* id = this:get_id() */
static int l_this_get_id(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:get_id", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	lua_pushnumber(L, e->id);
	return 1;
}

/* width, height = this:get_resolution()*/
static int l_this_get_resolution(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:get_resolution", "1.5", LEVEL_VERSION_1_5);

	lua_pushnumber(L, _tms.window_width);
	lua_pushnumber(L, _tms.window_height);

	return 2;
}

/* ratio = this:get_ratio()*/
static int l_this_get_ratio(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:get_ratio", "1.5", LEVEL_VERSION_1_5);

	lua_pushnumber(L, (float)_tms.window_width/_tms.window_height);

	return 1;
}

/* this:set_sprite_blending(int) */
static int l_this_set_sprite_blending(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:set_sprite_blending", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	e->blending_mode = luaL_checkint(L, 2);

	if (e->blending_mode < 0 || e->blending_mode > 2) {
		lua_pushstring(L, "Invalid blending mode");
		lua_error(L);
	}

	return 0;
}

/* this:set_sprite_filtering(int) */
static int l_this_set_sprite_filtering(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:set_sprite_filtering", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	e->filtering = luaL_checkint(L, 2);

	if (e->filtering < 0 || e->filtering > 1) {
		lua_pushstring(L, "Invalid sprite filtering");
		lua_error(L);
	}

	return 0;
}

/* this:set_sprite_texel(x, y, r, g, b, a) */
static int l_this_set_sprite_texel(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:set_sprite_texel", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	int u = luaL_checkint(L, 2);
	int v = luaL_checkint(L, 3);
	float r = luaL_checknumber(L, 4);
	float g = luaL_checknumber(L, 5);
	float b = luaL_checknumber(L, 6);
	float a = luaL_checknumber(L, 7);

	if (!e->normal_draw) {
		e->normal_draw = new draw_data(e);
	}

	draw_data *draw = e->normal_draw;

	if (u < 0 || u >= draw->texture_width || v < 0 || v >= draw->texture_height) {
		lua_pushfstring(L, "texel coordinate out of range (%d/%d)", u, v);
		lua_error(L);
	}

	draw->texture->data[draw->texture_width*4*v + 4*u] = (unsigned char)tclampf(roundf(r*255.f), 0, 255.f);
	draw->texture->data[draw->texture_width*4*v + 4*u+1] = (unsigned char)tclampf(roundf(g*255.f), 0, 255.f);
	draw->texture->data[draw->texture_width*4*v + 4*u+2] = (unsigned char)tclampf(roundf(b*255.f), 0.f, 255.f);
	draw->texture->data[draw->texture_width*4*v + 4*u+3] = (unsigned char)tclampf(roundf(a*255.f), 0.f, 255.f);

	draw->texture_modified = true;

	return 0;
}

/* this:clear_texels() */
static int l_this_clear_texels(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:clear_texels", "1.4", LEVEL_VERSION_1_4);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	unsigned char clear_value = 0;

	if (lua_gettop(L) > 1) {
		float clr = luaL_checknumber(L, 2);
		clear_value = (unsigned char)tclampf(roundf(clr*255.f), 0, 255.f);
	}

	if (!e->normal_draw) {
		e->normal_draw = new draw_data(e);
	}

	draw_data *draw = e->normal_draw;

	tms_texture_clear_buffer(draw->texture, clear_value);

	draw->texture_modified = true;

	return 0;
}

/* this:set_draw_tint(r,g,b,a) */
static int l_this_set_draw_tint(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:set_draw_tint", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	e->draw_tint.r = luaL_checknumber(L, 2);
	e->draw_tint.g = luaL_checknumber(L, 3);
	e->draw_tint.b = luaL_checknumber(L, 4);
	e->draw_tint.a = luaL_checknumber(L, 5);

	return 0;
}

/* this:set_draw_z(z) */
static int l_this_set_draw_z(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:set_draw_z", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	e->draw_z = luaL_checknumber(L, 2);

	return 0;
}

/* this:set_draw_coordinates(int) */
static int l_this_set_draw_coordinates(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:set_draw_coordinates", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	int mode = luaL_checkint(L, 2);

	if (mode < 0 || mode > 2) {
		lua_pushstring(L, "Invalid camera mode.");
		lua_error(L);
	} else {
		e->coordinate_mode = mode;

		if (e->coordinate_mode == ESCRIPT_LOCAL && lua_gettop(L) == 3) {
			e->local_id = luaL_checknumber(L, 3);
		} else {
			e->local_id = 0;
		}
	}

	return 0;
}

/* this:draw_sprite(x, y, r, w, h, bx, by, tx, ty) */
static int l_this_draw_sprite(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:draw_sprite", "1.3.0.2", LEVEL_VERSION_1_3_0_2);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	if (!e->normal_draw) {
		e->normal_draw = new draw_data(e);
	}

	draw_data *draw = e->normal_draw;

	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float r = luaL_checknumber(L, 4);
	float w = luaL_checknumber(L, 5);
	float h = luaL_checknumber(L, 6);
	float bx = (float)luaL_checkint(L, 7);
	float by = (float)luaL_checkint(L, 8);
	float tx = (float)luaL_checkint(L, 9);
	float ty = (float)luaL_checkint(L, 10);

	if (tx == bx) tx = bx+.5f;
	if (ty == by) ty = by+.5f;

	tvec2 uvb = {(float)bx / draw->texture_width, (float)by / draw->texture_height};
	tvec2 uvt = {(float)tx / draw->texture_width, (float)ty / draw->texture_height};

	if (draw->sprite_count < MAX_SPRITES) {
		struct lua_vert *_b = (struct lua_vert*)draw->verts->get_buffer();
		int n = draw->sprite_count;

		entity *local_entity = 0;
		b2Vec2 local_position;

		if (e->coordinate_mode == ESCRIPT_LOCAL) {
			local_entity = (e->local_id != 0 ? W->get_entity_by_id(e->local_id) : 0);

			if (local_entity) {
				local_position = local_entity->get_position();
				r += local_entity->get_angle();
			} else {
				local_position = e->get_position();
				r += e->get_angle();
			}
		}

		if (r != 0.f) {
			float cs, sn;
			tmath_sincos(r, &sn, &cs);

			for (int ix=0; ix<4; ix++) {
				_b[n*4+ix] = base[ix];

				_b[n*4+ix].pos.x *= w;
				_b[n*4+ix].pos.y *= h;

				float _x = _b[n*4+ix].pos.x * cs - _b[n*4+ix].pos.y * sn;
				float _y = _b[n*4+ix].pos.x * sn + _b[n*4+ix].pos.y * cs;
				_b[n*4+ix].pos.x = _x;
				_b[n*4+ix].pos.y = _y;

				if (e->coordinate_mode == ESCRIPT_LOCAL) {
					_b[n*4+ix].pos.x += local_position.x;
					_b[n*4+ix].pos.y += local_position.y;
				}

				_b[n*4+ix].pos.x += x;
				_b[n*4+ix].pos.y += y;
				_b[n*4+ix].pos.z += e->draw_z;

				_b[n*4+ix].color.r = e->draw_tint.r;
				_b[n*4+ix].color.g = e->draw_tint.g;
				_b[n*4+ix].color.b = e->draw_tint.b;
				_b[n*4+ix].color.a = e->draw_tint.a;

				_b[n*4+ix].uv.x = ((ix == 0 || ix == 3) ? uvt.x : uvb.x);
				_b[n*4+ix].uv.y = (ix < 2 ? uvt.y : uvb.y);
			}
		} else {
			for (int ix=0; ix<4; ix++) {
				_b[n*4+ix] = base[ix];

				_b[n*4+ix].pos.x *= w;
				_b[n*4+ix].pos.y *= h;

				if (e->coordinate_mode == ESCRIPT_LOCAL) {
					_b[n*4+ix].pos.x += local_position.x;
					_b[n*4+ix].pos.y += local_position.y;
				}

				_b[n*4+ix].pos.x += x;
				_b[n*4+ix].pos.y += y;

				_b[n*4+ix].pos.z += e->draw_z;

				_b[n*4+ix].color.r = e->draw_tint.r;
				_b[n*4+ix].color.g = e->draw_tint.g;
				_b[n*4+ix].color.b = e->draw_tint.b;
				_b[n*4+ix].color.a = e->draw_tint.a;

				_b[n*4+ix].uv.x = ((ix == 0 || ix == 3) ? uvt.x : uvb.x);
				_b[n*4+ix].uv.y = (ix < 2 ? uvt.y : uvb.y);
			}
		}

		if (e->solving) {
			draw->sprite_count ++;
		} else {
			draw->pending_sprite_count ++;
		}
	}

	return 0;
}

/* this:draw_line(x1, y1, x2, y2, w) */
static int l_this_draw_line(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:draw_line", "1.4", LEVEL_VERSION_1_4);

	escript_line line;

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	line.x1 = luaL_checknumber(L, 2);
	line.y1 = luaL_checknumber(L, 3);
	line.x2 = luaL_checknumber(L, 4);
	line.y2 = luaL_checknumber(L, 5);
	line.w1 = luaL_checknumber(L, 6);
	line.w2 = line.w1;
	line.z1 = e->draw_z;
	line.z2 = e->draw_z;
	line.r1 = e->draw_tint.r;
	line.g1 = e->draw_tint.g;
	line.b1 = e->draw_tint.b;
	line.a1 = e->draw_tint.a;
	line.r2 = e->draw_tint.r;
	line.g2 = e->draw_tint.g;
	line.b2 = e->draw_tint.b;
	line.a2 = e->draw_tint.a;

	e->add_line(line);

	return 0;
}

/* this:draw_gradient_line(x1, y1, x2, y2, w, r, g, b, a) */
static int l_this_draw_gradient_line(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:draw_gradient_line", "1.5", LEVEL_VERSION_1_5);

	escript_line line;

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	line.x1 = luaL_checknumber(L, 2);
	line.y1 = luaL_checknumber(L, 3);
	line.x2 = luaL_checknumber(L, 4);
	line.y2 = luaL_checknumber(L, 5);
	line.w1 = luaL_checknumber(L, 6);
	line.w2 = line.w1;
	line.z1 = e->draw_z;
	line.z2 = e->draw_z;
	line.r1 = e->draw_tint.r;
	line.g1 = e->draw_tint.g;
	line.b1 = e->draw_tint.b;
	line.a1 = e->draw_tint.a;
	line.r2 = luaL_checknumber(L, 7);
	line.g2 = luaL_checknumber(L, 8);
	line.b2 = luaL_checknumber(L, 9);
	line.a2 = luaL_checknumber(L, 10);

	e->add_line(line);

	return 0;
}

/* this:draw_line_3d(x1, y1, z1, x2, y2, z2, w) */
static int l_this_draw_line_3d(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:draw_line_3d", "1.5", LEVEL_VERSION_1_5);

	escript_line line;

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	line.x1 = luaL_checknumber(L, 2);
	line.y1 = luaL_checknumber(L, 3);
	line.z1 = luaL_checknumber(L, 4);
	line.x2 = luaL_checknumber(L, 5);
	line.y2 = luaL_checknumber(L, 6);
	line.z2 = luaL_checknumber(L, 7);
	line.w1 = luaL_checknumber(L, 8);
	line.w2 = line.w1;
	line.r1 = e->draw_tint.r;
	line.g1 = e->draw_tint.g;
	line.b1 = e->draw_tint.b;
	line.a1 = e->draw_tint.a;
	line.r2 = e->draw_tint.r;
	line.g2 = e->draw_tint.g;
	line.b2 = e->draw_tint.b;
	line.a2 = e->draw_tint.a;

	e->add_line(line);

	return 0;
}

/* this:draw_gradient_line_3d(x1, y1, z1, x2, y2, z2, w, r, g, b, a) */
static int l_this_draw_gradient_line_3d(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:draw_gradient_line_3d", "1.5", LEVEL_VERSION_1_5);

	escript_line line;

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));
	line.x1 = luaL_checknumber(L, 2);
	line.y1 = luaL_checknumber(L, 3);
	line.z1 = luaL_checknumber(L, 4);
	line.x2 = luaL_checknumber(L, 5);
	line.y2 = luaL_checknumber(L, 6);
	line.z2 = luaL_checknumber(L, 7);
	line.w1 = luaL_checknumber(L, 8);
	line.w2 = line.w1;
	line.r1 = e->draw_tint.r;
	line.g1 = e->draw_tint.g;
	line.b1 = e->draw_tint.b;
	line.a1 = e->draw_tint.a;
	line.r2 = luaL_checknumber(L, 9);
	line.g2 = luaL_checknumber(L, 10);
	line.b2 = luaL_checknumber(L, 11);
	line.a2 = luaL_checknumber(L, 12);

	e->add_line(line);

	return 0;
}

/* r, g, b, a = this:get_sprite_texel(x, y) */
static int l_this_get_sprite_texel(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:get_sprite_texel", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	int u = luaL_checkint(L, 2);
	int v = luaL_checkint(L, 3);

	if (!e->normal_draw) {
		lua_pushnumber(L, 0.f);
		lua_pushnumber(L, 0.f);
		lua_pushnumber(L, 0.f);
		lua_pushnumber(L, 0.f);
		return 4;
	}

	if (u < 0 || u >= e->normal_draw->texture_width || v < 0 || v >= e->normal_draw->texture_height) {
		lua_pushstring(L, "texel coordinate out of range");
		lua_error(L);
	}

	struct tms_texture *tex = e->normal_draw->texture;
	int width = e->normal_draw->texture_width;

	lua_pushnumber(L, tex->data[width*4*v + 4*u+0]/255.f); // r
	lua_pushnumber(L, tex->data[width*4*v + 4*u+1]/255.f); // g
	lua_pushnumber(L, tex->data[width*4*v + 4*u+2]/255.f); // b
	lua_pushnumber(L, tex->data[width*4*v + 4*u+3]/255.f); // a

	return 4;
}

/* this:init_draw(width, height) */
static int l_this_init_draw(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:init_draw", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	if (!e->first_run) {
		lua_pushstring(L, "Draw can only be initialized in the init()-function.");
		lua_error(L);
	}

	int width = luaL_checkint(L, 2);
	int height = luaL_checkint(L, 3);

	width = upper_power_of_two(width);
	height = upper_power_of_two(height);

	if (width < 1 || width > 1024 || height < 1 || height > 1024) {
		lua_pushstring(L, "Draw width/height out of range. Must be between 1 and 1024.");
		lua_error(L);
	}

	if (!e->normal_draw) {
		e->normal_draw = new draw_data(e, width, height);
	}

	return 0;
}

/* this:set_static_sprite_texel(x, y, r, g, b, a)*/
static int l_this_set_static_sprite_texel(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:set_static_sprite_texel", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	int u = luaL_checkint(L, 2);
	int v = luaL_checkint(L, 3);
	float r = luaL_checknumber(L, 4);
	float g = luaL_checknumber(L, 5);
	float b = luaL_checknumber(L, 6);
	float a = luaL_checknumber(L, 7);

	if (!e->static_draw) {
		e->static_draw = new draw_data(e);
	}

	draw_data *draw = e->static_draw;

	if (u < 0 || u >= draw->texture_width || v < 0 || v >= draw->texture_height) {
		lua_pushfstring(L, "texel coordinate out of range (%d/%d)", u, v);
		lua_error(L);
	}

	draw->texture->data[draw->texture_width*4*v + 4*u] = (unsigned char)tclampf(roundf(r*255.f), 0, 255.f);
	draw->texture->data[draw->texture_width*4*v + 4*u+1] = (unsigned char)tclampf(roundf(g*255.f), 0, 255.f);
	draw->texture->data[draw->texture_width*4*v + 4*u+2] = (unsigned char)tclampf(roundf(b*255.f), 0.f, 255.f);
	draw->texture->data[draw->texture_width*4*v + 4*u+3] = (unsigned char)tclampf(roundf(a*255.f), 0.f, 255.f);

	draw->texture_modified = true;

	return 0;
}

/* this:clear_static_texels() */
static int l_this_clear_static_texels(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:clear_static_texels", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	if (!e->static_draw) {
		e->static_draw = new draw_data(e);
	}

	draw_data *draw = e->static_draw;

	int numargs = lua_gettop(L);

	if (numargs > 2) {
		unsigned char colors[4] = {
			127, 127, 127, 127
		};

		float v = luaL_checknumber(L, 2);
		colors[0] = (unsigned char)tclampf(roundf(v*255.f), 0, 255.f);

		if (numargs >= 3) {
			float v = luaL_checknumber(L, 3);
			colors[1] = (unsigned char)tclampf(roundf(v*255.f), 0, 255.f);
		}
		if (numargs >= 4) {
			float v = luaL_checknumber(L, 4);
			colors[2] = (unsigned char)tclampf(roundf(v*255.f), 0, 255.f);
		}
		if (numargs >= 5) {
			float v = luaL_checknumber(L, 5);
			colors[3] = (unsigned char)tclampf(roundf(v*255.f), 0, 255.f);
		}

		uint32_t buf_sz = draw->texture_width * draw->texture_height * draw->texture_num_channels;
		for (uint32_t i=0; i<buf_sz; i += draw->texture_num_channels) {
			for (uint8_t c=0; c<draw->texture_num_channels; ++c) {
				draw->texture->data[i+c] = colors[c];
			}
		}
	} else {
		unsigned char clear_value = 0;

		if (numargs == 2) {
			float clr = luaL_checknumber(L, 2);
			clear_value = (unsigned char)tclampf(roundf(clr*255.f), 0, 255.f);
		}

		tms_texture_clear_buffer(draw->texture, clear_value);
	}

	draw->texture_modified = true;

	return 0;
}

/* this:add_static_sprite(x, y, r, w, h, bx, by, tx, ty) */
static int l_this_add_static_sprite(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:add_static_sprite", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	if (!e->static_draw) {
		e->static_draw = new draw_data(e);
	}

	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float r = luaL_checknumber(L, 4);
	float w = luaL_checknumber(L, 5);
	float h = luaL_checknumber(L, 6);
	float bx = (float)luaL_checkint(L, 7);
	float by = (float)luaL_checkint(L, 8);
	float tx = (float)luaL_checkint(L, 9);
	float ty = (float)luaL_checkint(L, 10);

	if (tx == bx) tx = bx+.5f;
	if (ty == by) ty = by+.5f;

	e->add_static_sprite(x, y, r, w, h, bx, by, tx, ty);

	return 0;
}

/* this:clear_static_sprites() */
static int l_this_clear_static_sprites(lua_State *L)
{
	ESCRIPT_VERSION_ERROR(L, "this:clear_static_sprites", "1.5", LEVEL_VERSION_1_5);

	escript *e = *(static_cast<escript**>(luaL_checkudata(L, 1, "This")));

	if (!e->static_draw) {
		e->static_draw = new draw_data(e);
	}

	e->static_draw->sprite_count = 0;
	e->static_sprites.clear();

	return 0;
}

// ---

static const luaL_Reg this_meta[] = {
    { NULL, NULL }
};
static const luaL_Reg this_methods[] = {
#define LUA_REG(name) { #name, l_this_##name }
    LUA_REG(write),
    LUA_REG(read),
    LUA_REG(has_plug),
    LUA_REG(write_frequency),
    LUA_REG(listen_on_frequency),
    LUA_REG(read_frequency),
    LUA_REG(first_run),
    LUA_REG(get_position),
    LUA_REG(get_id),
    LUA_REG(get_resolution),
    LUA_REG(get_ratio),

    /* draw stuff */
    LUA_REG(set_sprite_blending),
    LUA_REG(set_sprite_filtering),
    LUA_REG(set_sprite_texel),
    LUA_REG(clear_texels),

    LUA_REG(set_draw_tint),
    {"set_sprite_tint", l_this_set_draw_tint}, // backwards compat

    LUA_REG(set_draw_z),
    {"set_sprite_z", l_this_set_draw_z}, // backwards compat

    LUA_REG(set_draw_coordinates),

    LUA_REG(draw_sprite),
    LUA_REG(draw_line),
    LUA_REG(draw_gradient_line),
    LUA_REG(draw_line_3d),
    LUA_REG(draw_gradient_line_3d),

    LUA_REG(get_sprite_texel),

    LUA_REG(init_draw),

    LUA_REG(set_static_sprite_texel),
    LUA_REG(clear_static_texels),
    LUA_REG(add_static_sprite),
    LUA_REG(clear_static_sprites),

    { NULL, NULL }
#undef LUA_REG
};

void register_this(lua_State *L, escript *e)
{
    int lib_id, meta_id;

    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);

    luaL_newmetatable(L, "This");
    meta_id = lua_gettop(L);
    luaL_setfuncs(L, this_meta, 0);

    luaL_newlib(L, this_methods);
    lua_setfield(L, meta_id, "__index");

    luaL_newlib(L, this_meta);
    lua_setfield(L, meta_id, "__metatable");

    lua_setmetatable(L, lib_id);

    escript **ee = static_cast<escript**>(lua_newuserdata(L, sizeof(entity*)));
    *(ee) = e;
    luaL_setmetatable(L, "This");
    lua_setglobal(L, "this");

    lua_pop(L, 1);
}
