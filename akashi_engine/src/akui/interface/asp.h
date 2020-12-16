#pragma once

#include <libakserver/akserver.h>

#include <vector>

class QWidget;

namespace akashi {
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

            bool seek(const int num, const int den) override;

            bool relative_seek(const int num, const int den) override;

            bool frame_step(void) override;

            bool frame_back_step(void) override;

          private:
            QWidget* m_root;
            PlayerWidget* m_player;
        };

        class ASPGeneralAPIImpl : public akashi::server::ASPGeneralAPI {
          public:
            explicit ASPGeneralAPIImpl(QWidget* root) : m_root(root){};
            virtual ~ASPGeneralAPIImpl() = default;

            bool terminate(void) override;

          private:
            QWidget* m_root;
        };

    }
}
