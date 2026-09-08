#include "PoisonPatch.h"

/// <summary>
/// Bootleg version copied from PO3s Tweaks: https://github.com/powerof3/po3-Tweaks/
/// </summary>
namespace PoisonPatch {
    struct ShowPoisonConfirmationPrompt
	{
		static void thunk(char*, void (*PoisonWeapon)(std::uint8_t a_result), std::uint8_t a_result, std::uint32_t, std::int32_t, char*, char*)
		{
			PoisonWeapon(a_result);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

    void Install() {
        REL::Relocation<std::uintptr_t> target{RELOCATION_ID(39406, 40481)};
        stl::write_thunk_call<ShowPoisonConfirmationPrompt>(target.address() + 0x10B);
        
        logger::debug("Installed poison patch"sv);
    }
}