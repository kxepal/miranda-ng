@echo off

REM You can run this script either from "tools\lpgen\" folder or from "langpacks\<your_langpack>\"

echo ========================
echo Langpack refresher
echo Author: Robyer
echo ========================

echo Set your variables in script and remove this line. & PAUSE & goto end

REM === VARIABLES TO SET ===

set "language=czech"												& REM name of language from "langpacks\" folder
set "useOldLangpack=yes" 											& REM "yes" or "no", if "yes" make sure you correctly set path to old langpack file below
set "oldLangpackPath=Deprecated\old langpack\langpack_czech2.txt" 	& REM old langpack is searched in "langpacks\<your_langpack>\" folder

REM =========================

if "%useOldLangpack%" == "yes" (
	set phase=0
) else (
	set phase=1
)

:start

cd "..\..\tools\lpgen\"

mkdir "Plugins"
mkdir "Untranslated"

if "%phase%" == "0" (
	REM load strings from old langpack
	cscript /nologo translate.js /log:"yes" /out:"Plugins" /untranslated:"Untranslated" /outfile:"langpack_%language%.txt" /path:"..\..\langpacks\%language%\Plugins" /core:"..\..\langpacks\%language%\=CORE=.txt" /langpack:"..\..\langpacks\%language%\%oldLangpackPath%"
) else (
	REM load strings from recently created langpack (also to distribute strings between files)
	cscript /nologo translate.js /log:"yes" /out:"Plugins" /untranslated:"Untranslated" /outfile:"langpack_%language%.txt" /path:"..\..\langpacks\%language%\Plugins" /core:"..\..\langpacks\%language%\=CORE=.txt" /langpack:"..\..\langpacks\%language%\Langpack_%language%.txt"
)
	
rm -r "../../langpacks/%language%/Plugins"
rm -r "../../langpacks/%language%/Untranslated"
rm -r "../../langpacks/%language%/=CORE=.txt"
rm -r "../../langpacks/%language%/Langpack_%language%.txt"

mkdir "..\..\langpacks\%language%\Plugins"
mkdir "..\..\langpacks\%language%\Untranslated"
copy "Plugins\" "..\..\langpacks\%language%\Plugins\"
copy "Untranslated\" "..\..\langpacks\%language%\Untranslated\"
copy "=CORE=.txt" "..\..\langpacks\%language%\=CORE=.txt"

rm -r "Plugins"
rm -r "Untranslated"
rm "=CORE=.txt"
rm "Langpack_%language%.txt"

cd "..\..\langpacks\tool\"
LangpackSuite.exe \q \p%language%

if "%phase%" == "1" (
	goto end
) else (
	set "phase=1"
	goto start
)

:end

REM =========================