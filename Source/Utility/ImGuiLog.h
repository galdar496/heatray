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

#include <atomic>
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

    void addNewItem(const std::string_view item, const Log::Type type) override {
        std::string newItem{item};
        newItem.append("\n"); // Auto-add a newline to avoid the user having to worry about it.
        std::unique_lock<std::mutex> lock(m_mutex);
        m_textBuffer[static_cast<size_t>(type)].append(newItem.c_str());
        m_newTextAvailable[static_cast<size_t>(type)].store(true);
    }

    void clear() { 
        for (size_t i = 0; i < static_cast<size_t>(Log::Type::kCount); ++i) {
            m_textBuffer[i].clear();
        }
    }

    const ImGuiTextBuffer& textBuffer(const Log::Type type, bool &newTextAvailable) {
        newTextAvailable = m_newTextAvailable[static_cast<size_t>(type)].exchange(false);
        return m_textBuffer[static_cast<size_t>(type)];
    }
private:
    ImGuiLog() = default;
    ImGuiTextBuffer m_textBuffer[static_cast<size_t>(Log::Type::kCount)];
    std::atomic_bool m_newTextAvailable[static_cast<size_t>(Log::Type::kCount)] = {false};
    std::mutex m_mutex;
};

}  // namespace util.

