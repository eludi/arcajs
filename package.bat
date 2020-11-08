make static
upx arcajs.exe
zip -0 tmp.zip .\arcajs.exe
zip -9 -j tmp.zip %1\*
.\zzipsetstub.exe tmp.zip arcajs.exe
del %1.exe
ren tmp.zip %1.exe
