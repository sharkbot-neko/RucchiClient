#include "script/scripting_client.h"
#include "client/client.h"
#include "client/minimap.h"
#include "client/fontengine.h"
#include "gui/cheatMenu.h"
#include <cstddef>
#include <iostream>
#include <settings.h>

CheatMenu::CheatMenu(Client *client) : m_client(client)
{
    client_ = client;
}

void CheatMenu::draw2DRectangleOutline(video::IVideoDriver *driver, const core::recti& pos, video::SColor color) {
	driver->draw2DLine(pos.UpperLeftCorner, core::position2di(pos.LowerRightCorner.X, pos.UpperLeftCorner.Y), color);
	driver->draw2DLine(core::position2di(pos.LowerRightCorner.X, pos.UpperLeftCorner.Y), pos.LowerRightCorner, color);
	driver->draw2DLine(pos.LowerRightCorner, core::position2di(pos.UpperLeftCorner.X, pos.LowerRightCorner.Y), color);
	driver->draw2DLine(core::position2di(pos.UpperLeftCorner.X, pos.LowerRightCorner.Y), pos.UpperLeftCorner, color);
}

void CheatMenu::drawRect(video::IVideoDriver *driver, std::string name,
		int x, int y,
		int width, int height,
		bool active, bool selected)
{
		video::SColor *bgcolor = &m_bg_color,
								  *fontcolor = &m_font_color;

		if (active)
				bgcolor = &m_active_bg_color;
		if (selected)
				fontcolor = &m_selected_font_color;

		driver->draw2DRectangle(*bgcolor, core::rect<s32>(x, y, x + width, y + height));

		if (selected)
				draw2DRectangleOutline(
					driver,
					core::rect<s32>(x - 1, y - 1, x + width, y + height),
					*fontcolor);

		int fx = x + 5,
				fy = y + (height - m_fontsize.Y) / 2;

		core::rect<s32> fontbounds(
						fx, fy, fx + m_fontsize.X * name.size(), fy + m_fontsize.Y);
		m_font->draw(name.c_str(), fontbounds, *fontcolor, false, false);
}

void CheatMenu::drawEntry(video::IVideoDriver *driver, std::string name, int number,
		bool selected, bool active, CheatMenuEntryType entry_type)
{
	int x = m_gap, y = m_gap, width = m_entry_width, height = m_entry_height;
	video::SColor *bgcolor = &m_bg_color, *fontcolor = &m_font_color;
	if (entry_type == CHEAT_MENU_ENTRY_TYPE_HEAD) {
		bgcolor = &m_active_bg_color;
		height = m_head_height;
	} else {
		bool is_category = entry_type == CHEAT_MENU_ENTRY_TYPE_CATEGORY;
		y += m_gap + m_head_height +
			 (number + (is_category ? 0 : m_selected_category)) *
					 (m_entry_height + m_gap);
		x += (is_category ? 0 : m_gap + m_entry_width);
		if (active)
			bgcolor = &m_active_bg_color;
		if (selected)
			fontcolor = &m_selected_font_color;
	}
	driver->draw2DRectangle(*bgcolor, core::rect<s32>(x, y, x + width, y + height));
	if (selected)
		draw2DRectangleOutline(
			driver,
			core::rect<s32>(x - 1, y - 1, x + width, y + height),
			*fontcolor);
	int fx = x + 5, fy = y + (height - m_fontsize.Y) / 2;
	core::rect<s32> fontbounds(
			fx, fy, fx + m_fontsize.X * name.size(), fy + m_fontsize.Y);
	m_font->draw(name.c_str(), fontbounds, *fontcolor, false, false);
}

int negmod(int n, int base)
{
	n = n % base;
	return (n < 0) ? base + n : n;
}

void CheatMenu::draw(video::IVideoDriver *driver, bool show_debug)
{
		int category_count = 0;
		for (auto category = cheatMenu.begin();
						category != cheatMenu.end(); category++) {
				bool is_selected = category_count == m_selected_category;
				drawEntry(driver, (category)->name, category_count, is_selected, false,
								CHEAT_MENU_ENTRY_TYPE_CATEGORY);
				if (is_selected && m_cheat_layer) {
						int cheat_n = (category)->children.size();
						int height = driver->getScreenSize().Height;
						int target = height / (m_entry_height + m_gap) + 1; // +1 for the "and more" effect
						int target_normal =
								(height - (m_selected_category * (m_entry_height + m_gap)))
								/ (m_entry_height + m_gap);

						if (cheat_n < target_normal) {
								int cheat_count = 0;
								for (auto cheat = (category)->children.begin();
												cheat != (category)->children.end(); cheat++) {
										drawEntry(driver, (cheat)->name, cheat_count,
														cheat_count == m_selected_cheat,
														(cheat)->toggled);
										cheat_count++;
								}
						} else {
								int base = m_selected_cheat - m_selected_category - 1;
								int drawn = 0;
								for (int i = base; i < base + target; i++, drawn++) {
										int idx = negmod(i, cheat_n);
										Cheats cheat = category->children[idx];
										int y = (drawn * (m_entry_height + m_gap)) + m_gap;
										drawRect(driver, cheat.name,
														m_gap * 2 + m_entry_width, y,
														m_entry_width, m_entry_height,
														cheat.toggled,
														idx == m_selected_cheat);
								}
						}
				}
				category_count++;
		}
}

