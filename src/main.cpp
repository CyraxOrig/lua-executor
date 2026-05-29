#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

#include "ExecutorLayer.hpp"
#include "LuaBindings.hpp"

using namespace geode::prelude;

// Exposed from LuaBindings.cpp
extern bool g_noclipEnabled;

// ═════════════════════════════════════════════════════════════════════════════
//  PauseLayer hook — adds "Lua" button to the pause menu
// ═════════════════════════════════════════════════════════════════════════════
class $modify(LuaExecPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto* btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Lua", "bigFont.fnt", "GJ_button_06.png", 0.5f),
            this,
            menu_selector(LuaExecPauseLayer::onOpenLuaExecutor)
        );
        btn->setID("lua-executor-btn");

        // Try to add to an existing button area
        CCMenu* target = nullptr;
        if (auto* m = typeinfo_cast<CCMenu*>(getChildByID("right-button-menu")))
            target = m;
        else if (auto* m = typeinfo_cast<CCMenu*>(getChildByID("left-button-menu")))
            target = m;

        if (target) {
            target->addChild(btn);
            target->updateLayout();
        } else {
            // Fallback: create our own menu in the top-right
            auto* menu = CCMenu::create(btn, nullptr);
            menu->setPosition({ CCDirector::get()->getWinSize().width - 40.0f, 40.0f });
            addChild(menu, 10);
        }
    }

    void onOpenLuaExecutor(CCObject*) {
        ExecutorLayer::toggle();
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  PlayLayer hook — noclip implementation
//  Blocks the killPlayer call when g_noclipEnabled is true
// ═════════════════════════════════════════════════════════════════════════════
class $modify(LuaExecPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (g_noclipEnabled && player == m_player1) {
            // Silently ignore death
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  Keyboard hook — toggle executor with configured key (default F4)
// ═════════════════════════════════════════════════════════════════════════════
class $modify(LuaExecKeyboard, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat) {
        if (isKeyDown && !isKeyRepeat) {
            // Map setting string to key code
            auto keySetting = Mod::get()->getSettingValue<std::string>("open-keybind");

            enumKeyCodes targetKey = KEY_F4;
            if (keySetting == "F1")  targetKey = KEY_F1;
            else if (keySetting == "F2")  targetKey = KEY_F2;
            else if (keySetting == "F3")  targetKey = KEY_F3;
            else if (keySetting == "F4")  targetKey = KEY_F4;
            else if (keySetting == "F5")  targetKey = KEY_F5;
            else if (keySetting == "F6")  targetKey = KEY_F6;
            else if (keySetting == "F7")  targetKey = KEY_F7;
            else if (keySetting == "F8")  targetKey = KEY_F8;
            else if (keySetting == "F9")  targetKey = KEY_F9;
            else if (keySetting == "F10") targetKey = KEY_F10;
            else if (keySetting == "F11") targetKey = KEY_F11;
            else if (keySetting == "F12") targetKey = KEY_F12;

            if (key == targetKey) {
                ExecutorLayer::toggle();
                return true;  // consume event
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat);
    }
};

// ═════════════════════════════════════════════════════════════════════════════
//  Mod entry point
// ═════════════════════════════════════════════════════════════════════════════
$on_mod(Loaded) {
    log::info("Lua Executor loaded! Press {} to open.",
        Mod::get()->getSettingValue<std::string>("open-keybind"));
}
