project(ChordalityTest)
find_package(Boost REQUIRED COMPONENTS date_time)

include_directories(Boost_INCLUDE_DIRS ../src)

add_executable(test test.cpp)
target_link_libraries(test Boost::date_time)
target_compile_features(test PRIVATE cxx_std_17)