// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont
//============================================================
//
//  osdlib_unix.cpp - OS specific low level code for POSIX-like systems
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// MAME headers
#include "osdcore.h"
#include "osdlib.h"

#ifdef SDLMAME_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

#if !defined(__SWITCH__)
#include <dlfcn.h>
#endif
#if !defined(__SWITCH__)
#include <sys/mman.h>
#endif
#include <sys/types.h>
#include <unistd.h>

#if defined(__SWITCH__)
#include <switch.h>
#endif


//============================================================
//  osd_getenv
//============================================================

#if defined(__SWITCH__)
#include <malloc.h>
#endif

//============================================================
//  osd_getenv
//============================================================

const char *osd_getenv(const char *name)
{
	return getenv(name);
}

//============================================================
extern "C" int setenv(const char *name, const char *value, int overwrite);

int osd_setenv(const char *name, const char *value, int overwrite)
{
	return setenv(name, value, overwrite);
}

//============================================================
//  osd_process_kill
//============================================================

void osd_process_kill()
{
#if defined(__SWITCH__)
	exit(0);
#else
	kill(getpid(), SIGKILL);
#endif
}


//============================================================
//  osd_break_into_debugger
//============================================================

void osd_break_into_debugger(const char *message)
{
#if defined(__linux__)
	bool do_break = false;
	FILE *const f = std::fopen("/proc/self/status", "r");
	if (f)
	{
		using namespace std::literals;

		std::string_view const tag = "TracerPid:\t"sv;
		char buf[128];
		bool ignore = false;
		while (std::fgets(buf, std::size(buf), f))
		{
			// ignore excessively long lines
			auto const len = strnlen(buf, std::size(buf));
			bool const noeol = !len || ('\n' != buf[len - 1]);
			if (ignore || noeol)
			{
				ignore = noeol;
				continue;
			}

			if (!std::strncmp(buf, tag.data(), tag.length()))
			{
				long tpid;
				if ((std::sscanf(buf + tag.length(), "%ld", &tpid) == 1) && (0 != tpid))
					do_break = true;
				break;
			}
		}
		std::fclose(f);
	}
#elif defined(MAME_DEBUG)
	bool const do_break = true;
#else
	bool const do_break = false;
#endif
	if (do_break)
	{
		printf("MAME exception: %s\n", message);
		printf("Attempting to fall into debugger\n");
#if !defined(__SWITCH__)
		kill(getpid(), SIGTRAP);
#endif
	}
	else
	{
		printf("Ignoring MAME exception: %s\n", message);
	}
}


//============================================================
//  osd_get_cache_line_size
//============================================================

std::pair<std::error_condition, unsigned> osd_get_cache_line_size() noexcept
{
#if defined(__SWITCH__)
	return std::make_pair(std::error_condition(), 64U);
#elif defined(__linux__)
	FILE *const f = std::fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
	if (!f)
		return std::make_pair(std::error_condition(errno, std::generic_category()), 0U);

	unsigned result = 0;
	auto const cnt = std::fscanf(f, "%u", &result);
	std::fclose(f);
	if (1 == cnt)
		return std::make_pair(std::error_condition(), result);
	else
		return std::make_pair(std::errc::io_error, 0U);
#else // defined(__linux__)
	return std::make_pair(std::errc::not_supported, 0U);
#endif
}


#ifdef SDLMAME_ANDROID
std::string osd_get_clipboard_text() noexcept
{
	return std::string();
}

std::error_condition osd_set_clipboard_text(std::string_view text) noexcept
{
	return std::errc::io_error; // TODO: better error code?
}
#else
//============================================================
//  osd_get_clipboard_text
//============================================================

std::string osd_get_clipboard_text() noexcept
{
	// TODO: better error handling
	std::string result;

	if (SDL_HasClipboardText())
	{
		char *const temp = SDL_GetClipboardText();
		if (temp)
		{
			try
			{
				result.assign(temp);
			}
			catch (std::bad_alloc const &)
			{
			}
			SDL_free(temp);
		}
	}
	return result;
}


//============================================================
//  osd_set_clipboard_text
//============================================================

std::error_condition osd_set_clipboard_text(std::string_view text) noexcept
{
	try
	{
		std::string const clip(text); // need to do this to ensure there's a terminating NUL for SDL
		#ifdef SDLMAME_SDL3
		if (!SDL_SetClipboardText(clip.c_str()))
		#else
		if (0 > SDL_SetClipboardText(clip.c_str()))
		#endif
		{
			// SDL_GetError returns a message, can't really convert it to an error condition
			return std::errc::io_error; // TODO: better error code?
		}

		return std::error_condition();
	}
	catch (std::bad_alloc const &)
	{
		return std::errc::not_enough_memory;
	}
}
#endif