void CheatMenu::drawHUD(video::IVideoDriver *driver, double dtime)
{
	m_rainbow_offset += dtime;

	m_rainbow_offset = fmod(m_rainbow_offset, 6.0f);

	std::vector<std::pair<std::string, core::dimension2d<u32>>> enabled_cheats;

	int cheat_count = 0;

	for (auto category = cheatMenu.begin();
			category != cheatMenu.end(); category++) {
		for (auto cheat = category->children.begin();
				cheat != category->children.end(); cheat++) {
			if ((cheat)->toggled) {
				std::string cheat_str = (cheat)->name;
				core::dimension2d<u32> dim = 
							m_font->getDimension(utf8_to_wide(cheat_str).c_str());
				enabled_cheats.push_back(std::make_pair(cheat_str, dim));
				cheat_count++;
			}
		}
	}

	if (enabled_cheats.empty())
		return;

	core::dimension2d<u32> screensize = driver->getScreenSize();
	u32 y = 5;

	std::sort(enabled_cheats.begin(), enabled_cheats.end(),
			  [](const auto &a, const auto &b) {
				  return a.second.Width > b.second.Width;
			  });

	Minimap *mapper = m_client->getMinimap();
	if (mapper != nullptr && mapper->getModeIndex() != 0)
		y = screensize.Height - 15;

	std::vector<video::SColor> colors;

	for (int i = 0; i < cheat_count; i++) {
		video::SColor color = video::SColor(255, 0, 0, 0);
		f32 h = (f32)i * 2.0f / (f32)cheat_count - m_rainbow_offset;
		if (h < 0)
			h = 6.0f + h;
		f32 x = (1 - fabs(fmod(h, 2.0f) - 1.0f)) * 255.0f;
		switch ((int)h) {
		case 0:
			color = video::SColor(255, 255, x, 0);
			break;
		case 1:
			color = video::SColor(255, x, 255, 0);
			break;
		case 2:
			color = video::SColor(255, 0, 255, x);
			break;
		case 3:
			color = video::SColor(255, 0, x, 255);
			break;
		case 4:
			color = video::SColor(255, x, 0, 255);
			break;
		case 5:
			color = video::SColor(255, 255, 0, x);
			break;
		}
		colors.push_back(color);
	}

	int i = 0;
	video::SColor infoColor(230, 230, 230, 230);
	for (std::pair<std::string, core::dimension2d<u32>> &cheat : enabled_cheats) {
		std::string cheat_full_str = cheat.first;
		core::dimension2d<u32> dim = cheat.second;

		size_t brace_position = cheat_full_str.find('[');
		if (brace_position != std::string::npos) {
			std::string cheat_str = cheat_full_str.substr(0, brace_position);
			std::string info_str = cheat_full_str.substr(brace_position);

			core::dimension2d<u32> cheat_dim = m_font->getDimension(utf8_to_wide(cheat_str).c_str());
			core::dimension2d<u32> info_dim = m_font->getDimension(utf8_to_wide(info_str).c_str());

			u32 x_cheat = screensize.Width - 5 - dim.Width;
			u32 x_info = x_cheat + cheat_dim.Width;

			core::rect<s32> cheat_bounds(x_cheat, y, x_cheat + cheat_dim.Width, y + cheat_dim.Height);
			m_font->draw(cheat_str.c_str(), cheat_bounds, colors[i], false, false);

			core::rect<s32> info_bounds(x_info, y, x_info + info_dim.Width, y + info_dim.Height);
			m_font->draw(info_str.c_str(), info_bounds, infoColor, false, false);

		} else {
			u32 x = screensize.Width - 5 - dim.Width;

			core::rect<s32> cheat_bounds(x, y, x + dim.Width, y + dim.Height);
			m_font->draw(cheat_full_str.c_str(), cheat_bounds, colors[i], false, false);
		}

		if (mapper != nullptr && mapper->getModeIndex() == 0) {
			y += dim.Height;
		} else {
			y -= dim.Height;
		}

		i++;
	}
}

void CheatMenu::selectUp()
{
    int max_index;
    int *selected_item_index;

    if (m_cheat_layer) {
        max_index = cheatMenu.begin()->children[m_selected_category].children.size() - 1;
        selected_item_index = &m_selected_cheat;
    } else {
        max_index = cheatMenu.begin()->children.size() - 1;
        selected_item_index = &m_selected_category;
    }

    --(*selected_item_index);

    if (*selected_item_index < 0) {
        *selected_item_index = max_index;
    }
}

void CheatMenu::selectDown()
{
    int max_index;
    int *selected_item_index;

    if (m_cheat_layer) {
        max_index = cheatMenu.begin()->children[m_selected_category].children.size() - 1;
        selected_item_index = &m_selected_cheat;
    } else {
        max_index = cheatMenu.begin()->children.size() - 1;
        selected_item_index = &m_selected_category;
    }

    ++(*selected_item_index);

    if (*selected_item_index > max_index) {
        *selected_item_index = 0;
    }
}

void CheatMenu::selectRight()
{
	if (m_cheat_layer)
		return;
	m_cheat_layer = true;
	m_selected_cheat = 0;
}

void CheatMenu::selectLeft()
{
	if (!m_cheat_layer)
		return;
	m_cheat_layer = false;
}

void CheatMenu::selectConfirm()
{
    if (m_cheat_layer) {
        Cheats& selected_cheat =cheatMenu.begin()->children[m_selected_category]
                             .children[m_selected_cheat];
        CheatsContext context = {
            client_
        };
        selected_cheat.function(context);
        selected_cheat.toggled = !selected_cheat.toggled;
    } else {
    }
}