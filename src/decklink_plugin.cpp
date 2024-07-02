// SPDX-License-Identifier: Apache-2.0
#include <GL/glew.h>
#include <GL/gl.h>

#include <filesystem>

#include "decklink_audio_device.hpp"
#include "decklink_plugin.hpp"
#include "decklink_output.hpp"

#include "xstudio/utility/helpers.hpp"
#include "xstudio/utility/logging.hpp"

#include <ImfRgbaFile.h>
#include <vector>
#include <dlfcn.h>

#define kDeckLinkAPI_Name "libDeckLinkAPI.so"

using namespace xstudio;
using namespace xstudio::ui;
using namespace xstudio::bm_decklink_plugin_1_0;

namespace {
    static std::map<std::string, BMDPixelFormat> bmd_pixel_formats(
        {
            {"8 bit YUV", bmdFormat8BitYUV},
            {"10 bit YUV", bmdFormat10BitYUV},
            {"8 bit ARGB", bmdFormat8BitARGB},
            {"8 bit BGRA", bmdFormat8BitBGRA},
            {"10 bit RGB", bmdFormat10BitRGB},
            {"12 bit RGB", bmdFormat12BitRGB},
            {"12 bit RGB-LE", bmdFormat12BitRGBLE},
            {"10 bit RGB Video Range", bmdFormat10BitRGBX},
            {"10 bit RGB-LE Video Range", bmdFormat10BitRGBXLE}
        });

static const std::string version1_ui_qml(R"(
import QtQuick 2.12
import BlackmagicSDI 1.0
DecklinkSettingsButton {
}
)");
}

