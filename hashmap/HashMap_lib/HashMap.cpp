//
// Created by hadi on 11/29/23.
//

#include "HashMap.h"
#include "Bucket.h"
#include <iostream>
#include <time.h>

using namespace std;

template<typename K, typename V, typename F>
HashMap<K, V, F>::HashMap() {
    table = new Bucket<K, V> *[capacity]();
    cout << "construct\n";
}

template<typename K, typename V, typename F>
HashMap<K, V, F>::HashMap(const HashMap& hm) {
    table = new Bucket<K, V> *[capacity]();
    for (int i = 0; i < capacity; i++) {
        Bucket<K, V> *cur = hm.table[i];
        while (cur != NULL) {
            this->put(cur->getKey(), cur->getValue());
            cur = cur->getNext();
        }
    }
    cout << "copy\n";
}

template<typename K, typename V, typename F>
HashMap<K, V, F>::HashMap(HashMap&& hm) : table(hm.table) { // why move constructor shod be marked noexcept
    hm.table = NULL;
    cout << "move\n";
}

template<typename K, typename V, typename F>
HashMap<K, V, F>::~HashMap() {
    if (table == NULL) {
        return;
    }
    for (int i = 0; i < capacity; i++) {
        Bucket<K, V> *cur = table[i];
        while (cur != NULL) {
            Bucket<K, V> *nxt = cur->getNext();
            delete cur;
            cur = nxt;
        }
    }
    delete[] table;
    cout << "destruct\n";
}

template<typename K, typename V, typename F>
V HashMap<K, V, F>::get(const K &key) const {
    int index = hashFunc(key) * hashCode % capacity;
    Bucket<K, V> *node = table[index];
    while (node != NULL) {
        if (node->getKey() == key) {
            return node->getValue();
        }
        node = node->getNext();
    }
    return V();
}

template<typename K, typename V, typename F>
V HashMap<K, V, F>::operator[](const K& key) const {
    return get(key);
}

template<typename K, typename V, typename F>
void HashMap<K, V, F>::set(const K &key, const V &value) {
    counter++;
    if (counter > capacity) {
        sizeUp();
    }
    put(key, value);
}
// ye nokte jazabi ke dare ke sare debugesh dahanam sevice shod inke nmitoonam khod node ro set kona
// fek konam chon engar vaghan be oon khoone eshare nmikone dar hali ke nextesh mikone

template<typename K, typename V, typename F>
void HashMap<K, V, F>::put(const K &key, const V &value) {
    int index = hashFunc(key) * hashCode % capacity;
    Bucket<K, V> *node = table[index]; // aya mishe ba refrence zad beja pointer? //fek nakonam chon NULL mikhaym
    if (node == NULL) { // bucket is empty
        table[index] = new Bucket<K, V>(key, value);
        delete node;
        return;
    }
    if (node->getKey() == key) { // it is head of bucket
        table[index]->setValue(value);
        delete node;
        return;
    }
    while (node->getNext() != NULL) { // it is somewhere else
        if (node->getNext()->getKey() == key) {
            node->getNext()->setValue(value);
            delete node;
            return;
        }
        node = node->getNext();
    }
    node->setNext(new Bucket<K, V>(key, value)); // it doesn't exists
    delete node;
}

template<typename K, typename V, typename F>
void HashMap<K, V, F>::sizeUp() {
    srand(time(0));
    hashCode = rand();
    capacity *= 2;
    Bucket<K, V> **newTable = new Bucket<K, V> *[capacity]();
    for (int i = 0; i < capacity; i++) {
        Bucket<K, V> *cur = table[i];
        while (cur != NULL) {
            int index = hashFunc(cur->getKey()) * hashCode % capacity; // in hatekato doss nadaram
            Bucket<K, V> *node = newTable[index];
            if (node == NULL) {
                newTable[index] = new Bucket<K, V>(cur->getKey(), cur->getValue());
                delete node;
            }
            else if (node->getKey() == cur->getKey()) {
                newTable[index]->setValue(cur->getValue());
                delete node; // ina lazeme aya
            }
            else {
                bool done = 0;
                while (node->getNext() != NULL) {
                    if (node->getNext()->getKey() == cur->getKey()) {
                        node->getNext()->setValue(cur->getValue());
                        delete node;
                        done = 1;
                        break;
                    }
                    node = node->getNext();
                }
                if (!done) {
                    node->setNext(new Bucket<K, V>(cur->getKey(), cur->getValue()));
                    delete node;
                }
            }
            cur = cur->getNext();
        }
    }
    table = newTable;
    if (table != NULL) {
        for (int i = 0; i < capacity; i++) {
            Bucket<K, V> *cur = table[i];
            while (cur != NULL) {
                Bucket<K, V> *nxt = cur->getNext();
                delete cur;
                cur = nxt;
            }
        }
        delete[] table;
    }
    table = newTable;
    delete[] newTable;
}