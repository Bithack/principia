#pragma once

#include <deque>
#include <stdint.h>

class pscreen;

class game_graph
{
  private:
    std::deque<double> data;
    const char *name;
    uint16_t max_data_points;
    double interval;
    double timer;

    float height;
    float width;

    double min;
    float x;
    float y;

    /* Cached data used in render */
    double max;

    void recalculate();

  public:
    game_graph(const char *_name);

    void reset();

    void set_size(float width, float height);
    void set_position(float x, float y);
    void set_interval(double interval);
    void set_max_data_points(uint16_t max_data_points);
    void set_min(double min);
    void push(double dt, double val);

    void render(pscreen *host) const;

    const std::deque<double>& get_data() const;
};
