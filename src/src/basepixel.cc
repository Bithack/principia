#include "basepixel.hh"
#include "object_factory.hh"

#define USE_NEW_SEARCH
#define PIXEL_GRID .5f

int basepixel::radius = 32;
float basepixel::start_x = 0.f;
float basepixel::start_y = 0.f;
basepixel *basepixel::found[PIXEL_GRID_MAX*PIXEL_GRID_MAX];
int basepixel::search_width = 0;
bool basepixel::disable_search = false;

basepixel::basepixel()
{
    this->set_flag(ENTITY_IS_STATIC, true);
    this->got_pos = false;

    this->width = 1.f;
    this->height = 1.f;

    this->num_sliders = 1;
}

void
basepixel::setup()
{
    this->update_appearance();
}

void
basepixel::on_pause()
{
    this->update_appearance();
}

void
basepixel::on_load(bool created, bool has_state)
{
    this->on_slider_change(-1, (float)this->properties[0].v.i8 / 3.f);
    this->update_appearance();
}

void
basepixel::update_fixture()
{
    if (this->body) {
        b2PolygonShape *shape = static_cast<b2PolygonShape*>(this->fx->GetShape());
        float sz = this->get_size() * 1.001f;
        shape->SetAsBox(sz, sz, this->_pos, this->_angle);
    }
}

float
basepixel::get_slider_value(int s)
{
    return (float)this->properties[0].v.i8 / 3.f;
}

float
basepixel::get_slider_snap(int s)
{
    return 1.f/3.f;
}

void
basepixel::construct()
{
    this->set_position(this->get_position().x, this->get_position().y, 0);
    this->update_fixture();
    this->update_appearance();
}

bool
basepixel::ReportFixture(b2Fixture *f)
{
    entity *e = static_cast<entity*>(f->GetUserData());

    if (e && (e->g_id == O_PIXEL || e->g_id == O_TPIXEL) && e != this && e->get_layer() == this->get_layer()) {
        float angle_mod = std::abs(fmodf(e->_angle, M_PI/2.));
        if (angle_mod < (M_PI/2.)-.05 && angle_mod > .05) return true;

        b2Vec2 cur_pos(this->start_x, this->start_y);
        double min_x, min_y;
        b2PolygonShape *s = static_cast<b2PolygonShape*>(f->GetShape());

        tms_assertf(s->m_count > 0, "Pixel vertex count must be > 0");

        for (int n=0; n<s->m_count; ++n) {
            if (n == 0 || s->m_vertices[n].x < min_x) min_x = s->m_vertices[n].x;
            if (n == 0 || s->m_vertices[n].y < min_y) min_y = s->m_vertices[n].y;
        }

        min_x = roundf(min_x*2.f)/2.f;
        min_y = roundf(min_y*2.f)/2.f;

        b2Vec2 rpos(min_x-cur_pos.x, min_y-cur_pos.y);
        double before_round_x = (rpos.x / .5);
        double before_round_y = (rpos.y / .5);
        int grid_x = roundf(rpos.x / PIXEL_GRID);
        int grid_y = roundf(rpos.y / PIXEL_GRID);
        int base_x = this->search_width / 2;
        int base_y = this->search_width / 2;
        int side = 1;

        switch (e->properties[0].v.i8) {
            case 0: side = 1; break;
            case 1: side = 2; break;
            case 2: side = 4; break;
            case 3: side = 8; break;
        }

#ifdef PIXEL_DEBUG
        b2Vec2 pos = e->get_position();
        double before_grid_x = roundf(before_round_x);
        double before_grid_y = roundf(before_round_y);
        int grid2_x = roundf(min_x / PIXEL_GRID);
        int grid2_y = roundf(min_y / PIXEL_GRID);

        tms_debugf("pixel id: %u", e->id);
        tms_debugf("pos_x: %f", pos.x);
        tms_debugf("pos_y: %f", pos.y);
        tms_debugf("cur_pos_x: %f", cur_pos.x);
        tms_debugf("cur_pos_y: %f", cur_pos.y);
        tms_debugf("min_x: %f", min_x);
        tms_debugf("min_y: %f", min_y);
        tms_debugf("rpos.x: %f", rpos.x);
        tms_debugf("rpos.y: %f", rpos.y);
        tms_debugf("before_round.x: %f", before_round_x);
        tms_debugf("before_round.y: %f", before_round_y);
        tms_debugf("before_grid.x: %f", before_grid_x);
        tms_debugf("before_grid.y: %f", before_grid_y);
        tms_debugf("Grid: %d/%d", grid_x, grid_y);
        tms_debugf("Grid 2: %d/%d", grid2_x, grid2_y);
        tms_debugf("Base: %d/%d", base_x, base_y);
        tms_debugf("Side: %d", side);
#endif

        for (int i=0; i<side; ++i) {
            for (int j=0; j<side; ++j) {
                int rx = base_x+grid_x+j;
                int ry = base_y+grid_y+i;

                if (rx < 0 || rx >= this->search_width) continue;
                if (ry < 0 || ry >= this->search_width) continue;

                int index = rx + (ry * this->search_width);
                if (index >= 0 && index < this->search_width*this->search_width && !this->found[index]) {
                    if (this->found[index]) continue;
                    if (e != this)
                        //tms_debugf("filling %d [%d/%d]", index, rx, ry);
                    this->found[index] = static_cast<basepixel*>(e);
                } else {
                    //tms_debugf("can't set index %d. (%d/%d)", index, i, j);
                }
            }
        }
        //tms_debugf("Found pixel at [%d/%d] %.2f/%.2f [%.2f/%.2f]", grid_x, grid_y, pos.x, pos.y, rpos.x, rpos.y);
    }

    return true;
}

