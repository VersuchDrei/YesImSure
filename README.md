# Yes, I'm Sure

Skips the "are you sure?" confirmation prompts in Skyrim's crafting menus.

## Supported runtimes
* Skyrim SE 1.5.97
* Skyrim AE 1.6.x
* Skyrim AE 1.7.99 / 1.7.104 *(patch offsets re-verified against the 1.7.104
  binary — no offset changes were needed; all 7 prompts tested in-game on 1.7.104)*

## Build dependencies
* [CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG) &ge; 7.0.0 — vendored as the
  `extern/CommonLibSSE` git submodule (branch `ng`).
* [vcpkg](https://github.com/microsoft/vcpkg) for the remaining packages
  (`tomlplusplus`, `xbyak`, `spdlog`, `directxtk`, `directxmath`, `rapidcsv`, `fmt`).

```
git clone --recurse-submodules https://github.com/VersuchDrei/YesImSure.git
# or, in an existing clone:
git submodule update --init --recursive
```

Then configure with a CMake preset, e.g. `cmake --preset build-release-msvc`
(`VCPKG_ROOT` must point at a vcpkg checkout).

## End user dependencies
* [SKSE64](https://skse.silverlock.org/) — the build matching your game version.
* [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
  with the database for your game version.

## Patches
Patch | Description
--- | ---
`ConstructibleObjectMenu` | Skips message prompts related to the constructible object menu.
`AlchemyMenu` | Skips message prompts related to the alchemy object menu.
`SmithingMenu` | Skips message prompts related to the smithing object menu.
`EnchantmentLearned` | Skips the enchantment learned message prompt.
`EnchantmentCrafted` | Skips the enchantment crafted message prompt.
`EnchantingMenuExit` | Skips the exit menu prompt when you are in the middle of enchanting an item.
`Poison` | Skips the poison weapon prompt.
