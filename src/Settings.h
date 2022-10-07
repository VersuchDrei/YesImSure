#pragma once

#include "AutoTOML.hpp"

#define MAKE_SETTING(a_type, a_group, a_key, a_value) \
    inline a_type a_key { a_group##s, #a_key##s, a_value }

namespace Settings {
    using bSetting = AutoTOML::bSetting;

    inline void Load() {
        try {
            const auto table = toml::parse_file("Data/SKSE/Plugins/YesImSure.toml"s);
            for (const auto& setting : AutoTOML::ISetting::get_settings()) {
                setting->load(table);
            }
        } catch (const toml::parse_error& e) {
            std::ostringstream ss;
            ss << "Error parsing file \'" << *e.source().path << "\':\n"
               << '\t' << e.description() << '\n'
               << "\t\t(" << e.source().begin << ')';
            //logger::error(ss.str());
            util::report_and_fail("failed to load settings"sv);
        } catch (const std::exception& e) {
            util::report_and_fail(e.what());
        } catch (...) {
            util::report_and_fail("unknown failure"sv);
        }
    }

    MAKE_SETTING(bSetting, "Patches", ConstructibleObjectMenu, true);
    MAKE_SETTING(bSetting, "Patches", AlchemyMenu, true);
    MAKE_SETTING(bSetting, "Patches", SmithingMenu, true);
    MAKE_SETTING(bSetting, "Patches", EnchantmentLearned, false);
    MAKE_SETTING(bSetting, "Patches", EnchantmentCrafted, false);
    MAKE_SETTING(bSetting, "Patches", EnchantingMenuExit, true);
    MAKE_SETTING(bSetting, "Patches", Poison, true);
}  // namespace Settings

#undef MAKE_SETTING
