"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "../private - Loader/boot.sln" /t:Clean,Build /property:Configuration=Release /property:Platform=x64
copy "..\private - Loader\x64\Release\bootx64.efi" .
pause