bool
basepixel::search(float start_x, float start_y, int search_width, float *rx, float *ry)
{
    if (basepixel::disable_search)
        return true;

    this->search_width = search_width;
    this->start_x = roundf(start_x * 2.f) / 2.f;
    this->start_y = roundf(start_y * 2.f) / 2.f;
    float offset = search_width * 0.5f * 0.5f;
    b2Vec2 lower(start_x - offset, start_y - offset);
    b2Vec2 upper(start_x + offset, start_y + offset);
    b2AABB aabb;
    aabb.lowerBound = lower;
    aabb.upperBound = upper;
    memset(this->found, 0, sizeof(basepixel*)*(PIXEL_GRID_MAX*PIXEL_GRID_MAX));
    this->body->GetWorld()->QueryAABB(this, aabb);

#if 0
  printf("------- start ----------\n");
  for (int y=search_width-1; y>=0; --y) {
      for (int x=0; x<search_width; x++) {
          int index = x + (y * search_width);
          if (this->found[index]) {
              if (this->found[index] == this)
                  printf("X ");
              else
                  printf("1 ");
          } else {
              printf("0 ");
          }
      }
      printf("\n");
  }
  printf("------- end ----------\n");
#endif

    int side = 1;
    switch (this->properties[0].v.i8) {
        case 0: side = 1; break;
        case 1: side = 2; break;
        case 2: side = 4; break;
        case 3: side = 8; break;
    }

    (*rx) = 0.0f;
    (*ry) = 0.0f;

    int X = search_width;
    int Y = search_width;
    int x = 0, y = 0, dx = 0, dy = -1;
    int t = std::max(X,Y);
    int grid_x, grid_y;
    bool success = false;
    //tms_debugf("search_width: %d", search_width);

    int hside = std::max(side/2,1);

    for (int sz=0; sz<search_width/2; sz++) {
        for (int iy=-sz; iy<sz+1; iy++) {
            for (int ix=-sz; ix<sz+1; ix++) {
                if (ix == -sz || iy == -sz || iy == sz || ix == sz) {

                    grid_x = search_width / 2 + ix - hside;
                    grid_y = search_width / 2 + iy - hside;

                    bool free = true;

                    for (int nn=0; nn<side; ++nn) {
                        for (int n=0; n<side; ++n) {
                            int rx = grid_x+n;
                            int ry = grid_y+nn;

                            if (rx < 0 || rx >= this->search_width) continue;
                            if (ry < 0 || ry >= this->search_width) continue;

                            int index = rx + (ry * this->search_width);
                            if (this->found[index] && this->found[index] != this) {
                                free = false;
                                break;
                            } else
                                this->found[index] = this;
                        }
                    }

                    if (free) {
                        (*rx) = start_x + (grid_x-search_width/2+hside)*PIXEL_GRID;
                        (*ry) = start_y + (grid_y-search_width/2+hside)*PIXEL_GRID;
                        success = true;
                        goto done;
                    }
                }
            }
        }
    }

done:
    return success;
}

void
basepixel::gather_connected_pixels(std::set<entity*> *entities, int depth/*=-1*/)
{
    tms_debugf("UNIMPLEMENTED FUNCTION");
    /*
    float ox, oy;
    b2Vec2 cp = this->get_position();
    std::set<basepixel*> pixels;
    entities->insert(this);
    if (depth == 0) return;
    this->search(cp.x, cp.y, basepixel::radius, &ox, &oy, &pixels);
    free(this->found);
    for (std::set<basepixel*>::iterator i = pixels.begin();
            i != pixels.end(); ++i) {
        if (entities->find(*i) == entities->end()) {
            ((basepixel*)(*i))->gather_connected_pixels(entities, depth - 1);
        }
    }
    */
}

void
basepixel::set_position(float x, float y, uint8_t fr)
{
    float gox = fmod(this->width, PIXEL_GRID);
    float goy = fmod(this->height, PIXEL_GRID);

    x = roundf(x/PIXEL_GRID)*PIXEL_GRID+gox;
    y = roundf(y/PIXEL_GRID)*PIXEL_GRID+goy;

    if (this->body && !basepixel::disable_search) {
        float rx=0.f, ry=0.f;

        this->got_pos = this->search(x, y, basepixel::radius, &rx, &ry);
        //free(this->found);
        //this->found = 0;

        x = rx;
        y = ry;
    }

    b2Vec2 p = b2Vec2(x,y);
    if (b2Distance(p, this->get_position()) > .0001f)
        entity::set_position(x,y,fr);
}

void
basepixel::on_slider_change(int s, float value)
{
    uint32_t size = (uint32_t)roundf(value*3.f);
    this->set_property(0, size);
    //this->set_mesh(mesh_factory::box[size]);

    if (s != -1) {
        this->disconnect_all();
        this->recreate_shape();
    }
}

void
basepixel::recreate_shape(bool skip_search, bool dynamic)
{
    if (this->body && this->fx) {
        this->body->DestroyFixture(this->fx);
        this->fx = 0;
    }

    float s = this->get_size() * 1.001f;
    this->create_rect(dynamic?b2_dynamicBody : b2_staticBody, s,s, this->material);

    if (!skip_search) {
        this->set_position(this->get_position().x, this->get_position().y, 0);
    }
}

void
basepixel::add_to_world()
{
    this->recreate_shape(true);
}

