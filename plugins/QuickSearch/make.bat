@echo off
set p1=%1
set p2=%2
if  "%p1%" == "" (echo "please specify target platform by adding 'fpc' or 'fpc64 parameter to command line!'"&&pause&&goto :EOF)
if  "%p2%" == "" (echo "please specify target output directory by adding 10 for bin10 or 11 for bin11 to command line!'"&&pause&&goto :EOF)
if /i '%1' == 'fpc' (
  set OUTDIR="..\..\bin%2\Release\Plugins"
  set FPCBIN=fpc.exe
) else if /i '%1' == 'fpc64' (
  set OUTDIR="..\..\bin%2\Release64\Plugins"
  set FPCBIN=ppcrossx64.exe
)
set PROJECT=QuickSearch

if not exist %OUTDIR% mkdir %OUTDIR%
md tmp

rem brcc32.exe qs.rc -foqs.res

%FPCBIN% @..\Utils.pas\fpc.cfg %PROJECT%.dpr %3 %4 %5 %6 %7 %8 %9

move .\tmp\%PROJECT%.dll %OUTDIR%
del /Q tmp\*
rd tmp
