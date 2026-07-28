add_executable(cel2png main.cpp)
target_link_libraries(cel2png PRIVATE 
  libdevilutionx_so
  DevilutionX::SDL
  tl
)
target_include_directories(cel2png PRIVATE
  ${CMAKE_SOURCE_DIR}/Source
  ${CMAKE_BINARY_DIR}/Source
  ${CMAKE_SOURCE_DIR}/3rdParty/tl
)
target_link_directories(cel2png PRIVATE ${CMAKE_BINARY_DIR})
