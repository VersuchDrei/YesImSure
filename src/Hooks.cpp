#include "Hooks.h"

#include "Settings.h"

namespace Hooks {
    namespace {
        // CommonLibSSE-NG >= 7 dropped RE::DebugNotification. Call the game function
        // directly (SE 52050 / AE 52933 - the AE id the poison patch already uses).
        void DebugNotification(const char* a_message) {
            using func_t = void (*)(const char*, const char*, bool);
            static const REL::Relocation<func_t> func{RELOCATION_ID(52050, 52933)};
            func(a_message, nullptr, true);
        }

        // CommonLibSSE-NG nests this class one namespace deeper than its siblings
        // (RE::CraftingSubMenus::CraftingSubMenus::AlchemyMenu). Alias it so the call
        // sites read like the other sub-menus.
        using AlchemyMenu = RE::CraftingSubMenus::CraftingSubMenus::AlchemyMenu;

        template <class T, std::uint64_t FUNC_ID>
        void SkipSubMenuMenuPrompt() {
            const REL::Relocation<void(T*)> func{REL::ID(FUNC_ID)};
            const auto ui = RE::UI::GetSingleton();
            const auto craftingMenu = ui ? ui->GetMenu<RE::CraftingMenu>() : nullptr;
            const auto subMenu = craftingMenu ? static_cast<T*>(craftingMenu->GetCraftingSubMenu()) : nullptr;
            if (subMenu) {
                func(subMenu);
            }
        }

        struct SubMenuPatchCode : public Xbyak::CodeGenerator {
        public:
            SubMenuPatchCode(std::size_t a_callAddr, std::size_t a_retAddr) {
                Xbyak::Label callLbl;
                Xbyak::Label retLbl;

                call(ptr[rip + callLbl]);
                jmp(ptr[rip + retLbl]);

                L(callLbl);
                dq(a_callAddr);

                L(retLbl);
                dq(a_retAddr);
            }
        };

        template <std::uint64_t FUNC_ID, std::size_t CAVE_START, std::size_t CAVE_END, std::size_t JUMP_OUT>
        void InstallSubMenuPatch(std::uintptr_t a_skipFuncAddr) {
            constexpr std::size_t CAVE_SIZE = CAVE_END - CAVE_START;

            const REL::Relocation<std::uintptr_t> funcBase{REL::ID(FUNC_ID)};

            SubMenuPatchCode patch(a_skipFuncAddr, funcBase.address() + JUMP_OUT);
            patch.ready();
            assert(patch.getSize() <= CAVE_SIZE);

            REL::safe_fill(funcBase.address() + CAVE_START, REL::NOP, CAVE_SIZE);
            REL::safe_write(funcBase.address() + CAVE_START,
                            std::span{patch.getCode<const std::byte*>(), patch.getSize()});
        }

        void RefreshInventoryMenu() {
            const auto ui = RE::UI::GetSingleton();
            const auto invMenu = ui->GetMenu<RE::InventoryMenu>();
            if (invMenu && invMenu->GetRuntimeData().itemList) {
                invMenu->GetRuntimeData().itemList->Update();
            }
        }

        RE::InventoryEntryData* GetEquippedEntryData(RE::AIProcess* a_process, [[maybe_unused]] bool a_leftHand) {
            const auto middleHigh = a_process->middleHigh;
            if (!middleHigh) {
                return nullptr;
            }

            const auto hand = middleHigh->rightHand;
            if (hand && hand->object && hand->object->Is(RE::FormType::Weapon)) {
                if (!hand->extraLists) {
                    return hand;
                }

                bool applied = false;
                for (auto& xList : *hand->extraLists) {
                    if (xList->HasType(RE::ExtraDataType::kPoison)) {
                        applied = true;
                        break;
                    }
                }

                if (!applied) {
                    return hand;
                }
            }

            return middleHigh->leftHand ? middleHigh->leftHand : middleHigh->rightHand;
        }

