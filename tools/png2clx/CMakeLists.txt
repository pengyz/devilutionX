add_executable(png2clx main.cpp)
target_link_libraries(png2clx PRIVATE 
  libdevilutionx_so
  DevilutionX::SDL
  tl
  SDL2_image
)
target_include_directories(png2clx PRIVATE
  ${CMAKE_SOURCE_DIR}/Source
  ${CMAKE_BINARY_DIR}/Source
  ${CMAKE_SOURCE_DIR}/3rdParty/tl
)
target_link_directories(png2clx PRIVATE ${CMAKE_BINARY_DIR})
