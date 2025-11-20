#include "script/scripting_client.h"
#include "client/client.h"
#include "client/minimap.h"
#include "client/fontengine.h"
#include "gui/cheatMenu.h"
#include <cstddef>
#include "client/fontengine.h"
#include <iostream>
#include <settings.h>
#include <IGUIFont.h>

CheatMenu::CheatMenu(Client* client)
    : m_client(client)
{
    // initialize selection safely
    m_font = g_fontengine->getFont();
    m_selected_category = 0;
    m_selected_cheat = 0;
    m_cheat_layer = false;
}

bool CheatMenu::IsCheatEnabled(const std::string& name) {
    for (const auto& category : cheatMenu) {
        for (const auto& cheat : category.children) {

            if (!cheat.is_toggle_cheat)
                continue;

            if (cheat.name == name && cheat.toggled)
                return true;
        }
    }
    return false;
}

// drawing helpers
void CheatMenu::draw2DRectangleOutline(video::IVideoDriver* driver, const core::recti& pos, video::SColor color) {
    if (!driver) return;
    driver->draw2DLine(pos.UpperLeftCorner, core::position2di(pos.LowerRightCorner.X, pos.UpperLeftCorner.Y), color);
    driver->draw2DLine(core::position2di(pos.LowerRightCorner.X, pos.UpperLeftCorner.Y), pos.LowerRightCorner, color);
    driver->draw2DLine(pos.LowerRightCorner, core::position2di(pos.UpperLeftCorner.X, pos.LowerRightCorner.Y), color);
    driver->draw2DLine(core::position2di(pos.UpperLeftCorner.X, pos.LowerRightCorner.Y), pos.UpperLeftCorner, color);
}

void CheatMenu::drawRect(video::IVideoDriver* driver, const std::string& name,
        int x, int y, int width, int height, bool active, bool selected)
{
    if (!driver) return;
    video::SColor bgcolor = m_bg_color;
    video::SColor fontcolor = m_font_color;

    if (active) bgcolor = m_active_bg_color;
    if (selected) fontcolor = m_selected_font_color;

    driver->draw2DRectangle(bgcolor, core::rect<s32>(x, y, x + width, y + height));

    if (selected)
        draw2DRectangleOutline(driver, core::rect<s32>(x - 1, y - 1, x + width, y + height), fontcolor);

    if (!m_font) return; // safe guard

    int fx = x + 5;
    int fy = y + (height - static_cast<int>(m_fontsize.Y)) / 2;
    core::rect<s32> fontbounds(fx, fy, fx + static_cast<int>(m_fontsize.X) * static_cast<int>(name.size()), fy + static_cast<int>(m_fontsize.Y));
    m_font->draw(name.c_str(), fontbounds, fontcolor, false, false);
}

void CheatMenu::drawEntry(video::IVideoDriver* driver, const std::string& name, int number,
        bool selected, bool active, CheatMenuEntryType entry_type)
{
    if (!driver) return;

    int x = m_gap;
    int y = m_gap;
    int width = m_entry_width;
    int height = m_entry_height;
    video::SColor bgcolor = m_bg_color;
    video::SColor fontcolor = m_font_color;

    if (entry_type == CHEAT_MENU_ENTRY_TYPE_HEAD) {
        bgcolor = m_active_bg_color;
        height = m_head_height;
    } else {
        bool is_category = entry_type == CHEAT_MENU_ENTRY_TYPE_CATEGORY;
        y += m_gap + m_head_height + (number + (is_category ? 0 : m_selected_category)) * (m_entry_height + m_gap);
        x += (is_category ? 0 : m_gap + m_entry_width);
        if (active) bgcolor = m_active_bg_color;
        if (selected) fontcolor = m_selected_font_color;
    }

    driver->draw2DRectangle(bgcolor, core::rect<s32>(x, y, x + width, y + height));
    if (selected)
        draw2DRectangleOutline(driver, core::rect<s32>(x - 1, y - 1, x + width, y + height), fontcolor);

    if (!m_font) return; // safe guard

    int fx = x + 5;
    int fy = y + (height - static_cast<int>(m_fontsize.Y)) / 2;
    core::rect<s32> fontbounds(fx, fy, fx + static_cast<int>(m_fontsize.X) * static_cast<int>(name.size()), fy + static_cast<int>(m_fontsize.Y));
    m_font->draw(name.c_str(), fontbounds, fontcolor, false, false);
}

