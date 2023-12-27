project(halsim_socketcan)

include(CompileWarnings)

file(GLOB halsim_socketcan_src src/main/native/cpp/*.cpp)

add_library(halsim_socketcan SHARED ${halsim_socketcan_src})
wpilib_target_warnings(halsim_socketcan)
set_target_properties(halsim_socketcan PROPERTIES DEBUG_POSTFIX "d")
target_link_libraries(halsim_socketcan PUBLIC hal wpinet)

target_include_directories(halsim_socketcan PRIVATE src/main/native/include)

set_property(TARGET halsim_socketcan PROPERTY FOLDER "libraries")

install(TARGETS halsim_socketcan EXPORT halsim_socketcan DESTINATION "${main_lib_dest}")
