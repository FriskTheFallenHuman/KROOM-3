# - Find the Windows SDK aka Platform SDK
#
# Relevant Wikipedia article: http://en.wikipedia.org/wiki/Microsoft_Windows_SDK
#
# Pass "COMPONENTS tools" to ignore Visual Studio version checks: in case
# you just want the tool binaries to run, rather than the libraries and headers
# for compiling.
#
# Variables set:
#  WINDOWSSDK_FOUND
#  WINDOWSSDK_LATEST_DIR
#  WINDOWSSDK_LATEST_NAME
#  WINDOWSSDK_FOUND_PREFERENCE
#  WINDOWSSDK_PREFERRED_DIR
#  WINDOWSSDK_PREFERRED_NAME
#  WINDOWSSDK_DIRS
#  WINDOWSSDK_PREFERRED_FIRST_DIRS
#
# Functions:
#  windowssdk_name_lookup(<directory> <outvar>)
#  windowssdk_build_lookup(<directory> <outvar>)
#  get_windowssdk_from_component(<file or dir> <outvar>)
#  get_windowssdk_library_dirs(<directory> <outvar>)
#  get_windowssdk_library_dirs_multiple(<outvar> <directory> ...)
#  get_windowssdk_include_dirs(<directory> <outvar>)
#  get_windowssdk_include_dirs_multiple(<outvar> <directory> ...)
#
# Original Author:
# 2012 Rylie Pavlik <rylie@ryliepavlik.com>
# https://ryliepavlik.com/
# Iowa State University HCI Graduate Program/VRAC
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
# SPDX-License-Identifier: BSL-1.0

set(_preferred_sdk_dirs) # pre-output
set(_win_sdk_dirs) # pre-output
set(_win_sdk_versanddirs) # pre-output
set(_win_sdk_buildsanddirs) # pre-output
set(_winsdk_vistaonly) # search parameters
set(_winsdk_kits) # search parameters

set(_WINDOWSSDK_ANNOUNCE OFF)
if(NOT WINDOWSSDK_FOUND AND (NOT WindowsSDK_FIND_QUIETLY))
	set(_WINDOWSSDK_ANNOUNCE ON)
endif()
macro(_winsdk_announce)
	if(_WINSDK_ANNOUNCE)
		message(STATUS ${ARGN})
	endif()
endmacro()

# Known Windows 10 SDK build numbers (order newest first)
set(_winsdk_win10vers
	10.0.26100.0
	10.0.22621.0
	10.0.22000.0
	10.0.20348.0
	10.0.19041.0
	10.0.18362.0 # Win10 1903
	10.0.17763.0 # Win10 1809
	10.0.17134.0 # Redstone 4
	10.0.17133.0
	10.0.16299.0 # Redstone 3
	10.0.15063.0 # Redstone 2
	10.0.14393.0 # Redstone 1
	10.0.10586.0 # TH2
	10.0.10240.0 # Win10 RTM
	10.0.10150.0
	10.0.10056.0
)

if(WindowsSDK_FIND_COMPONENTS MATCHES "tools")
	set(_WINDOWSSDK_IGNOREMSVC ON)
	_winsdk_announce("Checking for tools from Windows/Platform SDKs...")
else()
	set(_WINDOWSSDK_IGNOREMSVC OFF)
	_winsdk_announce("Checking for Windows/Platform SDKs...")
endif()

function(_winsdk_conditional_append _vername _build _path)
	if(("${_path}" MATCHES "registry") OR (NOT EXISTS "${_path}"))
		return()
	endif()
	list(FIND _win_sdk_dirs "${_path}" _win_sdk_idx)
	if(_win_sdk_idx GREATER -1)
		return()
	endif()
	_winsdk_announce( " - ${_vername}, Build ${_build} @ ${_path}")
	list(APPEND _win_sdk_dirs "${_path}")
	set(_win_sdk_dirs "${_win_sdk_dirs}" CACHE INTERNAL "" FORCE)
	list(APPEND _win_sdk_versanddirs "${_vername}" "${_path}")
	set(_win_sdk_versanddirs "${_win_sdk_versanddirs}" CACHE INTERNAL "" FORCE)
	list(APPEND _win_sdk_buildsanddirs "${_build}" "${_path}")
	set(_win_sdk_buildsanddirs "${_win_sdk_buildsanddirs}" CACHE INTERNAL "" FORCE)
endfunction()

