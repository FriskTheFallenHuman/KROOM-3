include(ExternalProject)

set(SWSDK_VERSION "1.65")
set(SWSDK_RELEASE "x86")
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	set(SWSDK_RELEASE "x64")
endif()

ExternalProject_Add(
	PipeDownloader
	URL			"https://github.com/julianxhokaxhiu/SteamworksSDKCI/releases/download/${SWSDK_VERSION}/SteamworksSDK-v${SWSDK_VERSION}.0_${SWSDK_RELEASE}.zip"
	DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/PipeDownloader/steam
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set_target_properties(PipeDownloader PROPERTIES FOLDER "third-party")