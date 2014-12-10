function(add_gtest name)
  add_executable(${name}
    ${ARGN}
  )
  target_link_libraries(${name} gtest_main)
  add_test(${name} ${name})
endfunction()



add_gtest(custom_hashmap_test
 custom_hashmap_test.cpp
)
target_link_libraries(custom_hashmap_test hoard_core)
