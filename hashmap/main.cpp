#include <iostream>
#include <vector>

using namespace std;

template <typename K, typename V> class Bucket{
public:
    Bucket(K key, V value) : key(key), value(value) {
        next = NULL;
    };

    Bucket(const Bucket& b) {
        value = b.value;
        key = b.key;
        next = b.next;
    }

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
        cout << "construct\n";
    }

    HashMap(const HashMap& hm) {
        table = new Bucket<K, V> *[MAX_BUCKETS]();
        for (int i = 0; i < MAX_BUCKETS; i++) {
            Bucket<K, V> *cur = hm.table[i];
            while (cur != NULL) {
                this->set(cur->getKey(), cur->getValue());
                cur = cur->getNext();
            }
            cur = NULL;
        }
        cout << "copy\n";
    }

    HashMap(HashMap&& hm) : table(hm.table) {
        hm.table = nullptr;
        cout << "move\n";
    }

    virtual ~HashMap() {
        for (int i = 0; i < MAX_BUCKETS; i++) {
            Bucket<K, V> *cur = table[i];
            while (cur != NULL) {
                Bucket<K, V> *nxt = cur->getNext();
                delete cur;
                cur = nxt;
            }
            cur = NULL;
        }
        delete[] table;
        cout << "destruct\n";
    }

    int hashFunction(const K& key) const { // chera static nmishe
        return (int) key % MAX_BUCKETS;
    }

    V get(const K& key) const {
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

    // Overloading the subscript operator for reading values
    V operator[](const K& key) const {
        return get(key);
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
    } // ye nokte jazabi ke dare ke sare debugesh dahanam sevice shod inke nmitoonam khod node ro set kona
    // fek konam chon engar vaghan be oon khoone eshare nmikone dar hali ke nextesh mikone

    // Overloading the subscript operator for setting values
/*    V operator[](const K& key) {
        return get(key);
    }
*/

private:
    int MAX_BUCKETS = 1e6;
    Bucket<K, V> **table;
    //F hashFunc; // age khastim az function dasti betoonim estefade konim
};

HashMap<int, int> fun(HashMap<int, int> mp) {
    HashMap<int, int> mpt;
    mpt.set(1, mp[1]);
    return mpt;
}
int main() {
    HashMap<int, int> mp;
    mp.set(1, 6);
    mp.set(2, 5);
    mp.set(1, 3);
    //mp[1] = 3;
    HashMap<int, int> mp2(mp);
    mp2.set(1, 4);
    cout << mp[1] << " " << mp2[2];
    HashMap<int, int> mp3 = fun(mp);
    cout << mp3[1] << " " << fun(mp2)[1];
    vector<HashMap<int, int>> vec;
    vec.push_back(mp);
    cout << "checkpoint";
    vec.push_back(fun(mp2));
    return 0;
}
