@echo off
setlocal EnableExtensions
pushd "%~dp0"

echo Cleaning LaTeX auxiliary files...
if exist BaoCaoCode.aux del /Q "BaoCaoCode.aux"
if exist BaoCaoCode.log del /Q "BaoCaoCode.log"
if exist BaoCaoCode.toc del /Q "BaoCaoCode.toc"
if exist BaoCaoCode.lof del /Q "BaoCaoCode.lof"
if exist BaoCaoCode.lot del /Q "BaoCaoCode.lot"
if exist BaoCaoCode.out del /Q "BaoCaoCode.out"
if exist BaoCaoCode.bbl del /Q "BaoCaoCode.bbl"
if exist BaoCaoCode.blg del /Q "BaoCaoCode.blg"
if exist BaoCaoCode.synctex.gz del /Q "BaoCaoCode.synctex.gz"
echo Done cleaning.
echo.

chcp 65001 >nul 2>&1
echo Building LaTeX document (pass 1)...
pdflatex -synctex=1 -interaction=nonstopmode -file-line-error BaoCaoCode.tex
echo.
echo Building LaTeX document (pass 2 - TOC and refs)...
pdflatex -synctex=1 -interaction=nonstopmode -file-line-error BaoCaoCode.tex
echo.
echo Done!
pause
popd
endlocal