        void InstallPoisonPatch() {
            constexpr std::size_t JUMP_OUT = 0x148;

            const REL::Relocation<std::uintptr_t> funcBase{RELOCATION_ID(39406, 40481)};

            // nop until callback gets loaded
            {
                constexpr std::size_t CAVE_START = 0xA3;
                constexpr std::size_t CAVE_END = 0xD7;
                REL::safe_fill(funcBase.address() + CAVE_START, REL::NOP, CAVE_END - CAVE_START);
            }

            // hook callback
            {
                constexpr std::size_t CAVE_START = 0xDE;
                constexpr std::size_t CAVE_END = 0x112;
                constexpr std::size_t CAVE_SIZE = CAVE_END - CAVE_START;

                struct Patch : public Xbyak::CodeGenerator {
                public:
                    Patch(std::size_t a_callAddr, std::size_t a_retAddr) {
                        Xbyak::Label callLbl;
                        Xbyak::Label retLbl;

                        mov(rcx, 2);
                        call(rdx);
                        call(ptr[rip + callLbl]);
                        jmp(ptr[rip + retLbl]);

                        L(callLbl);
                        dq(a_callAddr);

                        L(retLbl);
                        dq(a_retAddr);
                    }
                };

                Patch patch(reinterpret_cast<std::uintptr_t>(&RefreshInventoryMenu), funcBase.address() + JUMP_OUT);
                patch.ready();
                assert(patch.getSize() <= CAVE_SIZE);

                REL::safe_fill(funcBase.address() + CAVE_START, REL::NOP, CAVE_SIZE);
                REL::safe_write(funcBase.address() + CAVE_START,
                                std::span{patch.getCode<const std::byte*>(), patch.getSize()});
            }

            // swap messagebox error for debug notification
            {
                constexpr std::size_t CAVE_START = 0x119;
                constexpr std::size_t CAVE_END = 0x148;
                constexpr std::size_t CAVE_SIZE = CAVE_END - CAVE_START;

                struct Patch : public Xbyak::CodeGenerator {
                public:
                    Patch(std::size_t a_callAddr, std::size_t a_retAddr) {
                        Xbyak::Label callLbl;
                        Xbyak::Label retLbl;

                        mov(rdx, 0);
                        mov(r8, 1);
                        call(ptr[rip + callLbl]);
                        jmp(ptr[rip + retLbl]);

                        L(callLbl);
                        dq(a_callAddr);

                        L(retLbl);
                        dq(a_retAddr);
                    }
                };

                const REL::Relocation<std::uintptr_t> dbgNotif{REL::ID(52933)};
                Patch patch(dbgNotif.address(), funcBase.address() + JUMP_OUT);
                patch.ready();
                assert(patch.getSize() <= CAVE_SIZE);

                REL::safe_fill(funcBase.address() + CAVE_START, REL::NOP, CAVE_SIZE);
                REL::safe_write(funcBase.address() + CAVE_START,
                                std::span{patch.getCode<const std::byte*>(), patch.getSize()});
            }

            // Fix for applying poison to left hand
            {
                auto& trampoline = SKSE::GetTrampoline();
                trampoline.write_call<5>(funcBase.address() + 0x2F, &GetEquippedEntryData);

                const REL::Relocation<std::uintptr_t> funcBase2{RELOCATION_ID(39407, 40482)};
                trampoline.write_call<5>(funcBase2.address() + 0x32, &GetEquippedEntryData);
            }

            logger::debug("Installed poison patch"sv);
        }

        void NotifyEnchantmentLearned(const char* a_fmt, RE::TESForm* a_item) {
            const auto fullName = a_item->As<RE::TESFullName>();
            const auto name = fullName ? fullName->GetFullName() : "";
            std::size_t len = std::snprintf(0, 0, a_fmt, name) + 1;
            const auto msg = std::make_unique<char[]>(len);
            std::snprintf(msg.get(), len, a_fmt, name);
            DebugNotification(msg.get());
        }

        void InstallEnchantmentLearnedPatch(const bool isSE) {

            // skip "are you sure?"
            {
                if (isSE) {
                    constexpr std::uint64_t SKIP_FUNC = 50459;
                    constexpr std::uint64_t CALL_FUNC = 50440;
                    constexpr std::size_t CAVE_START = 0xBB;
                    constexpr std::size_t CAVE_END = 0x1D3;
                    constexpr std::size_t JUMP_OUT = 0x38D;
                    const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                        SkipSubMenuMenuPrompt<RE::CraftingSubMenus::EnchantConstructMenu, SKIP_FUNC>);
                    InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
                } else {
                    constexpr std::uint64_t SKIP_FUNC = 51363;
                    constexpr std::uint64_t CALL_FUNC = 51344;
                    constexpr std::size_t CAVE_START = 0xC2;
                    constexpr std::size_t CAVE_END = 0x1D7;
                    constexpr std::size_t JUMP_OUT = 0x4DF;
                    const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                        SkipSubMenuMenuPrompt<RE::CraftingSubMenus::EnchantConstructMenu, SKIP_FUNC>);
                    InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
				}
                
            }

            const REL::Relocation<std::uintptr_t> funcBase{RELOCATION_ID(50459, 51363)};

            // nop until format string gets loaded
            {
                constexpr std::size_t CAVE_START = 0x15D;
                constexpr std::size_t CAVE_END = 0x1A7;
                REL::safe_fill(funcBase.address() + CAVE_START, REL::NOP, CAVE_END - CAVE_START);
            }