BMDecklinkPlugin::BMDecklinkPlugin(
    caf::actor_config &cfg, const utility::JsonStore &init_settings)
    : plugin::StandardPlugin(cfg, "BMDecklinkPlugin", init_settings) 
{

    // here we try to open the decklink driver libs. If they are not installed
    // on the system abort construction of the plugin (caught by plugin manager)
	if (!dlopen(kDeckLinkAPI_Name, RTLD_NOW|RTLD_GLOBAL))
	{
		throw(std::runtime_error("Blackmagic Decklink SDI output disabled: drivers not found."));
	}

    // add attributes used for configuring the SDI output
    pixel_formats_ = add_string_choice_attribute("Pixel Format", "Pix Fmt", "10 bit YUV", utility::map_key_to_vec(bmd_pixel_formats));
    pixel_formats_->expose_in_ui_attrs_group("Decklink Settings");
    pixel_formats_->set_preference_path("/plugin/decklink/pixel_format");

    sdi_output_is_running_ = add_boolean_attribute("Enabled", "Enabled", false);
    sdi_output_is_running_->expose_in_ui_attrs_group("Decklink Settings");

    status_message_ = add_string_attribute("Status", "Status", "Not Started");
    status_message_->expose_in_ui_attrs_group("Decklink Settings");

    is_in_error_ = add_boolean_attribute("In Error", "In Error", false);
    is_in_error_->expose_in_ui_attrs_group("Decklink Settings");

    track_main_viewport_ = add_boolean_attribute("Track Viewport", "Track Viewport", false);
    track_main_viewport_->expose_in_ui_attrs_group("Decklink Settings");

    resolutions_ = add_string_choice_attribute(
            "Output Resolution",
            "Out. Res.");

    resolutions_->expose_in_ui_attrs_group("Decklink Settings");
    resolutions_->set_preference_path("/plugin/decklink/output_resolution");

    frame_rates_ = add_string_choice_attribute(
        "Refresh Rate",
        "Hz");
    frame_rates_->expose_in_ui_attrs_group("Decklink Settings");
    frame_rates_->set_preference_path("/plugin/decklink/output_frame_rate");

    // This button is only needed for V1 user interface
    module::QmlCodeAttribute *button = add_qml_code_attribute(
        "MyCode",
        version1_ui_qml);

    button->expose_in_ui_attrs_group("media_tools_buttons_0");
    button->set_role_data(module::Attribute::ToolbarPosition, 500.0);

    start_stop_ =
        add_boolean_attribute("Start Stop", "Start Stop", false);
    start_stop_->expose_in_ui_attrs_group("Decklink Settings");

    auto_start_ = add_boolean_attribute("Auto Start", "Auto Start", false);
    auto_start_->set_preference_path("/plugin/decklink/auto_start_sdi");

    render_16bitrgba_ = add_boolean_attribute("Use RGBA16", "Use RGBA16", false);
    render_16bitrgba_->set_preference_path("/plugin/decklink/use_rgba16_render_bitdepth");

    samples_water_level_ = add_integer_attribute("Audio Samples Water Level", "Audio Samples Water Level", 4096);
    samples_water_level_->set_preference_path("/plugin/decklink/audio_samps_water_level");

    audio_sync_delay_milliseconds_= add_integer_attribute("Audio Sync Delay (milliseconds)", "Audio Sync Delay (milliseconds)", 0);
    audio_sync_delay_milliseconds_->set_preference_path("/plugin/decklink/audio_sync_delay");
    audio_sync_delay_milliseconds_->expose_in_ui_attrs_group("Decklink Settings");

     // provide an extension to the base class message handler to handle timed
    // callbacks to fade the laser pen strokes
    message_handler_extensions_ = {
        [=](offscreen_viewport_atom, caf::actor offscreen_vp) {

        },
        [=](media_reader::ImageBufPtr incoming) {
            dcl_output_->incoming_frame(incoming);
        },
        [=](utility::event_atom, const bool running) {

            if (sdi_output_is_running_->value() != running) {
                sdi_output_is_running_->set_value(running);
                if (!running) {
                    send(
                        offscreen_viewport_,
                        video_output_actor_atom_v,
                        caf::actor()
                        );                
                }
            }

        },
        [=](utility::event_atom, const bool running, int frame_width, int frame_height) {

            sdi_output_is_running_->set_value(running);

            // this message tells the offscreen viewport that we are a 'video
            // output actor'. The viewport will send us images in the desired
            // resolution as it redraws itself
            if (running) {
                // tell the offscreen viewport to start rendering and sending
                // us framebuffer captures
                send(
                    offscreen_viewport_,
                    video_output_actor_atom_v,
                    caf::actor_cast<caf::actor>(this),
                    frame_width, 
                    frame_height,
                    render_16bitrgba_->value() ? viewport::RGBA_16 : viewport::RGBA_10_10_10_2 // *see note below
                    );

                // NOTE: We can use viewport::RGBA_10_10_10_2 as the desired framebuffer
                // format for xstudio rendering. This matches the decklink buffer if RGB 10 Bit
                // is selected for the SDI output, and is very efficient. The GPU shader 
                // scales and packs RGB bits into the RGBA8 bit target buffer pixels as 10_10_10RGB
                // so we only copy 32 bits per pixel from GPU to SDI. However, any overlays
                // and blending that happens at render time does not come out correct as true
                // rgba fragment values aren't calculated by the shader.
                // Instead we can use 16bit output from the GPU. In testing I'm finding it takes 
                // 20ms on my R6000 card to render a 4k frame and grab it off the card, so 
                // we can't hit 4k@60Hz in this mde. With RGBA_10_10_10_2 4k@60Hz is possible
                // on our powerful playback workstations.

            } else {
                // tell the offscreen viewport to strop rendering
                send(
                    offscreen_viewport_,
                    video_output_actor_atom_v,
                    caf::actor()
                    );
            }

        },
        [=](const std::string &status_msg, bool is_error) {
            status_message_->set_value(status_msg);
            // assuming we get a non-error status that we're not in an
            // error state
            is_in_error_->set_value(is_error);
        },
        [=](caf::error & err) {
            status_message_->set_value(to_string(err));
            is_in_error_->set_value(true);
        }};

    // Get the 'StudioUI' which lives in the Qt context and therefore is able to
    // create offscreen viewports for us
    auto studio_ui = system().registry().template get<caf::actor>(studio_ui_registry);

    // tell the studio actor to create an offscreen viewport for us
    request(studio_ui, infinite, offscreen_viewport_atom_v, "decklink_viewport").then(
        [=](caf::actor offscreen_vp) {

            // this is the offscreen renderer that we asked for below.
            offscreen_viewport_ = offscreen_vp;
            
            // now we fetch the main viewport (named 'viewport0') from the global_playhead_events_actor that
            // keeps track of viewports and their playheads.
            auto playhead_events_actor = system().registry().template get<caf::actor>(global_playhead_events_actor);
            request(playhead_events_actor, infinite, ui::viewport::viewport_atom_v, "viewport0").then(
                [=](caf::actor vp) {

                    main_viewport_ = vp;

                    // connect our colour pipeline to the main viewport colour pipeline. This 
                    // connects the OCIO View and Exposure controls
                    request(main_viewport_, infinite, colour_pipeline::colour_pipeline_atom_v).then(
                        [=](caf::actor main_viewport_colour_pipe) {                             
                            request(offscreen_viewport_, infinite, colour_pipeline::colour_pipeline_atom_v).then(
                                [=](caf::actor my_colour_pipe) {

                                // The message is handled by the Module base class
                                send(
                                    main_viewport_colour_pipe,
                                    module::link_module_atom_v,
                                    my_colour_pipe,
                                    false,
                                    false,
                                    true);

                                },
                                [=](caf::error &err) mutable {
                                    spdlog::critical("{} {}", __PRETTY_FUNCTION__, to_string(err));
                                });
                        },
                        [=](caf::error &err) mutable {
                            spdlog::critical("{} {}", __PRETTY_FUNCTION__, to_string(err));
                        });
                },
                [=](caf::error &err) mutable {
                    spdlog::critical("{} {}", __PRETTY_FUNCTION__, to_string(err));
                });

            // now we have an offscreen viewport to send us frame buffers
            // we can initialise the card and start output
            initialise();

        },
        [=](caf::error & err) mutable {
            status_message_->set_value(to_string(err));
            is_in_error_->set_value(true);
        });

}

