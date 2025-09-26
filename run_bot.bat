@echo off
REM Change this path to your Cute Chess CLI
set CUTECHESS="C:\Users\hudso\CLionProjects\untitled\cutechess\cutechess-cli.exe"

REM Path to your bot engine
set BOT_EXE="C:\Users\hudso\CLionProjects\untitled\cmake-build-debug\bot_exec.exe"

REM Path to Stockfish engine
set STOCKFISH_EXE="C:\Users\hudso\CLionProjects\untitled\stockfish\stockfish-windows-x86-64-avx2.exe"

REM Number of games to play
set NUM_GAMES=50

REM Loop to run multiple games
for /L %%i in (1,1,%NUM_GAMES%) do (
    echo Running game %%i...
    %CUTECHESS% ^
    -engine cmd=%BOT_EXE% proto=uci name=Bot ^
    -engine cmd=%STOCKFISH_EXE% proto=uci name=Stockfish ^
    -each tc=10+0 ^
    -repeat ^
    -pgnout "game_%%i.pgn"
)

pause
