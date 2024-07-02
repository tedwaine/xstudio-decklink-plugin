#include "pixel_swizzler.hpp"
#include <memory>
#include <thread>
#include <vector>

using namespace xstudio::bm_decklink_plugin_1_0;

void PixelSwizzler::cpy16bitRGBA_to_10bitRGB(
            void * _dst,
            void * _src,
            size_t num_pix) 
{

    // could SSE instructions be used here, or will compiler achieve that
    // for us?
    auto swizzle_chunk = [](uint32_t * _dst, uint16_t * _src, size_t n) {

        while (n--) {             

            uint32_t red = *(_src++) >> 6;
            uint32_t green = *(_src++) >> 6;
            uint32_t blue = *(_src++) >> 6;
            _src++; // skip alpha
            uint32_t le = (blue) + (green << 10) + (red << 20);   
            *(_dst++) = __builtin_bswap32(le);
        }

    };

    // Note: my instinct tells me that spawning threads for
    // every copy operation (which might happen 60 times a second)
    // is not efficient but it seems that having a threadool doesn't
    // make any real difference, the overhead of thread creation
    // is tiny.
    std::vector<std::thread> memcpy_threads;
    size_t step = ((num_pix / n_threads_) / 4096) * 4096;

    uint32_t *dst = (uint32_t *)_dst;
    uint16_t *src = (uint16_t *)_src;

    for (int i = 0; i < n_threads_; ++i) {
        memcpy_threads.emplace_back(swizzle_chunk, dst, src, std::min(num_pix, step));
        dst += step;
        src += step*4;
        num_pix -= step;
    }

    // ensure any threads still running to copy data to this texture are done
    for (auto &t : memcpy_threads) {
        if (t.joinable())
            t.join();
    }
}

void PixelSwizzler::cpy16bitRGBA_to_10bitRGBLE(
            void * _dst,
            void * _src,
            size_t num_pix) 
{

    auto swizzle_chunk = [](uint32_t * _dst, uint16_t * _src, size_t n) {

        while (n--) {             

            uint32_t red = *(_src++) >> 6;
            uint32_t green = *(_src++) >> 6;
            uint32_t blue = *(_src++) >> 6;
            _src++; // skip alpha
            *(_dst++) = (blue) + (green << 10) + (red << 20);   
        }

    };

    // Note: my instinct tells me that spawning threads for
    // every copy operation (which might happen 60 times a second)
    // is not efficient but it seems that having a threadool doesn't
    // make any real difference, the overhead of thread creation
    // is tiny.
    std::vector<std::thread> memcpy_threads;
    size_t step = ((num_pix / n_threads_) / 4096) * 4096;

    uint32_t *dst = (uint32_t *)_dst;
    uint16_t *src = (uint16_t *)_src;

    for (int i = 0; i < n_threads_; ++i) {
        memcpy_threads.emplace_back(swizzle_chunk, dst, src, std::min(num_pix, step));
        dst += step;
        src += step*4;
        num_pix -= step;
    }

    // ensure any threads still running to copy data to this texture are done
    for (auto &t : memcpy_threads) {
        if (t.joinable())
            t.join();
    }
}

