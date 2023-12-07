#include <iostream>
#include "HashMap_lib/HashMap.h"

using namespace std;

int main() {
    HashMap<int, int> mp;
    mp.set(2, 2);
    cout << mp[2];
    return 0;
}
