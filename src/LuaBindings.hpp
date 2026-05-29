#pragma once
#include <Geode/Geode.hpp>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <string>
#include <vector>
#include <functional>

using namespace geode::prelude;

// ─── Console output callback ─────────────────────────────────────────────────
// Set this before executing scripts so print() calls can update your UI
inline std::function<void(const std::string&, bool isError)> g_consoleCallback;

// ─── LuaState wrapper ────────────────────────────────────────────────────────
class LuaState {
public:
    lua_State* L = nullptr;

    LuaState();
    ~LuaState();

    // Execute a string of Lua code; returns error message or "" on success
    std::string execute(const std::string& code);

    // Register all GD bindings into L
    void registerGDBindings();

private:
    void registerPrintOverride();
    void registerGDTable();
    void registerPlayerTable();
    void registerLevelTable();
    void registerGameManagerTable();
};

// ─── Helpers ─────────────────────────────────────────────────────────────────
namespace LuaHelpers {
    // Safe get of PlayLayer; returns nullptr if not in a level
    inline PlayLayer* getPlayLayer() {
        return PlayLayer::get();
    }

    // Safe get of player; returns nullptr if not in a level
    inline PlayerObject* getPlayer() {
        auto* pl = getPlayLayer();
        return pl ? pl->m_player1 : nullptr;
    }

    // Push a formatted error onto the Lua stack
    inline int pushError(lua_State* L, const char* msg) {
        lua_pushnil(L);
        lua_pushstring(L, msg);
        return 2;
    }

    // Emit a line to the console callback
    inline void emit(const std::string& msg, bool isError = false) {
        if (g_consoleCallback) g_consoleCallback(msg, isError);
    }
}
