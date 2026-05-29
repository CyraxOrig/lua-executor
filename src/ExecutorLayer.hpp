#pragma once
#include <Geode/Geode.hpp>
#include "LuaBindings.hpp"
#include <vector>
#include <string>

using namespace geode::prelude;

// ─── Console line ─────────────────────────────────────────────────────────────
struct ConsoleLine {
    std::string text;
    bool isError;
};

// ─── ExecutorLayer ────────────────────────────────────────────────────────────
// A draggable popup window with:
//   • Code editor (scrollable CCTextFieldTTF)
//   • Execute / Clear / Help buttons
//   • Console output panel
class ExecutorLayer : public CCLayer {
public:
    static ExecutorLayer* create();
    bool init() override;

    // Show/hide as a scene overlay
    void show();
    void hide();

    // Toggle visibility
    static void toggle();

    // Static instance accessor (singleton overlay)
    static ExecutorLayer* getInstance();

protected:
    // UI nodes
    CCScale9Sprite* m_background       = nullptr;
    CCScale9Sprite* m_editorBg         = nullptr;
    CCScale9Sprite* m_consoleBg        = nullptr;
    CCTextFieldTTF*  m_codeField        = nullptr;
    CCLabelTTF*      m_consoleLabel     = nullptr;
    CCScrollView*    m_consoleScroll    = nullptr;
    CCMenu*          m_buttonMenu       = nullptr;
    CCLabelBMFont*   m_titleLabel       = nullptr;
    CCLabelTTF*      m_lineCountLabel   = nullptr;

    // State
    LuaState         m_lua;
    std::vector<ConsoleLine> m_lines;
    bool             m_isDragging       = false;
    CCPoint          m_dragOffset       = {};
    std::string      m_lastCode;

    // Internal helpers
    void buildUI();
    void buildButtons();
    void buildConsole();

    void onExecute(CCObject*);
    void onClear(CCObject*);
    void onHelp(CCObject*);
    void onClose(CCObject*);
    void onCopyConsole(CCObject*);

    void appendConsole(const std::string& text, bool isError);
    void clearConsole();
    void refreshConsoleView();

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;

    // Make header text for editor (line numbers hint)
    void updateLineCount();
};
