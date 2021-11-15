//
//  ImGuiLog.h
//  Heatray
//
//  Log specific for an ImGui console.
//
//

#pragma once

#include "Log.h"

#include "../../3rdParty/imgui/imgui.h"

#include <mutex>

namespace util {
 
class ImGuiLog final : public Log {
public:
    virtual ~ImGuiLog() = default;

    static void install() {
        ImGuiLog* log = new ImGuiLog();
        std::shared_ptr<ImGuiLog> newLog(log);
        Log::setInstance(newLog);
    }

    void addNewItem(const std::string &item, Log::Type type) override {
        std::string new_item = (item + "\n");
        std::unique_lock<std::mutex> lock(m_mutex);
        m_textBuffer[static_cast<size_t>(type)].append(new_item.c_str());
    }

    void clear() { 
        for (size_t i = 0; i < static_cast<size_t>(Log::Type::kCount); ++i) {
            m_textBuffer[i].clear();
        }
    }

    const ImGuiTextBuffer& textBuffer(const Log::Type type) const { return m_textBuffer[static_cast<size_t>(type)]; }
private:
    ImGuiLog() = default;
    ImGuiTextBuffer m_textBuffer[static_cast<size_t>(Log::Type::kCount)];
    std::mutex m_mutex;
};

}  // namespace util.

