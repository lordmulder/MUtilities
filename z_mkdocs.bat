@echo off
setlocal EnableDelayedExpansion

:: ------------------------------------------
:: Setup Paths
:: ------------------------------------------

set "PATH_PANDOC=C:\Program Files (x86)\Pandoc"

:: ------------------------------------------
:: Create Documents
:: ------------------------------------------

echo ===========================================================================
echo Building software documentation..."
echo ===========================================================================
echo.

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

echo.
