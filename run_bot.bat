@echo off
REM Change this path to your Cute Chess CLI
set CUTECHESS="C:\Users\hudso\CLionProjects\untitled\cutechess\cutechess-cli.exe"

REM Path to your engine
set ENGINE_EXE="C:\Users\hudso\CLionProjects\untitled\cmake-build-debug\bot_exec.exe"

REM Run a game: Bot1 vs Bot2, 1 minute per side
%CUTECHESS% ^
 -engine cmd=%ENGINE_EXE% proto=uci name=Bot1 ^
 -engine cmd=%ENGINE_EXE% proto=uci name=Bot2 ^
 -each tc=1+0 ^
 -pgnout game.pgn

pause