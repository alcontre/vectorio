cmake_minimum_required(VERSION 3.15)

project(VectorMMIO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VECTORMMIO_NO_BURST_WRITE
   "Use 32-bit scalar MMIO transfers instead of wider bursts" OFF)

add_library(VectorMMIO STATIC src/VectorMMIO.cpp)

target_include_directories(VectorMMIO
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
)

if(VECTORMMIO_NO_BURST_WRITE)
   target_compile_definitions(VectorMMIO PRIVATE VECTORMMIO_NO_BURST_WRITE)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
   include(CTest)

   if(BUILD_TESTING)
      function(add_vector_mmio_simulation_test max_burst_words)
         set(target_name VectorMMIO_sim_${max_burst_words})
         add_library(${target_name} STATIC src/VectorMMIO.cpp)
         target_include_directories(${target_name}
            PUBLIC
               $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         )
         target_compile_definitions(${target_name}
            PRIVATE
               VECTORMMIO_INSTR_SIM
               VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS=${max_burst_words}
         )

         add_executable(${target_name}_test tests/VectorMMIO_ladder_test.cpp)
         target_link_libraries(${target_name}_test PRIVATE ${target_name})
         target_compile_definitions(${target_name}_test
            PRIVATE
               VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS=${max_burst_words}
         )
         add_test(NAME ${target_name}_test COMMAND ${target_name}_test)
      endfunction()

      add_vector_mmio_simulation_test(1)
      add_vector_mmio_simulation_test(2)
      add_vector_mmio_simulation_test(4)
      add_vector_mmio_simulation_test(8)
      add_vector_mmio_simulation_test(16)
   endif()
endif()