// Main draw
void CheatMenu::draw(video::IVideoDriver* driver, bool /*show_debug*/)
{
    if (!driver) return;

    int category_count = static_cast<int>(cheatMenu.size());
    for (int ci = 0; ci < category_count; ++ci) {
        const Cheats& category = cheatMenu[ci];
        bool is_selected = (ci == m_selected_category) && (!m_cheat_layer);
        drawEntry(driver, category.name, ci, is_selected, false, CHEAT_MENU_ENTRY_TYPE_CATEGORY);

        if (ci == m_selected_category && m_cheat_layer) {
            int cheat_n = static_cast<int>(category.children.size());
            int height = driver->getScreenSize().Height;
            int target = height / (m_entry_height + m_gap) + 1;
            int target_normal = (height - (m_selected_category * (m_entry_height + m_gap))) / (m_entry_height + m_gap);

            if (cheat_n == 0) {
                // nothing to draw
            } else if (cheat_n < target_normal) {
                for (int cheat_count = 0; cheat_count < cheat_n; ++cheat_count) {
                    const Cheats& cheat = category.children[cheat_count];
                    drawEntry(driver, cheat.name, cheat_count, cheat_count == m_selected_cheat, cheat.toggled, CHEAT_MENU_ENTRY_TYPE_ENTRY);
                }
            } else {
                int base = m_selected_cheat - m_selected_category - 1;
                int drawn = 0;
                for (int i = base; i < base + target; ++i, ++drawn) {
                    if (cheat_n <= 0) break;
                    int idx = negmod(i, cheat_n);
                    const Cheats& cheat = category.children[idx];
                    int y = (drawn * (m_entry_height + m_gap)) + m_gap;
                    drawRect(driver, cheat.name, m_gap * 2 + m_entry_width, y, m_entry_width, m_entry_height, cheat.toggled, idx == m_selected_cheat);
                }
            }
        }
    }
}

void CheatMenu::drawHUD(video::IVideoDriver* driver, double dtime)
{
    if (!driver) return;
    if (!m_font) return;

    m_rainbow_offset += dtime;
    m_rainbow_offset = fmod(m_rainbow_offset, 6.0);

    std::vector<std::pair<std::string, core::dimension2d<u32>>> enabled_cheats;
    for (const auto& category : cheatMenu) {
        for (const auto& cheat : category.children) {
            if (cheat.toggled) {
                std::string cheat_str = cheat.name;
                core::dimension2d<u32> dim = m_font->getDimension(utf8_to_wide(cheat_str).c_str());
                enabled_cheats.emplace_back(cheat_str, dim);
            }
        }
    }

    if (enabled_cheats.empty()) return;

    core::dimension2d<u32> screensize = driver->getScreenSize();
    u32 y = 5;

    std::sort(enabled_cheats.begin(), enabled_cheats.end(),
              [](const auto &a, const auto &b){ return a.second.Width > b.second.Width; });

    Minimap* mapper = m_client ? m_client->getMinimap() : nullptr;
    if (mapper != nullptr && mapper->getModeIndex() != 0)
        y = screensize.Height - 15;

    std::vector<video::SColor> colors;
    int cheat_count = static_cast<int>(enabled_cheats.size());
    for (int i = 0; i < cheat_count; ++i) {
        video::SColor color(255, 255, 255, 255);
        float h = (float)i * 2.0f / (float)cheat_count - static_cast<float>(m_rainbow_offset);
        if (h < 0) h = 6.0f + h;
        float x = (1 - fabs(fmod(h, 2.0f) - 1.0f)) * 255.0f;
        switch ((int)h) {
            case 0: color = video::SColor(255, 255, (u32)x, 0); break;
            case 1: color = video::SColor(255, (u32)x, 255, 0); break;
            case 2: color = video::SColor(255, 0, 255, (u32)x); break;
            case 3: color = video::SColor(255, 0, (u32)x, 255); break;
            case 4: color = video::SColor(255, (u32)x, 0, 255); break;
            case 5: color = video::SColor(255, 255, 0, (u32)x); break;
            default: color = video::SColor(255, 255, 255, 255); break;
        }
        colors.push_back(color);
    }

    int i = 0;
    video::SColor infoColor(230, 230, 230, 230);
    for (auto &p : enabled_cheats) {
        const std::string& cheat_full_str = p.first;
        auto dim = p.second;

        size_t brace_position = cheat_full_str.find('[');
        if (brace_position != std::string::npos) {
            std::string cheat_str = cheat_full_str.substr(0, brace_position);
            std::string info_str = cheat_full_str.substr(brace_position);
            core::dimension2d<u32> cheat_dim = m_font->getDimension(utf8_to_wide(cheat_str).c_str());
            core::dimension2d<u32> info_dim = m_font->getDimension(utf8_to_wide(info_str).c_str());

            s32 x_cheat = static_cast<s32>(screensize.Width) - 5 - static_cast<s32>(dim.Width);
            s32 x_info = x_cheat + static_cast<s32>(cheat_dim.Width);

            core::rect<s32> cheat_bounds(x_cheat, y, x_cheat + static_cast<s32>(cheat_dim.Width), y + static_cast<s32>(cheat_dim.Height));
            m_font->draw(cheat_str.c_str(), cheat_bounds, colors[i], false, false);

            core::rect<s32> info_bounds(x_info, y, x_info + static_cast<s32>(info_dim.Width), y + static_cast<s32>(info_dim.Height));
            m_font->draw(info_str.c_str(), info_bounds, infoColor, false, false);
        } else {
            s32 x = static_cast<s32>(screensize.Width) - 5 - static_cast<s32>(dim.Width);
            core::rect<s32> cheat_bounds(x, y, x + static_cast<s32>(dim.Width), y + static_cast<s32>(dim.Height));
            m_font->draw(cheat_full_str.c_str(), cheat_bounds, colors[i], false, false);
        }

        if (mapper != nullptr && mapper->getModeIndex() == 0) {
            y += p.second.Height;
        } else {
            y -= p.second.Height;
        }
        ++i;
    }
}

