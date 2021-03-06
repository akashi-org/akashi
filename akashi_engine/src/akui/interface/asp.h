#pragma once

#include <libakserver/akserver.h>

#include <vector>

class QWidget;

namespace akashi {
    namespace state {
        enum class PlayState;
    }
    namespace ui {

        class PlayerWidget;
        class ASPGUIAPIImpl : public akashi::server::ASPGUIAPI {
          public:
            explicit ASPGUIAPIImpl(QWidget* root) : m_root(root){};
            virtual ~ASPGUIAPIImpl() = default;

            std::vector<std::string> get_widgets(void) override;
            bool click(const std::string& widget_name) override;

          private:
            QWidget* m_root;
        };

        class ASPMediaAPIImpl : public akashi::server::ASPMediaAPI {
          public:
            explicit ASPMediaAPIImpl(QWidget* root);
            virtual ~ASPMediaAPIImpl() = default;

            std::string take_snapshot(void) override;

            bool toggle_fullscreen(void) override;

            bool seek(const int num, const int den) override;

            bool relative_seek(const double ratio) override;

            bool frame_step(void) override;

            bool frame_back_step(void) override;

            std::vector<int64_t> current_time(void) override;

            bool change_playstate(const state::PlayState& play_state) override;

            bool change_playvolume(const double volume) override;

          private:
            QWidget* m_root;
            PlayerWidget* m_player;
        };

        class ASPGeneralAPIImpl : public akashi::server::ASPGeneralAPI {
          public:
            explicit ASPGeneralAPIImpl(QWidget* root);
            virtual ~ASPGeneralAPIImpl() = default;

            bool eval(const std::string& file_path, const std::string& elem_name) override;
            bool terminate(void) override;

          private:
            QWidget* m_root;
            PlayerWidget* m_player;
        };

    }
}
