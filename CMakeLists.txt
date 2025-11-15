cmake_minimum_required(VERSION 3.13)
project(vknextchain)

if(NOT DEFINED VKNEXTCHAIN_PULL_IN_VULKAN_HEADERS)
  option(VKNEXTCHAIN_PULL_IN_VULKAN_HEADERS "Vulkan-Headers as a transitive dependency" ON)
endif()

add_library(vknextchain INTERFACE)
target_include_directories(vknextchain INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(TARGET vulkan)
  target_link_libraries(vknextchain INTERFACE vulkan)
endif()

if(VKNEXTCHAIN_PULL_IN_VULKAN_HEADERS)
  add_subdirectory(Vulkan-Headers)
  if(TARGET Vulkan-Headers)
    target_link_libraries(vknextchain INTERFACE Vulkan-Headers)
  endif()
endif()
