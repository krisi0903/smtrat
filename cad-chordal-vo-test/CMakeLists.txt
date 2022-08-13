project(ChordalityTest)
find_package(Boost)

include_directories(Boost_INCLUDE_DIRS ../src)

add_executable(test test.cpp)
target_compile_features(test PRIVATE cxx_std_17)