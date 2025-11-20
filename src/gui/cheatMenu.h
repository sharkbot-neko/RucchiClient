#pragma once

#include "script/scripting_client.h"
#include "client/client.h"
#include "client/minimap.h"
#include "client/fontengine.h"
#include "gui/cheatMenu.h"
#include <cstddef>
#include <iostream>
#include <settings.h>
#include <cmath>
#include <algorithm>
#include "client/fontengine.h"
#include <IGUIFont.h>

// --- Data types -----------------------------------------------------------------

struct CheatsContext {
    Client* client;
};

enum CheatMenuEntryType {
    CHEAT_MENU_ENTRY_TYPE_HEAD,
    CHEAT_MENU_ENTRY_TYPE_CATEGORY,
    CHEAT_MENU_ENTRY_TYPE_ENTRY,
};

struct Cheats {
    CheatMenuEntryType type = CHEAT_MENU_ENTRY_TYPE_ENTRY;
    std::string name;
    std::function<void(CheatsContext)> function;
    bool toggled = false;
    bool is_toggle_cheat = false;
    std::vector<Cheats> children; // for categories

    // Category constructor
    Cheats(std::string n, std::vector<Cheats> ch)
        : type(CHEAT_MENU_ENTRY_TYPE_CATEGORY), name(std::move(n)), children(std::move(ch)) {}

    // Entry constructor
    template<typename Func>
    Cheats(std::string n, Func fn, bool is_toggle_cheat, bool initial = false)
        : type(CHEAT_MENU_ENTRY_TYPE_ENTRY), name(std::move(n)), function(fn), toggled(initial), is_toggle_cheat(is_toggle_cheat) {}

    Cheats() = default;
};

// -------------------------------------------------------------------------------

// Helper: safe modulo for possibly negative numbers
static int negmod(int n, int base) {
    if (base <= 0) return 0;
    n %= base;
    if (n < 0) n += base;
    return n;
}

// -------------------------------------------------------------------------------

class CheatMenu {
public:
    CheatMenu(Client* client);

    // drawing
    void draw(video::IVideoDriver* driver, bool show_debug);
    void drawHUD(video::IVideoDriver* driver, double dtime);

    // navigation
    void selectUp();
    void selectDown();
    void selectLeft();
    void selectRight();
    void selectConfirm();

    // check
    bool IsCheatEnabled(const std::string& name);

    // Public so user code may populate it
    std::vector<Cheats> cheatMenu = {
        Cheats("Player", { 
            Cheats("Kill Me", [](CheatsContext context) { 
                Client* client_ = context.client;
                client_->sendDamage(20);
                return;
            }, false) 
        }),
        
        Cheats("Render", { 
            Cheats("Coordinate display", [](CheatsContext context) { 
                return;
            }, true) 
        }),

        Cheats("Debug", { 
            Cheats("Test", [](CheatsContext context) { 
                return;
            }, true) 
        }),
    };

    bool m_cheat_layer = false;
    bool m_cheatmenu_opend = false;

private:
    Client* m_client = nullptr;
	gui::IGUIFont *m_font = nullptr;
	v2u32 m_fontsize;

	int m_head_height = 50;
	int m_entry_height = 40;
	int m_entry_width = 200;
	int m_gap = 3;

    video::SColor m_bg_color = video::SColor(200, 40, 40, 40);
    video::SColor m_active_bg_color = video::SColor(200, 60, 60, 60);
    video::SColor m_font_color = video::SColor(230, 230, 230, 230);
    video::SColor m_selected_font_color = video::SColor(255, 255, 255, 255);

    double m_rainbow_offset = 0.0;

    // Selection state
    int m_selected_category = 0; // index into cheatMenu
    int m_selected_cheat = 0;    // index into cheatMenu[m_selected_category].children

    // Internal helpers
    bool has_categories() const { return !cheatMenu.empty(); }
    bool has_category_index(int idx) const { return idx >= 0 && idx < static_cast<int>(cheatMenu.size()); }
    bool has_cheat_index(int catIdx, int cheatIdx) const {
        if (!has_category_index(catIdx)) return false;
        return cheatIdx >= 0 && cheatIdx < static_cast<int>(cheatMenu[catIdx].children.size());
    }

    void draw2DRectangleOutline(video::IVideoDriver* driver, const core::recti& pos, video::SColor color);
    void drawRect(video::IVideoDriver* driver, const std::string& name,
                  int x, int y, int width, int height, bool active, bool selected);
    void drawEntry(video::IVideoDriver* driver, const std::string& name, int number,
                   bool selected, bool active, CheatMenuEntryType entry_type);
};