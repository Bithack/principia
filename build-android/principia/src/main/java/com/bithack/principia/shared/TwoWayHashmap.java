package com.bithack.principia.shared;

import java.util.Hashtable;
import java.util.Map;

public class TwoWayHashmap<K extends Object, V extends Object> {

  private Map<K,V> forward = new Hashtable<K, V>();
  private Map<V,K> backward = new Hashtable<V, K>();

  public synchronized void add(K key, V value) {
    forward.put(key, value);
    backward.put(value, key);
  }

  public synchronized V getForward(K key) {
    return forward.get(key);
  }

  public synchronized K getBackward(V key) {
    return backward.get(key);
  }
}
