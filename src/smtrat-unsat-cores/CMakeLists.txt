include(${carl_INCLUDE_DIR}/../cmake/carlmacros.cmake)

add_definitions(-fPIC)

configure_everything()

file(GLOB_RECURSE smtrat-unsat-cores_sources "*.cpp")

add_new_libraries(smtrat-unsat-cores ${PROJECT_VERSION_LIB} "${smtrat-unsat-cores_sources}" "resources")
target_include_dirs_from(smtrat-unsat-cores-objects "" PUBLIC smtrat-common-shared carl-arith-shared carl-covering-shared)
target_link_libraries(smtrat-unsat-cores-shared smtrat-common-shared carl-arith-shared carl-covering-shared)
target_link_libraries(smtrat-unsat-cores-static smtrat-common-static carl-arith-static carl-covering-static)

install_libraries(smtrat smtrat-unsat-cores-shared smtrat-unsat-cores-static)

