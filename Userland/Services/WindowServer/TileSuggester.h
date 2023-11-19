#pragma once

#include <LibCore/EventReceiver.h>
#include <WindowServer/Window.h>


namespace WindowServer {

class TileSuggester final : public Core::EventReceiver {
    C_OBJECT(TileSuggester)
public:
    static TileSuggester& the();

    virtual ~TileSuggester() override = default;

    bool is_visible() const { return m_visible; }
    void show(Window const& tiled_window)
    {
        set_visible(true, tiled_window);
    }
    void hide() { set_visible(false); }

    void on_key_event(KeyEvent const&);

    void refresh(Optional<Window const&> tiled_window = {});

    void refresh_if_needed();

private:
    TileSuggester();

    int thumbnail_width() const { return 64; }
    int thumbnail_height() const { return 64; }
    int item_height() const { return 14 + thumbnail_height(); }
    int padding() const { return 30; }
    int item_padding() const { return 10; }

    void set_visible(bool, Optional<Window const&> tiled_window = {});
    void draw();
    void redraw();
    void select_window_at_index(int index);
    Gfx::IntRect item_rect(int index) const;
    void clear_hovered_index();

    virtual void event(Core::Event&) override;

    RefPtr<Window> m_switcher_window;
    Gfx::IntRect m_rect;
    bool m_visible { false };
    WeakPtr<Window> m_tiled_window;
    Vector<WeakPtr<Window>> m_windows;
    int m_hovered_index { 0 };
    WindowTileType m_suggested_tile_type { WindowTileType::None };
};

}
