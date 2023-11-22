#include <iostream>
using namespace std;

template <typename K, typename V> class Bucket{
public:
    Bucket(K key, V value) : key(key), value(value) {
        next = NULL;
    };

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

template <typename K, typename V/*, typename F = KeyHash<K>*/> class HashMap {
public:
    HashMap() {
        table = new Bucket<K, V> *[MAX_BUCKETS]();
    }

    virtual ~HashMap() {

    }

    const int hashFunction(const K& key) const { // chera static nmishe
        return (int) key % MAX_BUCKETS;
    }

    V get(const K& key) {
        int index = hashFunction(key);
        Bucket<K, V> *node = table[index];
        while (node != NULL) {
            if (node->getKey() == key) {
                return node->getValue();
            }
            node = node->getNext();
        }
        return V();
    }

    void set(const K& key, const V& value) {
        int index = hashFunction(key);
        Bucket<K, V> *node = table[index]; // aya mishe ba refrence zad beja pointer?
        if (node == NULL) { // bucket is empty
            table[index] = new Bucket<K, V>(key, value);
            return;
        }
        if (node->getKey() == key) { // it is head od bucket
            node->setValue(value);
            table[index] = new Bucket<K, V>(key, value);
            return;
        }
        while (node->getNext() != NULL) { // it is somewhere else
            if (node->getNext()->getKey() == key) {
                node->getNext()->setValue(value);
                return;
            }
            node = node->getNext();
        }
        node->setNext(new Bucket<K, V>(key, value)); // it doesn't exists
    } // ye nokte jazabi ke dare ke sare debugesh dahanam sevice shod inke nmitoonam khod node ro set konam
    // fek konam chon engar vaghan be oon khoone eshare nmikone dar hali ke nextesh mikone

private:
    static const int MAX_BUCKETS = 1e6;
    Bucket<K, V> **table;
    //F hashFunc; // age khastim az function dasti betoonim estefade konim
};

int main() {
    HashMap<int, int> mp;
    mp.set(1, 6);
    mp.set(1, 3);
    cout << mp.get(1);
    return 0;
}
