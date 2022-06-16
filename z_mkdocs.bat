@echo off
setlocal EnableDelayedExpansion

:: ------------------------------------------
:: Setup Paths
:: ------------------------------------------

set "PATH_PANDOC=C:\Program Files\Pandoc"
set "PATH_DOXYGN=%~dp0\..\Prerequisites\Doxygen\doxygen.exe"

:: ------------------------------------------
:: Create Documents
:: ------------------------------------------

echo ===========================================================================
echo Building software documentation..."
echo ===========================================================================
echo.

pushd "%~dp0"

for %%i in ("%~dp0\*.md") do (
	echo PANDOC: %%~nxi
	"%PATH_PANDOC%\pandoc.exe" --from markdown_github+pandoc_title_block+header_attributes --to html5 --toc -N --standalone -H "%~dp0\etc\style\style.css" "%%~i" --output "%%~dpni.html"
	if not "!ERRORLEVEL!"=="0" (
		echo.
		echo Something went wrong^^!
		echo.
		pause && exit
	)
)

echo DOXYGEN: Doxyfile
"%PATH_DOXYGN%" "%~dp0\Doxyfile"
if not "!ERRORLEVEL!"=="0" (
	echo.
	echo Something went wrong^^!
	echo.
	pause && exit
)

echo.

pause