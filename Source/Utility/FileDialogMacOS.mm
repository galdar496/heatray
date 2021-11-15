#include "Utility/FileDialog.h"

#import <AppKit/AppKit.h>

namespace util
{

std::vector<std::string> OpenFileDialog(const std::string &extension)
{
    NSOpenPanel * openPanel = [NSOpenPanel openPanel];
    
    NSInteger buttonPressed = [openPanel runModal];
    
    if (buttonPressed == NSModalResponseOK)
    {
        std::vector<std::string> retVal;

        for (int i = 0; i < [[openPanel URLs] count]; ++i)
        {
            NSURL * selectedFile = [[openPanel URLs] objectAtIndex:i];

            retVal.push_back([[selectedFile path] UTF8String]);
        }

        return retVal;
    }

    return std::vector<std::string>();
}

std::vector<std::string> SaveFileDialog(const std::string &extension)
{
    return std::vector<std::string>();
}

} // namespace util
