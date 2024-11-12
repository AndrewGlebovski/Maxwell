# Executables
add_executable(Client client.cpp)

# Link
target_link_libraries(Client PRIVATE Utils)
