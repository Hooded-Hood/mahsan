//
// Created by hadi on 11/29/23.
//

#include "Bucket.h"
#include <iostream>

template<typename K, typename V>
Bucket<K, V>::Bucket(K key, V value):key(key), value(value) {
    next = NULL;
}

template<typename K, typename V>
Bucket<K, V>::Bucket(const Bucket& b) {
    value = b.value;
    key = b.key;
    next = b.next;
}


