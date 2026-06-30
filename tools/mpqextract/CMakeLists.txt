add_executable(mpqextract main.c)
target_link_libraries(mpqextract PRIVATE mpqfs)
target_include_directories(mpqextract PRIVATE ${mpqfs_INCLUDE_DIR})
