//
// Created by hadi on 11/29/23.
//

#ifndef HASHMAP_HASHMAP_H
#define HASHMAP_HASHMAP_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include "Bucket.h"

template <typename K>
struct KeyHash {
    unsigned long operator()(const K& key) const;
};

template<typename K>
unsigned long KeyHash<K>::operator()(const K &key) const {
    return reinterpret_cast<unsigned long>(key);
}

template <typename K, typename V, typename F = KeyHash<K>>
class HashMap {
public:
    HashMap();

    HashMap(const HashMap &hm);

    HashMap(HashMap &&hm);

    virtual ~HashMap(); // why virtual?

    V get(const K &key) const;

    // Overloading the subscript operator for reading values
    V operator[](const K &key) const;

    void set(const K &key, const V &value);

    void sizeUp();

    // Overloading the subscript operator for setting values
/*    V operator[](const K& key) {
        return get(key);
    }
*/

private:
    int counter = 0;
    int capacity = 1;
    int hashCode = 1;
    Bucket<K, V> **table;
    F hashFunc;// = unordered_map<K, V>().hash_function(); // age khastim az function dasti betoonim estefade konim
    void put(const K &key, const V &value);
};

#endif //HASHMAP_HASHMAP_H
