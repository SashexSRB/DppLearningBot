cmake_minimum_required(VERSION 4.0)
project(DppBot_Learning)

set(CMAKE_CXX_STANDARD 20)

add_executable(DppBot_Learning main.cpp
        events/interactionCreate.h
        events/ready.h
        includes/dotenv.h
        commands/deps/songRepo.h
)

add_executable(CommandRegistrator register_commands.cpp
        commands/definitions.h
        includes/dotenv.h
        events/events.h
        commands/deps/pbCtrl.h
)

target_link_libraries(DppBot_Learning PRIVATE dpp mpg123)
target_link_libraries(CommandRegistrator PRIVATE dpp mpg123)