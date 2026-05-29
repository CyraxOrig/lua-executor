#include "LuaBindings.hpp"
#include <Geode/Geode.hpp>
#include <sstream>

using namespace geode::prelude;
using namespace LuaHelpers;

// ═════════════════════════════════════════════════════════════════════════════
//  print() override — redirects to the executor console
// ═════════════════════════════════════════════════════════════════════════════
static int lua_gdPrint(lua_State* L) {
    int n = lua_gettop(L);
    std::string result;
    for (int i = 1; i <= n; i++) {
        if (i > 1) result += "\t";
        if (lua_isstring(L, i))      result += lua_tostring(L, i);
        else if (lua_isboolean(L, i)) result += lua_toboolean(L, i) ? "true" : "false";
        else if (lua_isnil(L, i))    result += "nil";
        else if (lua_isnumber(L, i)) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%g", lua_tonumber(L, i));
            result += buf;
        } else {
            result += lua_typename(L, lua_type(L, i));
        }
    }
    emit(result);
    return 0;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.getPlayer()  →  table with player state
// ═════════════════════════════════════════════════════════════════════════════
static int lua_getPlayer(lua_State* L) {
    auto* p = getPlayer();
    if (!p) return pushError(L, "Not in a level");

    lua_newtable(L);

    // Position
    auto pos = p->getPosition();
    lua_pushnumber(L, pos.x); lua_setfield(L, -2, "x");
    lua_pushnumber(L, pos.y); lua_setfield(L, -2, "y");

    // Rotation
    lua_pushnumber(L, p->getRotation()); lua_setfield(L, -2, "rotation");

    // Scale
    lua_pushnumber(L, p->getScaleX()); lua_setfield(L, -2, "scaleX");
    lua_pushnumber(L, p->getScaleY()); lua_setfield(L, -2, "scaleY");

    // Velocity
    lua_pushnumber(L, p->m_yVelocity); lua_setfield(L, -2, "velocityY");

    // Flags
    lua_pushboolean(L, p->m_isDead);     lua_setfield(L, -2, "isDead");
    lua_pushboolean(L, p->m_isOnGround); lua_setfield(L, -2, "isOnGround");
    lua_pushboolean(L, p->m_isUpsideDown); lua_setfield(L, -2, "isUpsideDown");

    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.teleport(x, y)
// ═════════════════════════════════════════════════════════════════════════════
static int lua_teleport(lua_State* L) {
    auto* p = getPlayer();
    if (!p) return pushError(L, "Not in a level");

    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    p->setPosition({ x, y });
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.setPosition(x, y)  — alias for teleport
// ═════════════════════════════════════════════════════════════════════════════
static int lua_setPosition(lua_State* L) { return lua_teleport(L); }

// ═════════════════════════════════════════════════════════════════════════════
//  gd.setVelocity(vy)
// ═════════════════════════════════════════════════════════════════════════════
static int lua_setVelocity(lua_State* L) {
    auto* p = getPlayer();
    if (!p) return pushError(L, "Not in a level");
    p->m_yVelocity = luaL_checknumber(L, 1);
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.kill()
// ═════════════════════════════════════════════════════════════════════════════
static int lua_kill(lua_State* L) {
    auto* p = getPlayer();
    if (!p) return pushError(L, "Not in a level");
    p->killPlayer();
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.noclip(enabled: bool)
// ═════════════════════════════════════════════════════════════════════════════
// We toggle a global that a hook reads — see main.cpp
bool g_noclipEnabled = false;

static int lua_noclip(lua_State* L) {
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    g_noclipEnabled = lua_toboolean(L, 1);
    emit(std::string("Noclip ") + (g_noclipEnabled ? "enabled" : "disabled"));
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.setSpeed(speed: number)
//  Sets PlayLayer's game speed (1.0 = normal, 2.0 = double, etc.)
// ═════════════════════════════════════════════════════════════════════════════
static int lua_setSpeed(lua_State* L) {
    auto* pl = getPlayLayer();
    if (!pl) return pushError(L, "Not in a level");
    float spd = (float)luaL_checknumber(L, 1);
    if (spd <= 0.0f || spd > 20.0f)
        return pushError(L, "Speed must be between 0 and 20");
    CCDirector::get()->getScheduler()->setTimeScale(spd);
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.getSpeed()  →  number
// ═════════════════════════════════════════════════════════════════════════════
static int lua_getSpeed(lua_State* L) {
    lua_pushnumber(L, CCDirector::get()->getScheduler()->getTimeScale());
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.getAttempts()  →  number
// ═════════════════════════════════════════════════════════════════════════════
static int lua_getAttempts(lua_State* L) {
    auto* pl = getPlayLayer();
    if (!pl) return pushError(L, "Not in a level");
    lua_pushinteger(L, pl->m_attempts);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.getPercent()  →  number (0–100)
// ═════════════════════════════════════════════════════════════════════════════
static int lua_getPercent(lua_State* L) {
    auto* pl = getPlayLayer();
    if (!pl) return pushError(L, "Not in a level");
    lua_pushnumber(L, pl->getCurrentPercentInt());
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.getLevelInfo()  →  table
// ═════════════════════════════════════════════════════════════════════════════
static int lua_getLevelInfo(lua_State* L) {
    auto* pl = getPlayLayer();
    if (!pl || !pl->m_level) return pushError(L, "Not in a level");
    auto* lvl = pl->m_level;

    lua_newtable(L);
    lua_pushstring(L, lvl->m_levelName.c_str());  lua_setfield(L, -2, "name");
    lua_pushinteger(L, lvl->m_levelID.value());   lua_setfield(L, -2, "id");
    lua_pushstring(L, lvl->m_creatorName.c_str()); lua_setfield(L, -2, "creator");
    lua_pushinteger(L, lvl->m_stars.value());     lua_setfield(L, -2, "stars");
    lua_pushinteger(L, (int)lvl->m_difficulty);   lua_setfield(L, -2, "difficulty");
    lua_pushboolean(L, lvl->m_isVerified);        lua_setfield(L, -2, "verified");
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.notify(msg: string, [duration: number])
//  Shows a Geode notification popup
// ═════════════════════════════════════════════════════════════════════════════
static int lua_notify(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    float dur = lua_isnumber(L, 2) ? (float)lua_tonumber(L, 2) : 3.0f;
    Notification::create(msg, NotificationIcon::None, dur)->show();
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.setGravity(val: number)
// ═════════════════════════════════════════════════════════════════════════════
static int lua_setGravity(lua_State* L) {
    auto* p = getPlayer();
    if (!p) return pushError(L, "Not in a level");
    float g = (float)luaL_checknumber(L, 1);
    p->m_gravityMod = g;
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.isInLevel()  →  bool
// ═════════════════════════════════════════════════════════════════════════════
static int lua_isInLevel(lua_State* L) {
    lua_pushboolean(L, getPlayLayer() != nullptr);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.isPaused()  →  bool
// ═════════════════════════════════════════════════════════════════════════════
static int lua_isPaused(lua_State* L) {
    auto* pl = getPlayLayer();
    lua_pushboolean(L, pl ? pl->m_isPaused : false);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.respawn()
// ═════════════════════════════════════════════════════════════════════════════
static int lua_respawn(lua_State* L) {
    auto* pl = getPlayLayer();
    if (!pl) return pushError(L, "Not in a level");
    pl->resetLevel();
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.setPlayerColor(r, g, b)   primary color
// ═════════════════════════════════════════════════════════════════════════════
static int lua_setPlayerColor(lua_State* L) {
    auto* p = getPlayer();
    if (!p) return pushError(L, "Not in a level");

    int r = (int)luaL_checkinteger(L, 1);
    int g = (int)luaL_checkinteger(L, 2);
    int b = (int)luaL_checkinteger(L, 3);

    p->setColor({ (GLubyte)r, (GLubyte)g, (GLubyte)b });
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.setScale(sx, [sy])
// ═════════════════════════════════════════════════════════════════════════════
static int lua_setScale(lua_State* L) {
    auto* p = getPlayer();
    if (!p) return pushError(L, "Not in a level");

    float sx = (float)luaL_checknumber(L, 1);
    float sy = lua_isnumber(L, 2) ? (float)lua_tonumber(L, 2) : sx;

    p->setScaleX(sx);
    p->setScaleY(sy);
    lua_pushboolean(L, true);
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.getCoins()  →  { normal, user }
// ═════════════════════════════════════════════════════════════════════════════
static int lua_getCoins(lua_State* L) {
    auto* gm = GameManager::get();
    lua_newtable(L);
    lua_pushinteger(L, gm->m_playerCoins);  lua_setfield(L, -2, "normal");
    lua_pushinteger(L, gm->m_playerUserCoins); lua_setfield(L, -2, "user");
    return 1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  gd.schedule(fn, delay_secs)  — calls fn after delay (uses CCScheduler)
//  NOTE: fn is called ONCE; for repeating, call schedule again inside fn
// ═════════════════════════════════════════════════════════════════════════════
//  This is tricky to implement safely without reference counting the lua_State.
//  We use a simple CCNode-based wrapper.
struct LuaScheduleData {
    lua_State* L;
    int fnRef;
};

static int lua_schedule(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    float delay = (float)luaL_checknumber(L, 2);

    // Store function reference
    lua_pushvalue(L, 1);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    auto* node = CCNode::create();
    auto* data = new LuaScheduleData{ L, ref };

    node->runAction(CCSequence::create(
        CCDelayTime::create(delay),
        CCCallFuncN::create(node, [data](CCNode*) {
            lua_rawgeti(data->L, LUA_REGISTRYINDEX, data->fnRef);
            if (lua_pcall(data->L, 0, 0, 0) != LUA_OK) {
                std::string err = lua_tostring(data->L, -1);
                emit("[schedule error] " + err, true);
                lua_pop(data->L, 1);
            }
            luaL_unref(data->L, LUA_REGISTRYINDEX, data->fnRef);
            delete data;
        }),
        nullptr
    ));

    CCDirector::get()->getRunningScene()->addChild(node);
    lua_pushboolean(L, true);
    return 1;
}

// ─── Registration table ───────────────────────────────────────────────────────
static const luaL_Reg gd_lib[] = {
    // Player
    { "getPlayer",      lua_getPlayer      },
    { "teleport",       lua_teleport       },
    { "setPosition",    lua_setPosition    },
    { "setVelocity",    lua_setVelocity    },
    { "setGravity",     lua_setGravity     },
    { "setScale",       lua_setScale       },
    { "setPlayerColor", lua_setPlayerColor },
    { "kill",           lua_kill           },
    { "noclip",         lua_noclip         },
    { "respawn",        lua_respawn        },
    // Level / game state
    { "isInLevel",      lua_isInLevel      },
    { "isPaused",       lua_isPaused       },
    { "getAttempts",    lua_getAttempts    },
    { "getPercent",     lua_getPercent     },
    { "getLevelInfo",   lua_getLevelInfo   },
    { "getSpeed",       lua_getSpeed       },
    { "setSpeed",       lua_setSpeed       },
    { "getCoins",       lua_getCoins       },
    // Utility
    { "notify",         lua_notify         },
    { "schedule",       lua_schedule       },
    { nullptr, nullptr }
};

// ═════════════════════════════════════════════════════════════════════════════
//  LuaState implementation
// ═════════════════════════════════════════════════════════════════════════════

LuaState::LuaState() {
    L = luaL_newstate();
    luaL_openlibs(L);  // open standard libs (math, string, table, etc.)
    registerGDBindings();
}

LuaState::~LuaState() {
    if (L) {
        lua_close(L);
        L = nullptr;
    }
}

void LuaState::registerPrintOverride() {
    // Replace global print() with our console emitter
    lua_pushcfunction(L, lua_gdPrint);
    lua_setglobal(L, "print");
}

void LuaState::registerGDTable() {
    lua_newtable(L);
    luaL_setfuncs(L, gd_lib, 0);

    // gd.VERSION = "1.0.0"
    lua_pushstring(L, "1.0.0");
    lua_setfield(L, -2, "VERSION");

    lua_setglobal(L, "gd");
}

void LuaState::registerGDBindings() {
    registerPrintOverride();
    registerGDTable();

    // Add a helpful startup message with available APIs
    const char* helpScript = R"(
-- Available APIs:
-- gd.getPlayer()       → table {x,y,rotation,velocityY,isDead,...}
-- gd.teleport(x,y)
-- gd.setVelocity(vy)
-- gd.setSpeed(s)       -- 1.0 = normal, 2.0 = double
-- gd.getSpeed()
-- gd.setGravity(g)
-- gd.setScale(sx,sy?)
-- gd.setPlayerColor(r,g,b)
-- gd.kill()
-- gd.noclip(bool)
-- gd.respawn()
-- gd.isInLevel()
-- gd.isPaused()
-- gd.getAttempts()
-- gd.getPercent()
-- gd.getLevelInfo()
-- gd.getCoins()
-- gd.notify(msg,dur?)
-- gd.schedule(fn, delaySecs)
)";
    // We just set it as a global string for reference; don't execute
    lua_pushstring(L, helpScript);
    lua_setglobal(L, "__HELP__");
}

std::string LuaState::execute(const std::string& code) {
    int status = luaL_loadstring(L, code.c_str());
    if (status != LUA_OK) {
        std::string err = lua_tostring(L, -1);
        lua_pop(L, 1);
        return "[Syntax Error] " + err;
    }

    status = lua_pcall(L, 0, 0, 0);
    if (status != LUA_OK) {
        std::string err = lua_tostring(L, -1);
        lua_pop(L, 1);
        return "[Runtime Error] " + err;
    }

    return "";  // success
}