//============================================================
//  osd_getpid
//============================================================

int osd_getpid() noexcept
{
	return getpid();
}


#if defined(__SWITCH__)
struct switch_drc_allocation {
	void *rx_addr = nullptr;
	void *rw_addr = nullptr;
	void *src_addr = nullptr;
	size_t size = 0;
	size_t near_size = 0;
	size_t boundary = 0; // split point between RX [near_size, boundary) and RW [boundary, size)
	VirtmemReservation *rv = nullptr;
	bool has_dual_map = false;
	Jit jit = {};
	uint32_t num_changes = 0;
	uint64_t total_ticks = 0;
};

std::mutex s_drc_mutex;
std::vector<switch_drc_allocation> s_drc_allocations;
static switch_drc_allocation *s_active_dual_drc = nullptr;

void *osd_switch_get_rx_ptr(void *ptr) noexcept
{
	if (!ptr) return nullptr;
	if (s_active_dual_drc && ptr >= s_active_dual_drc->rw_addr && (char*)ptr < (char*)s_active_dual_drc->rw_addr + s_active_dual_drc->size)
	{
		return (char*)s_active_dual_drc->rx_addr + ((char*)ptr - (char*)s_active_dual_drc->rw_addr);
	}
	if (s_active_dual_drc && ptr >= s_active_dual_drc->rx_addr && (char*)ptr < (char*)s_active_dual_drc->rx_addr + s_active_dual_drc->size)
	{
		return ptr;
	}
	return ptr;
}

void *osd_switch_get_rw_ptr(void *ptr) noexcept
{
	if (!ptr) return nullptr;
	if (s_active_dual_drc && ptr >= s_active_dual_drc->rx_addr && (char*)ptr < (char*)s_active_dual_drc->rx_addr + s_active_dual_drc->size)
	{
		return (char*)s_active_dual_drc->rw_addr + ((char*)ptr - (char*)s_active_dual_drc->rx_addr);
	}
	if (s_active_dual_drc && ptr >= s_active_dual_drc->rw_addr && (char*)ptr < (char*)s_active_dual_drc->rw_addr + s_active_dual_drc->size)
	{
		return ptr;
	}
	return ptr;
}

static bool switch_map_code_chunk(switch_drc_allocation *alloc, size_t offset, size_t size, u32 perm)
{
	if (size == 0)
		return true;
	Handle proc = envGetOwnProcessHandle();
	u64 dst = (u64)alloc->rx_addr + offset;
	u64 s = (u64)alloc->src_addr + offset;

	Result rc = svcMapProcessCodeMemory(proc, dst, s, size);
	if (R_FAILED(rc))
	{
		printf("switch: svcMapProcessCodeMemory(+0x%lx, 0x%lx) rc=0x%08x\n", (unsigned long)offset, (unsigned long)size, rc);
		return false;
	}

	rc = svcSetProcessMemoryPermission(proc, dst, size, perm);
	if (R_FAILED(rc))
	{
		printf("switch: svcSetProcessMemoryPermission(+0x%lx, 0x%lx, perm=%u) rc=0x%08x\n", (unsigned long)offset, (unsigned long)size, (unsigned)perm, rc);
		svcUnmapProcessCodeMemory(proc, dst, s, size);
		return false;
	}
	return true;
}

static bool switch_unmap_code_chunk(switch_drc_allocation *alloc, size_t offset, size_t size)
{
	if (size == 0)
		return true;
	Handle proc = envGetOwnProcessHandle();
	u64 dst = (u64)alloc->rx_addr + offset;
	u64 s = (u64)alloc->src_addr + offset;

	Result rc = svcUnmapProcessCodeMemory(proc, dst, s, size);
	if (R_FAILED(rc))
	{
		printf("switch: svcUnmapProcessCodeMemory(+0x%lx, 0x%lx) rc=0x%08x\n", (unsigned long)offset, (unsigned long)size, rc);
		return false;
	}
	return true;
}
#endif


namespace osd {

namespace {

#if defined(__SWITCH__)
class dynamic_module_posix_impl : public dynamic_module
{
public:
	dynamic_module_posix_impl(std::vector<std::string> &&libraries) : m_libraries(std::move(libraries))
	{
	}

