package com.bithack.principia.shared;

public class Level
{
    private int id;
    private String name;
    private int save_id;
    private int level_type;

    public Level(int id, String name)
    {
        this.id = id;
        this.name = name;
    }

    public String get_name()
    {
        return this.name;
    }

    public int get_id()
    {
        return this.id;
    }

    public int get_save_id()
    {
        return this.save_id;
    }

    public int get_level_type()
    {
        return this.level_type;
    }

    public void set_save_id(int save_id)
    {
        this.save_id = save_id;
    }

    public void set_level_type(int level_type)
    {
        this.level_type = level_type;
    }

    @Override
    public String toString()
    {
        return this.get_name();
    }
}
