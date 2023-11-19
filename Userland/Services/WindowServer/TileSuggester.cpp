#include <LibGfx/StylePainter.h>
#include <WindowServer/Compositor.h>
#include <WindowServer/TileSuggester.h>
#include <WindowServer/WindowManager.h>

namespace WindowServer {
static TileSuggester* s_the;

TileSuggester& TileSuggester::the()
{
    VERIFY(s_the);
    return *s_the;
}

TileSuggester::TileSuggester()
{
    s_the = this;
}

void TileSuggester::set_visible(bool visible, Optional<Window const&> tiled_window)
{
    if (m_visible == visible)
        return;
    m_visible = visible;
    Compositor::the().invalidate_occlusions();
    if (m_switcher_window)
        m_switcher_window->set_visible(visible);
    if (!m_visible)
        return;
    clear_hovered_index();
    refresh(move(tiled_window));
}

void TileSuggester::event(Core::Event& event)
{
    if (event.type() == Event::WindowLeft) {
        clear_hovered_index();
        return;
    }

    if (!static_cast<WindowServer::Event&>(event).is_mouse_event())
        return;

    auto& mouse_event = static_cast<MouseEvent&>(event);
    int new_hovered_index = -1;
    for (size_t i = 0; i < m_windows.size(); ++i) {
        auto item_rect = this->item_rect(i);
        if (item_rect.contains(mouse_event.position())) {
            new_hovered_index = i;
            break;
        }
    }

    if (mouse_event.type() == Event::MouseMove) {
        if (m_hovered_index != new_hovered_index) {
            m_hovered_index = new_hovered_index;
            redraw();
        }
    }

    if (new_hovered_index == -1) {
        if (mouse_event.type() == Event::MouseDown)
            hide();
        return;
    }

    if (mouse_event.type() == Event::MouseDown)
        select_window_at_index(new_hovered_index);

    event.accept();
}

void TileSuggester::on_key_event(KeyEvent const& event)
{
    if (event.type() != Event::Type::KeyDown)
        return;

    if (event.key() == Key_Return) {
        if (m_hovered_index >= 0 && m_hovered_index < static_cast<int>(m_windows.size()))
            select_window_at_index(m_hovered_index);
        return;
    }

    if (event.key() == Key_Escape) {
        hide();
        return;
    }

    if (event.key() != Key_Up && event.key() != Key_Down) {
        return;
    }
    VERIFY(!m_windows.is_empty());

    int new_hovered_index = (event.key() == Key_Up) ? m_hovered_index - 1 : m_hovered_index + 1;
    new_hovered_index %= static_cast<int>(m_windows.size());
    if (new_hovered_index < 0)
        new_hovered_index = static_cast<int>(m_windows.size()) - 1;

    VERIFY(new_hovered_index < static_cast<int>(m_windows.size()));
    m_hovered_index = new_hovered_index;
    redraw();
}

void TileSuggester::select_window_at_index(int index)
{
    VERIFY(m_suggested_tile_type != WindowTileType::None);

    auto* highlight_window = m_windows.at(index).ptr();
    VERIFY(highlight_window);
    VERIFY(highlight_window->tile_type() != m_suggested_tile_type);
    hide();
    highlight_window->set_tiled(m_suggested_tile_type);
}

Gfx::IntRect TileSuggester::item_rect(int index) const
{
    return {
        padding(),
        padding() + index * item_height(),
        m_rect.width() - padding() * 2,
        item_height()
    };
}

void TileSuggester::redraw()
{
    draw();
    Compositor::the().invalidate_screen(m_rect);
}

void TileSuggester::draw()
{
    if (!m_tiled_window)
        return;

    auto palette = WindowManager::the().palette();

    Gfx::IntRect rect = { {}, m_rect.size() };
    Gfx::Painter painter(*m_switcher_window->backing_store());
    painter.clear_rect(rect, Color::Transparent);

    // FIXME: Perhaps the TileSuggester could render as an overlay instead.
    //        That would require adding support for event handling to overlays.
    if (auto* shadow_bitmap = WindowManager::the().overlay_rect_shadow()) {
        // FIXME: Support other scale factors.
        int scale_factor = 1;
        Gfx::StylePainter::paint_simple_rect_shadow(painter, rect, shadow_bitmap->bitmap(scale_factor), true, true);
    }

    for (size_t index = 0; index < m_windows.size(); ++index) {
        // FIXME: Ideally we wouldn't be in draw() without having pruned destroyed windows from the list already.
        if (m_windows.at(index) == nullptr)
            continue;
        auto& window = *m_windows.at(index);
        auto item_rect = this->item_rect(index);
        Color text_color;
        Color rect_text_color;
        if (static_cast<int>(index) == m_hovered_index)
            Gfx::StylePainter::paint_frame(painter, item_rect, palette, Gfx::FrameStyle::RaisedPanel);
        text_color = Color::White;
        rect_text_color = Color(Color::White).with_alpha(0xcc);
        item_rect.shrink(item_padding(), 0);
        Gfx::IntRect thumbnail_rect = { item_rect.location().translated(0, 5), { thumbnail_width(), thumbnail_height() } };
        if (window.backing_store())
            painter.draw_scaled_bitmap(thumbnail_rect, *window.backing_store(), window.backing_store()->rect(), 1.0f, Gfx::Painter::ScalingMode::BilinearBlend);
        Gfx::IntRect icon_rect = { thumbnail_rect.bottom_right().translated(-window.icon().width() - 1, -window.icon().height() - 1), { window.icon().width(), window.icon().height() } };
        painter.blit(icon_rect.location(), window.icon(), window.icon().rect());
        painter.draw_text(item_rect.translated(thumbnail_width() + 12, 0).translated(1), window.computed_title(), WindowManager::the().window_title_font(), Gfx::TextAlignment::CenterLeft, text_color.inverted());
        painter.draw_text(item_rect.translated(thumbnail_width() + 12, 0), window.computed_title(), WindowManager::the().window_title_font(), Gfx::TextAlignment::CenterLeft, text_color);
        painter.draw_text(item_rect, window.rect().to_deprecated_string(), Gfx::TextAlignment::CenterRight, rect_text_color);
    }
}

void TileSuggester::refresh(Optional<Window const&> tiled_window)
{
    if (tiled_window.has_value()) {
        m_tiled_window = tiled_window.value();
    } else {
        VERIFY(m_tiled_window.has_value());
    }

    switch (m_tiled_window->tile_type()) {
        case WindowTileType::Left:
            m_suggested_tile_type = WindowTileType::Right;
            break;
        case WindowTileType::Right:
            m_suggested_tile_type = WindowTileType::Left;
            break;
        case WindowTileType::Top:
            m_suggested_tile_type = WindowTileType::Bottom;
            break;
        case WindowTileType::Bottom:
            m_suggested_tile_type = WindowTileType::Top;
            break;
        case WindowTileType::TopLeft:
            m_suggested_tile_type = WindowTileType::BottomLeft;
            break;
        case WindowTileType::TopRight:
            m_suggested_tile_type = WindowTileType::BottomRight;
            break;
        case WindowTileType::BottomLeft:
            m_suggested_tile_type = WindowTileType::TopLeft;
            break;
        case WindowTileType::BottomRight:
            m_suggested_tile_type = WindowTileType::TopRight;
            break;

        case WindowTileType::None:
        case WindowTileType::VerticallyMaximized:
        case WindowTileType::HorizontallyMaximized:
        case WindowTileType::Maximized:
        default:
            hide();
            return;
    }

    auto& wm = WindowManager::the();
    m_windows.clear();
    int window_count = 0;
    int longest_title_width = 0;
    bool tiled_window_already_exists = false;

    wm.current_window_stack().for_each_window_of_type_from_front_to_back(
        WindowType::Normal,
        [&](Window& window) {
            if (window.is_frameless() || window.is_modal() || &window == m_tiled_window.ptr())
                return IterationDecision::Continue;

            if (window.tile_type() == m_suggested_tile_type) {
                tiled_window_already_exists = true;
                return IterationDecision::Break;
            }

            ++window_count;
            longest_title_width = max(longest_title_width, wm.font().width(window.computed_title()));
            m_windows.append(window);
            return IterationDecision::Continue;
        },
        true);

    if (m_windows.is_empty() || tiled_window_already_exists) {
        hide();
        return;
    }

    int space_for_window_details = 200;
    m_rect.set_width(thumbnail_width() + longest_title_width + space_for_window_details + padding() * 2 + item_padding() * 2);
    m_rect.set_height(window_count * item_height() + padding() * 2);
    m_rect.center_within(wm.tiled_window_rect(*m_tiled_window, {}, m_suggested_tile_type));
    if (!m_switcher_window) {
        m_switcher_window = Window::construct(*this, WindowType::WindowSwitcher);
        m_switcher_window->set_has_alpha_channel(true);
    }
    m_switcher_window->set_rect(m_rect);

    redraw();
}

void TileSuggester::refresh_if_needed()
{
    if (m_visible)
        refresh();
}

void TileSuggester::clear_hovered_index()
{
    if (m_hovered_index == -1)
        return;
    m_hovered_index = -1;
    redraw();
}

}
