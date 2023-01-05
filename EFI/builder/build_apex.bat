"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "../private - APEX/km/km.sln" /t:Clean,Build /property:Configuration=Release /property:Platform=x64
copy "..\private - APEX\km\x64\Release\km.sys" .
..\tools\b2b32.exe km.sys PRIV
copy "map_driver_resource.h" "..\private - APEX\loader\" /y
del "km.sys"
del "map_driver_resource.h"
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "../private - APEX/loader/km_loader.sln" /t:Clean,Build /property:Configuration=Release /property:Platform=x64
copy "..\private - APEX\loader\x64\Release\km_loader.sys" "apex.bin" /y
pause

