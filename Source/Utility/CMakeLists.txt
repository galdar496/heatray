add_library(Utility STATIC
	FileIO.h
	FileIO.cpp
	Random.h
	ShaderCodeLoader.h
	ShaderCodeLoader.cpp
	TextureLoader.h
	TextureLoader.cpp
	Timer.h
)

if (APPLE)
	add_library(OpenFileDialog STATIC
		OpenFileDialog.h
		OpenFileDialogMacOS.mm
	)
	target_link_libraries(OpenFileDialog
		objc
		"-framework AppKit"
		)
endif()

if (WIN32)
	add_library(OpenFileDialog STATIC
		OpenFileDialog.h
		OpenFileDialogWindows.cpp
	)
endif()