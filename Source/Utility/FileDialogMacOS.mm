#include "Utility/FileDialog.h"

#import <AppKit/AppKit.h>

namespace util
{

std::vector<std::string> OpenFileDialog(const std::string_view extension)
{
    NSOpenPanel * openPanel = [NSOpenPanel openPanel];
    
    NSInteger buttonPressed = [openPanel runModal];
    
    if (buttonPressed == NSModalResponseOK) {
        std::vector<std::string> retVal;

        for (int i = 0; i < [[openPanel URLs] count]; ++i) {
            NSURL * selectedFile = [[openPanel URLs] objectAtIndex:i];

            retVal.push_back([[selectedFile path] UTF8String]);
        }

        return retVal;
    }

    return std::vector<std::string>();
}

std::vector<std::string> SaveFileDialog(const std::string_view extension)
{
    NSSavePanel * savePanel = [NSSavePanel savePanel];
    
    NSInteger buttonPressed = [savePanel runModal];
    
    if (buttonPressed == NSModalResponseOK) {
        std::vector<std::string> retVal;

        NSURL * selectedFile = [savePanel URL];
        retVal.push_back([[selectedFile path] UTF8String] + std::string(".") + std::string{extension});
        return retVal;
    }

    return std::vector<std::string>();
}

} // namespace util