function(_winsdk_conditional_append_preferred _info _path)
	if(("${_path}" MATCHES "registry") OR (NOT EXISTS "${_path}"))
		return()
	endif()
	get_filename_component(_path "${_path}" ABSOLUTE)
	list(FIND _win_sdk_preferred_sdk_dirs "${_path}" _win_sdk_idx)
	if(_win_sdk_idx GREATER -1)
		return()
	endif()
	_winsdk_announce( " - Found \"preferred\" SDK ${_info} @ ${_path}")
	list(APPEND _win_sdk_preferred_sdk_dirs "${_path}")
	set(_win_sdk_preferred_sdk_dirs "${_win_sdk_dirs}" CACHE INTERNAL "" FORCE)
	_winsdk_conditional_append("${_info}" "" "${_path}")
endfunction()

function(_winsdk_check_microsoft_sdks_registry _winsdkver)
	set(SDKKEY "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\${_winsdkver}")
	get_filename_component(_sdkdir "[${SDKKEY};InstallationFolder]" ABSOLUTE)
	set(_sdkname "Windows SDK ${_winsdkver}")
	set(_build ${ARGN})
	get_filename_component(_sdkproductname "[${SDKKEY};ProductName]" NAME)
	if(NOT "${_sdkproductname}" MATCHES "registry")
		set(_sdkname "${_sdkname} (${_sdkproductname})")
	endif()
	get_filename_component(_sdkver "[${SDKKEY};ProductVersion]" NAME)
	if(NOT "${_sdkver}" MATCHES "registry" AND NOT MATCHES)
		if(NOT "${_sdkver}" MATCHES "\\.\\.")
			set(_build ${_sdkver})
			if(NOT "${_sdkname}" MATCHES "${_sdkver}")
				set(_sdkname "${_sdkname} (${_sdkver})")
			endif()
		endif()
	endif()
	_winsdk_conditional_append("${_sdkname}" "${_build}" "${_sdkdir}")
endfunction()

function(_winsdk_check_windows_kits_registry _winkit_name _winkit_build _winkit_key)
	get_filename_component(_sdkdir
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;${_winkit_key}]"
		ABSOLUTE)
	_winsdk_conditional_append("${_winkit_name}" "${_winkit_build}" "${_sdkdir}")
endfunction()

function(_winsdk_check_win10_kits _winkit_build)
	get_filename_component(_sdkdir
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]"
		ABSOLUTE)
	if(("${_sdkdir}" MATCHES "registry") OR (NOT EXISTS "${_sdkdir}"))
		return()
	endif()
	if(EXISTS "${_sdkdir}/Include/${_winkit_build}/um")
		_winsdk_conditional_append("Windows Kits 10 (Build ${_winkit_build})" "${_winkit_build}" "${_sdkdir}")
	endif()
endfunction()

function(_winsdk_check_platformsdk_registry _platformsdkname _build _platformsdkguid)
	foreach(_winsdk_hive HKEY_LOCAL_MACHINE HKEY_CURRENT_USER)
		get_filename_component(_sdkdir
			"[${_winsdk_hive}\\SOFTWARE\\Microsoft\\MicrosoftSDK\\InstalledSDKs\\${_platformsdkguid};Install Dir]"
			ABSOLUTE)
		_winsdk_conditional_append("${_platformsdkname} (${_build})" "${_build}" "${_sdkdir}")
	endforeach()
endfunction()

# Determine toolchain to know if Vista‑only SDKs are allowed
set(_winsdk_vistaonly_ok OFF)
if(MSVC AND NOT _WINDOWSSDK_IGNOREMSVC)
	if(MSVC_VERSION LESS 1700)
	elseif("${CMAKE_VS_PLATFORM_TOOLSET}" MATCHES "_xp")
	elseif("${CMAKE_VS_PLATFORM_TOOLSET}" STREQUAL "v100" OR "${CMAKE_VS_PLATFORM_TOOLSET}" STREQUAL "v90")
	else()
		set(_winsdk_vistaonly_ok ON)
		if(_WINDOWSSDK_ANNOUNCE AND NOT _WINDOWSSDK_VISTAONLY_PESTERED)
			set(_WINDOWSSDK_VISTAONLY_PESTERED ON CACHE INTERNAL "" FORCE)
			message(STATUS "FindWindowsSDK: Detected Visual Studio 2012 or newer, not using the _xp toolset variant: including SDK versions that drop XP support in search!")
		endif()
	endif()
endif()
if(_WINDOWSSDK_IGNOREMSVC)
	set(_winsdk_vistaonly_ok ON)
