//
// Created by hadi on 11/29/23.
//
#include "gtest/gtest.h"
// #include "HashMap.h"
#include "../HashMap_lib/HashMap.h"// ye jaye kar milange. bejaye in balayeesh bayad kar mikard

TEST(StringHash, 1) {
    HashMap<std::string, std::string> mp;
    mp.set("Hey", "Hello");
    EXPECT_EQ(mp["Hey"], "Hello");
}

template <typename K>
struct myKeyHash {
    unsigned long operator()(const K& key) const{
        return 1;
    }
};

TEST(Conflict, 2) {
    HashMap<int, int, myKeyHash<int>> mp;
    mp.set(1, 1);
    mp.set(2, 2);
    EXPECT_NE(mp[1], mp[2]);
    EXPECT_NE(mp[1], 1);
    EXPECT_NE(mp[1], 2);
}

struct myIntKeyHash {
    unsigned long operator()(const int& key) const{
        return key;
    }
};

TEST(CustomizedHash, 3) {
    HashMap<int, int, myKeyHash<int>> mp;
    mp.set(1, 1);
    mp.set(2, 2);
    EXPECT_NE(mp[1], 1);
    EXPECT_NE(mp[1], 2);
}

HashMap<int, int> fun(HashMap<int, int> mp) {
    HashMap<int, int> mpt;
    mp.set(1, 3);
    mpt.set(1, mp[1]);
    return mpt;
}

TEST(CopyCunstructor, 4) {
    HashMap<int, int> mp;
    mp.set(1, 1);
    mp.set(2, 1);
    HashMap<int, int> mp2(mp);
    EXPECT_EQ(mp2[1], 1);
    EXPECT_EQ(mp2[2], 1);
    mp2.set(1, 2);
    EXPECT_EQ(mp[1], 1);
    EXPECT_EQ(mp2[1], 2);
    HashMap<int, int> mp3 = fun(mp);
    EXPECT_EQ(mp[1], 1);
    EXPECT_EQ(mp3[1], 3);
    EXPECT_EQ(fun(mp)[1], 3);
}

TEST(MoveCunstructor, 5) {
    HashMap<int, int> mp;
    HashMap<int, int> mp2;
    std::vector<HashMap<int, int>> vec;
    vec.push_back(mp);
    std::cout << "checkpoint";
    vec.push_back(fun(mp2));
    EXPECT_EQ(vec[1][1], 3);
}