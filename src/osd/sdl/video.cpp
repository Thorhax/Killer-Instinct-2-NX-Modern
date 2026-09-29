// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont
//============================================================
//
//  video.cpp - SDL video handling
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#include "window.h"

// MAMEOS headers
#include "osdsdl.h"
#include "modules/lib/osdlib.h"
#include "modules/monitor/monitor_module.h"
#include "modules/render/render_module.h"

// MAME headers
#include "emu.h"
#include "emuopts.h"
#include "main.h"
#include "rendutil.h"
#include "uiinput.h"

#include <SDL2/SDL.h>

#if defined(__SWITCH__)
#include "nxperf.h"
#include <cstdio>
#include <ctime>
#endif


//============================================================
//  GLOBAL VARIABLES
//============================================================

osd_video_config video_config;


//============================================================
//  PROTOTYPES
//============================================================

static void get_resolution(const char *defdata, const char *data, osd_window_config *config, int report_error);


//============================================================
//  video_init
//============================================================

bool sdl_osd_interface::video_init()
{
	int index;

	// extract data from the options
	extract_video_config();

	// we need the beam width in a float, contrary to what the core does.
	video_config.beamwidth = options().beam_width_min();

	// initialize the window system so we can make windows
	if (!window_init())
		return false;

	// create the windows
	for (index = 0; index < video_config.numscreens; index++)
	{
		osd_window_config conf;
		get_resolution(options().resolution(), options().resolution(index), &conf, true);

		// create window ...
		auto win = std::make_unique<sdl_window_info>(machine(), *m_render, index, m_monitor_module->pick_monitor(reinterpret_cast<osd_options &>(options()), index), &conf);
		if (win->window_init())
			return false;

		s_window_list.emplace_back(std::move(win));
	}

#if defined(__SWITCH__)
	if (!s_window_list.empty())
		m_focus_window = dynamic_cast<sdl_window_info *>(s_window_list.front().get());
#endif

	if (m_render->is_interactive())
	{
		SDL_Window *const sdlwindow = dynamic_cast<sdl_window_info &>(*osd_common_t::s_window_list.front()).platform_window();
		SDL_RaiseWindow(sdlwindow);

#ifdef SDLMAME_MACOSX
		// ensure focus is acquired before the input modules start polling
		process_events();
		if (!has_focus())
			osd_printf_verbose("Window did not acquire input focus\n");
#endif
	}

	return true;
}

//============================================================
//  video_exit
//============================================================

void sdl_osd_interface::video_exit()
{
	window_exit();
}

//============================================================
//  update
//============================================================

#if defined(__SWITCH__)
namespace {

// once-per-second performance summary written to kinst.log
struct nx_perf_report
{
	uint64_t interval_start = 0;
	uint64_t last_frame = 0;
	double emu_start = 0.0;
	unsigned frames = 0;
	unsigned drawn = 0;
	uint64_t worst_frame = 0;
	uint64_t render_total = 0;
	uint64_t render_worst = 0;
	unsigned seconds = 0;

	void frame(running_machine &machine, bool skipped, uint64_t render_ticks)
	{
		uint64_t const now = nxperf::ticks();
		if (!interval_start)
		{
			interval_start = last_frame = now;
			emu_start = machine.time().as_double();
			return;
		}

		worst_frame = std::max(worst_frame, now - last_frame);
		last_frame = now;
		frames++;
		if (!skipped)
		{
			drawn++;
			render_total += render_ticks;
			render_worst = std::max(render_worst, render_ticks);
		}

		uint64_t const elapsed = now - interval_start;
		if (elapsed < nxperf::TICKS_PER_SEC)
			return;

		double const emu_now = machine.time().as_double();
		double const speed = 100.0 * (emu_now - emu_start) / (double(elapsed) / double(nxperf::TICKS_PER_SEC));
		seconds += unsigned(elapsed / nxperf::TICKS_PER_SEC);

		uint64_t dc, dt, dw, rc, rt, rw, hc, ht, hw, ac, at, aw;
		nxperf::drc_compile.take(dc, dt, dw);
		nxperf::disk_read.take(rc, rt, rw);
		nxperf::disk_hit.take(hc, ht, hw);
		nxperf::audio_short.take(ac, at, aw);
		uint64_t uc, ut, uw, pc, pt, pw, fc, ft, fw;
		nxperf::tex_upload.take(uc, ut, uw);
		nxperf::gpu_present.take(pc, pt, pw);
		nxperf::sdl_fail.take(fc, ft, fw);
		uint64_t gc, gt, gw;
		nxperf::gpu_finish.take(gc, gt, gw);
		uint64_t mem_used, mem_heap, mem_borrowed, mem_ipc, mem_device;
		nxperf::mem_stats(mem_used, mem_heap, mem_borrowed, mem_ipc, mem_device);

		std::time_t const wall = std::time(nullptr) + nxperf::utc_offset.load();
		std::tm tmv;
		gmtime_r(&wall, &tmv);

		std::printf("perf %02d:%02d:%02d t=%us speed=%.1f%% skip=%d frames=%u drawn=%u worst_frame=%.1fms"
				" | render %.1fms (max %.1f)"
				" | drc %llu blk %.1fms (max %.1f)"
				" | disk ram %llu %.1fms, file %llu %.1fms (max %.1f)"
				" | audio short %llu (%llu samples)"
				" | upload %.1fms (max %.1f) present %.1fms (max %.1f) gpu %.1fms (max %.1f) sdlfail %llu tex %lld/%llu"
				" | mem used %lluMB malloc %lluMB borrowed %lluMB ipc %lluMB dev %lluMB\n",
				tmv.tm_hour, tmv.tm_min, tmv.tm_sec,
				seconds, speed, machine.video().effective_frameskip(), frames, drawn, nxperf::ticks_to_ms(worst_frame),
				nxperf::ticks_to_ms(render_total), nxperf::ticks_to_ms(render_worst),
				(unsigned long long)dc, nxperf::ticks_to_ms(dt), nxperf::ticks_to_ms(dw),
				(unsigned long long)hc, nxperf::ticks_to_ms(ht),
				(unsigned long long)rc, nxperf::ticks_to_ms(rt), nxperf::ticks_to_ms(rw),
				(unsigned long long)ac, (unsigned long long)at,
				nxperf::ticks_to_ms(ut), nxperf::ticks_to_ms(uw), nxperf::ticks_to_ms(pt), nxperf::ticks_to_ms(pw),
				nxperf::ticks_to_ms(gt), nxperf::ticks_to_ms(gw),
				(unsigned long long)fc, (long long)nxperf::tex_live.load(), (unsigned long long)nxperf::tex_created.load(),
				(unsigned long long)(mem_used >> 20), (unsigned long long)(mem_heap >> 20), (unsigned long long)(mem_borrowed >> 20),
				(unsigned long long)(mem_ipc >> 20), (unsigned long long)(mem_device >> 20));

		interval_start = now;
		emu_start = emu_now;
		frames = drawn = 0;
		worst_frame = render_total = render_worst = 0;
	}
};

nx_perf_report s_nx_perf;

} // anonymous namespace
#endif

