"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "../private - CSGO/km/km.sln" /t:Clean,Build /property:Configuration=Release /property:Platform=x64
copy "..\private - CSGO\km\x64\Release\km.sys" .
..\tools\b2b32.exe km.sys PRIV
copy "map_driver_resource.h" "..\private - CSGO\loader\" /y
del "map_driver_resource.h"
del "km.sys"
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "../private - CSGO/loader/km_loader.sln" /t:Clean,Build /property:Configuration=Release /property:Platform=x64
copy "..\private - CSGO\loader\x64\Release\km_loader.sys" "csgo.bin" /y
pause