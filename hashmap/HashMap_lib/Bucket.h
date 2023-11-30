//
// Created by hadi on 11/29/23.
//

#ifndef HASHMAP_BUCKET_H
#define HASHMAP_BUCKET_H

template <typename K, typename V>
class Bucket {
public:
    Bucket(K key, V value);

    Bucket(const Bucket &b);

    K getKey() const {
        return key;
    }

    V getValue() const {
        return value;
    }

    Bucket *getNext() const {
        return next;
    }

    void setValue(V value) {
        Bucket::value = value;
    }

    void setNext(Bucket *next) {
        Bucket::next = next;
    }

private:
    K key;
    V value;
    Bucket *next;
};

#endif //HASHMAP_BUCKET_H
