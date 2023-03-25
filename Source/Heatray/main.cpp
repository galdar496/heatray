//
//  main.m
//  Heatray5 macOS
//
//  Created by Cody White on 3/17/23.
//

#include "AppDelegate.h"

#include "Utility/ConsoleLog.h"

#include <AppKit/AppKit.hpp>

int main(int argc, const char *argv[]) {
    // Setup default logging to the console.
    util::ConsoleLog::install();
    
    NS::AutoreleasePool *autoreleasePool = NS::AutoreleasePool::alloc()->init();
    
    AppDelegate delegate;
    NS::Application *sharedApplication = NS::Application::sharedApplication();
    sharedApplication->setDelegate(&delegate);
    sharedApplication->run();
    
    autoreleasePool->release();
    return 0;
}
