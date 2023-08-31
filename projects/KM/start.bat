sc create ec-csgo binPath="%~dp0\x64\Release\km.sys" type=kernel
sc start ec-csgo
pause
sc stop ec-csgo
sc delete ec-csgo

