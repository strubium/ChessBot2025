cmake_minimum_required(VERSION 3.20)
project(ChessBot C)

# Set C standard and optimization
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

# Include current source directory + tinycthread directory
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party/tinycthread)

# Build the library (add tinycthread.c)
add_library(chess STATIC
        chessapi.c
        bitboard.c
        third_party/tinycthread/tinycthread.c   # <-- add tinycthread source here
)

# Build the token counter
add_executable(c_token_count c_token_count.c)

# Build the bot executables
add_executable(bot_exec example_bot.c)
add_executable(bot_exec_template base_bot.c)
add_executable(bot_exec_ai bot_ai.c)

# Link the chess library and math library
target_link_libraries(bot_exec chess m)
target_link_libraries(bot_exec_template chess m)
target_link_libraries(bot_exec_ai chess m)

# Ensure console subsystem for Windows
set_target_properties(bot_exec_ai PROPERTIES LINK_FLAGS "-mconsole")
set_target_properties(bot_exec PROPERTIES LINK_FLAGS "-mconsole")
set_target_properties(bot_exec_template PROPERTIES LINK_FLAGS "-mconsole")

# Run token count after bot_ai build
add_custom_command(TARGET bot_exec_ai
        POST_BUILD
        COMMAND c_token_count ${CMAKE_SOURCE_DIR}/bot_ai.c
        COMMENT "Counting tokens in bot_ai.c..."
)

add_custom_command(TARGET bot_exec_template
        POST_BUILD
        COMMAND c_token_count ${CMAKE_SOURCE_DIR}/base_bot.c
        COMMENT "Counting tokens in bot_ai.c..."
)
