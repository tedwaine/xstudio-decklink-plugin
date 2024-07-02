#pragma once

#include <GL/gl.h>
#include <mutex>
#include <atomic>
#include <deque>
#include <vector>

#include "extern/DeckLinkAPI.h"
#include "xstudio/media_reader/image_buffer.hpp"
#include "pixel_swizzler.hpp"

namespace xstudio {
    namespace bm_decklink_plugin_1_0 {

class AVOutputCallback;

// For now we recieve a 10 bit YUV image from xstudio. We use BMD frame conversion
// utility to convert to the selected output pixel format. This does mean
// that 12 bit outputs don't actually have 12 bits.
class RGB10BitVideoFrame : public IDeckLinkVideoFrame
{
private:

	long					_width;
	long					_height;
	BMDFrameFlags			_flags;
	std::vector<uint8_t>	_pixelBuffer;

	std::atomic<ULONG>	_refCount;

public:
	RGB10BitVideoFrame(long width, long height, BMDFrameFlags flags);
	virtual ~RGB10BitVideoFrame() {};

	// IDeckLinkVideoFrame interface
	virtual long			STDMETHODCALLTYPE	GetWidth(void)			{ return _width; };
	virtual long			STDMETHODCALLTYPE	GetHeight(void)			{ return _height; };
	virtual long			STDMETHODCALLTYPE	GetRowBytes(void)		{ return _width * 4; };
	virtual HRESULT			STDMETHODCALLTYPE	GetBytes(void** buffer);
	virtual BMDFrameFlags	STDMETHODCALLTYPE	GetFlags(void)			{ return _flags; };
	virtual BMDPixelFormat	STDMETHODCALLTYPE	GetPixelFormat(void)	{ return bmdFormat10BitRGBX; };
	
	// Dummy implementations of remaining methods in IDeckLinkVideoFrame
	virtual HRESULT			STDMETHODCALLTYPE	GetAncillaryData(IDeckLinkVideoFrameAncillary** ancillary) { return E_NOTIMPL; };
	virtual HRESULT			STDMETHODCALLTYPE	GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode** timecode) { return E_NOTIMPL;	};

	// IUnknown interface
	virtual HRESULT			STDMETHODCALLTYPE	QueryInterface(REFIID iid, LPVOID *ppv);
	virtual ULONG			STDMETHODCALLTYPE	AddRef();
	virtual ULONG			STDMETHODCALLTYPE	Release();
};


class DecklinkOutput
{
private:

	AVOutputCallback*		            output_callback_;
	std::mutex  				mutex_;

	GLenum				glStatus;
	GLuint				idFrameBuf, idColorBuf, idDepthBuf;
	char*				pFrameBuf;

	// DeckLink
	uint32_t					frame_width_;
	uint32_t					frame_height_;
	
	IDeckLink*					decklink_interface_;
	IDeckLinkOutput*			decklink_output_interface_;
	IDeckLinkVideoConversion *  frame_converter_;
	
	BMDTimeValue				frame_duration_;
	BMDTimeScale				frame_timescale_;
	uint32_t					uiFPS;
	uint32_t					uiTotalFrames;

	media_reader::ImageBufPtr	raster_frame_;
	std::mutex  				frames_mutex_;
	bool						running_ = {false};

	void query_display_modes();
		void report_status(const std::string & status_message);
		void report_error(const std::string & status_message);


	std::map<std::string, std::vector<std::string>> refresh_rate_per_output_resolution_;
	std::map<std::pair<std::string, std::string>, BMDDisplayMode> display_modes_;

	BMDPixelFormat current_pix_format_;
	BMDDisplayMode current_display_mode_;
	std::string display_mode_name_;

	RGB10BitVideoFrame * intermediate_frame_ = {nullptr};

	caf::actor viewport_, parent_plugin_;

public:

	DecklinkOutput(caf::actor viewport, caf::actor plugin);
	~DecklinkOutput();

	bool init_decklink();

	bool start_sdi_output();
    void set_preroll();
	bool stop_sdi_output(const std::string &error = std::string());
	void StartStop();

	void fill_decklink_video_frame(IDeckLinkVideoFrame* decklink_video_frame);
	void copy_audio_samples_to_decklink_buffer(const bool preroll);
	void receive_samples_from_xstudio(uint16_t * samples, unsigned long num_samps);
	long num_samples_in_buffer();
	void set_display_mode(const std::string & resolution, const std::string  &refresh_rate, const BMDPixelFormat pix_format);
	void set_audio_samples_water_level(const int w) { samples_water_level_ = (uint32_t)w; }
	void set_audio_sync_delay_milliseconds(const long ms_delay) { audio_sync_delay_milliseconds_ = ms_delay; }

	void incoming_frame(const media_reader::ImageBufPtr & frame);

	[[nodiscard]] int frameWidth() const { return static_cast<int>(frame_width_); }
	[[nodiscard]] int frameHeight() const { return static_cast<int>(frame_height_); }

	std::vector<std::string> get_available_refresh_rates(const std::string & output_resolution) const;

	std::vector<std::string> output_resolution_names() const {
		std::vector<std::string> result;
		for (const auto &p: refresh_rate_per_output_resolution_) {
			result.push_back(p.first);
		}
		std::sort(result.begin(), result.end());
		return result;
	}	

	std::vector<int16_t> audio_samples_buffer_;
	std::mutex audio_samples_buf_mutex_, audio_samples_cv_mutex_, bmd_mutex_;
	std::condition_variable audio_samples_cv_;
	bool fetch_more_samples_from_xstudio_ = {false};
	unsigned long samples_delivered_ = {0};
	uint32_t samples_water_level_ = {4096};
	long audio_sync_delay_milliseconds_ = {0};
	PixelSwizzler pixel_swizzler_;

};

class AVOutputCallback : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback
{
private:

    struct RefCt {

        int fetchAndAddAcquire(const int delta) {
            
            std::lock_guard<std::mutex> l(m);
            int old = count;
            count += delta;
            return old;
        }
    	std::atomic<int> count = 1;
        std::mutex m;
    } ref_count_;

	DecklinkOutput*	    owner_;

public:

	AVOutputCallback (DecklinkOutput* pOwner);

	// IUnknown
	HRESULT	QueryInterface (REFIID /*iid*/, LPVOID* /*ppv*/) override;
	ULONG	AddRef () override;
	ULONG	Release () override;

	// IDeckLinkAudioOutputCallback
	HRESULT RenderAudioSamples (bool preroll) override;

	// IDeckLinkVideoOutputCallback
	HRESULT	ScheduledFrameCompleted (IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result) override;
	HRESULT	ScheduledPlaybackHasStopped () override;
};

}}