endif()

set(_winsdk_msvc_greater_1200 OFF)
if(_WINDOWSSDK_IGNOREMSVC OR (MSVC AND (MSVC_VERSION GREATER 1200)))
	set(_winsdk_msvc_greater_1200 ON)
endif()
set(_winsdk_msvc_greater_1310 OFF)
if(_WINDOWSSDK_IGNOREMSVC OR (MSVC AND (MSVC_VERSION GREATER 1310)))
	set(_winsdk_msvc_greater_1310 ON)
endif()
set(_winsdk_msvc_less_1600 OFF)
if(_WINDOWSSDK_IGNOREMSVC OR (MSVC AND (MSVC_VERSION LESS 1600)))
	set(_winsdk_msvc_less_1600 ON)
endif()
set(_winsdk_msvc_not_less_1800 OFF)
if(_WINDOWSSDK_IGNOREMSVC OR (MSVC AND (NOT MSVC_VERSION LESS 1800)))
	set(_winsdk_msvc_not_less_1800 ON)
endif()

# --- Start search ---
if(_winsdk_msvc_greater_1310)
	# Preferred: environment variable
	if(EXISTS "$ENV{WindowsSDKDir}" AND (NOT "$ENV{WindowsSDKDir}" STREQUAL ""))
		_winsdk_conditional_append_preferred("WindowsSDKDir environment variable" "$ENV{WindowsSDKDir}")
	endif()
	if(_winsdk_msvc_less_1600)
		get_filename_component(_sdkdir "[HKEY_CURRENT_USER\\Software\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]" ABSOLUTE)
		_winsdk_conditional_append_preferred("Per-user current Windows SDK" "${_sdkdir}")
		get_filename_component(_sdkdir "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]" ABSOLUTE)
		_winsdk_conditional_append_preferred("System-wide current Windows SDK" "${_sdkdir}")
	endif()

	# Scan known SDK versions
	if(_winsdk_vistaonly_ok AND _winsdk_msvc_not_less_1800)
		_winsdk_check_microsoft_sdks_registry(v10.0A)
		_winsdk_check_microsoft_sdks_registry(v10.0 10.0.10240.0)
		foreach(_win10build ${_winsdk_win10vers})
			_winsdk_check_win10_kits(${_win10build})
		endforeach()
	endif()

	_winsdk_check_microsoft_sdks_registry(v8.1A 8.1.51636)
	if(_winsdk_vistaonly_ok AND _winsdk_msvc_not_less_1800)
		_winsdk_check_microsoft_sdks_registry(v8.1 8.1.25984.0)
		_winsdk_check_windows_kits_registry("Windows Kits 8.1" 8.1.25984.0 KitsRoot81)
	endif()

	if(_winsdk_vistaonly_ok)
		_winsdk_check_microsoft_sdks_registry(v8.0A 8.0.50727)
		_winsdk_check_microsoft_sdks_registry(v8.0 6.2.9200.16384)
		_winsdk_check_windows_kits_registry("Windows Kits 8.0" 6.2.9200.16384 KitsRoot)
	endif()

	_winsdk_check_microsoft_sdks_registry(v7.1A 7.1.51106)
	if(_winsdk_vistaonly_ok)
		_winsdk_check_microsoft_sdks_registry(v7.1 7.1.7600.0.30514)
	endif()

	_winsdk_check_microsoft_sdks_registry(v7.0A 6.1.7600.16385)
	_winsdk_check_microsoft_sdks_registry(v7.0 6.1.7600.16385)
	_winsdk_check_microsoft_sdks_registry(v6.1 6.1.6000.16384.10)
	_winsdk_check_microsoft_sdks_registry(v6.0A 6.1.6723.1)
	_winsdk_check_microsoft_sdks_registry(v6.0 6.0.6000.16384)
endif()

if(_winsdk_msvc_greater_1200)
	_winsdk_check_platformsdk_registry("Microsoft Platform SDK for Windows Server 2003 R2" "5.2.3790.2075.51" "D2FF9F89-8AA2-4373-8A31-C838BF4DBBE1")
	_winsdk_check_platformsdk_registry("Microsoft Platform SDK for Windows Server 2003 SP1" "5.2.3790.1830.15" "8F9E5EF3-A9A5-491B-A889-C58EFFECE8B3")
endif()