            // swap messagebox for debug notification
            {
                constexpr std::size_t CAVE_START = 0x1AE;
                constexpr std::size_t CAVE_END = 0x1EB;
                constexpr std::size_t CAVE_SIZE = CAVE_END - CAVE_START;

                struct Patch : public Xbyak::CodeGenerator {
                public:
                    Patch(std::size_t a_callAddr, std::size_t a_retAddr) {
                        Xbyak::Label callLbl;
                        Xbyak::Label retLbl;

                        mov(rcx, rbx);  // rbx == const char*
                        mov(rdx, rsi);  // rsi == TESForm*
                        call(ptr[rip + callLbl]);
                        jmp(ptr[rip + retLbl]);

                        L(callLbl);
                        dq(a_callAddr);

                        L(retLbl);
                        dq(a_retAddr);
                    }
                };

                constexpr std::size_t JUMP_OUT = 0x1EB;
                Patch patch(reinterpret_cast<std::uintptr_t>(NotifyEnchantmentLearned), funcBase.address() + JUMP_OUT);
                patch.ready();
                assert(patch.getSize() <= CAVE_SIZE);

                REL::safe_fill(funcBase.address() + CAVE_START, REL::NOP, CAVE_SIZE);
                REL::safe_write(funcBase.address() + CAVE_START,
                                std::span{patch.getCode<const std::byte*>(), patch.getSize()});
            }

