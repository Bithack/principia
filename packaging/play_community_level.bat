@echo off
setlocal enabledelayedexpansion

set domain=null
set level=null

goto main


:select_domain
	:principia_web
		set domain=principia-web.se
		goto select_level

	:archive_principia_web
		set domain=archive.principia-web.se
		goto select_level

	:custom
		cls
		echo.
		echo Enter the domain of the community site you want to use.
		echo (Example: grejer.voxelmanip.se, NO "https://" or trailing slashes!)
		echo.
		set /p domain=^>

		if "!domain!" NEQ "" goto select_level

		goto custom


:select_level
	cls
	echo.
	echo Enter the numeric ID of the level you want to play.
	echo.
	echo (To find a level's ID, see the URL "/level/<ID>", or
	echo the "Level ID: <ID>" below the level's thumbnail.)
	echo.
	set /p level=^>

	if "!level!" NEQ "" goto run_principia

	goto select_level


:run_principia
	REM | See https://principia-web.se/wiki/Principia_Protocol for info about the protocol
	start principia.exe principia://!domain!/play/lvl/db/!level! > nul
	goto select_level


:main
	echo.
	echo This is a helper script to allow playing levels with a portable version of
	echo Principia without needing to install registry keys.
	echo.
	echo For more info about the portable Windows build of Principia see:
	echo https://principia-web.se/wiki/Windows_Portable

	echo.
	echo Select a community site domain:
	echo 1. principia-web.se
	echo 2. archive.principia-web.se
	echo 3. Custom domain
	echo.

	:main_loop
		choice /c 123 /n

		if %ERRORLEVEL% == 1 goto principia_web
		if %ERRORLEVEL% == 2 goto archive_principia_web
		if %ERRORLEVEL% == 3 goto custom

	goto main_loop
