//
//  AppDelegate.h
//  Heatray5 macOS
//
//  Created by Cody White on 3/17/23.
//

#pragma once

#include "HeatrayRenderer/HeatrayRenderer_macOS.hpp"

#include <AppKit/AppKit.hpp>
#include <MetalKit/MTKView.hpp>

// Forward declarations.
class MTKViewDelegate;

class AppDelegate : public NS::ApplicationDelegate {
public:
    AppDelegate() = default;
    ~AppDelegate();
    
    NS::Menu *createMenuBar();
    
    virtual void applicationWillFinishLaunching(NS::Notification* notification) override;
    virtual void applicationDidFinishLaunching(NS::Notification* notification) override;
    virtual bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* sender) override;

private:
    NS::Window *m_window = nullptr;
    MTK::View *m_view = nullptr;
    MTL::Device *m_device = nullptr;
    MTKViewDelegate *m_viewDelegate = nullptr;
};

class MTKViewDelegate : public MTK::ViewDelegate
{
public:
    MTKViewDelegate() = delete;
    MTKViewDelegate(MTL::Device* device, MTK::View* view, const CGRect& frame);
    virtual ~MTKViewDelegate() override;
    virtual void drawInMTKView(MTK::View* view) override;
    virtual void drawableSizeWillChange(MTK::View* view, CGSize size) override;

private:
    HeatrayRenderer m_renderer;
};
