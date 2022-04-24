project(Epoll_socket)

add_executable(server server.cpp)
TARGET_LINK_LIBRARIES(server pthread)

add_executable(client client.cpp)