	virtual ~dynamic_module_posix_impl() override = default;

protected:
	virtual generic_fptr_t get_symbol_address(char const *symbol) override
	{
		return nullptr;
	}

private:
	std::vector<std::string> m_libraries;
};
#else
class dynamic_module_posix_impl : public dynamic_module
{
public:
	dynamic_module_posix_impl(std::vector<std::string> &&libraries) : m_libraries(std::move(libraries))
	{
	}

	virtual ~dynamic_module_posix_impl() override
	{
		if (m_module)
			dlclose(m_module);
	}

protected:
	virtual generic_fptr_t get_symbol_address(char const *symbol) override
	{
		/*
		 * given a list of libraries, if a first symbol is successfully loaded from
		 * one of them, all additional symbols will be loaded from the same library
		 */
		if (m_module)
			return reinterpret_cast<generic_fptr_t>(dlsym(m_module, symbol));

		for (auto const &library : m_libraries)
		{
			void *const module = dlopen(library.c_str(), RTLD_LAZY);

			if (module != nullptr)
			{
				generic_fptr_t const function = reinterpret_cast<generic_fptr_t>(dlsym(module, symbol));

				if (function)
				{
					m_module = module;
					return function;
				}
				else
				{
					dlclose(module);
				}
			}
		}

		return nullptr;
	}

private:
	std::vector<std::string> m_libraries;
	void *                   m_module = nullptr;
};
#endif

} // anonymous namespace


bool invalidate_instruction_cache(void const *start, std::size_t size) noexcept
{
#if defined(__SWITCH__)
	void *rw = osd_switch_get_rw_ptr(const_cast<void *>(start));
	void *rx = osd_switch_get_rx_ptr(const_cast<void *>(start));
	armDCacheFlush(rw, size);
	armICacheInvalidate(rx, size);
#elif !defined(SDLMAME_EMSCRIPTEN)
	char const *const begin(reinterpret_cast<char const *>(start));
	char const *const end(begin + size);
	__builtin___clear_cache(const_cast<char *>(begin), const_cast<char *>(end));
#endif
	return true;
}


void *virtual_memory_allocation::do_alloc(std::initializer_list<std::size_t> blocks, unsigned intent, std::size_t &size, std::size_t &page_size) noexcept
{
	long const p = 4096;
	std::size_t s(0);
	for (std::size_t b : blocks)
		s += (b + p - 1) / p;
	s *= p;
	if (!s)
		return nullptr;

#if defined(__SWITCH__)
	if (!(intent & EXECUTE))
	{
		void *result = memalign(p, s);
		if (result)
		{
			std::memset(result, 0, s);
			size = s;
			page_size = p;
		}
		return result;
	}

	size_t near_size = 0;
	if (blocks.size() > 0)
	{
		near_size = (*blocks.begin() + p - 1) / p * p;
	}

	// First try modern Switch dual-mapping JIT (Atmosphere / HOS 4.0.0+)
	if (envIsSyscallHinted(0x4B) && envIsSyscallHinted(0x4C))
	{
		switch_drc_allocation alloc;
		alloc.size = s;
		alloc.near_size = near_size;

		Result rc = jitCreate(&alloc.jit, s);
		if (R_SUCCEEDED(rc) && alloc.jit.type == JitType_CodeMemory)
		{
			alloc.rx_addr = jitGetRxAddr(&alloc.jit);
			alloc.rw_addr = jitGetRwAddr(&alloc.jit);
			alloc.has_dual_map = true;
			alloc.boundary = s;

			std::memset(alloc.rw_addr, 0, s);

			{
				std::lock_guard<std::mutex> guard(s_drc_mutex);
				s_drc_allocations.push_back(alloc);
				s_active_dual_drc = &s_drc_allocations.back();
			}

			printf("switch: JIT dual-mapping allocated: rx=%p, rw=%p, size=0x%lx, near_size=0x%lx\n",
				alloc.rx_addr, alloc.rw_addr, (unsigned long)s, (unsigned long)near_size);

			size = s;
			page_size = p;
			return alloc.rw_addr;
		}
		else if (R_SUCCEEDED(rc))
		{
			jitClose(&alloc.jit);
		}
	}

	// Fallback path: code memory syscalls 0x73/0x77/0x78
	if (!envIsSyscallHinted(0x73) || !envIsSyscallHinted(0x77) || !envIsSyscallHinted(0x78))
	{
		printf("switch: Code memory syscalls 0x73/0x77/0x78 not hinted!\n");
		return nullptr;
	}

	void *src = memalign(p, s);
	if (!src)
	{
		printf("switch: memalign failed for DRC size 0x%lx\n", (unsigned long)s);
		return nullptr;
	}
	std::memset(src, 0, s);

	virtmemLock();
	void *rx = virtmemFindCodeMemory(s, p);
	VirtmemReservation *rv = rx ? virtmemAddReservation(rx, s) : nullptr;
	virtmemUnlock();

	if (!rx || !rv)
	{
		printf("switch: virtmemFindCodeMemory / virtmemAddReservation failed for size 0x%lx\n", (unsigned long)s);
		free(src);
		return nullptr;
	}

	switch_drc_allocation alloc;
	alloc.rx_addr = rx;
	alloc.rw_addr = rx;
	alloc.src_addr = src;
	alloc.size = s;
	alloc.near_size = near_size;
	alloc.rv = rv;
	alloc.has_dual_map = false;
	alloc.num_changes = 0;
	alloc.total_ticks = 0;

	bool map_ok = false;
	if (near_size > 0 && near_size < s)
	{
		map_ok = switch_map_code_chunk(&alloc, 0, near_size, Perm_Rw) &&
		         switch_map_code_chunk(&alloc, near_size, s - near_size, Perm_Rw);
		alloc.boundary = near_size;
	}
	else
	{
		map_ok = switch_map_code_chunk(&alloc, 0, s, Perm_Rw);
		alloc.boundary = 0;
	}

	if (!map_ok)
	{
		printf("switch: initial code memory mapping failed\n");
		if (near_size > 0 && near_size < s)
			switch_unmap_code_chunk(&alloc, 0, near_size);
		virtmemLock();
		virtmemRemoveReservation(rv);
		virtmemUnlock();
		free(src);
		return nullptr;
	}

	{
		std::lock_guard<std::mutex> guard(s_drc_mutex);
		s_drc_allocations.push_back(alloc);
	}

	printf("switch: DRC cache allocated (fallback W^X): rx=%p, src=%p, size=0x%lx, near_size=0x%lx\n", rx, src, (unsigned long)s, (unsigned long)near_size);

	size = s;
	page_size = p;
	return rx;
#else
#if defined __NetBSD__
	int req((NONE == intent) ? PROT_NONE : 0);
	if (intent & READ)
		req |= PROT_READ;
	if (intent & WRITE)
		req |= PROT_WRITE;
	if (intent & EXECUTE)
		req |= PROT_EXEC;
	int const prot(PROT_MPROTECT(req));
#else
	int const prot(PROT_NONE);
#endif
#if defined(SDLMAME_BSD) || defined(SDLMAME_MACOSX) || defined(SDLMAME_EMSCRIPTEN)
	int const fd(-1);
#else
	// TODO: portable applications are supposed to use -1 for anonymous mappings - detect whatever requires 0 specifically
	int const fd(0);
#endif
	void *const result(mmap(nullptr, s, prot, MAP_ANON | MAP_SHARED, fd, 0));
	if (result == (void *)-1)
		return nullptr;
	size = s;
	page_size = p;
	return result;
#endif
}

void virtual_memory_allocation::do_free(void *start, std::size_t size) noexcept
{
#if defined(__SWITCH__)
	{
		std::lock_guard<std::mutex> guard(s_drc_mutex);
		for (auto it = s_drc_allocations.begin(); it != s_drc_allocations.end(); ++it)
		{
			if (it->rx_addr == start || it->rw_addr == start)
			{
				if (s_active_dual_drc == &(*it))
					s_active_dual_drc = nullptr;

				if (it->has_dual_map)
				{
					jitClose(&it->jit);
					s_drc_allocations.erase(it);
					printf("switch: JIT dual-mapping freed (%p)\n", start);
					return;
				}
				else
				{
					if (it->near_size > 0)
					{
						switch_unmap_code_chunk(&*it, 0, it->near_size);
					}
					if (it->boundary > it->near_size)
					{
						switch_unmap_code_chunk(&*it, it->near_size, it->boundary - it->near_size);
					}
					if (it->size > it->boundary)
					{
						switch_unmap_code_chunk(&*it, it->boundary, it->size - it->boundary);
					}

					virtmemLock();
					virtmemRemoveReservation(it->rv);
					virtmemUnlock();
					free(it->src_addr);
					s_drc_allocations.erase(it);
					printf("switch: DRC cache freed (%p)\n", start);
					return;
				}
			}
		}
	}
	free(start);
#else
	munmap(reinterpret_cast<char *>(start), size);
#endif
}

bool virtual_memory_allocation::do_set_access(void *start, std::size_t size, unsigned access) noexcept
{
#if defined(__SWITCH__)
	std::lock_guard<std::mutex> guard(s_drc_mutex);
	switch_drc_allocation *found = nullptr;
	for (auto &a : s_drc_allocations)
	{
		if ((start >= a.rx_addr && (char*)start < (char*)a.rx_addr + a.size) ||
		    (start >= a.rw_addr && (char*)start < (char*)a.rw_addr + a.size))
		{
			found = &a;
			break;
		}
	}

	if (!found)
	{
		return true;
	}

	// Dual mapping is simultaneously writable at rw_addr and executable at rx_addr
	if (found->has_dual_map)
	{
		return true;
	}

	// Horizon OS strictly prohibits RWX pages. Return false to force MAME DRC to use W^X mode.
	if ((access & (WRITE | EXECUTE)) == (WRITE | EXECUTE))
	{
		return false;
	}

	size_t const offset = (char*)start - (char*)found->rx_addr;
	bool const wants_exec = (access & EXECUTE) != 0;

	size_t target_boundary;
	if (wants_exec)
	{
		armDCacheFlush(start, size);
		armICacheInvalidate(start, size);

		target_boundary = offset + size;
		if (target_boundary > found->size)
			target_boundary = found->size;
		if (target_boundary < found->near_size)
			target_boundary = found->near_size;
	}
	else
	{
		target_boundary = offset;
		if (target_boundary < found->near_size)
			target_boundary = found->near_size;
		if (target_boundary > found->boundary)
			target_boundary = found->boundary;
	}

	if (target_boundary == found->boundary)
	{
		return true;
	}

	uint64_t t0 = armGetSystemTick();

	if (target_boundary > found->boundary)
	{
		size_t delta = target_boundary - found->boundary;
		if (!switch_unmap_code_chunk(found, found->boundary, delta) ||
		    !switch_map_code_chunk(found, found->boundary, delta, Perm_Rx))
		{
			// Partial remap failed; rebuild entire dynamic area using exactly-mapped chunks
			if (found->boundary > found->near_size)
				switch_unmap_code_chunk(found, found->near_size, found->boundary - found->near_size);
			if (found->size > found->boundary)
				switch_unmap_code_chunk(found, found->boundary, found->size - found->boundary);

			if (target_boundary > found->near_size)
				switch_map_code_chunk(found, found->near_size, target_boundary - found->near_size, Perm_Rx);
			if (found->size > target_boundary)
				switch_map_code_chunk(found, target_boundary, found->size - target_boundary, Perm_Rw);
		}
	}
	else
	{
		size_t delta = found->boundary - target_boundary;
		if (!switch_unmap_code_chunk(found, target_boundary, delta) ||
		    !switch_map_code_chunk(found, target_boundary, delta, Perm_Rw))
		{
			// Partial remap failed; rebuild entire dynamic area using exactly-mapped chunks
			if (found->boundary > found->near_size)
				switch_unmap_code_chunk(found, found->near_size, found->boundary - found->near_size);
			if (found->size > found->boundary)
				switch_unmap_code_chunk(found, found->boundary, found->size - found->boundary);

			if (target_boundary > found->near_size)
				switch_map_code_chunk(found, found->near_size, target_boundary - found->near_size, Perm_Rx);
			if (found->size > target_boundary)
				switch_map_code_chunk(found, target_boundary, found->size - target_boundary, Perm_Rw);
		}
	}

	found->boundary = target_boundary;
	found->num_changes++;
	found->total_ticks += (armGetSystemTick() - t0);

	return true;
#else
	int prot((NONE == access) ? PROT_NONE : 0);
	if (access & READ)
		prot |= PROT_READ;
	if (access & WRITE)
		prot |= PROT_WRITE;
	if (access & EXECUTE)
		prot |= PROT_EXEC;
	return mprotect(reinterpret_cast<char *>(start), size, prot) == 0;
#endif
}


dynamic_module::ptr dynamic_module::open(std::vector<std::string> &&names)
{
	return std::make_unique<dynamic_module_posix_impl>(std::move(names));
}

} // namespace osd
