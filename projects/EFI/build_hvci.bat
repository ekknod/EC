"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "bootx64/bootx64.sln" /t:Clean,Build /property:Configuration=Release /property:Platform=x64
copy "bootx64\x64\Release\bootx64.efi" .
