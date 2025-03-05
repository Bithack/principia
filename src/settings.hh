#pragma once

#include <map>
#include <cstring>
#include <inttypes.h>
#include <tms/backend/print.h>

enum {
    S_INT8,
    S_INT32,
    S_UINT8,
    S_UINT32,
    S_FLOAT,
    S_BOOL,
};

#define TUTORIAL_REPAIR_STATION      (1UL << 0UL)
#define TUTORIAL_FACTORY             (1UL << 1UL)

#define TUTORIAL_PICKUP_EQUIPMENT    (1UL << 2UL)
#define TUTORIAL_REPAIR_STATION_DROP (1UL << 3UL)
#define TUTORIAL_ZAP_WOOD            (1UL << 4UL)
#define TUTORIAL_CAVE_FIRST_TIME     (1UL << 5UL)
#define TUTORIAL_BUILD_LADDERS       (1UL << 6UL)

struct cmp_str
{
   bool operator()(char const *a, char const *b) const
   {
      return strcmp(a, b) < 0;
   }
};

class setting
{
  public:
    uint8_t type;
    union {
        int8_t   i8;
        int32_t  i;
        uint8_t  u8;
        uint32_t u32;
        float    f;
        int8_t   b;
    } v;
    bool clamp;

    setting()
    {
        this->clear();
    }

    setting(uint8_t type)
    {
        this->clear();
        this->type = type;
    }

    void set(int val)
    {
        this->v.i = val;
    }

    void set(uint8_t val)
    {
        this->v.u8 = val;
    }

    bool set(float val)
    {
        bool changed = (this->v.f != val);
        this->v.f = val;
        return changed;
    }

    bool set(bool val)
    {
        bool changed = (this->v.b != val);
        this->v.b = val ? 1 : 0;
        return changed;
    }

    // "bool" operators
    inline bool is_true()
    {
        return (this->v.b > 0);
    }

    inline bool is_false()
    {
        return (this->v.b == 0);
    }

    inline bool is_uninitialized()
    {
        return (this->v.b == -1);
    }

    void clear()
    {
        this->type = 0;
        this->clamp = false;
        memset(&this->v, 0, sizeof(this->v));
    }
};

extern class _settings {
  private:
    std::map<const char*, setting*, cmp_str> _data;

    std::map<const char*, setting*>::iterator add(const char* key)
    {
        std::pair<std::map<const char*, setting*>::iterator, bool> ret;
        ret = this->_data.insert(std::pair<const char*, setting*>(key, new setting()));

        if (ret.second) {
            return (ret.first);
        }

        /* XXX: error handling */
        return this->_data.end();
    }

  public:

    setting*& operator[] (const char* key) {
#ifdef DEBUG
        tms_assertf(this->_data.find(key) != this->_data.end(), "No setting with key '%s' found", key);
#endif
        return this->_data[key];
    }

    bool isset(const char *key)
    {
        return this->_data.find(key) != this->_data.end();
    }

    void add(const char* key, uint32_t type, int val)
    {
        std::map<const char*, setting*>::iterator it = this->add(key);
        it->second->type = type;

        switch (type) {
            case S_BOOL:
                it->second->v.b = (int8_t)val;
                break;

            default:
                it->second->v.i = val;
                break;
        }
    }

    void addu8(const char* key, uint32_t type, uint8_t val)
    {
        std::map<const char*, setting*>::iterator it = this->add(key);
        it->second->type = type;
        it->second->v.u8 = val;
    }

    void add(const char* key, uint32_t type, float val, bool clamp=false)
    {
        std::map<const char*, setting*>::iterator it = this->add(key);
        it->second->type = type;
        it->second->v.f = val;
        it->second->clamp = clamp;
    }

    void add(const char* key, uint32_t type, bool val)
    {
        std::map<const char*, setting*>::iterator it = this->add(key);
        it->second->type = type;
        it->second->v.i8 = val ? 1 : 0;
    }

    void init();
    bool load();
    bool save();
    void clean();

    ~_settings();
} settings;