void PixelSwizzler::cpy16bitRGBA_to_12bitRGBLE(
            void * _dst,
            void * _src,
            size_t num_pix) 
{

    // again, SSE instructions could make this soooo much better unless
    // the compiler is being clever for us.
    auto swizzle_chunk = [](uint16_t * _dst, uint16_t * _src, size_t n) {

        int shift = 0;
        while (n>=4) {             

            // 4 channels worth of pixel data. truncated to 12 bits each
            uint16_t q = *(_src++) >> 4;
            uint16_t r = *(_src++) >> 4;
            uint16_t s = *(_src++) >> 4;
            _src++; // skip alpha
            uint16_t t = *(_src++) >> 4;

            *(_dst++) = q + ((r&15) << 12);
            *(_dst++) = (r >> 4) + ((s&255) << 8);
            *(_dst++) = (s >> 8) + (t << 4);

            q = *(_src++) >> 4;
            r = *(_src++) >> 4;
            _src++; // skip alpha
            s = *(_src++) >> 4;
            t = *(_src++) >> 4;

            *(_dst++) = q + ((r&15) << 12);
            *(_dst++) = (r >> 4) + ((s&255) << 8);
            *(_dst++) = (s >> 8) + (t << 4);

            q = *(_src++) >> 4;
            _src++; // skip alpha
            r = *(_src++) >> 4;
            s = *(_src++) >> 4;
            t = *(_src++) >> 4;

            *(_dst++) = q + ((r&15) << 12);
            *(_dst++) = (r >> 4) + ((s&255) << 8);
            *(_dst++) = (s >> 8) + (t << 4);

            _src++; // skip alpha
            n-=4;

        }

    };

    std::vector<std::thread> memcpy_threads;
    size_t step = ((num_pix / n_threads_) / 4128) * 4128;

    uint16_t *dst = (uint16_t *)_dst;
    uint16_t *src = (uint16_t *)_src;

    for (int i = 0; i < n_threads_; ++i) {
        memcpy_threads.emplace_back(swizzle_chunk, dst, src, std::min(num_pix, step));
        dst += (step*9)/4;
        src += step*4;
        num_pix -= step;
    }

    // ensure any threads still running to copy data to this texture are done
    for (auto &t : memcpy_threads) {
        if (t.joinable())
            t.join();
    }
}

void PixelSwizzler::cpy16bitRGBA_to_12bitRGB(
            void * _dst,
            void * _src,
            size_t num_pix) 
{

    // again, SSE instructions could make this soooo much better unless
    // the compiler is being clever for us.
    auto swizzle_chunk = [](uint16_t * _dst, uint16_t * _src, size_t n) {

        uint32_t * _mdst = (uint32_t *)_dst;
        size_t mn = (n*9)/8;

        while (n>=4) {             

            // 4 channels worth of pixel data. truncated to 12 bits each
            uint16_t q = *(_src++) >> 4;
            uint16_t r = *(_src++) >> 4;
            uint16_t s = *(_src++) >> 4;
            _src++; // skip alpha
            uint16_t t = *(_src++) >> 4;

            *(_dst++) = q + ((r&15) << 12);
            *(_dst++) = (r >> 4) + ((s&255) << 8);
            *(_dst++) = (s >> 8) + (t << 4);

            q = *(_src++) >> 4;
            r = *(_src++) >> 4;
            _src++; // skip alpha
            s = *(_src++) >> 4;
            t = *(_src++) >> 4;

            *(_dst++) = q + ((r&15) << 12);
            *(_dst++) = (r >> 4) + ((s&255) << 8);
            *(_dst++) = (s >> 8) + (t << 4);

            q = *(_src++) >> 4;
            _src++; // skip alpha
            r = *(_src++) >> 4;
            s = *(_src++) >> 4;
            t = *(_src++) >> 4;

            *(_dst++) = q + ((r&15) << 12);
            *(_dst++) = (r >> 4) + ((s&255) << 8);
            *(_dst++) = (s >> 8) + (t << 4);

            _src++; // skip alpha
            n-=4;

        }

        while (mn--) {
            *_mdst = __builtin_bswap32(*_mdst);
            _mdst++;
        }

    };

    std::vector<std::thread> memcpy_threads;
    size_t step = ((num_pix / n_threads_) / 4128) * 4128;

    uint16_t *dst = (uint16_t *)_dst;
    uint16_t *src = (uint16_t *)_src;

    for (int i = 0; i < n_threads_; ++i) {
        memcpy_threads.emplace_back(swizzle_chunk, dst, src, std::min(num_pix, step));
        dst += (step*9)/4;
        src += step*4;
        num_pix -= step;
    }

    // ensure any threads still running to copy data to this texture are done
    for (auto &t : memcpy_threads) {
        if (t.joinable())
            t.join();
    }
}

