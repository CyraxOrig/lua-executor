#include "ExecutorLayer.hpp"
#include <Geode/Geode.hpp>
#include <sstream>

using namespace geode::prelude;

// ─── Singleton storage ────────────────────────────────────────────────────────
static ExecutorLayer* s_instance = nullptr;

ExecutorLayer* ExecutorLayer::getInstance() { return s_instance; }

ExecutorLayer* ExecutorLayer::create() {
    auto* ret = new ExecutorLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

// ═════════════════════════════════════════════════════════════════════════════
//  init
// ═════════════════════════════════════════════════════════════════════════════
bool ExecutorLayer::init() {
    if (!CCLayer::init()) return false;

    auto winSize = CCDirector::get()->getWinSize();

    // Touch listener
    setTouchEnabled(true);
    setTouchMode(kCCTouchesOneByOne);

    // Background overlay (semi-transparent click-through)
    auto overlay = CCLayerColor::create({ 0, 0, 0, 0 });
    addChild(overlay, 0);

    buildUI();

    // Hook the Lua console callback
    g_consoleCallback = [this](const std::string& msg, bool isError) {
        // Must dispatch on main thread
        CCDirector::get()->getScheduler()->performFunctionInCocosThread([this, msg, isError]() {
            appendConsole(msg, isError);
        });
    };

    s_instance = this;
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  buildUI
// ═════════════════════════════════════════════════════════════════════════════
void ExecutorLayer::buildUI() {
    auto winSize = CCDirector::get()->getWinSize();

    constexpr float W = 480.0f;
    constexpr float H = 320.0f;
    const CCPoint center = { winSize.width * 0.5f, winSize.height * 0.5f };

    // ── Main window ──────────────────────────────────────────────────────────
    m_background = CCScale9Sprite::create("GJ_square02.png", { 0, 0, 80, 80 });
    m_background->setContentSize({ W, H });
    m_background->setPosition(center);
    m_background->setZOrder(1);
    addChild(m_background);

    // ── Title bar ─────────────────────────────────────────────────────────────
    auto titleBg = CCScale9Sprite::create("GJ_square01.png", { 0, 0, 80, 80 });
    titleBg->setContentSize({ W, 24.0f });
    titleBg->setPosition({ center.x, center.y + H * 0.5f - 12.0f });
    titleBg->setZOrder(2);
    addChild(titleBg);

    m_titleLabel = CCLabelBMFont::create("Lua Executor v1.0", "goldFont.fnt");
    m_titleLabel->setScale(0.45f);
    m_titleLabel->setPosition({ center.x, center.y + H * 0.5f - 12.0f });
    m_titleLabel->setZOrder(3);
    addChild(m_titleLabel);

    // ── Code editor area ──────────────────────────────────────────────────────
    constexpr float EDITOR_H  = 160.0f;
    constexpr float CONSOLE_H = 100.0f;
    constexpr float BTN_H     = 30.0f;
    constexpr float PAD       = 8.0f;

    float editorY  = center.y + H * 0.5f - 24.0f - PAD - EDITOR_H * 0.5f;
    float consoleY = editorY - EDITOR_H * 0.5f - PAD - CONSOLE_H * 0.5f;
    float btnY     = consoleY - CONSOLE_H * 0.5f - PAD - BTN_H * 0.5f;

    m_editorBg = CCScale9Sprite::create("square02_001.png", { 0, 0, 80, 80 });
    m_editorBg->setContentSize({ W - PAD * 2, EDITOR_H });
    m_editorBg->setPosition({ center.x, editorY });
    m_editorBg->setOpacity(200);
    m_editorBg->setZOrder(2);
    addChild(m_editorBg);

    // Placeholder / TTF code field
    m_codeField = CCTextFieldTTF::textFieldWithPlaceHolder(
        "-- Write your Lua code here...\n-- Example:\n-- print(gd.getPercent())",
        "Roboto-Regular.ttf", 11.0f
    );
    m_codeField->setPosition({ center.x, editorY });
    m_codeField->setDimensions({ W - PAD * 4, EDITOR_H - 8.0f });
    m_codeField->setZOrder(3);
    m_codeField->setColor({ 220, 240, 200 });
    addChild(m_codeField);

    // ── Console area ──────────────────────────────────────────────────────────
    m_consoleBg = CCScale9Sprite::create("square02_001.png", { 0, 0, 80, 80 });
    m_consoleBg->setContentSize({ W - PAD * 2, CONSOLE_H });
    m_consoleBg->setPosition({ center.x, consoleY });
    m_consoleBg->setOpacity(180);
    m_consoleBg->setColor({ 10, 10, 10 });
    m_consoleBg->setZOrder(2);
    addChild(m_consoleBg);

    // Console output label (TTF, left-aligned, small)
    m_consoleLabel = CCLabelTTF::create(
        "> Executor ready. Type code above and press Execute.",
        "Roboto-Regular.ttf", 9.0f,
        { W - PAD * 4, CONSOLE_H - 6.0f },
        kCCTextAlignmentLeft,
        kCCVerticalTextAlignmentTop
    );
    m_consoleLabel->setPosition({ center.x, consoleY });
    m_consoleLabel->setColor({ 100, 255, 100 });
    m_consoleLabel->setZOrder(3);
    addChild(m_consoleLabel);

    // ── Buttons ───────────────────────────────────────────────────────────────
    buildButtons();
    for (auto* child : CCArrayExt<CCNode*>(m_buttonMenu->getChildren())) {
        // position is set inside buildButtons relative to btnY, center.x
    }
    m_buttonMenu->setPosition({ center.x, btnY });

    // ── Close button ──────────────────────────────────────────────────────────
    auto closeSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    closeSprite->setScale(0.8f);
    auto closeBtn = CCMenuItemSpriteExtra::create(closeSprite, this,
        menu_selector(ExecutorLayer::onClose));
    closeBtn->setPosition({ center.x + W * 0.5f - 14.0f, center.y + H * 0.5f - 14.0f });

    auto closeMenu = CCMenu::create(closeBtn, nullptr);
    closeMenu->setPosition({ 0, 0 });
    closeMenu->setZOrder(5);
    addChild(closeMenu);
}

// ═════════════════════════════════════════════════════════════════════════════
//  buildButtons
// ═════════════════════════════════════════════════════════════════════════════
void ExecutorLayer::buildButtons() {
    auto makeBtn = [](const char* label, float scale = 0.55f) -> ButtonSprite* {
        return ButtonSprite::create(label, "bigFont.fnt", "GJ_button_01.png", scale);
    };

    auto btnExec = CCMenuItemSpriteExtra::create(
        makeBtn("Execute"), this, menu_selector(ExecutorLayer::onExecute));

    auto btnClear = CCMenuItemSpriteExtra::create(
        makeBtn("Clear"), this, menu_selector(ExecutorLayer::onClear));

    auto btnHelp = CCMenuItemSpriteExtra::create(
        makeBtn("Help"), this, menu_selector(ExecutorLayer::onHelp));

    auto btnCopy = CCMenuItemSpriteExtra::create(
        makeBtn("Copy Log"), this, menu_selector(ExecutorLayer::onCopyConsole));

    m_buttonMenu = CCMenu::create(btnExec, btnClear, btnHelp, btnCopy, nullptr);
    m_buttonMenu->setLayout(RowLayout::create()
        ->setGap(8.0f)
        ->setAxisAlignment(AxisAlignment::Center)
    );
    m_buttonMenu->setContentWidth(460.0f);
    m_buttonMenu->updateLayout();
    m_buttonMenu->setZOrder(4);
    addChild(m_buttonMenu);
}

// ═════════════════════════════════════════════════════════════════════════════
//  show / hide / toggle
// ═════════════════════════════════════════════════════════════════════════════
void ExecutorLayer::show() {
    if (this->getParent()) return;
    auto* scene = CCDirector::get()->getRunningScene();
    scene->addChild(this, 999);
    setVisible(true);

    // Pop-in animation
    setScale(0.8f);
    runAction(CCEaseBackOut::create(CCScaleTo::create(0.2f, 1.0f)));
}

void ExecutorLayer::hide() {
    runAction(CCSequence::create(
        CCEaseBackIn::create(CCScaleTo::create(0.15f, 0.8f)),
        CCCallFunc::create(this, callfunc_selector(ExecutorLayer::removeFromParent)),
        nullptr
    ));
}

void ExecutorLayer::toggle() {
    if (s_instance && s_instance->getParent()) {
        s_instance->hide();
    } else {
        if (!s_instance) {
            s_instance = ExecutorLayer::create();
            s_instance->retain();
        }
        s_instance->show();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  Button callbacks
// ═════════════════════════════════════════════════════════════════════════════
void ExecutorLayer::onExecute(CCObject*) {
    std::string code = m_codeField->getString();

    if (code.empty() || code == m_codeField->getPlaceHolder()) {
        appendConsole("[Error] No code to execute", true);
        return;
    }

    if (Mod::get()->getSettingValue<bool>("auto-clear-console")) {
        clearConsole();
    }

    appendConsole("> Executing...", false);

    std::string err = m_lua.execute(code);
    if (!err.empty()) {
        appendConsole(err, true);
    } else {
        appendConsole("> Done.", false);
    }

    m_lastCode = code;
}

void ExecutorLayer::onClear(CCObject*) {
    m_codeField->setString("");
    clearConsole();
}

void ExecutorLayer::onHelp(CCObject*) {
    clearConsole();
    appendConsole("=== Lua Executor GD API ===", false);
    appendConsole("gd.isInLevel()            → bool", false);
    appendConsole("gd.getPlayer()            → {x,y,rotation,velocityY,...}", false);
    appendConsole("gd.teleport(x, y)", false);
    appendConsole("gd.setVelocity(vy)", false);
    appendConsole("gd.setSpeed(s)            1.0=normal", false);
    appendConsole("gd.getSpeed()             → number", false);
    appendConsole("gd.setGravity(g)", false);
    appendConsole("gd.setScale(sx, sy?)", false);
    appendConsole("gd.setPlayerColor(r,g,b)", false);
    appendConsole("gd.kill() / gd.respawn()", false);
    appendConsole("gd.noclip(bool)", false);
    appendConsole("gd.getAttempts()          → number", false);
    appendConsole("gd.getPercent()           → 0-100", false);
    appendConsole("gd.getLevelInfo()         → {name,id,creator,...}", false);
    appendConsole("gd.getCoins()             → {normal,user}", false);
    appendConsole("gd.notify(msg, dur?)", false);
    appendConsole("gd.schedule(fn, secs)", false);
    appendConsole("print(...)                → this console", false);
}

void ExecutorLayer::onClose(CCObject*) {
    hide();
}

void ExecutorLayer::onCopyConsole(CCObject*) {
    std::string all;
    for (auto& line : m_lines)
        all += line.text + "\n";

#ifdef GEODE_IS_WINDOWS
    OpenClipboard(nullptr);
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, all.size() + 1);
    memcpy(GlobalLock(hg), all.c_str(), all.size() + 1);
    GlobalUnlock(hg);
    SetClipboardData(CF_TEXT, hg);
    CloseClipboard();
#endif

    Notification::create("Console copied!", NotificationIcon::Success, 2.0f)->show();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Console helpers
// ═════════════════════════════════════════════════════════════════════════════
void ExecutorLayer::appendConsole(const std::string& text, bool isError) {
    constexpr size_t MAX_LINES = 200;

    // Split on newlines
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        m_lines.push_back({ line, isError });
    }

    while (m_lines.size() > MAX_LINES)
        m_lines.erase(m_lines.begin());

    refreshConsoleView();
}

void ExecutorLayer::clearConsole() {
    m_lines.clear();
    refreshConsoleView();
}

void ExecutorLayer::refreshConsoleView() {
    if (!m_consoleLabel) return;

    // Build last N lines that fit
    constexpr int VISIBLE_LINES = 9;
    int start = std::max(0, (int)m_lines.size() - VISIBLE_LINES);

    std::string display;
    bool hasError = false;
    for (int i = start; i < (int)m_lines.size(); i++) {
        if (i > start) display += "\n";
        display += m_lines[i].text;
        if (m_lines[i].isError) hasError = true;
    }

    m_consoleLabel->setString(display.c_str());

    // Color: red if last line is error, green otherwise
    if (!m_lines.empty() && m_lines.back().isError)
        m_consoleLabel->setColor({ 255, 80, 80 });
    else
        m_consoleLabel->setColor({ 100, 255, 100 });
}

// ═════════════════════════════════════════════════════════════════════════════
//  Touch handling — draggable window
// ═════════════════════════════════════════════════════════════════════════════
bool ExecutorLayer::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    auto pos = touch->getLocation();
    if (!m_background) return false;

    // Check if touch is in title bar (drag zone)
    auto winSize = CCDirector::get()->getWinSize();
    CCRect titleRect = {
        m_background->getPositionX() - m_background->getContentSize().width  * 0.5f,
        m_background->getPositionY() + m_background->getContentSize().height * 0.5f - 24.0f,
        m_background->getContentSize().width,
        24.0f
    };

    if (titleRect.containsPoint(pos)) {
        m_isDragging = true;
        m_dragOffset = m_background->getPosition() - pos;
        return true;
    }

    // Swallow touches inside the window
    CCRect winRect = {
        m_background->getPositionX() - m_background->getContentSize().width  * 0.5f,
        m_background->getPositionY() - m_background->getContentSize().height * 0.5f,
        m_background->getContentSize().width,
        m_background->getContentSize().height
    };

    return winRect.containsPoint(pos);
}

void ExecutorLayer::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (!m_isDragging || !m_background) return;

    CCPoint newPos = touch->getLocation() + m_dragOffset;

    // Clamp to screen
    auto winSize = CCDirector::get()->getWinSize();
    float hw = m_background->getContentSize().width  * 0.5f;
    float hh = m_background->getContentSize().height * 0.5f;

    newPos.x = std::clamp(newPos.x, hw, winSize.width  - hw);
    newPos.y = std::clamp(newPos.y, hh, winSize.height - hh);

    // Move all children by delta
    CCPoint delta = newPos - m_background->getPosition();
    for (auto* child : CCArrayExt<CCNode*>(getChildren())) {
        child->setPosition(child->getPosition() + delta);
    }
}

void ExecutorLayer::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    m_isDragging = false;
}
