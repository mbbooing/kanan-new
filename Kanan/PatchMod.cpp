#include <algorithm>

#include <imgui.h>

#include <Scan.hpp>

#include "Log.hpp"
#include "PatchMod.hpp"

using namespace std;

namespace kanan {
    PatchMod::PatchMod(string patchName, string tooltip)
        : m_isEnabled{ false },
        m_patches{},
        m_hasFailingPatch{ false },
        m_patchName{ move(patchName) },
        m_tooltip{ move(tooltip) },
        m_configName{}
    {
        // Generate the config name from the name of the patch.
        m_configName = m_patchName;

        // Remove spaces.
        m_configName.erase(remove_if(begin(m_configName), end(m_configName), isspace), end(m_configName));

        // Add .Enabled
        m_configName.append(".Enabled");
    }

    bool PatchMod::addPatch(const string& pattern, int offset, vector<int16_t> patchBytes) {
        auto address = scan("client.exe", pattern);

        if (!address) {
            log("[%s] Failed to find pattern %s", m_patchName.c_str(), pattern.c_str());

            m_hasFailingPatch = true;

            return false;
        }

        log("[%s] Found address %p for pattern %s", m_patchName.c_str(), *address, pattern.c_str());

        Patch patch{};

        patch.address = *address + offset;
        patch.bytes = move(patchBytes);

        m_patches.emplace_back(patch);

        return true;
    }

    void PatchMod::onPatchUI() {
        if (m_patches.empty() || m_hasFailingPatch) {
            return;
        }

        if (ImGui::Checkbox(m_patchName.c_str(), &m_isEnabled)) {
            apply();
        }

        if (!m_tooltip.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip(m_tooltip.c_str());
        }
    }

    void PatchMod::onConfigLoad(const Config& cfg) {
        m_isEnabled = cfg.get<bool>(m_configName).value_or(false);

        if (m_isEnabled) {
            apply();
        }
    }

    void PatchMod::onConfigSave(Config& cfg) {
        cfg.set<bool>(m_configName, m_isEnabled);
    }

    void PatchMod::apply() {
        if (m_patches.empty() || m_hasFailingPatch) {
            return;
        }

        log("[%s] Toggling %s", m_patchName.c_str(), m_isEnabled ? "on" : "off");

        if (m_isEnabled) {
            for (auto& p : m_patches) {
                patch(p);
            }
        }
        else {
            for (auto& p : m_patches) {
                undoPatch(p);
            }
        }
    }
}