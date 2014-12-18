#include <map>
#include "gtest/gtest.h"
#include "AllocFreeHashMap.h"

TEST(no_alloc_hashmap, alloc) {
	constexpr size_t max_val = 1023;
	hoard::AllocFreeHashMap my_map{};


	for (int k = 0; k < 3; k++) {

		for (size_t i = 0; i < max_val; i++) {

			hoard::AllocFreeHashMap::key_type key = reinterpret_cast<hoard::AllocFreeHashMap::key_type>( i);
//        hoard::print("key is: ", key, "\n");
			my_map.Add(key, i + 500);
			ASSERT_TRUE(my_map.Contains(key));

			for (size_t j = 0; j <= i; j++) {
				hoard::AllocFreeHashMap::key_type key2 = reinterpret_cast<hoard::AllocFreeHashMap::key_type>( j);
				if (!my_map.Contains(key2)) {
//                hoard::print("after add: ", i, " lost: ", j, "\n");
//                my_map.PrintState();
					ASSERT_TRUE(my_map.Contains(key2));
				}
			}
			for (size_t j = i + 1; j <= max_val; j++) {
				hoard::AllocFreeHashMap::key_type key2 = reinterpret_cast<hoard::AllocFreeHashMap::key_type>( j);
				if (my_map.Contains(key2)) {
//                hoard::print("after add: ", i, " added: ", j, "\n");
					ASSERT_TRUE(!my_map.Contains(key2));
				}
			}
//        hoard::print("\n\n new print ", i," \n \n");

		}

//        my_map.PrintState();
//    hoard::print("hi from gtest!!!\n");
		for (size_t i = 0; i < max_val; i++) {
//        ASSERT_EQ(my_map.size(), max_val - i);
//        hoard::print("beg\n");
			hoard::AllocFreeHashMap::key_type key = (hoard::AllocFreeHashMap::key_type) i;
			ASSERT_TRUE(my_map.Contains(key));

//        hoard::print("after cont\n");
//        hoard::print("iter: ", (size_t)  i, " val: ", my_map.get(key), "\n");
			ASSERT_TRUE(my_map.Get(key) == i + 500);
//        hoard::print(i, my_map.get(key));
			ASSERT_TRUE(my_map.Remove(key));
			for (size_t j = 0; j <= i; j++) {
				hoard::AllocFreeHashMap::key_type key2 = reinterpret_cast<hoard::AllocFreeHashMap::key_type>( j);
				if (my_map.Contains(key2)) {
//                hoard::print("after dell: ", i, " added: ", j,  "\n");
					ASSERT_TRUE(!my_map.Contains(key2));
				}
			}
			for (size_t j = i + 1; j < max_val; j++) {
				hoard::AllocFreeHashMap::key_type key2 = reinterpret_cast<hoard::AllocFreeHashMap::key_type>( j);
				if (!my_map.Contains(key2)) {
//                hoard::print("after del: ", i, " lost: ", j, "\n");
					ASSERT_TRUE(my_map.Contains(key2));
				}
			}
			ASSERT_FALSE(my_map.Remove(key));

		}
	}


}