            logger::debug("Installed enchantment learned patch"sv);
        }

        void CloseEnchantingMenu() {
            const auto uiStr = RE::InterfaceStrings::GetSingleton();
            const auto factory = RE::MessageDataFactoryManager::GetSingleton();
            const auto creator = factory->GetCreator<RE::BSUIMessageData>(uiStr->bsUIMessageData);
            const auto msg = creator->Create();
            msg->fixedStr = "Cancel";
            const auto uiQueue = RE::UIMessageQueue::GetSingleton();
            uiQueue->AddMessage(uiStr->craftingMenu, RE::UI_MESSAGE_TYPE::kUserEvent, msg);
        }
    }  // namespace

    void Install() {
        // Runtime split: anything below 1.6.0 is SE (1.5.97 and prior); 1.6.x AND 1.7.x both
        // take the "AE" code paths below. Compare the version directly instead of relying on
        // REL::Module::Runtime, so a future minor bump can't silently misclassify as SE.
        const bool isSE = REL::Module::get().version() < REL::Version{1, 6, 0, 0};

        // Skyrim 1.7.99 / 1.7.104 (Aug 2026): the game binary was rebuilt and the Address
        // Library switched to a new on-disk format (v5), both handled transparently by
        // CommonLibSSE-NG >= 7.0.0 + the 1.7.104 Address Library. The AE crafting-menu
        // functions moved as one block (uniform +0x16080 from 1.6.1170, +0x260 from 1.7.99);
        // their bodies were not re-laid-out. Every hand-computed offset below (CAVE_START /
        // CAVE_END / JUMP_OUT for all 7 patches, the poison NOP ranges and write_call<5>
        // offsets, the EnchantmentLearned NOP/mbox ranges) was statically re-verified against
        // SkyrimSE.exe 1.7.104 - each lands on the same instruction with the same semantics,
        // so no offset changes are needed. Still: exercise all 7 prompts in-game before
        // release, and if a future update DOES shift a body, add a branch gated on
        //     !isSE && REL::Module::get().version() < REL::Version{1, 7, 99, 0}

        if (*Settings::ConstructibleObjectMenu) {
            if (isSE) {
                constexpr std::uint64_t CALL_FUNC = 50452;
                constexpr std::uint64_t SKIP_FUNC = 50476;
                constexpr std::size_t CAVE_START = 0x5F;
                constexpr std::size_t CAVE_END = 0x174;
                constexpr std::size_t JUMP_OUT = 0x192;
                const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                    SkipSubMenuMenuPrompt<RE::CraftingSubMenus::ConstructibleObjectMenu, SKIP_FUNC>);
                InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
			} else {
                constexpr std::uint64_t CALL_FUNC = 51357;
                constexpr std::uint64_t SKIP_FUNC = 51369;
                constexpr std::size_t CAVE_START = 0x5A;
                constexpr std::size_t CAVE_END = 0x169;
                constexpr std::size_t JUMP_OUT = 0x187;
                const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                    SkipSubMenuMenuPrompt<RE::CraftingSubMenus::ConstructibleObjectMenu, SKIP_FUNC>);
                InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
			}
            
            logger::debug("Installed constructible object menu patch"sv);
        }

        if (*Settings::AlchemyMenu) {
            if (isSE) {
                constexpr std::uint64_t CALL_FUNC = 50485;
                constexpr std::uint64_t SKIP_FUNC = 50447;
                constexpr std::size_t CAVE_START = 0x138;
                constexpr std::size_t CAVE_END = 0x2A9;
                constexpr std::size_t JUMP_OUT = 0x2AB;
                const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                    SkipSubMenuMenuPrompt<AlchemyMenu, SKIP_FUNC>);
                InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
			} else {
                constexpr std::uint64_t CALL_FUNC = 51377;
                constexpr std::uint64_t SKIP_FUNC = 51352;
                constexpr std::size_t CAVE_START = 0x14B;
                constexpr std::size_t CAVE_END = 0x2A0;
                constexpr std::size_t JUMP_OUT = 0x2A0;
                const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                    SkipSubMenuMenuPrompt<AlchemyMenu, SKIP_FUNC>);
                InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
			}
            
            logger::debug("Installed alchemy menu patch"sv);
        }

        if (*Settings::SmithingMenu) {
            if (isSE) {
                constexpr std::uint64_t CALL_FUNC = 50451;
                constexpr std::uint64_t SKIP_FUNC = 50477;
                constexpr std::size_t CAVE_START = 0x7C;
                constexpr std::size_t CAVE_END = 0x191;
                constexpr std::size_t JUMP_OUT = 0x1E5;
                const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                    SkipSubMenuMenuPrompt<RE::CraftingSubMenus::SmithingMenu, SKIP_FUNC>);
                InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
			} else {
                constexpr std::uint64_t CALL_FUNC = 51356;
                constexpr std::uint64_t SKIP_FUNC = 51370;
                constexpr std::size_t CAVE_START = 0x7D;
                constexpr std::size_t CAVE_END = 0x191;
                constexpr std::size_t JUMP_OUT = 0x1E4;
                const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                    SkipSubMenuMenuPrompt<RE::CraftingSubMenus::SmithingMenu, SKIP_FUNC>);
                InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
			}
            
            logger::debug("Installed smithing menu patch"sv);
        }

        if (*Settings::EnchantmentLearned) {
            InstallEnchantmentLearnedPatch(isSE);
        }

        {

            if (*Settings::EnchantmentCrafted) {
                if (isSE) {
                    constexpr std::uint64_t CALL_FUNC = 50487;
                    constexpr std::uint64_t SKIP_FUNC = 50450;
                    constexpr std::size_t CAVE_START = 0x160;
                    constexpr std::size_t CAVE_END = 0x1FD;
                    constexpr std::size_t JUMP_OUT = 0x260;
                    const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                        SkipSubMenuMenuPrompt<RE::CraftingSubMenus::EnchantConstructMenu, SKIP_FUNC>);
                    InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
				} else {
                    constexpr std::uint64_t CALL_FUNC = 51379;
                    constexpr std::uint64_t SKIP_FUNC = 51355;
                    constexpr std::size_t CAVE_START = 0x185;
                    constexpr std::size_t CAVE_END = 0x23F;
                    constexpr std::size_t JUMP_OUT = 0x2C2;
                    const auto fnAddr = reinterpret_cast<std::uintptr_t>(
                        SkipSubMenuMenuPrompt<RE::CraftingSubMenus::EnchantConstructMenu, SKIP_FUNC>);
                    InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
				}

                logger::debug("Installed enchantment crafted patch"sv);
            }

            if (*Settings::EnchantingMenuExit) {
                if (isSE) {
                    constexpr std::uint64_t CALL_FUNC = 50487;
                    constexpr std::size_t CAVE_START = 0x74;
                    constexpr std::size_t CAVE_END = 0x111;
                    constexpr std::size_t JUMP_OUT = 0x111;
                    const auto fnAddr = reinterpret_cast<std::uintptr_t>(CloseEnchantingMenu);
                    InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
				} else {
                    constexpr std::uint64_t CALL_FUNC = 51379;
                    constexpr std::size_t CAVE_START = 0x6F;
                    constexpr std::size_t CAVE_END = 0x129;
                    constexpr std::size_t JUMP_OUT = 0x129;
                    const auto fnAddr = reinterpret_cast<std::uintptr_t>(CloseEnchantingMenu);
                    InstallSubMenuPatch<CALL_FUNC, CAVE_START, CAVE_END, JUMP_OUT>(fnAddr);
				}

                logger::debug("Installed enchanting menu exit patch"sv);
            }
        }

        if (*Settings::Poison) {
            InstallPoisonPatch();
        }

        logger::debug("Installed hooks"sv);
    }
}  // namespace Hooks
