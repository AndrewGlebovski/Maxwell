# Executables
add_executable(Server server.cpp)

# Link
target_link_libraries(Server PRIVATE Utils)
