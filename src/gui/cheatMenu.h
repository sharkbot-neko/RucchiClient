#pragma once

#include "client/client.h"
#include <cstddef>
#include <string>
#include <iostream>
#include <IVideoDriver.h>
#include "client/fontengine.h"

enum CheatMenuEntryType
{
	CHEAT_MENU_ENTRY_TYPE_HEAD,
	CHEAT_MENU_ENTRY_TYPE_CATEGORY,
	CHEAT_MENU_ENTRY_TYPE_ENTRY,
};

struct CheatsContext {
	Client* client;
};

struct Cheats {
    CheatMenuEntryType type;
    std::string name;
    std::function<void(CheatsContext)> function;
    bool toggled = false;
    std::vector<Cheats> children;

    Cheats(std::string n, std::vector<Cheats> ch)
        : type(CHEAT_MENU_ENTRY_TYPE_CATEGORY), name(std::move(n)), children(std::move(ch)) {}

    template<typename Func>
    Cheats(std::string n, Func fn, bool initial = false)
        : type(CHEAT_MENU_ENTRY_TYPE_ENTRY), name(std::move(n)), function(fn), toggled(initial) {}
};

class CheatMenu
{
public:

	CheatMenu(Client *client);

	ClientScripting *getScript() { return m_client->getScript(); }

	void draw(video::IVideoDriver *driver, bool show_debug);

	void drawHUD(video::IVideoDriver *driver, double dtime);

	void draw2DRectangleOutline(video::IVideoDriver *driver, const core::recti& pos, video::SColor color);

	void drawEntry(video::IVideoDriver *driver, std::string name, int number,
			bool selected, bool active,
			CheatMenuEntryType entry_type = CHEAT_MENU_ENTRY_TYPE_ENTRY);

	void selectUp();
	void selectDown();
	void selectLeft();
	void selectRight();
	void selectConfirm();

	bool m_cheat_layer = false;

	std::vector<Cheats> cheatMenu = {
		Cheats("Player", {
			Cheats("Test", [](CheatsContext context) {

			}, false)
		}),
	};

private:
	Client* client_;

	int m_selected_cheat = 0;
	int m_selected_category = 0;

	int m_head_height = 50;
	int m_entry_height = 40;
	int m_entry_width = 200;
	int m_gap = 3;

	video::SColor m_bg_color = video::SColor(192, 255, 145, 88);
	video::SColor m_active_bg_color = video::SColor(192, 255, 87, 53);
	video::SColor m_font_color = video::SColor(255, 0, 0, 0);
	video::SColor m_selected_font_color = video::SColor(255, 255, 252, 88);

	FontMode fontStringToEnum(std::string str);

	Client *m_client;

	gui::IGUIFont *m_font = nullptr;
	v2u32 m_fontsize;

	float m_rainbow_offset = 0.0;

    void drawRect(video::IVideoDriver *driver, std::string name,
                            int x, int y,
                            int width, int height,
                            bool active, bool selected);
};