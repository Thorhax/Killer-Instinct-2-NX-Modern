// license:BSD-3-Clause
//============================================================
//
//  nxperf.h - lightweight Switch performance counters
//
//  Header-only so it can be used from lib/util, devices and
//  the OSD layer without adding link dependencies.
//
//============================================================

#ifndef MAME_OSD_NXPERF_H
#define MAME_OSD_NXPERF_H

#pragma once

#include <atomic>
#include <cstdint>

namespace nxperf {

// ARM generic timer runs at 19.2 MHz on the Switch
constexpr uint64_t TICKS_PER_SEC = 19'200'000;

inline uint64_t ticks() noexcept
{
#if defined(__aarch64__)
	uint64_t v;
	asm volatile("mrs %0, cntpct_el0" : "=r"(v));
	return v;
#else
	return 0;
#endif
}

inline double ticks_to_ms(uint64_t t) noexcept { return double(t) * 1000.0 / double(TICKS_PER_SEC); }

struct counter
{
	std::atomic<uint64_t> count{ 0 };
	std::atomic<uint64_t> total{ 0 };
	std::atomic<uint64_t> worst{ 0 };

	void add(uint64_t t) noexcept
	{
		count.fetch_add(1, std::memory_order_relaxed);
		total.fetch_add(t, std::memory_order_relaxed);
		uint64_t w = worst.load(std::memory_order_relaxed);
		while (t > w && !worst.compare_exchange_weak(w, t, std::memory_order_relaxed)) { }
	}

	// read and reset for per-interval reporting
	void take(uint64_t &c, uint64_t &tot, uint64_t &w) noexcept
	{
		c = count.exchange(0, std::memory_order_relaxed);
		tot = total.exchange(0, std::memory_order_relaxed);
		w = worst.exchange(0, std::memory_order_relaxed);
	}
};

inline counter drc_compile;   // MIPS3 block compiles
inline counter disk_read;     // CHD hunk reads served from file (decompress/SD)
inline counter disk_hit;      // CHD hunk reads served from RAM
inline counter audio_short;   // audio callbacks that found too few samples (total = samples missing)
inline counter tex_upload;    // SDL texture data uploads (lock/update + copy)
inline counter gpu_present;   // SDL_RenderPresent
inline counter gpu_finish;    // glFinish after present = time until the GPU has caught up
inline counter sdl_fail;      // failed SDL texture create/lock/update/copy calls
inline std::atomic<int64_t> tex_live{ 0 };      // SDL textures currently alive
inline std::atomic<uint64_t> tex_created{ 0 };  // SDL textures created (cumulative)

// per-second perf lines in kinst.log are opt-in (sdmc:/switch/kinst/perf.txt)
inline std::atomic<bool> enabled{ false };

// filled in by the Switch OSD layer (needs libnx)
void mem_stats(uint64_t &used, uint64_t &heap, uint64_t &borrowed, uint64_t &ipc, uint64_t &device) noexcept;

// Registered by the Switch OSD layer at startup; moves the calling thread off
// the emulation core at low priority.  A pointer rather than a function so
// lib/util (CHD preload thread) needs no link dependency on the OSD.
inline void (*background_thread_hook)() noexcept = nullptr;
inline void set_background_thread() noexcept
{
	if (background_thread_hook)
		background_thread_hook();
}

// seconds east of UTC for the console's time zone, so log lines can be
// matched against capture filenames (which use local time)
inline std::atomic<int32_t> utc_offset{ 0 };

class scope
{
public:
	explicit scope(counter &c) noexcept : m_counter(c), m_start(ticks()) { }
	~scope() { m_counter.add(ticks() - m_start); }
private:
	counter &m_counter;
	uint64_t m_start;
};

} // namespace nxperf

#endif // MAME_OSD_NXPERF_H
