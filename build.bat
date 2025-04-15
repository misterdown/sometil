@echo off
:: ===== Configuration =====
set COMPILER=gcc
set STANDARD=c99
set OUTPUT=sometil.exe
set WARNINGS=-Wall -Wextra
set ERRORS=-Werror
set OPTIMIZATION=-O3

:: Source files (space-separated)
set SOURCES=src/main.c src/arena_allocator.c src/ifstream.c src/utf8_util.c src/dynamic_string.c

:: ===== Building =====
echo Building %OUTPUT% with %COMPILER% %STANDARD%...
%COMPILER% %SOURCES% -o %OUTPUT% -std=%STANDARD% %WARNINGS% %ERRORS% %OPTIMIZATION%

:: ===== Check success =====
if %errorlevel% equ 0 (
  echo Success: %OUTPUT%
  for %%F in (%OUTPUT%) do (
    set size=%%~zF
    echo Size: %size% bytes
  )
) else (
  echo Failed to build sometil
  exit /b 1
)