# Re-check preferred (in case they appear later)
if(_winsdk_msvc_greater_1310)
	if(EXISTS "$ENV{WindowsSDKDir}" AND (NOT "$ENV{WindowsSDKDir}" STREQUAL ""))
		_winsdk_conditional_append_preferred("WindowsSDKDir environment variable" "$ENV{WindowsSDKDir}")
	endif()
	if(_winsdk_msvc_less_1600)
		get_filename_component(_sdkdir "[HKEY_CURRENT_USER\\Software\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]" ABSOLUTE)
		_winsdk_conditional_append_preferred("Per-user current Windows SDK" "${_sdkdir}")
		get_filename_component(_sdkdir "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]" ABSOLUTE)
		_winsdk_conditional_append_preferred("System-wide current Windows SDK" "${_sdkdir}")
	endif()
endif()

# Lookup functions
function(windowssdk_name_lookup _dir _outvar)
	list(FIND _win_sdk_versanddirs "${_dir}" _diridx)
	math(EXPR _idx "${_diridx} - 1")
	if(${_idx} GREATER -1)
		list(GET _win_sdk_versanddirs ${_idx} _ret)
	else()
		set(_ret "NOTFOUND")
	endif()
	set(${_outvar} "${_ret}" PARENT_SCOPE)
endfunction()

function(windowssdk_build_lookup _dir _outvar)
	list(FIND _win_sdk_buildsanddirs "${_dir}" _diridx)
	math(EXPR _idx "${_diridx} - 1")
	if(${_idx} GREATER -1)
		list(GET _win_sdk_buildsanddirs ${_idx} _ret)
	else()
		set(_ret "NOTFOUND")
	endif()
	set(${_outvar} "${_ret}" PARENT_SCOPE)
endfunction()

# Final output variables
if(_win_sdk_dirs)
	list(GET _win_sdk_dirs 0 WINDOWSSDK_LATEST_DIR)
	windowssdk_name_lookup("${WINDOWSSDK_LATEST_DIR}" WINDOWSSDK_LATEST_NAME)
	set(WINDOWSSDK_DIRS ${_win_sdk_dirs})
	set(WINDOWSSDK_PREFERRED_DIR "${WINDOWSSDK_LATEST_DIR}")
	set(WINDOWSSDK_PREFERRED_NAME "${WINDOWSSDK_LATEST_NAME}")
	set(WINDOWSSDK_PREFERRED_FIRST_DIRS ${WINDOWSSDK_DIRS})
	set(WINDOWSSDK_FOUND_PREFERENCE OFF)
endif()

if(_win_sdk_preferred_sdk_dirs)
	list(GET _win_sdk_preferred_sdk_dirs 0 WINDOWSSDK_PREFERRED_DIR)
	windowssdk_name_lookup("${WINDOWSSDK_PREFERRED_DIR}" WINDOWSSDK_PREFERRED_NAME)
	set(WINDOWSSDK_PREFERRED_FIRST_DIRS ${_win_sdk_preferred_sdk_dirs} ${_win_sdk_dirs})
	list(REMOVE_DUPLICATES WINDOWSSDK_PREFERRED_FIRST_DIRS)
	set(WINDOWSSDK_FOUND_PREFERENCE ON)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WindowsSDK
	"No compatible version of the Windows SDK or Platform SDK found."
	WINDOWSSDK_DIRS)

