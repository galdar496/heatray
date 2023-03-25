//
//  AppDelegate.m
//  Heatray5 macOS
//
//  Created by Cody White on 3/17/23.
//

#include "AppDelegate.h"

#include <string_view>

constexpr std::string_view WINDOW_TITLE = "Heatray v5";
constexpr uint32_t DEFAULT_WINDOW_WIDTH = 800;
constexpr uint32_t DEFAULT_WINDOW_HEIGHT = 800;

AppDelegate::~AppDelegate() {
    m_view->release();
    m_window->release();
    m_device->release();
    delete m_viewDelegate;
    
    m_view = nullptr;
    m_window = nullptr;
    m_device = nullptr;
    m_viewDelegate = nullptr;
}

NS::Menu* AppDelegate::createMenuBar() {
    using NS::StringEncoding::UTF8StringEncoding;
    
    NS::Menu* mainMenu = NS::Menu::alloc()->init();
    NS::MenuItem* appMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* appMenu = NS::Menu::alloc()->init(NS::String::string(WINDOW_TITLE.data(), UTF8StringEncoding));
    
    NS::String* appName = NS::RunningApplication::currentApplication()->localizedName();
    NS::String* quitItemName = NS::String::string("Quit ", UTF8StringEncoding)->stringByAppendingString(appName);
    SEL quitCallback = NS::MenuItem::registerActionCallback("appQuit", [](void*, SEL, const NS::Object* sender) {
        NS::Application* app = NS::Application::sharedApplication();
        app->terminate(sender);
    });
    
    NS::MenuItem* appQuitItem = appMenu->addItem(quitItemName, quitCallback, NS::String::string("q", UTF8StringEncoding));
    appQuitItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
    appMenuItem->setSubmenu(appMenu);
    
    NS::MenuItem* windowMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* windowMenu = NS::Menu::alloc()->init(NS::String::string("Window", UTF8StringEncoding));
    
    SEL closeWindowCallback = NS::MenuItem::registerActionCallback("windowClose", [](void*, SEL, const NS::Object*) {
        NS::Application* app = NS::Application::sharedApplication();
        app->windows()->object<NS::Window>(0)->close();
    });
    NS::MenuItem* closeWindowItem = windowMenu->addItem(NS::String::string("Close Window", UTF8StringEncoding), closeWindowCallback, NS::String::string("w", UTF8StringEncoding));
    closeWindowItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
    
    windowMenuItem->setSubmenu(windowMenu);
    
    mainMenu->addItem(appMenuItem);
    mainMenu->addItem(windowMenuItem);
    
    appMenuItem->release();
    windowMenuItem->release();
    appMenu->release();
    windowMenu->release();
    
    return mainMenu->autorelease();
}

void AppDelegate::applicationWillFinishLaunching(NS::Notification *notification) {
    NS::Menu* menu = createMenuBar();
    NS::Application* app = reinterpret_cast<NS::Application*>(notification->object());
    app->setMainMenu(menu);
    app->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
}

void AppDelegate::applicationDidFinishLaunching(NS::Notification *notification) {
    CGRect frame = (CGRect){ {100.0, 100.0}, {DEFAULT_WINDOW_WIDTH + HeatrayRenderer::UI_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT} };
    
    m_window = NS::Window::alloc()->init(frame,
                                         NS::WindowStyleMaskClosable       |
                                         NS::WindowStyleMaskTitled         |
                                         NS::WindowStyleMaskResizable      |
                                         NS::WindowStyleMaskMiniaturizable,
                                         NS::BackingStoreBuffered,
                                         false);
    
    m_device = MTL::CreateSystemDefaultDevice();
    
    m_view = MTK::View::alloc()->init(frame, m_device);
    m_view->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    m_view->setClearColor(MTL::ClearColor::Make(0.0, 0.0, 0.0, 1.0));
    
    m_viewDelegate = new MTKViewDelegate(m_device, m_view, frame);
    m_view->setDelegate(m_viewDelegate);
    
    m_window->setContentView(m_view);
    m_window->setTitle(NS::String::string(WINDOW_TITLE.data(), NS::StringEncoding::UTF8StringEncoding));
    
    m_window->makeKeyAndOrderFront(nullptr);
    
    NS::Application *app = reinterpret_cast<NS::Application*>(notification->object());
    app->activateIgnoringOtherApps(true);
}

bool AppDelegate::applicationShouldTerminateAfterLastWindowClosed(NS::Application *sender) {
    return true;
}

MTKViewDelegate::MTKViewDelegate(MTL::Device* device, MTK::View* view, const CGRect& frame)
: MTK::ViewDelegate() {
    m_renderer.init(device, view, frame.size.width, frame.size.height);
}

MTKViewDelegate::~MTKViewDelegate() {
    m_renderer.destroy();
}

void MTKViewDelegate::drawInMTKView(MTK::View* view) {
    m_renderer.render(view);
}

void MTKViewDelegate::drawableSizeWillChange(MTK::View* view, CGSize size) {
    m_renderer.resize(uint32_t(size.width), uint32_t(size.height));
}
