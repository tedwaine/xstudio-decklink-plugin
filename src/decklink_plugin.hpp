// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "xstudio/plugin_manager/plugin_base.hpp"

namespace xstudio {
    namespace bm_decklink_plugin_1_0 {

    class DecklinkOutput;

    class BMDecklinkPlugin : public plugin::StandardPlugin {

        public:

        BMDecklinkPlugin(caf::actor_config &cfg, const utility::JsonStore &init_settings);
        virtual ~BMDecklinkPlugin();

        inline static const utility::Uuid PLUGIN_UUID =
            utility::Uuid("432c2c46-e77e-43b0-b0c0-117669d488bf");

        void on_exit() override;

        private:

      protected:

        void attribute_changed(const utility::Uuid &attribute_uuid, const int /*role*/) override;

        caf::message_handler message_handler_extensions() override {
            return message_handler_extensions_;
        }

        void initialise();

        caf::actor offscreen_viewport_;
        caf::actor main_viewport_;
        caf::message_handler message_handler_extensions_;
        DecklinkOutput * dcl_output_ = nullptr;

        module::StringChoiceAttribute *pixel_formats_ {nullptr};
        module::StringChoiceAttribute *resolutions_ {nullptr};
        module::StringChoiceAttribute *frame_rates_ {nullptr};
        module::StringAttribute *status_message_ {nullptr};
        module::BooleanAttribute *is_in_error_ {nullptr};
        module::BooleanAttribute *sdi_output_is_running_ {nullptr};
        module::BooleanAttribute *start_stop_ {nullptr};
        module::BooleanAttribute *track_main_viewport_ {nullptr};
        module::BooleanAttribute *auto_start_ {nullptr};
        module::BooleanAttribute *render_16bitrgba_ {nullptr};
        module::IntegerAttribute *samples_water_level_ {nullptr};
        module::IntegerAttribute *audio_sync_delay_milliseconds_ {nullptr};


    };
} // namespace bm_decklink_plugin_1_0
} // namespace xstudio
