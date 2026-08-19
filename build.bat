@echo off
rem ============================================================
rem  Build rtkpost_main.exe (RTKLIB post-processing driver)
rem  Requires: MinGW-W64 gcc + mingw32-make on PATH
rem  NOTE: gcc under a non-ASCII install path (e.g. a Chinese
rem  directory) fails to link; use an ASCII-path build such as
rem  WinGet WinLibs: C:\Users\...\WinGet\Packages\...\mingw64\bin
rem ============================================================
cd /d "%~dp0"

where gcc >nul 2>nul
if errorlevel 1 (
    echo [ERROR] gcc not found. Install MinGW-W64 and add it to PATH.
    exit /b 1
)

if exist Makefile (
    where mingw32-make >nul 2>nul
    if not errorlevel 1 (
        echo ==^> building with mingw32-make ...
        mingw32-make -f Makefile
        if not errorlevel 1 goto :OK
    )
)

echo ==^> building directly with gcc ...
set SRC=..\RTKLIB-rtklib_2.4.3\src
set OPTS=-DTRACE -DENAGLO -DENAQZS -DENAGAL -DENACMP -DENAIRN -DNFREQ=5
set CFLAGS=-O2 -Wall -Wno-unused-but-set-variable -I%SRC% %OPTS%
set SRCS=%SRC%\rtkcmn.c %SRC%\rinex.c %SRC%\rtkpos.c %SRC%\postpos.c %SRC%\solution.c
set SRCS=%SRCS% %SRC%\lambda.c %SRC%\geoid.c %SRC%\sbas.c %SRC%\preceph.c %SRC%\pntpos.c
set SRCS=%SRCS% %SRC%\ephemeris.c %SRC%\options.c %SRC%\ppp.c %SRC%\ppp_ar.c
set SRCS=%SRCS% %SRC%\rtcm.c %SRC%\rtcm2.c %SRC%\rtcm3.c %SRC%\rtcm3e.c %SRC%\ionex.c %SRC%\tides.c
gcc %CFLAGS% rtkpost_main.c %SRCS% -lm -lwinmm -o rtkpost_main.exe
if errorlevel 1 (
    echo [ERROR] build failed
    exit /b 1
)

:OK
echo.
echo build OK: %~dp0rtkpost_main.exe
echo.
echo sample run (single point, RTKLIB test data):
echo   rtkpost_main.exe -p 0 ..\RTKLIB-rtklib_2.4.3\test\data\rinex\07590920.05o ..\RTKLIB-rtklib_2.4.3\test\data\rinex\07590920.05n
echo.
echo full options: rtkpost_main.exe
exit /b 0

