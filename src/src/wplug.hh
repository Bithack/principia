#pragma once

#include "edevice.hh"
#include "cable.hh"

/* "Interface" for entities such as the Jumper, Mini transmitter and Receiver */
class wplug : public plug_base, public edevice
{
    int socket_dir;

  public:
    wplug(int socket_dir)
    {
        this->socket_dir = socket_dir;

        this->set_num_properties(3);
        this->properties[1].type = P_ID;
        this->properties[2].type = P_INT;
        /*
         * All plugs must contain at least 3 properties.
         * Property 0 = plug-specific
         * Property 1 = P_ID of connected edevice
         * Property 2 = Socket ID
         */
    }

    float get_angle();
    b2Vec2 get_position();

    void pre_write(void) { this->_pos = entity::get_position(); }

    virtual int connect(edevice *e, isocket *s);
    virtual void disconnect();
    virtual void reconnect();
    virtual void create_body();
    virtual void update_color() {};
};

/* Another interface for wireless plugs only! */
class wireless_plug : public wplug
{
  public:
    wireless_plug(int socket_dir) : wplug(socket_dir) { }

    /**
     * Write quickinfo in the standard "wireless" format:
     * NAME (f:FREQUENCY, id:ID, g_id:G_ID)
     * or
     * NAME (f:FREQUENCY)
     **/
    void write_quickinfo(char *out);

    /**
     * Render the wireless frequency on the plug, if that setting
     * is enabled.
     **/
    void update_effects();

    /**
     * Wireless plugs are always compatible with eachother,
     * so this list needs to be updated as other objects start using
     * the wireless plug interface.
     **/
    bool compatible_with(entity *o);
};