void BMDecklinkPlugin::on_exit() {
    // the dcl_output_ has caf::actor handles pointing to this (BMDecklinkPlugin)
    // instance. The BMDecklinkPlugin will therefore never get deleted due to
    // circular dependency so we use the on_exit
    delete dcl_output_;
    plugin::StandardPlugin::on_exit();
}

void BMDecklinkPlugin::attribute_changed(const utility::Uuid &attribute_uuid, const int role) 
{
    
    if (dcl_output_) {

        if (resolutions_ && attribute_uuid == resolutions_->uuid() && role == module::Attribute::Value) {

            const auto rates = dcl_output_->get_available_refresh_rates(resolutions_->value());
            frame_rates_->set_role_data(module::Attribute::StringChoices, rates);

            // pick a sensible refresh rate, if the current rate isn't available for new resolution
            auto i = std::find(rates.begin(), rates.end(), frame_rates_->value()); // prefer current rate
            if (i == rates.end()) {
                i = std::find(rates.begin(), rates.end(), "24.0"); // otherwise prefer 24.0
                if (i == rates.end()) {
                    i = std::find(rates.begin(), rates.end(), "60.0"); // use 60.0 otherwise
                    if (i == rates.end()) {
                        i = rates.begin();
                    }
                }
            }
            if (i != rates.end()) {
                frame_rates_->set_value(*i);
            }

        } else if (attribute_uuid == start_stop_->uuid()) {

            dcl_output_->StartStop();

        } 
        
        if (attribute_uuid == pixel_formats_->uuid() || attribute_uuid == resolutions_->uuid() || attribute_uuid == frame_rates_->uuid()) {

            try {

                if (bmd_pixel_formats.find(pixel_formats_->value()) == bmd_pixel_formats.end()) {
                    throw std::runtime_error(fmt::format("Invalid pixel format: {}", pixel_formats_->value()));
                }

                const BMDPixelFormat pix_fmt = bmd_pixel_formats[pixel_formats_->value()];
            

                dcl_output_->set_display_mode(
                    resolutions_->value(),
                    frame_rates_->value(),
                    pix_fmt);

            } catch (std::exception & e) {

                status_message_->set_value(e.what());
                is_in_error_->set_value(true);

            }

        } else if (attribute_uuid == track_main_viewport_->uuid()) {

            if (track_main_viewport_->value()) {
                send(main_viewport_, ui::viewport::other_viewport_atom_v, offscreen_viewport_, caf::actor());
            } else {
                send(main_viewport_, module::link_module_atom_v, offscreen_viewport_, false);
            }
                
        } else if (attribute_uuid == samples_water_level_->uuid()) {
            dcl_output_->set_audio_samples_water_level(samples_water_level_->value());
        } else if (attribute_uuid == audio_sync_delay_milliseconds_->uuid()) {
            dcl_output_->set_audio_sync_delay_milliseconds(audio_sync_delay_milliseconds_->value());
        }

    }
    StandardPlugin::attribute_changed(attribute_uuid, role);

}

void BMDecklinkPlugin::initialise() {

    try {

        dcl_output_ = new DecklinkOutput(offscreen_viewport_, caf::actor_cast<caf::actor>(this));

        resolutions_->set_role_data(module::Attribute::StringChoices, dcl_output_->output_resolution_names());

        dcl_output_->set_audio_samples_water_level(samples_water_level_->value());
        dcl_output_->set_audio_sync_delay_milliseconds(audio_sync_delay_milliseconds_->value());

        // spawn our audio output 'actor'. This 'actor' class will be passed audio samples
        // from the xstudio playhead and then hand them on to BMDecklinkPlugin class to pass
        // into the BMD API.
        DecklinkAudioOutputDevice::set_output(dcl_output_);
        auto a = spawn<audio::AudioOutputActor<DecklinkAudioOutputDevice>>();
        link_to(a);

        spdlog::info("Decklink Card Initialised");

        // this call is essential to set-up the base class
        make_behavior();

        connect_to_ui();

        // now we are set-up we can kick ourselves to fill in the refresh rate list etc.
        attribute_changed(resolutions_->uuid(), module::Attribute::Value);

        if (auto_start_->value()) {
            // start output immediately if auto_start_ is enabled (via prefs)
            dcl_output_->StartStop();
        }

    } catch (std::exception & e) {
        spdlog::critical("{} {}", __PRETTY_FUNCTION__, e.what());
    }

}

BMDecklinkPlugin::~BMDecklinkPlugin() {
}

extern "C" {
plugin_manager::PluginFactoryCollection *plugin_factory_collection_ptr() {
    return new plugin_manager::PluginFactoryCollection(
        std::vector<std::shared_ptr<plugin_manager::PluginFactory>>(
            {std::make_shared<plugin_manager::PluginFactoryTemplate<BMDecklinkPlugin>>(
                BMDecklinkPlugin::PLUGIN_UUID,
                "BlackMagic Decklink Viewport",
                plugin_manager::PluginFlags::PF_VIDEO_OUTPUT,
                false, // not 'resident' .. the StudioUI class instances the viewport plugins
                "Ted Waine",
                "BlackMagic Decklink SDI Output Plugin")}));
}
}