if(WINDOWSSDK_FOUND)
	# Determine architecture suffixes
	if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
		if(CMAKE_SIZEOF_VOID_P MATCHES "8")
			set(_winsdk_arch8 arm64)
		else()
			set(_winsdk_archbare /arm)
			set(_winsdk_arch arm)
			set(_winsdk_arch8 arm)
		endif()
	else()
		if(CMAKE_SIZEOF_VOID_P MATCHES "8")
			set(_winsdk_archbare /x64)
			set(_winsdk_arch amd64)
			set(_winsdk_arch8 x64)
		else()
			set(_winsdk_archbare )
			set(_winsdk_arch i386)
			set(_winsdk_arch8 x86)
		endif()
	endif()

	function(get_windowssdk_from_component _component _var)
		get_filename_component(_component "${_component}" ABSOLUTE)
		file(TO_CMAKE_PATH "${_component}" _component)
		foreach(_sdkdir ${WINDOWSSDK_DIRS})
			get_filename_component(_sdkdir "${_sdkdir}" ABSOLUTE)
			string(LENGTH "${_sdkdir}" _sdklen)
			file(RELATIVE_PATH _rel "${_sdkdir}" "${_component}")
			if(NOT "${_rel}" MATCHES "[.][.]")
				set(${_var} "${_sdkdir}" PARENT_SCOPE)
				return()
			endif()
		endforeach()
		set(${_var} "NOTFOUND" PARENT_SCOPE)
	endfunction()

	function(get_windowssdk_library_dirs _winsdk_dir _var)
		set(_dirs)
		set(_suffixes
			"lib${_winsdk_archbare}"
			"lib/${_winsdk_arch}"
			"lib/w2k/${_winsdk_arch}"
			"lib/wxp/${_winsdk_arch}"
			"lib/wnet/${_winsdk_arch}"
			"lib/wlh/${_winsdk_arch}"
			"lib/wlh/um/${_winsdk_arch8}"
			"lib/win7/${_winsdk_arch}"
			"lib/win7/um/${_winsdk_arch8}"
		)
		foreach(_ver wlh win7 win8 winv6.3)
			list(APPEND _suffixes
				"lib/${_ver}/${_winsdk_arch}"
				"lib/${_ver}/um/${_winsdk_arch8}"
				"lib/${_ver}/km/${_winsdk_arch8}"
			)
		endforeach()
		foreach(_mode umdf kmdf)
			file(GLOB _wdfdirs RELATIVE "${_winsdk_dir}" "${_winsdk_dir}/lib/wdf/${_mode}/${_winsdk_arch8}/*")
			if(_wdfdirs)
				list(APPEND _suffixes ${_wdfdirs})
			endif()
		endforeach()
		foreach(_win10ver ${_winsdk_win10vers})
			foreach(_component um km ucrt mmos)
				list(APPEND _suffixes "lib/${_win10ver}/${_component}/${_winsdk_arch8}")
			endforeach()
		endforeach()
		foreach(_suffix ${_suffixes})
			file(GLOB _libs "${_winsdk_dir}/${_suffix}/*.lib")
			if(_libs)
				list(APPEND _dirs "${_winsdk_dir}/${_suffix}")
			endif()
		endforeach()
		if("${_dirs}" STREQUAL "")
			set(_dirs NOTFOUND)
		else()
			list(REMOVE_DUPLICATES _dirs)
		endif()
		set(${_var} ${_dirs} PARENT_SCOPE)
	endfunction()

	function(get_windowssdk_include_dirs _winsdk_dir _var)
		set(_dirs)
		set(_subdirs shared um winrt km wdf mmos ucrt)
		set(_suffixes Include)
		foreach(_dir ${_subdirs})
			list(APPEND _suffixes "Include/${_dir}")
		endforeach()
		foreach(_ver ${_winsdk_win10vers})
			foreach(_dir ${_subdirs})
				list(APPEND _suffixes "Include/${_ver}/${_dir}")
			endforeach()
		endforeach()
		foreach(_suffix ${_suffixes})
			file(GLOB _headers "${_winsdk_dir}/${_suffix}/*.h")
			if(_headers)
				list(APPEND _dirs "${_winsdk_dir}/${_suffix}")
			endif()
		endforeach()
		if("${_dirs}" STREQUAL "")
			set(_dirs NOTFOUND)
		else()
			list(REMOVE_DUPLICATES _dirs)
		endif()
		set(${_var} ${_dirs} PARENT_SCOPE)
	endfunction()

	function(get_windowssdk_library_dirs_multiple _var)
		set(_dirs)
		foreach(_sdkdir ${ARGN})
			get_windowssdk_library_dirs("${_sdkdir}" _current_sdk_libdirs)
			if(_current_sdk_libdirs)
				list(APPEND _dirs ${_current_sdk_libdirs})
			endif()
		endforeach()
		if("${_dirs}" STREQUAL "")
			set(_dirs NOTFOUND)
		else()
			list(REMOVE_DUPLICATES _dirs)
		endif()
		set(${_var} ${_dirs} PARENT_SCOPE)
	endfunction()

	function(get_windowssdk_include_dirs_multiple _var)
		set(_dirs)
		foreach(_sdkdir ${ARGN})
			get_windowssdk_include_dirs("${_sdkdir}" _current_sdk_incdirs)
			if(_current_sdk_incdirs)
				list(APPEND _dirs ${_current_sdk_incdirs})
			endif()
		endforeach()
		if("${_dirs}" STREQUAL "")
			set(_dirs NOTFOUND)
		else()
			list(REMOVE_DUPLICATES _dirs)
		endif()
		set(${_var} ${_dirs} PARENT_SCOPE)
	endfunction()
endif()