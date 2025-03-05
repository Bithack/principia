#include "game-graph.hh"
#include "misc.hh"
#include "pscreen.hh"
#include "settings.hh"

#include <tms/bindings/cpp/cpp.hh>
#include <tms/core/tms.h>
#include <tms/core/ddraw.h>

#include <limits>

game_graph::game_graph(const char *_name)
    : name(_name)
    , max_data_points(100)
    , interval(0.5)
    , timer(0.0)
{ }

void
game_graph::recalculate()
{
    this->max = std::numeric_limits<double>::min();

    for (std::deque<double>::const_iterator it = this->data.begin();
            it != this->data.end(); ++it) {
        const double& val = *it;

        if (val > max) {
            this->max = val;
        }
    }
}

void
game_graph::reset()
{
    this->timer = 0.0;

    this->data.clear();
}

void
game_graph::set_size(float width, float height)
{
    this->width = width;
    this->height = height;
}

void
game_graph::set_position(float x, float y)
{
    this->x = x;
    this->y = y;
}

void
game_graph::set_interval(double interval)
{
    this->interval = interval;
}

void
game_graph::set_max_data_points(uint16_t max_data_points)
{
    this->max_data_points = max_data_points;
}

void
game_graph::set_min(double min)
{
    this->min = min;
}

void
game_graph::push(double dt, double val)
{
    tms_assertf(this->interval > 0.001, "Interval must be above 0.001");

    this->timer += dt;

    while (this->timer >= this->interval) {
        this->timer -= this->interval;

        this->data.push_back(val);

        if (this->data.size() > this->max_data_points) {
            this->data.pop_front();
        }

        this->recalculate();
    }
}

void
game_graph::render(pscreen *host) const
{
    switch (settings["display_fps"]->v.u8) {
        case 1: // render simple fps counter
            if (!this->data.empty()) {
                const double &val = this->data.back();
                host->add_text(
                        val,
                        font::small,
                        this->x-this->width/2.f,
                        this->y-this->height/2.f,
                        tvec4f(1.f, 1.f, 1.f, 1.f),
                        0,
                        true,
                        ALIGN_CENTER,
                        ALIGN_CENTER);
            }
            break;

        case 2: // render fps graph
        case 3:
            {
                struct tms_ddraw *dd = host->get_surface()->ddraw;

                {
                    /* Add text to the left side of the graph, displaying the current max value,
                     * and the set min value. */

                    host->add_text(
                            this->min,
                            font::small,
                            this->x-this->width/2.f,
                            this->y-this->height/2.f,
                            tvec4f(1.f, 1.f, 1.f, 1.f),
                            0,
                            true,
                            ALIGN_RIGHT,
                            ALIGN_BOTTOM);

                    host->add_text(
                            this->max,
                            font::small,
                            this->x-this->width/2.f,
                            this->y+this->height/2.f,
                            tvec4f(1.f, 1.f, 1.f, 1.f),
                            0,
                            true,
                            ALIGN_RIGHT,
                            ALIGN_TOP);

                    if (!this->data.empty()) {
                        const double &val = this->data.back();
                        host->add_text(
                                val,
                                font::small,
                                this->x+this->width/2.f,
                                this->y,
                                tvec4f(1.f, 1.f, 1.f, 1.f),
                                0,
                                true,
                                ALIGN_LEFT,
                                ALIGN_CENTER);
                    }
                }

                glEnable(GL_BLEND);
                tms_ddraw_set_color(dd, 0.f, 0.f, 0.f, 0.3f);
                tms_ddraw_square(dd,
                        this->x,
                        this->y,
                        this->width,
                        this->height);

                tms_ddraw_set_color(dd, FPS_GRAPH_OUTLINE_COLOR);
                tms_ddraw_lsquare(dd,
                        this->x,
                        this->y,
                        this->width,
                        this->height);

                bool first = true;
                tvec2 prev_point;

                float inner_width = this->width - 2.f;
                float inner_height = this->height - 2.f;

                float base_y = this->y - inner_height/2.f;
                float cur_x = this->x - inner_width/2.f;
                float x_step = inner_width / this->max_data_points;

                /*
                std::deque<double>::const_iterator last_it = this->data.end();
                -- last_it;
                */

                tms_ddraw_set_color(dd, FPS_GRAPH_COLOR);

                for (std::deque<double>::const_iterator it = this->data.begin();
                        it != this->data.end(); ++it) {
                    const double &val = *it;

                    tvec2 point;

                    point.x = cur_x;
                    point.y = base_y + (inner_height*((val-this->min)/((this->max-this->min))));

                    if (!first) {
                        const int x = it - this->data.begin();
                        if (x == (int)(this->max_data_points*0.9)) {
                            tms_ddraw_set_color(dd, FPS_GRAPH_COLOR_END);
                        }
                        tms_ddraw_line(dd,
                                prev_point.x,
                                prev_point.y,
                                point.x,
                                point.y
                                );
                    }

                    cur_x += x_step;

                    prev_point = point;

                    first = false;
                }
            }
            break;
    }
}

const std::deque<double>&
game_graph::get_data() const
{
    return this->data;
}
