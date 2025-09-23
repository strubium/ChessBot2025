cmake_minimum_required(VERSION 3.20)
project(ChessBot C)

# Use C23 standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include headers
include_directories(${CMAKE_SOURCE_DIR})

# ---------------------------
# Step 1: Build the library
# ---------------------------
add_library(chess STATIC
        chessapi.c
        bitboard.c
)

# ---------------------------
# Step 2: Build the bot executable
# ---------------------------
add_executable(bot_exec example_bot.c)

# Link bot_exec with chess library and math library
target_link_libraries(bot_exec
        chess
        m   # math library
)

