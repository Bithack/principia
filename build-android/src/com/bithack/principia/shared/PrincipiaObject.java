package com.bithack.principia.shared;

public class PrincipiaObject {
    private int id;
    private String name;

    public PrincipiaObject(int id, String name)
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

    @Override
    public String toString()
    {
        return this.get_name();
    }
}