// Navigation
void CheatMenu::selectUp()
{
    if (!has_categories()) return;

    if (m_cheat_layer) {
        if (!has_category_index(m_selected_category)) {
            m_selected_category = 0;
            m_selected_cheat = 0;
            return;
        }
        int n = static_cast<int>(cheatMenu[m_selected_category].children.size());
        if (n <= 0) { m_selected_cheat = 0; return; }
        --m_selected_cheat;
        if (m_selected_cheat < 0) m_selected_cheat = n - 1;
    } else {
        int n = static_cast<int>(cheatMenu.size());
        if (n <= 0) { m_selected_category = 0; return; }
        --m_selected_category;
        if (m_selected_category < 0) m_selected_category = n - 1;
        // ensure selected_cheat is valid for new category
        if (m_selected_cheat >= static_cast<int>(cheatMenu[m_selected_category].children.size()))
            m_selected_cheat = static_cast<int>(cheatMenu[m_selected_category].children.size()) - 1;
        if (m_selected_cheat < 0) m_selected_cheat = 0;
    }
}

void CheatMenu::selectDown()
{
    if (!has_categories()) return;

    if (m_cheat_layer) {
        if (!has_category_index(m_selected_category)) { m_selected_category = 0; m_selected_cheat = 0; return; }
        int n = static_cast<int>(cheatMenu[m_selected_category].children.size());
        if (n <= 0) { m_selected_cheat = 0; return; }
        ++m_selected_cheat;
        if (m_selected_cheat >= n) m_selected_cheat = 0;
    } else {
        int n = static_cast<int>(cheatMenu.size());
        if (n <= 0) { m_selected_category = 0; return; }
        ++m_selected_category;
        if (m_selected_category >= n) m_selected_category = 0;
        if (m_selected_cheat >= static_cast<int>(cheatMenu[m_selected_category].children.size()))
            m_selected_cheat = static_cast<int>(cheatMenu[m_selected_category].children.size()) - 1;
        if (m_selected_cheat < 0) m_selected_cheat = 0;
    }
}

void CheatMenu::selectRight()
{
    
    if (!m_cheat_layer) {
        m_cheat_layer = true;
        m_selected_cheat = 0;
        return;
    }
}

void CheatMenu::selectLeft()
{
    if (m_cheat_layer) {
        m_cheat_layer = false;
        return;
    }

    m_selected_category--;
    if (m_selected_category < 0)
        m_selected_category = cheatMenu.size() - 1;
}

void CheatMenu::selectConfirm()
{
    if (!m_cheat_layer) {
        // Optionally: toggle opening/closing or execute category-level action
        return;
    }

    if (!has_category_index(m_selected_category)) return;
    auto &category = cheatMenu[m_selected_category];
    if (category.children.empty()) return;
    if (!has_cheat_index(m_selected_category, m_selected_cheat)) return;

    Cheats &selected_cheat = category.children[m_selected_cheat];
    CheatsContext context{ m_client };
    if (selected_cheat.function) {
        try {
            selected_cheat.function(context);
        } catch (...) {
            std::cerr << "Cheat function threw exception" << std::endl;
        }
    }

    if (selected_cheat.is_toggle_cheat) {
        selected_cheat.toggled = !selected_cheat.toggled;
    }
}