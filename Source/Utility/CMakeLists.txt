add_library(Utility STATIC
	AABB.h
	BlueNoise.h
	FileIO.h
	FileIO.cpp
	Hash.h
	ImGuiLog.h
	Log.cpp
	Log.h
	Random.h
	ShaderCodeLoader.h
	ShaderCodeLoader.cpp
    StringUtils.h
	TextureLoader.h
	TextureLoader.cpp
	Timer.h
)

if (APPLE)
	add_library(FileDialog STATIC
		FileDialog.h
		FileDialogMacOS.mm
	)
	target_link_libraries(FileDialog
		objc
		"-framework AppKit"
		)
endif()

if (WIN32)
	add_library(FileDialog STATIC
		FileDialog.h
		FileDialogWindows.cpp
	)
endif()