void sdl_osd_interface::update(bool skip_redraw)
{
	osd_common_t::update(skip_redraw);

#if defined(__SWITCH__)
	uint64_t const render_start = nxperf::ticks();
#endif

	// if we're not skipping this redraw, update all windows
	if (!skip_redraw)
	{
//      profiler_mark(PROFILER_BLIT);
		for (auto const &window : osd_common_t::window_list())
			window->update();
//      profiler_mark(PROFILER_END);
	}

#if defined(__SWITCH__)
	s_nx_perf.frame(machine(), skip_redraw, nxperf::ticks() - render_start);
#endif

	// if we're running, disable some parts of the debugger
	if ((machine().debug_flags & DEBUG_FLAG_OSD_ENABLED) != 0)
		debugger_update();
}

//============================================================
//  extract_video_config
//============================================================

void sdl_osd_interface::extract_video_config()
{
	video_config.perftest    = options().video_fps();

	// global options: extract the data
	video_config.windowed      = options().window();
	video_config.prescale      = options().prescale();
	video_config.filter        = options().filter();
	video_config.numscreens    = options().numscreens();
	#ifdef SDLMAME_X11
	video_config.restrictonemonitor = !options().use_all_heads();
	#endif

	// if we are in debug mode, never go full screen
	if (machine().debug_flags & DEBUG_FLAG_OSD_ENABLED)
		video_config.windowed = true;

	video_config.switchres     = options().switch_res();
	video_config.centerh       = options().centerh();
	video_config.centerv       = options().centerv();
	video_config.waitvsync     = options().wait_vsync();
	video_config.syncrefresh   = options().sync_refresh();
	if (!video_config.waitvsync && video_config.syncrefresh)
	{
		osd_printf_warning("-syncrefresh specified without -waitvsync. Reverting to -nosyncrefresh\n");
		video_config.syncrefresh = 0;
	}

	if (video_config.prescale < 1 || video_config.prescale > 20)
	{
		osd_printf_warning("Invalid prescale option, reverting to '1'\n");
		video_config.prescale = 1;
	}

	// misc options: sanity check values

	// global options: sanity check values
	if (video_config.numscreens < 1 || video_config.numscreens > MAX_VIDEO_WINDOWS)
	{
		osd_printf_warning("Invalid numscreens value %d; reverting to 1\n", video_config.numscreens);
		video_config.numscreens = 1;
	}
}


//============================================================
//  get_resolution
//============================================================

static void get_resolution(const char *defdata, const char *data, osd_window_config *config, int report_error)
{
	config->width = config->height = config->depth = config->refresh = 0;
	if (strcmp(data, OSDOPTVAL_AUTO) == 0)
	{
		if (strcmp(defdata, OSDOPTVAL_AUTO) == 0)
			return;
		data = defdata;
	}

	if (sscanf(data, "%dx%dx%d", &config->width, &config->height, &config->depth) < 2 && report_error)
		osd_printf_error("Illegal resolution value = %s\n", data);

	const char * at_pos = strchr(data, '@');
	if (at_pos)
		if (sscanf(at_pos + 1, "%d", &config->refresh) < 1 && report_error)
			osd_printf_error("Illegal refresh rate in resolution value = %s\n", data);
}
