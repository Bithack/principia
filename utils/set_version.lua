#!/usr/bin/luajit

-- set_version.lua: Helper script for updating Principia version info.


-- Helper functions
--------------------------------------------------------------------------------

-- Split string on separator
local function split(inputstr, sep)
	local t = {}
	for str in string.gmatch(inputstr, '([^'..sep..']+)') do
		table.insert(t, str)
	end
	return t
end

-- Split up a version name into a table with digits.
local function split_vername(vername)
	-- Also trim any trash we may have on the version name.
	return split(vername:gsub(' Beta', ''):gsub('%s+', ''), '.')
end

-- Turn a version table with digits into a combined string.
local function combine_verdigits(verdigits)
	return verdigits[1]..'.'..verdigits[2]..'.'..verdigits[3]
end

-- Read lines into a table from a filepath.
local function read_lines_from_file(path)
	local file = io.open(path, 'r')
	local filecontent = {}
	for line in file:lines() do
		filecontent[#filecontent+1] = line
	end
	file:close()

	return filecontent
end

-- Write lines as a table to a filepath.
local function write_lines_to_file(path, lines)
	local file = io.open(path, 'w')
	file:write(table.concat(lines, "\n").."\n")
	file:close()
end

-- Just write a string to a file.
local function write_to_file(path, text)
	local file = io.open(path, 'w')
	file:write(text)
	file:close()
end

-- Replace lines that contain 'needle' in them.
--
-- **Arguments:**
-- - `lines`: Table of lines, which will be modified in-place by the function
-- - `replaces`: Table of replacements, the key is the needle that will be
--   checked for in the line, and the value is the replacement string FOR
--   THE ENTIRE LINE!
local function lines_replace(lines, replaces)
	for i,line in ipairs(lines) do
		for needle,replacement in pairs(replaces) do
			if line:find(needle) then
				lines[i] = replacement
			end
		end
	end
end


-- Functions for writing to files to update version information...
--------------------------------------------------------------------------------

local version_info_txt = 'packaging/version_info.txt'
local gradle_build = 'android/principia/build.gradle'
local nsi_file = 'packaging/principia_install.nsi'
local rc_file = 'packaging/principia.rc'

-- Read the current version info from version_info.txt
local function read_version_info()
	local file = io.open(version_info_txt)
	local current_version = {}
	local i = 1
	for line in file:lines() do
		if i == 2 then current_version.code = line end
		if i == 3 then current_version.name = line end

		i = i + 1
	end
	return current_version
end

-- Write `verinfo` into version_info.txt
local function write_version_info(verinfo)
	print("Writing version_info.txt...")

	write_lines_to_file(version_info_txt, {
		'# This file is used by set_version.lua to keep track of version info.'
			..' DO NOT EDIT IT MANUALLY, USE THE SCRIPT!',
		verinfo.code, verinfo.name
	})
end

-- Update the version code and version name in the Android Gradle build file.
local function update_android_version(verinfo)
	print("Updating Android version metadata...")

	local lines = read_lines_from_file(gradle_build)

	lines_replace(lines, {
		['versionCode'] = '        versionCode '..verinfo.code,
		['versionName'] = '        versionName "'..verinfo.name..'"'
	})

	write_lines_to_file(gradle_build, lines)
end

-- Write the version with the appropriate version info (`version.hh`).
local function write_version_header(verinfo)
	print("Writing version header...")

	local content = string.format([[
#pragma once

#ifdef DEBUG
	#define VER_EXTRA " [Debug]"
#else
	#define VER_EXTRA
#endif

#define PRINCIPIA_VERSION_CODE %s
#define PRINCIPIA_VERSION_STRING "%s" VER_EXTRA
]], verinfo.code, verinfo.name)

	write_to_file('src/version_info.hh', content)
end

-- Update version info in the Windows NSIS installer file (`principia_install.nsi`).
local function update_nsis_version(verinfo)
	print("Updating NSIS installer version...")

	local verdigits = split_vername(verinfo.name)
	local lines = read_lines_from_file(nsi_file)

	lines_replace(lines, {
		['!define VER_MAJOR'] = '!define VER_MAJOR '..verdigits[1],
		['!define VER_MINOR'] = '!define VER_MINOR '..verdigits[2],
		['!define VER_BUILD'] = '!define VER_BUILD '..verdigits[3],
		['!define VERSION']   = '!define VERSION "' ..verinfo.name..'"',
	})

	write_lines_to_file(nsi_file, lines)
end

-- Update version info in the Windows executable resource file (`principia.rc`)
local function update_windows_resource(verinfo)
	print("Updating Windows resource file...")

	local verdigits = split_vername(verinfo.name)
	local lines = read_lines_from_file(rc_file)

	lines_replace(lines, {
		['#define VER_MAJOR'] = '#define VER_MAJOR '..verdigits[1],
		['#define VER_MINOR'] = '#define VER_MINOR '..verdigits[2],
		['#define VER_PATCH'] = '#define VER_PATCH '..verdigits[3],

		['#define VER_STRING'] =
			'#define VER_STRING "'..combine_verdigits(verdigits)..'"',

		['#define PRODUCT_VER_STRING'] =
			'#define PRODUCT_VER_STRING "'..verinfo.name..'"'
	})

	write_lines_to_file(rc_file, lines)
end


-- Entrypoint functions...
--------------------------------------------------------------------------------

-- Main local function
local function main()
	print("set_version.lua - Helper script for updating Principia version info")
	print("-------------------------------------------------------------------")

	local cur_version_info = read_version_info()

	print("- Current version code: "..cur_version_info.code)
	print("- Current version name: "..cur_version_info.name)

	local newver = {}
	print("\nInput the new version code: ")
	newver.code = io.read('*l')
	print("Input the new version name: ")
	newver.name = io.read('*l')

	if newver.code == '' or newver.name == '' then
		print("Empty inputs, aborting.")
		return
	end

	print("")

	update_android_version(newver)
	write_version_header(newver)
	update_nsis_version(newver)
	update_windows_resource(newver)
	write_version_info(newver)

	print("")

	print("Done. Check the changes with git diff and commit when you're ready.")
	print("For more information about making a new Principia release see")
	print("https://principia-web.se/wiki/Making_a_Release")
end

local function test()
	print("Running set_version.lua tests...")

	-- Test by setting a custom version and then resetting,
	-- there should be no results in the Git diff by this if it works.
	local versions = {
		{code = 621, name = '6.2.1'},
		read_version_info()
	}

	for _,ver in ipairs(versions) do
		print("Setting to: ", ver.code, ver.name)

		update_android_version(ver)
		write_version_header(ver)
		update_nsis_version(ver)
		update_windows_resource(ver)
		write_version_info(ver)
	end
end

if #arg == 0 then
	main()
elseif #arg == 1 and arg[1] == "run_test_func" then
	test()
end
