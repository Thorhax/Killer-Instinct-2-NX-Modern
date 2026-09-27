// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, Brad Hughes, Vas Crabb
//============================================================
//
//  input_sdl.cpp - SDL 2.0 implementation of MAME input routines
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//  SixAxis info: left analog is axes 0 & 1, right analog is axes 2 & 3,
//                analog L2 is axis 12 and analog L3 is axis 13
//
//============================================================

#include "input_module.h"

#include "modules/osdmodule.h"

#if defined(OSD_SDL) && !defined(SDLMAME_SDL3)

#include "assignmenthelper.h"
#include "input_common.h"
#include "input_sdlcommon.h"

#include "interface/inputseq.h"
#include "modules/lib/osdobj_common.h"
#include "sdl/osdsdl.h"

// emu
#include "inpttype.h"

// standard SDL header
#include <SDL2/SDL.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#if defined(__SWITCH__)
#include <switch.h>
#endif

// winnt.h defines this
#ifdef DELETE
#undef DELETE
#endif


namespace osd {

namespace {

char const *const CONTROLLER_AXIS_XBOX[]{
		"LSX",
		"LSY",
		"RSX",
		"RSY",
		"LT",
		"RT" };

char const *const CONTROLLER_AXIS_PS[]{
		"LSX",
		"LSY",
		"RSX",
		"RSY",
		"L2",
		"R2" };

char const *const CONTROLLER_AXIS_SWITCH[]{
		"LSX",
		"LSY",
		"RSX",
		"RSY",
		"ZL",
		"ZR" };

char const *const CONTROLLER_BUTTON_XBOX360[]{
		"A",
		"B",
		"X",
		"Y",
		"Back",
		"Guide",
		"Start",
		"LSB",
		"RSB",
		"LB",
		"RB",
		"D-pad Up",
		"D-pad Down",
		"D-pad Left",
		"D-pad Right",
		"Share",
		"P1",
		"P2",
		"P3",
		"P4",
		"Touchpad" };

char const *const CONTROLLER_BUTTON_XBOXONE[]{
		"A",
		"B",
		"X",
		"Y",
		"View",
		"Logo",
		"Menu",
		"LSB",
		"RSB",
		"LB",
		"RB",
		"D-pad Up",
		"D-pad Down",
		"D-pad Left",
		"D-pad Right",
		"Share",
		"P1",
		"P2",
		"P3",
		"P4",
		"Touchpad" };

char const *const CONTROLLER_BUTTON_PS3[]{
		"Cross",
		"Circle",
		"Square",
		"Triangle",
		"Select",
		"PS",
		"Start",
		"L3",
		"R3",
		"L1",
		"R1",
		"D-pad Up",
		"D-pad Down",
		"D-pad Left",
		"D-pad Right",
		"Mute",
		"P1",
		"P2",
		"P3",
		"P4",
		"Touchpad" };

char const *const CONTROLLER_BUTTON_PS4[]{
		"Cross",
		"Circle",
		"Square",
		"Triangle",
		"Share",
		"PS",
		"Options",
		"L3",
		"R3",
		"L1",
		"R1",
		"D-pad Up",
		"D-pad Down",
		"D-pad Left",
		"D-pad Right",
		"Mute",
		"P1",
		"P2",
		"P3",
		"P4",
		"Touchpad" };

char const *const CONTROLLER_BUTTON_PS5[]{
		"Cross",
		"Circle",
		"Square",
		"Triangle",
		"Create",
		"PS",
		"Options",
		"L3",
		"R3",
		"L1",
		"R1",
		"D-pad Up",
		"D-pad Down",
		"D-pad Left",
		"D-pad Right",
		"Mute",
		"P1",
		"P2",
		"P3",
		"P4",
		"Touchpad" };

char const *const CONTROLLER_BUTTON_SWITCH[]{
		"A",
		"B",
		"X",
		"Y",
		"-",
		"Home",
		"+",
		"LSB",
		"RSB",
		"L",
		"R",
		"D-pad Up",
		"D-pad Down",
		"D-pad Left",
		"D-pad Right",
		"Capture",
		"RSR",
		"LSL",
		"RSL",
		"LSR",
		"Touchpad" };

[[maybe_unused]] char const *const CONTROLLER_BUTTON_STADIA[]{
		"A",
		"B",
		"X",
		"Y",
		"Options",
		"Logo",
		"Menu",
		"L3",
		"R3",
		"L1",
		"R1",
		"D-pad Up",
		"D-pad Down",
		"D-pad Left",
		"D-pad Right",
		"Capture",
		"P1",
		"P2",
		"P3",
		"P4",
		"Touchpad" };

[[maybe_unused]] char const *const CONTROLLER_BUTTON_SHIELD[]{
		"A",
		"B",
		"X",
		"Y",
		"Back",
		"Logo",
		"Start",
		"LSB",
		"RSB",
		"LB",
		"RB",
		"D-pad Up",
		"D-pad Down",
		"D-pad Left",
		"D-pad Right",
		"Share",
		"P1",
		"P2",
		"P3",
		"P4",
		"Touchpad" };

struct key_lookup_table
{
	int code;
	const char *name;
};

#define KE(x) { SDL_SCANCODE_ ## x, "SDL_SCANCODE_" #x },

key_lookup_table const sdl_lookup_table[] =
{
	KE(UNKNOWN)

	KE(A)
	KE(B)
	KE(C)
	KE(D)
	KE(E)
	KE(F)
	KE(G)
	KE(H)
	KE(I)
	KE(J)
	KE(K)
	KE(L)
	KE(M)
	KE(N)
	KE(O)
	KE(P)
	KE(Q)
	KE(R)
	KE(S)
	KE(T)
	KE(U)
	KE(V)
	KE(W)
	KE(X)
	KE(Y)
	KE(Z)

	KE(1)
	KE(2)
	KE(3)
	KE(4)
	KE(5)
	KE(6)
	KE(7)
	KE(8)
	KE(9)
	KE(0)

	KE(RETURN)
	KE(ESCAPE)
	KE(BACKSPACE)
	KE(TAB)
	KE(SPACE)

	KE(MINUS)
	KE(EQUALS)
	KE(LEFTBRACKET)
	KE(RIGHTBRACKET)
	KE(BACKSLASH)
	KE(NONUSHASH)
	KE(SEMICOLON)
	KE(APOSTROPHE)
	KE(GRAVE)
	KE(COMMA)
	KE(PERIOD)
	KE(SLASH)

	KE(CAPSLOCK)

	KE(F1)
	KE(F2)
	KE(F3)
	KE(F4)
	KE(F5)
	KE(F6)
	KE(F7)
	KE(F8)
	KE(F9)
	KE(F10)
	KE(F11)
	KE(F12)

	KE(PRINTSCREEN)
	KE(SCROLLLOCK)
	KE(PAUSE)
	KE(INSERT)
	KE(HOME)
	KE(PAGEUP)
	KE(DELETE)
	KE(END)
	KE(PAGEDOWN)
	KE(RIGHT)
	KE(LEFT)
	KE(DOWN)
	KE(UP)

	KE(NUMLOCKCLEAR)
	KE(KP_DIVIDE)
	KE(KP_MULTIPLY)
	KE(KP_MINUS)
	KE(KP_PLUS)
	KE(KP_ENTER)
	KE(KP_1)
	KE(KP_2)
	KE(KP_3)
	KE(KP_4)
	KE(KP_5)
	KE(KP_6)
	KE(KP_7)
	KE(KP_8)
	KE(KP_9)
	KE(KP_0)
	KE(KP_PERIOD)

	KE(NONUSBACKSLASH)
	KE(APPLICATION)
	KE(POWER)
	KE(KP_EQUALS)
	KE(F13)
	KE(F14)
	KE(F15)
	KE(F16)
	KE(F17)
	KE(F18)
	KE(F19)
	KE(F20)
	KE(F21)
	KE(F22)
	KE(F23)
	KE(F24)
	KE(EXECUTE)
	KE(HELP)
	KE(MENU)
	KE(SELECT)
	KE(STOP)
	KE(AGAIN)
	KE(UNDO)
	KE(CUT)
	KE(COPY)
	KE(PASTE)
	KE(FIND)
	KE(MUTE)
	KE(VOLUMEUP)
	KE(VOLUMEDOWN)
	KE(KP_COMMA)
	KE(KP_EQUALSAS400)

	KE(INTERNATIONAL1)
	KE(INTERNATIONAL2)
	KE(INTERNATIONAL3)
	KE(INTERNATIONAL4)
	KE(INTERNATIONAL5)
	KE(INTERNATIONAL6)
	KE(INTERNATIONAL7)
	KE(INTERNATIONAL8)
	KE(INTERNATIONAL9)
	KE(LANG1)
	KE(LANG2)
	KE(LANG3)
	KE(LANG4)
	KE(LANG5)
	KE(LANG6)
	KE(LANG7)
	KE(LANG8)
	KE(LANG9)

	KE(ALTERASE)
	KE(SYSREQ)
	KE(CANCEL)
	KE(CLEAR)
	KE(PRIOR)
	KE(RETURN2)
	KE(SEPARATOR)
	KE(OUT)
	KE(OPER)
	KE(CLEARAGAIN)
	KE(CRSEL)
	KE(EXSEL)

	KE(KP_00)
	KE(KP_000)
	KE(THOUSANDSSEPARATOR)
	KE(DECIMALSEPARATOR)
	KE(CURRENCYUNIT)
	KE(CURRENCYSUBUNIT)
	KE(KP_LEFTPAREN)
	KE(KP_RIGHTPAREN)
	KE(KP_LEFTBRACE)
	KE(KP_RIGHTBRACE)
	KE(KP_TAB)
	KE(KP_BACKSPACE)
	KE(KP_A)
	KE(KP_B)
	KE(KP_C)
	KE(KP_D)
	KE(KP_E)
	KE(KP_F)
	KE(KP_XOR)
	KE(KP_POWER)
	KE(KP_PERCENT)
	KE(KP_LESS)
	KE(KP_GREATER)
	KE(KP_AMPERSAND)
	KE(KP_DBLAMPERSAND)
	KE(KP_VERTICALBAR)
	KE(KP_DBLVERTICALBAR)
	KE(KP_COLON)
	KE(KP_HASH)
	KE(KP_SPACE)
	KE(KP_AT)
	KE(KP_EXCLAM)
	KE(KP_MEMSTORE)
	KE(KP_MEMRECALL)
	KE(KP_MEMCLEAR)
	KE(KP_MEMADD)
	KE(KP_MEMSUBTRACT)
	KE(KP_MEMMULTIPLY)
	KE(KP_MEMDIVIDE)
	KE(KP_PLUSMINUS)
	KE(KP_CLEAR)
	KE(KP_CLEARENTRY)
	KE(KP_BINARY)
	KE(KP_OCTAL)
	KE(KP_DECIMAL)
	KE(KP_HEXADECIMAL)

	KE(LCTRL)
	KE(LSHIFT)
	KE(LALT)
	KE(LGUI)
	KE(RCTRL)
	KE(RSHIFT)
	KE(RALT)
	KE(RGUI)

	KE(MODE)
	KE(AUDIONEXT)
	KE(AUDIOPREV)
	KE(AUDIOSTOP)
	KE(AUDIOPLAY)
	KE(AUDIOMUTE)
	KE(MEDIASELECT)
	KE(WWW)
	KE(MAIL)
	KE(CALCULATOR)
	KE(COMPUTER)
	KE(AC_SEARCH)
	KE(AC_HOME)
	KE(AC_BACK)
	KE(AC_FORWARD)
	KE(AC_STOP)
	KE(AC_REFRESH)
	KE(AC_BOOKMARKS)

	KE(BRIGHTNESSDOWN)
	KE(BRIGHTNESSUP)
	KE(DISPLAYSWITCH)
	KE(KBDILLUMTOGGLE)
	KE(KBDILLUMDOWN)
	KE(KBDILLUMUP)
	KE(EJECT)
	KE(SLEEP)

	KE(APP1)
	KE(APP2)
};


//============================================================
//  lookup_sdl_code
//============================================================

int lookup_sdl_code(std::string_view scode)
{
	auto const found = std::find_if(
			std::begin(sdl_lookup_table),
			std::end(sdl_lookup_table),
			[&scode] (auto const &key) { return scode == key.name; });
	return (std::end(sdl_lookup_table) != found) ? found->code : -1;
}


//============================================================
//  sdl_device
//============================================================

using sdl_device = event_based_device<SDL_Event>;


//============================================================
//  sdl_keyboard_device
//============================================================

class sdl_keyboard_device : public sdl_device
{
public:
	sdl_keyboard_device(
			std::string &&name,
			std::string &&id,
			input_module &module,
			keyboard_trans_table const &trans_table) :
		sdl_device(std::move(name), std::move(id), module),
		m_trans_table(trans_table),
		m_keyboard({{0}}),
		m_capslock_pressed(std::chrono::steady_clock::time_point::min())
	{
	}

	virtual void poll(bool relative_reset) override
	{
		sdl_device::poll(relative_reset);

#ifdef __APPLE__
		if (m_keyboard.state[SDL_SCANCODE_CAPSLOCK] && (std::chrono::steady_clock::now() > (m_capslock_pressed + std::chrono::milliseconds(30))))
			m_keyboard.state[SDL_SCANCODE_CAPSLOCK] = 0x00;
#endif
	}

	virtual void process_event(SDL_Event const &event) override
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			if (event.key.keysym.scancode == SDL_SCANCODE_CAPSLOCK)
				m_capslock_pressed = std::chrono::steady_clock::now();

			m_keyboard.state[event.key.keysym.scancode] = 0x80;
			break;

		case SDL_KEYUP:
#ifdef __APPLE__
			if (event.key.keysym.scancode == SDL_SCANCODE_CAPSLOCK)
				break;
#endif

			m_keyboard.state[event.key.keysym.scancode] = 0x00;
			break;
		}
	}

	virtual void reset() override
	{
		sdl_device::reset();
		memset(&m_keyboard, 0, sizeof(m_keyboard));
		m_capslock_pressed = std::chrono::steady_clock::time_point::min();
	}

	virtual void configure(input_device &device) override
	{
		// populate it
		for (int keynum = 0; m_trans_table[keynum].mame_key != ITEM_ID_INVALID; keynum++)
		{
			input_item_id itemid = m_trans_table[keynum].mame_key;
			device.add_item(
					m_trans_table[keynum].ui_name,
					std::string_view(),
					itemid,
					generic_button_get_state<u8>,
					&m_keyboard.state[m_trans_table[keynum].sdl_scancode]);
		}
	}

private:
	// state information for a keyboard
	struct keyboard_state
	{
		u8  state[0x3ff];
	};

	keyboard_trans_table const &m_trans_table;
	keyboard_state m_keyboard;
	std::chrono::steady_clock::time_point m_capslock_pressed;
};


//============================================================
//  sdl_mouse_device_base
//============================================================

class sdl_mouse_device_base : public sdl_device
{
public:
	virtual void poll(bool relative_reset) override
	{
		sdl_device::poll(relative_reset);

		if (relative_reset)
		{
			m_mouse.lV = std::exchange(m_v, 0);
			m_mouse.lH = std::exchange(m_h, 0);
		}
	}

	virtual void reset() override
	{
		sdl_device::reset();
		memset(&m_mouse, 0, sizeof(m_mouse));
		m_v = m_h = 0;
	}

protected:
	static constexpr unsigned MAX_BUTTONS = INPUT_MAX_BUTTONS;

	// state information for a mouse
	struct mouse_state
	{
		s32 lX, lY, lV, lH;
		u8  buttons[MAX_BUTTONS];
	};

	sdl_mouse_device_base(std::string &&name, std::string &&id, input_module &module) :
		sdl_device(std::move(name), std::move(id), module),
		m_mouse({0}),
		m_v(0),
		m_h(0)
	{
	}

	void add_common_items(input_device &device, unsigned buttons)
	{
		// add horizontal and vertical axes - relative for a mouse or absolute for a gun
		device.add_item(
				"X",
				std::string_view(),
				ITEM_ID_XAXIS,
				generic_axis_get_state<s32>,
				&m_mouse.lX);
		device.add_item(
				"Y",
				std::string_view(),
				ITEM_ID_YAXIS,
				generic_axis_get_state<s32>,
				&m_mouse.lY);

		// add buttons
		for (int button = 0; button < buttons; button++)
		{
			input_item_id itemid = input_item_id(ITEM_ID_BUTTON1 + button);
			int const offset = button ^ (((1 == button) || (2 == button)) ? 3 : 0);
			device.add_item(
					default_button_name(button),
					std::string_view(),
					itemid,
					generic_button_get_state<u8>,
					&m_mouse.buttons[offset]);
		}
	}

	mouse_state m_mouse;
	s32 m_v, m_h;
};


//============================================================
//  sdl_mouse_device
//============================================================

class sdl_mouse_device : public sdl_mouse_device_base
{
public:
	sdl_mouse_device(std::string &&name, std::string &&id, input_module &module) :
		sdl_mouse_device_base(std::move(name), std::move(id), module),
		m_x(0),
		m_y(0)
	{
	}

	virtual void poll(bool relative_reset) override
	{
		sdl_mouse_device_base::poll(relative_reset);

		if (relative_reset)
		{
			m_mouse.lX = std::exchange(m_x, 0);
			m_mouse.lY = std::exchange(m_y, 0);
		}
	}

	virtual void reset() override
	{
		sdl_mouse_device_base::reset();
		m_x = m_y = 0;
	}

	virtual void configure(input_device &device) override
	{
		add_common_items(device, 5);

		// add scroll axes
		device.add_item(
				"Scroll V",
				std::string_view(),
				ITEM_ID_ZAXIS,
				generic_axis_get_state<s32>,
				&m_mouse.lV);
		device.add_item(
				"Scroll H",
				std::string_view(),
				ITEM_ID_RZAXIS,
				generic_axis_get_state<s32>,
				&m_mouse.lH);
	}

	virtual void process_event(SDL_Event const &event) override
	{
		switch (event.type)
		{
		case SDL_MOUSEMOTION:
			m_x += event.motion.xrel * input_device::RELATIVE_PER_PIXEL;
			m_y += event.motion.yrel * input_device::RELATIVE_PER_PIXEL;
			break;

		case SDL_MOUSEBUTTONDOWN:
			m_mouse.buttons[event.button.button - 1] = 0x80;
			break;

		case SDL_MOUSEBUTTONUP:
			m_mouse.buttons[event.button.button - 1] = 0;
			break;

		case SDL_MOUSEWHEEL:
			// adjust SDL 1-per-click to match Win32 120-per-click
#if SDL_VERSION_ATLEAST(2, 0, 18)
			m_v += std::lround(event.wheel.preciseY * 120 * input_device::RELATIVE_PER_PIXEL);
			m_h += std::lround(event.wheel.preciseX * 120 * input_device::RELATIVE_PER_PIXEL);
#else
			m_v += event.wheel.y * 120 * input_device::RELATIVE_PER_PIXEL;
			m_h += event.wheel.x * 120 * input_device::RELATIVE_PER_PIXEL;
#endif
			break;
		}
	}

private:
	s32 m_x, m_y;
};


//============================================================
//  sdl_lightgun_device
//============================================================

class sdl_lightgun_device : public sdl_mouse_device_base
{
public:
	sdl_lightgun_device(std::string &&name, std::string &&id, input_module &module) :
		sdl_mouse_device_base(std::move(name), std::move(id), module),
		m_x(0),
		m_y(0),
		m_window(0)
	{
	}

	virtual void poll(bool relative_reset) override
	{
		sdl_mouse_device_base::poll(relative_reset);

		SDL_Window *const win(m_window ? SDL_GetWindowFromID(m_window) : nullptr);
		if (win)
		{
			int w, h;
			SDL_GetWindowSize(win, &w, &h);
			m_mouse.lX = normalize_absolute_axis(m_x, 0, w - 1);
			m_mouse.lY = normalize_absolute_axis(m_y, 0, h - 1);
		}
		else
		{
			m_mouse.lX = 0;
			m_mouse.lY = 0;
		}
	}

	virtual void reset() override
	{
		sdl_mouse_device_base::reset();
		m_x = m_y = 0;
		m_window = 0;
	}

	virtual void configure(input_device &device) override
	{
		add_common_items(device, 5);

		// add scroll axes
		device.add_item(
				"Scroll V",
				std::string_view(),
				ITEM_ID_ADD_RELATIVE1,
				generic_axis_get_state<s32>,
				&m_mouse.lV);
		device.add_item(
				"Scroll H",
				std::string_view(),
				ITEM_ID_ADD_RELATIVE2,
				generic_axis_get_state<s32>,
				&m_mouse.lH);
	}

	virtual void process_event(SDL_Event const &event) override
	{
		switch (event.type)
		{
		case SDL_MOUSEMOTION:
			m_x = event.motion.x;
			m_y = event.motion.y;
			m_window = event.motion.windowID;
			break;

		case SDL_MOUSEBUTTONDOWN:
			m_mouse.buttons[event.button.button - 1] = 0x80;
			m_x = event.button.x;
			m_y = event.button.y;
			m_window = event.button.windowID;
			break;

		case SDL_MOUSEBUTTONUP:
			m_mouse.buttons[event.button.button - 1] = 0;
			m_x = event.button.x;
			m_y = event.button.y;
			m_window = event.button.windowID;
			break;

		case SDL_MOUSEWHEEL:
			// adjust SDL 1-per-click to match Win32 120-per-click
#if SDL_VERSION_ATLEAST(2, 0, 18)
			m_v += std::lround(event.wheel.preciseY * 120 * input_device::RELATIVE_PER_PIXEL);
			m_h += std::lround(event.wheel.preciseX * 120 * input_device::RELATIVE_PER_PIXEL);
#else
			m_v += event.wheel.y * 120 * input_device::RELATIVE_PER_PIXEL;
			m_h += event.wheel.x * 120 * input_device::RELATIVE_PER_PIXEL;
#endif
			break;

		case SDL_WINDOWEVENT:
			if ((event.window.windowID == m_window) && (SDL_WINDOWEVENT_LEAVE == event.window.event))
				m_window = 0;
			break;
		}
	}

private:
	s32 m_x, m_y;
	u32 m_window;
};


//============================================================
//  sdl_dual_lightgun_device
//============================================================

class sdl_dual_lightgun_device : public sdl_mouse_device_base
{
public:
	sdl_dual_lightgun_device(std::string &&name, std::string &&id, input_module &module, u8 index) :
		sdl_mouse_device_base(std::move(name), std::move(id), module),
		m_index(index)
	{
	}

	virtual void configure(input_device &device) override
	{
		add_common_items(device, 2);
	}

	virtual void process_event(SDL_Event const &event) override
	{
		switch (event.type)
		{
		case SDL_MOUSEBUTTONDOWN:
			{
				SDL_Window *const win(SDL_GetWindowFromID(event.button.windowID));
				u8 const button = translate_button(event);
				if (win && ((button / 2) == m_index))
				{
					int w, h;
					SDL_GetWindowSize(win, &w, &h);
					m_mouse.buttons[(button & 1) << 1] = 0x80;
					m_mouse.lX = normalize_absolute_axis(event.button.x, 0, w - 1);
					m_mouse.lY = normalize_absolute_axis(event.button.y, 0, h - 1);
				}
			}
			break;

		case SDL_MOUSEBUTTONUP:
			{
				u8 const button = translate_button(event);
				if ((button / 2) == m_index)
					m_mouse.buttons[(button & 1) << 1] = 0;
			}
			break;
		}
	}

private:
	static u8 translate_button(SDL_Event const &event)
	{
		u8 const index(event.button.button - 1);
		return index ^ (((1 == index) || (2 == index)) ? 3 : 0);
	}

	u8 const m_index;
};


//============================================================
//  sdl_joystick_device_base
//============================================================

class sdl_joystick_device_base : public sdl_device
{
public:
	std::optional<std::string> const &serial() const { return m_serial; }
	SDL_JoystickID instance() const { return m_instance; }

	bool is_instance(SDL_JoystickID instance) const { return m_instance == instance; }

	bool reconnect_match(std::string_view g, char const *s) const
	{
		return
				(0 > m_instance) &&
				(id() == g) &&
				((s && serial() && (*serial() == s)) || (!s && !serial()));
	}

protected:
	sdl_joystick_device_base(
			std::string &&name,
			std::string &&id,
			input_module &module,
			char const *serial) :
		sdl_device(std::move(name), std::move(id), module),
		m_instance(-1)
	{
		if (serial)
			m_serial = serial;
	}

	void set_instance(SDL_JoystickID instance)
	{
		assert(0 > m_instance);
		assert(0 <= instance);

		m_instance = instance;
	}

	void clear_instance()
	{
		m_instance = -1;
	}

private:
	std::optional<std::string> m_serial;
	SDL_JoystickID m_instance;
};


//============================================================
//  sdl_joystick_device
//============================================================

class sdl_joystick_device : public sdl_joystick_device_base, public sdl_joystick_device_common
{
public:
	sdl_joystick_device(
			std::string &&name,
			std::string &&id,
			input_module &module,
			SDL_Joystick *joy,
			char const *serial) :
		sdl_joystick_device_base(
				std::move(name),
				std::move(id),
				module,
				serial),
		m_joydevice(joy),
		m_hapdevice(SDL_HapticOpenFromJoystick(joy))
	{
		set_instance(SDL_JoystickInstanceID(joy));
	}

	virtual void configure(input_device &device) override
	{
		configure_common(
				device,
				SDL_JoystickNumAxes(m_joydevice),
				SDL_JoystickNumButtons(m_joydevice),
				SDL_JoystickNumHats(m_joydevice),
				SDL_JoystickNumBalls(m_joydevice));
	}

	~sdl_joystick_device()
	{
		close_device();
	}

	virtual void poll(bool relative_reset) override
	{
		sdl_joystick_device_base::poll(relative_reset);

		if (relative_reset)
		{
			for (unsigned i = 0; MAX_AXES > i; ++i)
				m_joystick.balls[i] = std::exchange(m_ball[i], 0);
		}
	}

	virtual void reset() override
	{
		sdl_joystick_device_base::reset();
		clear_buffer();
	}

	virtual void process_event(SDL_Event const &event) override
	{
		if (!m_joydevice)
			return;

		switch (event.type)
		{
		case SDL_JOYAXISMOTION:
			if (event.jaxis.axis < MAX_AXES)
				m_joystick.axes[event.jaxis.axis] = (event.jaxis.value * 2);
			break;

		case SDL_JOYBALLMOTION:
			//printf("Ball %d %d\n", event.jball.xrel, event.jball.yrel);
			if (event.jball.ball < (MAX_AXES / 2))
			{
				m_ball[event.jball.ball * 2] += event.jball.xrel * input_device::RELATIVE_PER_PIXEL;
				m_ball[event.jball.ball * 2 + 1] += event.jball.yrel * input_device::RELATIVE_PER_PIXEL;
			}
			break;

		case SDL_JOYHATMOTION:
			if (event.jhat.hat < MAX_HATS)
			{
				m_joystick.hatsU[event.jhat.hat] = (event.jhat.value & SDL_HAT_UP) ? 0x80 : 0;
				m_joystick.hatsD[event.jhat.hat] = (event.jhat.value & SDL_HAT_DOWN) ? 0x80 : 0;
				m_joystick.hatsL[event.jhat.hat] = (event.jhat.value & SDL_HAT_LEFT) ? 0x80 : 0;
				m_joystick.hatsR[event.jhat.hat] = (event.jhat.value & SDL_HAT_RIGHT) ? 0x80 : 0;
			}
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			if (event.jbutton.button < MAX_BUTTONS)
				m_joystick.buttons[event.jbutton.button] = (event.jbutton.state == SDL_PRESSED) ? 0x80 : 0;
			break;

		case SDL_JOYDEVICEREMOVED:
			osd_printf_verbose("Joystick: %s [ID %s] disconnected\n", name(), id());
			clear_instance();
			clear_buffer();
			close_device();
			break;
		}
	}

	bool has_haptic() const
	{
		return m_hapdevice != nullptr;
	}

	void attach_device(SDL_Joystick *joy)
	{
		assert(joy);
		assert(!m_joydevice);

		set_instance(SDL_JoystickInstanceID(joy));
		m_joydevice = joy;
		m_hapdevice = SDL_HapticOpenFromJoystick(joy);

		osd_printf_verbose("Joystick: %s [ID %s] reconnected\n", name(), id());
	}

private:
	SDL_Joystick *m_joydevice;
	SDL_Haptic *m_hapdevice;

	void close_device()
	{
		if (m_joydevice)
		{
			if (m_hapdevice)
			{
				SDL_HapticClose(m_hapdevice);
				m_hapdevice = nullptr;
			}
			SDL_JoystickClose(m_joydevice);
			m_joydevice = nullptr;
		}
	}
};


//============================================================
//  sdl_sixaxis_joystick_device
//============================================================

class sdl_sixaxis_joystick_device : public sdl_joystick_device
{
public:
	using sdl_joystick_device::sdl_joystick_device;

	virtual void process_event(SDL_Event const &event) override
	{
		switch (event.type)
		{
		case SDL_JOYAXISMOTION:
			{
				int const axis = event.jaxis.axis;
				if (axis <= 3)
				{
					m_joystick.axes[event.jaxis.axis] = (event.jaxis.value * 2);
				}
				else
				{
					int const magic = (event.jaxis.value / 2) + 16384;
					m_joystick.axes[event.jaxis.axis] = magic;
				}
			}
			break;

		default:
			// Call the base for other events
			sdl_joystick_device::process_event(event);
			break;
		}
	}
};


//============================================================
//  sdl_game_controller_device
//============================================================

class sdl_game_controller_device : public sdl_joystick_device_base, protected joystick_assignment_helper
{
public:
	sdl_game_controller_device(
			std::string &&name,
			std::string &&id,
			input_module &module,
			SDL_GameController *ctrl,
			char const *serial) :
		sdl_joystick_device_base(
				std::move(name),
				std::move(id),
				module,
				serial),
		m_controller({{0}}),
		m_ctrldevice(ctrl)
	{
		set_instance(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(ctrl)));
	}

	~sdl_game_controller_device()
	{
		close_device();
	}

	virtual void configure(input_device &device) override
	{
		input_device::assignment_vector assignments;
#if defined(__SWITCH__)
		char const *const *axisnames = CONTROLLER_AXIS_SWITCH;
		char const *const *buttonnames = CONTROLLER_BUTTON_SWITCH;
		bool digitaltriggers = true;
#else
		char const *const *axisnames = CONTROLLER_AXIS_XBOX;
		char const *const *buttonnames = CONTROLLER_BUTTON_XBOX360;
		bool digitaltriggers = false;
#endif
		bool avoidpaddles = false;
		auto const ctrltype = SDL_GameControllerGetType(m_ctrldevice);
		switch (ctrltype)
		{
		case SDL_CONTROLLER_TYPE_UNKNOWN:
			osd_printf_verbose("Game Controller:   ...  unknown type\n");
#if defined(__SWITCH__)
			axisnames = CONTROLLER_AXIS_SWITCH;
			buttonnames = CONTROLLER_BUTTON_SWITCH;
			digitaltriggers = true;
#endif
			break;
		case SDL_CONTROLLER_TYPE_XBOX360:
			osd_printf_verbose("Game Controller:   ...  Xbox 360 type\n");
			axisnames = CONTROLLER_AXIS_XBOX;
			buttonnames = CONTROLLER_BUTTON_XBOX360;
			break;
		case SDL_CONTROLLER_TYPE_XBOXONE:
			osd_printf_verbose("Game Controller:   ...  Xbox One type\n");
			axisnames = CONTROLLER_AXIS_XBOX;
			buttonnames = CONTROLLER_BUTTON_XBOXONE;
			break;
		case SDL_CONTROLLER_TYPE_PS3:
			osd_printf_verbose("Game Controller:   ...  PlayStation 3 type\n");
			axisnames = CONTROLLER_AXIS_PS;
			buttonnames = CONTROLLER_BUTTON_PS3;
			break;
		case SDL_CONTROLLER_TYPE_PS4:
			osd_printf_verbose("Game Controller:   ...  PlayStation 4 type\n");
			axisnames = CONTROLLER_AXIS_PS;
			buttonnames = CONTROLLER_BUTTON_PS4;
			break;
		case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
			osd_printf_verbose("Game Controller:   ...  Switch Pro Controller type\n");
			axisnames = CONTROLLER_AXIS_SWITCH;
			buttonnames = CONTROLLER_BUTTON_SWITCH;
			digitaltriggers = true;
			break;
		//case SDL_CONTROLLER_TYPE_VIRTUAL:
		case SDL_CONTROLLER_TYPE_PS5:
			osd_printf_verbose("Game Controller:   ...  PlayStation 5 type\n");
			axisnames = CONTROLLER_AXIS_PS;
			buttonnames = CONTROLLER_BUTTON_PS5;
			break;
#if SDL_VERSION_ATLEAST(2, 0, 16)
		//case SDL_CONTROLLER_TYPE_AMAZON_LUNA:
		case SDL_CONTROLLER_TYPE_GOOGLE_STADIA:
			osd_printf_verbose("Game Controller:   ...  Google Stadia type\n");
			axisnames = CONTROLLER_AXIS_PS;
			buttonnames = CONTROLLER_BUTTON_STADIA;
			break;
#endif
#if SDL_VERSION_ATLEAST(2, 24, 0)
		case SDL_CONTROLLER_TYPE_NVIDIA_SHIELD:
			osd_printf_verbose("Game Controller:   ...  NVIDIA Shield type\n");
			axisnames = CONTROLLER_AXIS_XBOX;
			buttonnames = CONTROLLER_BUTTON_SHIELD;
			break;
		//case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
		//case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
		case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
			osd_printf_verbose("Game Controller:   ...  Joy-Con pair type\n");
			axisnames = CONTROLLER_AXIS_SWITCH;
			buttonnames = CONTROLLER_BUTTON_SWITCH;
			digitaltriggers = true;
			avoidpaddles = true;
			break;
#endif
		default: // default to Xbox 360 names
			osd_printf_verbose("Game Controller:   ...  unrecognized type (%d)\n", int(ctrltype));
			break;
		}

		// keep track of item numbers as we add controls
		std::pair<input_item_id, input_item_id> axisitems[SDL_CONTROLLER_AXIS_MAX];
		input_item_id buttonitems[SDL_CONTROLLER_BUTTON_MAX];
		std::tuple<input_item_id, SDL_GameControllerButton, SDL_GameControllerAxis> numberedbuttons[16];
		std::fill(
				std::begin(axisitems),
				std::end(axisitems),
				std::make_pair(ITEM_ID_INVALID, ITEM_ID_INVALID));
		std::fill(
				std::begin(buttonitems),
				std::end(buttonitems),
				ITEM_ID_INVALID);
		std::fill(
				std::begin(numberedbuttons),
				std::end(numberedbuttons),
				std::make_tuple(ITEM_ID_INVALID, SDL_CONTROLLER_BUTTON_INVALID, SDL_CONTROLLER_AXIS_INVALID));

		// add axes
		std::tuple<SDL_GameControllerAxis, input_item_id, bool> const axes[]{
				{ SDL_CONTROLLER_AXIS_LEFTX,        ITEM_ID_XAXIS,   false },
				{ SDL_CONTROLLER_AXIS_LEFTY,        ITEM_ID_YAXIS,   false },
				{ SDL_CONTROLLER_AXIS_RIGHTX,       ITEM_ID_ZAXIS,   false },
				{ SDL_CONTROLLER_AXIS_RIGHTY,       ITEM_ID_RZAXIS,  false },
				{ SDL_CONTROLLER_AXIS_TRIGGERLEFT,  ITEM_ID_SLIDER1, true },
				{ SDL_CONTROLLER_AXIS_TRIGGERRIGHT, ITEM_ID_SLIDER2, true } };
		for (auto [axis, item, buttontest] : axes)
		{
			bool avail = !buttontest || !digitaltriggers;
			avail = avail && SDL_GameControllerHasAxis(m_ctrldevice, axis);
			if (avail)
			{
				auto const binding = SDL_GameControllerGetBindForAxis(m_ctrldevice, axis);
				switch (binding.bindType)
				{
				case SDL_CONTROLLER_BINDTYPE_NONE:
					avail = false;
					break;
				case SDL_CONTROLLER_BINDTYPE_BUTTON:
					if (buttontest)
						avail = false;
					break;
				default:
					break;
				}
			}
			if (avail)
			{
				axisitems[axis].first = device.add_item(
						axisnames[axis],
						std::string_view(),
						item,
						generic_axis_get_state<s32>,
						&m_controller.axes[axis]);
			}
		}

		// add automatically numbered buttons
		std::tuple<SDL_GameControllerButton, SDL_GameControllerAxis, bool> const generalbuttons[]{
				{ SDL_CONTROLLER_BUTTON_A,             SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_B,             SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_X,             SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_Y,             SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_INVALID,       SDL_CONTROLLER_AXIS_TRIGGERLEFT,  true },
				{ SDL_CONTROLLER_BUTTON_INVALID,       SDL_CONTROLLER_AXIS_TRIGGERRIGHT, true },
				{ SDL_CONTROLLER_BUTTON_LEFTSTICK,     SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_RIGHTSTICK,    SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_PADDLE1,       SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_PADDLE2,       SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_PADDLE3,       SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_PADDLE4,       SDL_CONTROLLER_AXIS_INVALID,      true },
				{ SDL_CONTROLLER_BUTTON_GUIDE,         SDL_CONTROLLER_AXIS_INVALID,      false },
				{ SDL_CONTROLLER_BUTTON_MISC1,         SDL_CONTROLLER_AXIS_INVALID,      false },
				{ SDL_CONTROLLER_BUTTON_TOUCHPAD,      SDL_CONTROLLER_AXIS_INVALID,      false },
				};
		input_item_id button_item = ITEM_ID_BUTTON1;
		unsigned buttoncount = 0;
		for (auto [button, axis, field] : generalbuttons)
		{
			bool avail = true;
			input_item_id actual = ITEM_ID_INVALID;
			if (SDL_CONTROLLER_BUTTON_INVALID != button)
			{
				avail = SDL_GameControllerHasButton(m_ctrldevice, button);
				if (avail)
				{
					auto const binding = SDL_GameControllerGetBindForButton(m_ctrldevice, button);
					switch (binding.bindType)
					{
					case SDL_CONTROLLER_BINDTYPE_NONE:
						avail = false;
						break;
					default:
						break;
					}
				}
				if (avail)
				{
					actual = buttonitems[button] = device.add_item(
							buttonnames[button],
							std::string_view(),
							button_item++,
							generic_button_get_state<u8>,
							&m_controller.buttons[button]);
					if (field && (std::size(numberedbuttons) > buttoncount))
						std::get<1>(numberedbuttons[buttoncount]) = button;
				}
			}
			else
			{
				avail = SDL_GameControllerHasAxis(m_ctrldevice, axis);
				if (avail)
				{
					auto const binding = SDL_GameControllerGetBindForAxis(m_ctrldevice, axis);
					switch (binding.bindType)
					{
					case SDL_CONTROLLER_BINDTYPE_NONE:
						avail = false;
						break;
					case SDL_CONTROLLER_BINDTYPE_BUTTON:
						break;
					default:
						avail = digitaltriggers;
					}
				}
				if (avail)
				{
					actual = axisitems[axis].second = device.add_item(
							axisnames[axis],
							std::string_view(),
							button_item++,
							[] (void *device_internal, void *item_internal) -> int
							{
								return (*reinterpret_cast<s32 const *>(item_internal) <= -16'384) ? 1 : 0;
							},
							&m_controller.axes[axis]);
					if (field && (std::size(numberedbuttons) > buttoncount))
						std::get<2>(numberedbuttons[buttoncount]) = axis;
				}
			}

			// add default button assignments
			if (field && avail && (std::size(numberedbuttons) > buttoncount))
			{
				std::get<0>(numberedbuttons[buttoncount]) = actual;
				add_button_assignment(assignments, ioport_type(IPT_BUTTON1 + buttoncount++), { actual });
			}
		}

		// add buttons with fixed item IDs
		std::pair<SDL_GameControllerButton, input_item_id> const fixedbuttons[]{
				{ SDL_CONTROLLER_BUTTON_BACK,       ITEM_ID_SELECT },
				{ SDL_CONTROLLER_BUTTON_START,      ITEM_ID_START },
				{ SDL_CONTROLLER_BUTTON_DPAD_UP,    ITEM_ID_HAT1UP },
				{ SDL_CONTROLLER_BUTTON_DPAD_DOWN,  ITEM_ID_HAT1DOWN },
				{ SDL_CONTROLLER_BUTTON_DPAD_LEFT,  ITEM_ID_HAT1LEFT },
				{ SDL_CONTROLLER_BUTTON_DPAD_RIGHT, ITEM_ID_HAT1RIGHT } };
		for (auto [button, item] : fixedbuttons)
		{
			bool avail = true;
			avail = SDL_GameControllerHasButton(m_ctrldevice, button);
			if (avail)
			{
				auto const binding = SDL_GameControllerGetBindForButton(m_ctrldevice, button);
				switch (binding.bindType)
				{
				case SDL_CONTROLLER_BINDTYPE_NONE:
					avail = false;
					break;
				default:
					break;
				}
			}
			if (avail)
			{
				buttonitems[button] = device.add_item(
						buttonnames[button],
						std::string_view(),
						item,
						generic_button_get_state<u8>,
						&m_controller.buttons[button]);
			}
		}

		// try to get a "complete" joystick for primary movement controls
		input_item_id diraxis[2][2];
		choose_primary_stick(
				diraxis,
				axisitems[SDL_CONTROLLER_AXIS_LEFTX].first,
				axisitems[SDL_CONTROLLER_AXIS_LEFTY].first,
				axisitems[SDL_CONTROLLER_AXIS_RIGHTX].first,
				axisitems[SDL_CONTROLLER_AXIS_RIGHTY].first);

		// now set up controls using the primary joystick
		add_directional_assignments(
				assignments,
				diraxis[0][0],
				diraxis[0][1],
				buttonitems[SDL_CONTROLLER_BUTTON_DPAD_LEFT],
				buttonitems[SDL_CONTROLLER_BUTTON_DPAD_RIGHT],
				buttonitems[SDL_CONTROLLER_BUTTON_DPAD_UP],
				buttonitems[SDL_CONTROLLER_BUTTON_DPAD_DOWN]);

		// assign a secondary stick axis to joystick Z if available
		bool const zaxis = add_assignment(
				assignments,
				IPT_AD_STICK_Z,
				SEQ_TYPE_STANDARD,
				ITEM_CLASS_ABSOLUTE,
				ITEM_MODIFIER_NONE,
				{ diraxis[1][1], diraxis[1][0] });
		if (!zaxis)
		{
			// if both triggers are present, combine them, or failing that, fall back to a pair of buttons
			if ((ITEM_ID_INVALID != axisitems[SDL_CONTROLLER_AXIS_TRIGGERLEFT].first) && (ITEM_ID_INVALID != axisitems[SDL_CONTROLLER_AXIS_TRIGGERRIGHT].first))
			{
				assignments.emplace_back(
						IPT_AD_STICK_Z,
						SEQ_TYPE_STANDARD,
						input_seq(
								make_code(ITEM_CLASS_ABSOLUTE, ITEM_MODIFIER_NONE, axisitems[SDL_CONTROLLER_AXIS_TRIGGERLEFT].first),
								make_code(ITEM_CLASS_ABSOLUTE, ITEM_MODIFIER_REVERSE, axisitems[SDL_CONTROLLER_AXIS_TRIGGERRIGHT].first)));
			}
			else if (add_axis_inc_dec_assignment(assignments, IPT_AD_STICK_Z, buttonitems[SDL_CONTROLLER_BUTTON_LEFTSHOULDER], buttonitems[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]))
			{
				// took shoulder buttons
			}
			else if (add_axis_inc_dec_assignment(assignments, IPT_AD_STICK_Z, axisitems[SDL_CONTROLLER_AXIS_TRIGGERLEFT].second, axisitems[SDL_CONTROLLER_AXIS_TRIGGERRIGHT].second))
			{
				// took trigger buttons
			}
			else if (add_axis_inc_dec_assignment(assignments, IPT_AD_STICK_Z, buttonitems[SDL_CONTROLLER_BUTTON_PADDLE1], buttonitems[SDL_CONTROLLER_BUTTON_PADDLE2]))
			{
				// took P1/P2
			}
			else if (add_axis_inc_dec_assignment(assignments, IPT_AD_STICK_Z, buttonitems[SDL_CONTROLLER_BUTTON_PADDLE3], buttonitems[SDL_CONTROLLER_BUTTON_PADDLE4]))
			{
				// took P3/P4
			}
		}

		// prefer trigger axes for pedals, otherwise take half axes and buttons
		unsigned pedalbutton = 0;
		if (!add_assignment(assignments, IPT_PEDAL, SEQ_TYPE_STANDARD, ITEM_CLASS_ABSOLUTE, ITEM_MODIFIER_NEG, { axisitems[SDL_CONTROLLER_AXIS_TRIGGERRIGHT].first }))
		{
			add_assignment(
					assignments,
					IPT_PEDAL,
					SEQ_TYPE_STANDARD,
					ITEM_CLASS_ABSOLUTE,
					ITEM_MODIFIER_NEG,
					{ diraxis[1][1], diraxis[0][1] });
			bool const incbutton = add_assignment(
					assignments,
					IPT_PEDAL,
					SEQ_TYPE_INCREMENT,
					ITEM_CLASS_SWITCH,
					ITEM_MODIFIER_NONE,
					{ axisitems[SDL_CONTROLLER_AXIS_TRIGGERRIGHT].second, buttonitems[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] });
			if (!incbutton)
			{
				if (add_assignment(assignments, IPT_PEDAL, SEQ_TYPE_INCREMENT, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, { std::get<0>(numberedbuttons[pedalbutton]) }))
					++pedalbutton;
			}
		}
		if (!add_assignment(assignments, IPT_PEDAL2, SEQ_TYPE_STANDARD, ITEM_CLASS_ABSOLUTE, ITEM_MODIFIER_NEG, { axisitems[SDL_CONTROLLER_AXIS_TRIGGERLEFT].first }))
		{
			add_assignment(
					assignments,
					IPT_PEDAL2,
					SEQ_TYPE_STANDARD,
					ITEM_CLASS_ABSOLUTE,
					ITEM_MODIFIER_POS,
					{ diraxis[1][1], diraxis[0][1] });
			bool const incbutton = add_assignment(
					assignments,
					IPT_PEDAL2,
					SEQ_TYPE_INCREMENT,
					ITEM_CLASS_SWITCH,
					ITEM_MODIFIER_NONE,
					{ axisitems[SDL_CONTROLLER_AXIS_TRIGGERLEFT].second, buttonitems[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] });
			if (!incbutton)
			{
				if (add_assignment(assignments, IPT_PEDAL2, SEQ_TYPE_INCREMENT, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, { std::get<0>(numberedbuttons[pedalbutton]) }))
					++pedalbutton;
			}
		}
		add_assignment(assignments, IPT_PEDAL3, SEQ_TYPE_INCREMENT, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, { std::get<0>(numberedbuttons[pedalbutton]) });

		// potentially use thumb sticks and/or D-pad and A/B/X/Y diamond for twin sticks
		add_twin_stick_assignments(
				assignments,
				axisitems[SDL_CONTROLLER_AXIS_LEFTX].first,
				axisitems[SDL_CONTROLLER_AXIS_LEFTY].first,
				axisitems[SDL_CONTROLLER_AXIS_RIGHTX].first,
				axisitems[SDL_CONTROLLER_AXIS_RIGHTY].first,
				buttonitems[SDL_CONTROLLER_BUTTON_DPAD_LEFT],
				buttonitems[SDL_CONTROLLER_BUTTON_DPAD_RIGHT],
				buttonitems[SDL_CONTROLLER_BUTTON_DPAD_UP],
				buttonitems[SDL_CONTROLLER_BUTTON_DPAD_DOWN],
				buttonitems[SDL_CONTROLLER_BUTTON_X],
				buttonitems[SDL_CONTROLLER_BUTTON_B],
				buttonitems[SDL_CONTROLLER_BUTTON_Y],
				buttonitems[SDL_CONTROLLER_BUTTON_A]);

		// add assignments for buttons with fixed functions
		add_button_assignment(assignments, IPT_COIN1,   { buttonitems[SDL_CONTROLLER_BUTTON_BACK] });
		add_button_assignment(assignments, IPT_SELECT,  { buttonitems[SDL_CONTROLLER_BUTTON_BACK] });
		add_button_assignment(assignments, IPT_START1,  { buttonitems[SDL_CONTROLLER_BUTTON_START] });
		add_button_assignment(assignments, IPT_START,   { buttonitems[SDL_CONTROLLER_BUTTON_START] });
		add_button_assignment(assignments, IPT_UI_MENU, { buttonitems[SDL_CONTROLLER_BUTTON_GUIDE], buttonitems[SDL_CONTROLLER_BUTTON_RIGHTSTICK] });

		// the first button is always UI select
		if (add_button_assignment(assignments, IPT_UI_SELECT, { std::get<0>(numberedbuttons[0]) }))
		{
			if (SDL_CONTROLLER_BUTTON_INVALID != std::get<1>(numberedbuttons[0]))
				buttonitems[std::get<1>(numberedbuttons[0])] = ITEM_ID_INVALID;
			if (SDL_CONTROLLER_AXIS_INVALID != std::get<2>(numberedbuttons[0]))
				axisitems[std::get<2>(numberedbuttons[0])].second = ITEM_ID_INVALID;
		}

		// try to get a matching pair of buttons for previous/next group
		if (consume_button_pair(assignments, IPT_UI_PREV_GROUP, IPT_UI_NEXT_GROUP, axisitems[SDL_CONTROLLER_AXIS_TRIGGERLEFT].second, axisitems[SDL_CONTROLLER_AXIS_TRIGGERRIGHT].second))
		{
			// took digital triggers
		}
		else if (!avoidpaddles && consume_button_pair(assignments, IPT_UI_PREV_GROUP, IPT_UI_NEXT_GROUP, buttonitems[SDL_CONTROLLER_BUTTON_PADDLE1], buttonitems[SDL_CONTROLLER_BUTTON_PADDLE2]))
		{
			// took upper paddles
		}
		else if (consume_trigger_pair(assignments, IPT_UI_PREV_GROUP, IPT_UI_NEXT_GROUP, axisitems[SDL_CONTROLLER_AXIS_TRIGGERLEFT].first, axisitems[SDL_CONTROLLER_AXIS_TRIGGERRIGHT].first))
		{
			// took analog triggers
		}
		else if (!avoidpaddles && consume_button_pair(assignments, IPT_UI_PREV_GROUP, IPT_UI_NEXT_GROUP, buttonitems[SDL_CONTROLLER_BUTTON_PADDLE3], buttonitems[SDL_CONTROLLER_BUTTON_PADDLE4]))
		{
			// took lower paddles
		}
		else if (consume_axis_pair(assignments, IPT_UI_PREV_GROUP, IPT_UI_NEXT_GROUP, diraxis[1][1]))
		{
			// took secondary Y
		}
		else if (consume_axis_pair(assignments, IPT_UI_PREV_GROUP, IPT_UI_NEXT_GROUP, diraxis[1][0]))
		{
			// took secondary X
		}

		// try to get a matching pair of buttons for page up/down
		if (!avoidpaddles && consume_button_pair(assignments, IPT_UI_PAGE_UP, IPT_UI_PAGE_DOWN, buttonitems[SDL_CONTROLLER_BUTTON_PADDLE1], buttonitems[SDL_CONTROLLER_BUTTON_PADDLE2]))
		{
			// took upper paddles
		}
		else if (!avoidpaddles && consume_button_pair(assignments, IPT_UI_PAGE_UP, IPT_UI_PAGE_DOWN, buttonitems[SDL_CONTROLLER_BUTTON_PADDLE3], buttonitems[SDL_CONTROLLER_BUTTON_PADDLE4]))
		{
			// took lower paddles
		}
		else if (consume_trigger_pair(assignments, IPT_UI_PAGE_UP, IPT_UI_PAGE_DOWN, axisitems[SDL_CONTROLLER_AXIS_TRIGGERLEFT].first, axisitems[SDL_CONTROLLER_AXIS_TRIGGERRIGHT].first))
		{
			// took analog triggers
		}
		else if (consume_axis_pair(assignments, IPT_UI_PAGE_UP, IPT_UI_PAGE_DOWN, diraxis[1][1]))
		{
			// took secondary Y
		}

		// try to assign X button to UI clear
		if (add_button_assignment(assignments, IPT_UI_CLEAR, { buttonitems[SDL_CONTROLLER_BUTTON_X] }))
		{
			buttonitems[SDL_CONTROLLER_BUTTON_X] = ITEM_ID_INVALID;
		}
		else
		{
			// otherwise try to find an unassigned button
			for (auto [item, button, axis] : numberedbuttons)
			{
				if ((SDL_CONTROLLER_BUTTON_INVALID != button) && (ITEM_ID_INVALID != buttonitems[button]))
				{
					add_button_assignment(assignments, IPT_UI_CLEAR, { item });
					buttonitems[button] = ITEM_ID_INVALID;
					break;
				}
				else if ((SDL_CONTROLLER_AXIS_INVALID != axis) && (ITEM_ID_INVALID != axisitems[axis].second))
				{
					add_button_assignment(assignments, IPT_UI_CLEAR, { item });
					axisitems[axis].second = ITEM_ID_INVALID;
					break;
				}
			}
		}

		// try to assign B button to UI back and cancel
		add_button_assignment(assignments, IPT_UI_CANCEL, { buttonitems[SDL_CONTROLLER_BUTTON_B] });
		if (add_button_assignment(assignments, IPT_UI_BACK, { buttonitems[SDL_CONTROLLER_BUTTON_B] }))
		{
			buttonitems[SDL_CONTROLLER_BUTTON_B] = ITEM_ID_INVALID;
		}
		else
		{
			// otherwise try to find an unassigned button
			for (auto [item, button, axis] : numberedbuttons)
			{
				if ((SDL_CONTROLLER_BUTTON_INVALID != button) && (ITEM_ID_INVALID != buttonitems[button]))
				{
					add_button_assignment(assignments, IPT_UI_CLEAR, { item });
					buttonitems[button] = ITEM_ID_INVALID;
					break;
				}
				else if ((SDL_CONTROLLER_AXIS_INVALID != axis) && (ITEM_ID_INVALID != axisitems[axis].second))
				{
					add_button_assignment(assignments, IPT_UI_CLEAR, { item });
					axisitems[axis].second = ITEM_ID_INVALID;
					break;
				}
			}
		}

		// try to assign Y button to UI help
		if (add_button_assignment(assignments, IPT_UI_HELP, { buttonitems[SDL_CONTROLLER_BUTTON_Y] }))
		{
			buttonitems[SDL_CONTROLLER_BUTTON_Y] = ITEM_ID_INVALID;
		}
		else
		{
			// otherwise try to find an unassigned button
			for (auto [item, button, axis] : numberedbuttons)
			{
				if ((SDL_CONTROLLER_BUTTON_INVALID != button) && (ITEM_ID_INVALID != buttonitems[button]))
				{
					add_button_assignment(assignments, IPT_UI_HELP, { item });
					buttonitems[button] = ITEM_ID_INVALID;
					break;
				}
				else if ((SDL_CONTROLLER_AXIS_INVALID != axis) && (ITEM_ID_INVALID != axisitems[axis].second))
				{
					add_button_assignment(assignments, IPT_UI_HELP, { item });
					axisitems[axis].second = ITEM_ID_INVALID;
					break;
				}
			}
		}

		// put focus previous/next on the shoulder buttons if available - this can be overloaded with zoom
		if (add_button_pair_assignment(assignments, IPT_UI_FOCUS_PREV, IPT_UI_FOCUS_NEXT, buttonitems[SDL_CONTROLLER_BUTTON_LEFTSHOULDER], buttonitems[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]))
		{
			// took shoulder buttons
		}
		else if (add_axis_pair_assignment(assignments, IPT_UI_FOCUS_PREV, IPT_UI_FOCUS_NEXT, diraxis[1][0]))
		{
			// took secondary X
		}
		else if (add_axis_pair_assignment(assignments, IPT_UI_FOCUS_PREV, IPT_UI_FOCUS_NEXT, diraxis[1][1]))
		{
			// took secondary Y
		}

		// put zoom on the secondary stick if available, or fall back to shoulder buttons
		if (add_axis_pair_assignment(assignments, IPT_UI_ZOOM_OUT, IPT_UI_ZOOM_IN, diraxis[1][0]))
		{
			// took secondary X
			if (axisitems[SDL_CONTROLLER_AXIS_LEFTX].first == diraxis[1][0])
				add_button_assignment(assignments, IPT_UI_ZOOM_DEFAULT, { buttonitems[SDL_CONTROLLER_BUTTON_LEFTSTICK] });
			else if (axisitems[SDL_CONTROLLER_AXIS_RIGHTX].first == diraxis[1][0])
				add_button_assignment(assignments, IPT_UI_ZOOM_DEFAULT, { buttonitems[SDL_CONTROLLER_BUTTON_RIGHTSTICK] });
			diraxis[1][0] = ITEM_ID_INVALID;
		}
		else if (add_axis_pair_assignment(assignments, IPT_UI_ZOOM_IN, IPT_UI_ZOOM_OUT, diraxis[1][1]))
		{
			// took secondary Y
			if (axisitems[SDL_CONTROLLER_AXIS_LEFTY].first == diraxis[1][1])
				add_button_assignment(assignments, IPT_UI_ZOOM_DEFAULT, { buttonitems[SDL_CONTROLLER_BUTTON_LEFTSTICK] });
			else if (axisitems[SDL_CONTROLLER_AXIS_RIGHTY].first == diraxis[1][1])
				add_button_assignment(assignments, IPT_UI_ZOOM_DEFAULT, { buttonitems[SDL_CONTROLLER_BUTTON_RIGHTSTICK] });
			diraxis[1][1] = ITEM_ID_INVALID;
		}
		else if (consume_button_pair(assignments, IPT_UI_ZOOM_OUT, IPT_UI_ZOOM_IN, buttonitems[SDL_CONTROLLER_BUTTON_LEFTSHOULDER], buttonitems[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]))
		{
			// took shoulder buttons
		}

		// set default assignments
		device.set_default_assignments(std::move(assignments));
	}

	virtual void reset() override
	{
		sdl_joystick_device_base::reset();
		clear_buffer();
	}

	virtual void process_event(SDL_Event const &event) override
	{
		if (!m_ctrldevice)
			return;

		switch (event.type)
		{
		case SDL_CONTROLLERAXISMOTION:
			if (event.caxis.axis < SDL_CONTROLLER_AXIS_MAX)
			{
				switch (event.caxis.axis)
				{
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT: // MAME wants negative values for triggers
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
					m_controller.axes[event.caxis.axis] = -normalize_absolute_axis(event.caxis.value, -32'767, 32'767);
					break;
				default:
					m_controller.axes[event.caxis.axis] = normalize_absolute_axis(event.caxis.value, -32'767, 32'767);
				}
			}
			break;

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			if (event.cbutton.button < SDL_CONTROLLER_BUTTON_MAX)
				m_controller.buttons[event.cbutton.button] = (event.cbutton.state == SDL_PRESSED) ? 0x80 : 0x00;
			break;

		case SDL_CONTROLLERDEVICEREMOVED:
			osd_printf_verbose("Game Controller: %s [ID %s] disconnected\n", name(), id());
			clear_instance();
			clear_buffer();
			close_device();
			break;
		}
	}

	void attach_device(SDL_GameController *ctrl)
	{
		assert(ctrl);
		assert(!m_ctrldevice);

		set_instance(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(ctrl)));
		m_ctrldevice = ctrl;

		osd_printf_verbose("Game Controller: %s [ID %s] reconnected\n", name(), id());
	}

private:
	// state information for a game controller
	struct sdl_controller_state
	{
		s32 axes[SDL_CONTROLLER_AXIS_MAX];
		u8  buttons[SDL_CONTROLLER_BUTTON_MAX];
	};

	sdl_controller_state m_controller;
	SDL_GameController *m_ctrldevice;

	void clear_buffer()
	{
		memset(&m_controller, 0, sizeof(m_controller));
	}

	void close_device()
	{
		if (m_ctrldevice)
		{
			SDL_GameControllerClose(m_ctrldevice);
			m_ctrldevice = nullptr;
		}
	}
};


//============================================================
//  sdl_input_module
//============================================================

template <typename Info>
class sdl_input_module :
		public input_module_impl<Info, sdl_osd_interface>,
		protected sdl_event_manager::subscriber
{
public:
	sdl_input_module(char const *type, char const *name) :
		input_module_impl<Info, sdl_osd_interface>(type, name)
	{
	}

	virtual void exit() override
	{
		// unsubscribe for events
		unsubscribe();

		input_module_impl<Info, sdl_osd_interface>::exit();
	}

protected:
	virtual void handle_event(SDL_Event const &event) override
	{
		// dispatch event to every device by default
		this->devicelist().for_each_device(
				[&event] (auto &device) { device.queue_event(event); });
	}
};


//============================================================
//  sdl_keyboard_module
//============================================================

class sdl_keyboard_module : public sdl_input_module<sdl_keyboard_device>
{
public:
	sdl_keyboard_module() :
		sdl_input_module<sdl_keyboard_device>(OSD_KEYBOARDINPUT_PROVIDER, "sdl")
	{
	}

	virtual void input_init(running_machine &machine) override
	{
		sdl_input_module<sdl_keyboard_device>::input_init(machine);

		constexpr int event_types[] = {
				int(SDL_KEYDOWN),
				int(SDL_KEYUP) };

		subscribe(osd(), event_types);

		// Read our keymap and store a pointer to our table
		sdlinput_read_keymap();

		osd_printf_verbose("Keyboard: Start initialization\n");

		// SDL only has 1 keyboard add it now
		auto &devinfo = create_device<sdl_keyboard_device>(
				DEVICE_CLASS_KEYBOARD,
				"System keyboard",
				"System keyboard",
				*m_key_trans_table);

		osd_printf_verbose("Keyboard: Registered %s\n", devinfo.name());
		osd_printf_verbose("Keyboard: End initialization\n");
	}

private:
	void sdlinput_read_keymap()
	{
		keyboard_trans_table &default_table = keyboard_trans_table::instance();

		// Allocate a block of translation entries big enough to hold what's in the default table
		auto key_trans_entries = std::make_unique<key_trans_entry []>(default_table.size());

		// copy the elements from the default table and ask SDL for key names
		for (int i = 0; i < default_table.size(); i++)
		{
			key_trans_entries[i] = default_table[i];
			char const *const name = SDL_GetScancodeName(SDL_Scancode(default_table[i].sdl_scancode));
			if (name && *name)
				key_trans_entries[i].ui_name = name;
		}

		// Allocate the trans table to be associated with the machine so we don't have to free it
		m_key_trans_table = std::make_unique<keyboard_trans_table>(std::move(key_trans_entries), default_table.size());

		if (!options()->bool_value(SDLOPTION_KEYMAP))
			return;

		const char *const keymap_filename = dynamic_cast<sdl_options const &>(*options()).keymap_file();
		osd_printf_verbose("Keymap: Start reading keymap_file %s\n", keymap_filename);

		FILE *const keymap_file = fopen(keymap_filename, "r");
		if (!keymap_file)
		{
			osd_printf_warning("Keymap: Unable to open keymap %s, using default\n", keymap_filename);
			return;
		}

		int line = 1;
		int sdl2section = 0;
		while (!feof(keymap_file))
		{
			char buf[256];

			char *ret = fgets(buf, 255, keymap_file);
			if (ret && buf[0] != '\n' && buf[0] != '#')
			{
				buf[255] = 0;
				int len = strlen(buf);
				if (len && buf[len - 1] == '\n')
					buf[len - 1] = 0;
				if (strncmp(buf, "[SDL2]", 6) == 0)
				{
					sdl2section = 1;
				}
				else if (sdl2section == 1)
				{
					char mks[41] = {0};
					char sks[41] = {0};
					char kns[41] = {0};

					int n = sscanf(buf, "%40s %40s %40c\n", mks, sks, kns);
					if (n != 3)
						osd_printf_error("Keymap: Error on line %d : Expected 3 parameters, got %d\n", line, n);

					int index = default_table.lookup_mame_index(mks);
					int sk = lookup_sdl_code(sks);

					if (sk >= 0 && index >= 0)
					{
						key_trans_entry &entry = (*m_key_trans_table)[index];
						entry.sdl_scancode = sk;
						entry.ui_name = const_cast<char *>(m_ui_names.emplace_back(kns).c_str());
						osd_printf_verbose("Keymap: Mapped <%s> to <%s> with ui-text <%s>\n", sks, mks, kns);
					}
					else
					{
						osd_printf_error("Keymap: Error on line %d - %s key not found: %s\n", line, (sk<0) ? "sdl" : "mame", buf);
					}
				}
			}
			line++;
		}
		fclose(keymap_file);
		osd_printf_verbose("Keymap: Processed %d lines\n", line);
	}

	std::unique_ptr<keyboard_trans_table> m_key_trans_table;
	std::list<std::string> m_ui_names;
};


//============================================================
//  sdl_mouse_module
//============================================================

class sdl_mouse_module : public sdl_input_module<sdl_mouse_device>
{
public:
	sdl_mouse_module() : sdl_input_module<sdl_mouse_device>(OSD_MOUSEINPUT_PROVIDER, "sdl")
	{
	}

	virtual void input_init(running_machine &machine) override
	{
		sdl_input_module::input_init(machine);

		constexpr int event_types[] = {
				int(SDL_MOUSEMOTION),
				int(SDL_MOUSEBUTTONDOWN),
				int(SDL_MOUSEBUTTONUP),
				int(SDL_MOUSEWHEEL) };

		subscribe(osd(), event_types);

		osd_printf_verbose("Mouse: Start initialization\n");

		// SDL currently only supports one mouse
		auto &devinfo = create_device<sdl_mouse_device>(
				DEVICE_CLASS_MOUSE,
				"System mouse",
				"System mouse");

		osd_printf_verbose("Mouse: Registered %s\n", devinfo.name());
		osd_printf_verbose("Mouse: End initialization\n");
	}
};


//============================================================
//  sdl_lightgun_module
//============================================================

class sdl_lightgun_module : public sdl_input_module<sdl_mouse_device_base>
{
public:
	sdl_lightgun_module() : sdl_input_module<sdl_mouse_device_base>(OSD_LIGHTGUNINPUT_PROVIDER, "sdl")
	{
	}

	virtual void input_init(running_machine &machine) override
	{
		auto &sdlopts = dynamic_cast<sdl_options const &>(*options());
		sdl_input_module::input_init(machine);
		bool const dual(sdlopts.dual_lightgun());

		if (!dual)
		{
			constexpr int event_types[] = {
					int(SDL_MOUSEMOTION),
					int(SDL_MOUSEBUTTONDOWN),
					int(SDL_MOUSEBUTTONUP),
					int(SDL_MOUSEWHEEL),
					int(SDL_WINDOWEVENT) };
			subscribe(osd(), event_types);
		}
		else
		{
			constexpr int event_types[] = {
					int(SDL_MOUSEBUTTONDOWN),
					int(SDL_MOUSEBUTTONUP) };
			subscribe(osd(), event_types);
		}

		osd_printf_verbose("Lightgun: Start initialization\n");

		if (!dual)
		{
			auto &devinfo = create_device<sdl_lightgun_device>(
					DEVICE_CLASS_LIGHTGUN,
					"System pointer gun 1",
					"System pointer gun 1");
			osd_printf_verbose("Lightgun: Registered %s\n", devinfo.name());
		}
		else
		{
			auto &dev1info = create_device<sdl_dual_lightgun_device>(
					DEVICE_CLASS_LIGHTGUN,
					"System pointer gun 1",
					"System pointer gun 1",
					0);
			osd_printf_verbose("Lightgun: Registered %s\n", dev1info.name());

			auto &dev2info = create_device<sdl_dual_lightgun_device>(
					DEVICE_CLASS_LIGHTGUN,
					"System pointer gun 2",
					"System pointer gun 2",
					1);
			osd_printf_verbose("Lightgun: Registered %s\n", dev2info.name());
		}

		osd_printf_verbose("Lightgun: End initialization\n");
	}
};


//============================================================
//  sdl_joystick_module_base
//============================================================

class sdl_joystick_module_base : public sdl_input_module<sdl_joystick_device_base>
{
protected:
	sdl_joystick_module_base(char const *name) :
		sdl_input_module<sdl_joystick_device_base>(OSD_JOYSTICKINPUT_PROVIDER, name),
		m_initialized_joystick(false),
		m_initialized_haptic(false)
	{
	}

	virtual ~sdl_joystick_module_base()
	{
		assert(!m_initialized_joystick);
		assert(!m_initialized_haptic);
	}

	bool have_joystick() const { return m_initialized_joystick; }
	bool have_haptic() const { return m_initialized_haptic; }

	void init_joystick()
	{
		assert(!m_initialized_joystick);
		assert(!m_initialized_haptic);

		m_initialized_joystick = !SDL_InitSubSystem(SDL_INIT_JOYSTICK);
		if (!m_initialized_joystick)
		{
			osd_printf_error("Could not initialize SDL Joystick subsystem: %s.\n", SDL_GetError());
			return;
		}

		m_initialized_haptic = !SDL_InitSubSystem(SDL_INIT_HAPTIC);
		if (!m_initialized_haptic)
			osd_printf_verbose("Could not initialize SDL Haptic subsystem: %s.\n", SDL_GetError());
	}

	void quit_joystick()
	{
		if (m_initialized_joystick)
		{
			SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
			m_initialized_joystick = false;
		}

		if (m_initialized_haptic)
		{
			SDL_QuitSubSystem(SDL_INIT_HAPTIC);
			m_initialized_haptic = false;
		}
	}

	sdl_joystick_device *create_joystick_device(int index, bool sixaxis)
	{
		// open the joystick device
		SDL_Joystick *const joy = SDL_JoystickOpen(index);
		if (!joy)
		{
			osd_printf_error("Joystick: Could not open SDL joystick %d: %s.\n", index, SDL_GetError());
			return nullptr;
		}

		// get basic info
		char const *const name = SDL_JoystickName(joy);
		SDL_JoystickGUID guid = SDL_JoystickGetGUID(joy);
		char guid_str[256];
		guid_str[0] = '\0';
		SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str) - 1);
		char const *const serial = SDL_JoystickGetSerial(joy);
		std::string id(guid_str);
		if (serial)
			id.append(1, '-').append(serial);

		// print some diagnostic info
		osd_printf_verbose("Joystick: %s [GUID %s] Vendor ID %04X, Product ID %04X, Revision %04X, Serial %s\n",
				name ? name : "<nullptr>",
				guid_str,
				SDL_JoystickGetVendor(joy),
				SDL_JoystickGetProduct(joy),
				SDL_JoystickGetProductVersion(joy),
				serial ? serial : "<nullptr>");
		osd_printf_verbose("Joystick:   ...  %d axes, %d buttons %d hats %d balls\n",
				SDL_JoystickNumAxes(joy),
				SDL_JoystickNumButtons(joy),
				SDL_JoystickNumHats(joy),
				SDL_JoystickNumBalls(joy));
		if (SDL_JoystickNumButtons(joy) > sdl_joystick_device::MAX_BUTTONS)
			osd_printf_verbose("Joystick:   ...  Has %d buttons which exceeds supported %d buttons\n", SDL_JoystickNumButtons(joy), sdl_joystick_device::MAX_BUTTONS);

		// instantiate device
		sdl_joystick_device &devinfo = sixaxis
				? create_device<sdl_sixaxis_joystick_device>(DEVICE_CLASS_JOYSTICK, name ? name : guid_str, guid_str, joy, serial)
				: create_device<sdl_joystick_device>(DEVICE_CLASS_JOYSTICK, name ? name : guid_str, guid_str, joy, serial);

		if (devinfo.has_haptic())
			osd_printf_verbose("Joystick:   ...  Has haptic capability\n");
		else
			osd_printf_verbose("Joystick:   ...  Does not have haptic capability\n");

		return &devinfo;
	}

	void dispatch_joystick_event(SDL_Event const &event)
	{
		// figure out which joystick this event is destined for
		sdl_joystick_device_base *const target_device = find_joystick(event.jdevice.which); // FIXME: this depends on SDL_JoystickID being the same size as Sint32

		// if we find a matching joystick, dispatch the event to the joystick
		if (target_device)
			target_device->queue_event(event);
	}

	device_info *find_reconnect_match(SDL_JoystickGUID const &guid, char const *serial)
	{
		char guid_str[256];
		guid_str[0] = '\0';
		SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str) - 1);
		auto target_device = std::find_if(
				devicelist().begin(),
				devicelist().end(),
				[&guid_str, &serial] (auto const &device)
				{
					return device->reconnect_match(guid_str, serial);
				});
		return (devicelist().end() != target_device) ? target_device->get() : nullptr;
	}

	sdl_joystick_device_base *find_joystick(SDL_JoystickID instance)
	{
		for (auto &device : devicelist())
		{
			if (device->is_instance(instance))
				return device.get();
		}
		return nullptr;
	}

private:
	bool m_initialized_joystick;
	bool m_initialized_haptic;
};


//============================================================
//  sdl_joystick_module
//============================================================

class sdl_joystick_module : public sdl_joystick_module_base
{
public:
	sdl_joystick_module() : sdl_joystick_module_base("sdljoy")
	{
	}

	virtual void exit() override
	{
		sdl_joystick_module_base::exit();

		quit_joystick();
	}

	virtual void input_init(running_machine &machine) override
	{
		auto &sdlopts = dynamic_cast<sdl_options const &>(*options());
		bool const sixaxis_mode = sdlopts.sixaxis();

#if defined(__SWITCH__)
		SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
#else
		if (!sdlopts.debug() && sdlopts.background_input())
			SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
#endif
		SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");

		init_joystick();
		if (!have_joystick())
			return;

		sdl_joystick_module_base::input_init(machine);

		osd_printf_verbose("Joystick: Start initialization\n");
		for (int physical_stick = 0; physical_stick < SDL_NumJoysticks(); physical_stick++)
			create_joystick_device(physical_stick, sixaxis_mode);

		constexpr int event_types[] = {
				int(SDL_JOYAXISMOTION),
				int(SDL_JOYBALLMOTION),
				int(SDL_JOYHATMOTION),
				int(SDL_JOYBUTTONDOWN),
				int(SDL_JOYBUTTONUP),
				int(SDL_JOYDEVICEADDED),
				int(SDL_JOYDEVICEREMOVED) };
		subscribe(osd(), event_types);

		osd_printf_verbose("Joystick: End initialization\n");
	}

	virtual void handle_event(SDL_Event const &event) override
	{
		if (SDL_JOYDEVICEADDED == event.type)
		{
			SDL_Joystick *const joy = SDL_JoystickOpen(event.jdevice.which);
			if (!joy)
			{
				osd_printf_error("Joystick: Could not open SDL joystick %d: %s.\n", event.jdevice.which, SDL_GetError());
			}
			else
			{
				SDL_JoystickGUID guid = SDL_JoystickGetGUID(joy);
				char const *const serial = SDL_JoystickGetSerial(joy);
				auto *const target_device = find_reconnect_match(guid, serial);
				if (target_device)
				{
					auto &devinfo = dynamic_cast<sdl_joystick_device &>(*target_device);
					devinfo.attach_device(joy);
				}
				else
				{
					SDL_JoystickClose(joy);
				}
			}
		}
		else
		{
			dispatch_joystick_event(event);
		}
	}
};


//============================================================
//  sdl_game_controller_module
//============================================================

class sdl_game_controller_module : public sdl_joystick_module_base
{
public:
	sdl_game_controller_module() :
		sdl_joystick_module_base("sdlgame"),
		m_initialized_game_controller(false)
	{
	}

	virtual void exit() override
	{
		sdl_joystick_module_base::exit();

		if (m_initialized_game_controller)
			SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);

		quit_joystick();
	}

	virtual void input_init(running_machine &machine) override
	{
		auto &sdlopts = dynamic_cast<sdl_options const &>(*options());
		bool const sixaxis_mode = sdlopts.sixaxis();

#if defined(__SWITCH__)
		SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
#else
		if (!sdlopts.debug() && sdlopts.background_input())
			SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
#endif
		SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");

		init_joystick();
		if (!have_joystick())
			return;

		m_initialized_game_controller = !SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
		if (m_initialized_game_controller)
		{
			char const *const mapfile = sdlopts.controller_mapping_file();
			if (mapfile && *mapfile && std::strcmp(mapfile, OSDOPTVAL_NONE))
			{
				auto const count = SDL_GameControllerAddMappingsFromFile(mapfile);
				if (0 <= count)
					osd_printf_verbose("Game Controller: %d controller mapping(s) added from file [%s].\n", count, mapfile);
				else
					osd_printf_error("Game Controller: Error adding mappings from file [%s]: %s.\n", mapfile, SDL_GetError());
			}
		}
		else
		{
			osd_printf_warning("Could not initialize SDL Game Controller: %s.\n", SDL_GetError());
		}

		sdl_joystick_module_base::input_init(machine);

		osd_printf_verbose("Game Controller: Start initialization\n");
		for (int physical_stick = 0; physical_stick < SDL_NumJoysticks(); physical_stick++)
		{
			// try to open as a game controller
			SDL_GameController *ctrl = nullptr;
			if (m_initialized_game_controller && SDL_IsGameController(physical_stick))
			{
				ctrl = SDL_GameControllerOpen(physical_stick);
				if (!ctrl)
					osd_printf_warning("Game Controller: Could not open SDL game controller %d: %s.\n", physical_stick, SDL_GetError());
			}

			// fall back to joystick API if necessary
			if (!ctrl)
				create_joystick_device(physical_stick, sixaxis_mode);
			else
				create_game_controller_device(physical_stick, ctrl);
		}

		constexpr int joy_event_types[] = {
				int(SDL_JOYAXISMOTION),
				int(SDL_JOYBALLMOTION),
				int(SDL_JOYHATMOTION),
				int(SDL_JOYBUTTONDOWN),
				int(SDL_JOYBUTTONUP),
				int(SDL_JOYDEVICEADDED),
				int(SDL_JOYDEVICEREMOVED) };
		constexpr int event_types[] = {
				int(SDL_JOYAXISMOTION),
				int(SDL_JOYBALLMOTION),
				int(SDL_JOYHATMOTION),
				int(SDL_JOYBUTTONDOWN),
				int(SDL_JOYBUTTONUP),
				int(SDL_JOYDEVICEADDED),
				int(SDL_JOYDEVICEREMOVED),
				int(SDL_CONTROLLERAXISMOTION),
				int(SDL_CONTROLLERBUTTONDOWN),
				int(SDL_CONTROLLERBUTTONUP),
				int(SDL_CONTROLLERDEVICEADDED),
				int(SDL_CONTROLLERDEVICEREMOVED) };
		if (m_initialized_game_controller)
			subscribe(osd(), event_types);
		else
			subscribe(osd(), joy_event_types);

		osd_printf_verbose("Game Controller: End initialization\n");
	}

	virtual void handle_event(SDL_Event const &event) override
	{
		switch (event.type)
		{
		case SDL_JOYDEVICEADDED:
			{
				// make sure this isn't an event for a reconnected game controller
				auto const controller = find_joystick(SDL_JoystickGetDeviceInstanceID(event.jdevice.which));
				if (find_joystick(SDL_JoystickGetDeviceInstanceID(event.jdevice.which)))
				{
					osd_printf_verbose(
							"Game Controller: Got SDL joystick added event for reconnected game controller %s [ID %s]\n",
							controller->name(),
							controller->id());
					break;
				}

				SDL_Joystick *const joy = SDL_JoystickOpen(event.jdevice.which);
				if (!joy)
				{
					osd_printf_error("Joystick: Could not open SDL joystick %d: %s.\n", event.jdevice.which, SDL_GetError());
					break;
				}

				SDL_JoystickGUID guid = SDL_JoystickGetGUID(joy);
				char const *const serial = SDL_JoystickGetSerial(joy);
				auto *const target_device = find_reconnect_match(guid, serial);
				if (target_device)
				{
					// if this downcast fails, opening as a game controller worked initially but failed on reconnection
					auto *const devinfo = dynamic_cast<sdl_joystick_device *>(target_device);
					if (devinfo)
						devinfo->attach_device(joy);
					else
						SDL_JoystickClose(joy);
				}
				else
				{
					SDL_JoystickClose(joy);
				}
			}
			break;

		// for devices supported by the game controller API, this is received before the corresponding SDL_JOYDEVICEADDED
		case SDL_CONTROLLERDEVICEADDED:
			if (m_initialized_game_controller)
			{
				SDL_GameController *const ctrl = SDL_GameControllerOpen(event.cdevice.which);
				if (!ctrl)
				{
					osd_printf_error("Game Controller: Could not open SDL game controller %d: %s.\n", event.cdevice.which, SDL_GetError());
					break;
				}

				SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(event.cdevice.which);
				char const *const serial = SDL_GameControllerGetSerial(ctrl);
				auto *const target_device = find_reconnect_match(guid, serial);
				if (target_device)
				{
					// downcast can fail if there was an error opening the device as a game controller the first time
					auto *const devinfo = dynamic_cast<sdl_game_controller_device *>(target_device);
					if (devinfo)
						devinfo->attach_device(ctrl);
					else
						SDL_GameControllerClose(ctrl);
				}
				else
				{
					SDL_GameControllerClose(ctrl);
				}
			}
			break;

		default:
			dispatch_joystick_event(event);
		}
	}

private:
	sdl_game_controller_device *create_game_controller_device(int index, SDL_GameController *ctrl)
	{
		// get basic info
		char const *const name = SDL_GameControllerName(ctrl);
		SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(index);
		char guid_str[256];
		guid_str[0] = '\0';
		SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str) - 1);
		char const *const serial = SDL_GameControllerGetSerial(ctrl);
		std::string id(guid_str);
		if (serial)
			id.append(1, '-').append(serial);

		// print some diagnostic info
		osd_printf_verbose("Game Controller: %s [GUID %s] Vendor ID %04X, Product ID %04X, Revision %04X, Serial %s\n",
				name ? name : "<nullptr>",
				guid_str,
				SDL_GameControllerGetVendor(ctrl),
				SDL_GameControllerGetProduct(ctrl),
				SDL_GameControllerGetProductVersion(ctrl),
				serial ? serial : "<nullptr>");
		char *const mapping = SDL_GameControllerMapping(ctrl);
		if (mapping)
		{
			osd_printf_verbose("Game Controller:   ...  mapping [%s]\n", mapping);
			SDL_free(mapping);
		}
		else
		{
			osd_printf_verbose("Game Controller:   ...  no mapping\n");
		}

		// instantiate device
		sdl_game_controller_device &devinfo = create_device<sdl_game_controller_device>(
				DEVICE_CLASS_JOYSTICK,
				name ? name : guid_str,
				guid_str,
				ctrl,
				serial);
		return &devinfo;
	}

	bool m_initialized_game_controller;
};

#if defined(__SWITCH__)

class switch_joystick_device : public device_info, protected joystick_assignment_helper
{
public:
	switch_joystick_device(std::string &&name, std::string &&id, input_module &module, int player_index) :
		device_info(std::move(name), std::move(id), module),
		m_axes{ 0, 0, 0, 0 },
		m_buttons{ 0 },
		m_player_index(player_index)
	{
		std::memset(&m_pad, 0, sizeof(m_pad));
		if (player_index == 0)
			padInitializeWithMask(&m_pad, (1ULL << HidNpadIdType_Handheld) | (1ULL << HidNpadIdType_No1));
		else
			padInitializeWithMask(&m_pad, 1ULL << player_index);

		padUpdate(&m_pad);
	}

	virtual ~switch_joystick_device() override = default;

	bool is_connected() const
	{
		return padIsConnected(&m_pad);
	}

	virtual void reset() override
	{
		std::fill(std::begin(m_axes), std::end(m_axes), 0);
		std::fill(std::begin(m_buttons), std::end(m_buttons), 0);
	}

	virtual void poll(bool relative_reset) override
	{
		padUpdate(&m_pad);

		const std::uint64_t cur = padGetButtons(&m_pad);

		for (int i = 0; i < 16; i++)
			m_buttons[i] = (cur & s_buttons[i].mask) ? 0x80 : 0x00;

		const HidAnalogStickState stick_l = padGetStickPos(&m_pad, 0);
		const HidAnalogStickState stick_r = padGetStickPos(&m_pad, 1);

		m_axes[0] = std::clamp<std::int32_t>((stick_l.x * 65536) / 32767, -65536, 65536);
		m_axes[1] = -std::clamp<std::int32_t>((stick_l.y * 65536) / 32767, -65536, 65536);
		m_axes[2] = std::clamp<std::int32_t>((stick_r.x * 65536) / 32767, -65536, 65536);
		m_axes[3] = -std::clamp<std::int32_t>((stick_r.y * 65536) / 32767, -65536, 65536);
	}

	virtual void configure(osd::input_device &device) override
	{
		device.add_item("Left Stick X", std::string_view(), ITEM_ID_XAXIS, generic_axis_get_state<std::int32_t>, &m_axes[0]);
		device.add_item("Left Stick Y", std::string_view(), ITEM_ID_YAXIS, generic_axis_get_state<std::int32_t>, &m_axes[1]);
		device.add_item("Right Stick X", std::string_view(), ITEM_ID_RXAXIS, generic_axis_get_state<std::int32_t>, &m_axes[2]);
		device.add_item("Right Stick Y", std::string_view(), ITEM_ID_RYAXIS, generic_axis_get_state<std::int32_t>, &m_axes[3]);

		for (int i = 0; i < 16; i++)
		{
			device.add_item(
				s_buttons[i].name,
				std::string_view(),
				s_buttons[i].item_id,
				generic_button_get_state<std::uint8_t>,
				&m_buttons[i]);
		}

		input_device::assignment_vector assignments;

		joystick_assignment_helper::add_directional_assignments(
			assignments,
			ITEM_ID_XAXIS,
			ITEM_ID_YAXIS,
			ITEM_ID_HAT1LEFT,
			ITEM_ID_HAT1RIGHT,
			ITEM_ID_HAT1UP,
			ITEM_ID_HAT1DOWN);

		add_button_assignment(assignments, IPT_BUTTON1, { ITEM_ID_BUTTON4 });  // Y = Quick Punch
		add_button_assignment(assignments, IPT_BUTTON3, { ITEM_ID_BUTTON3 });  // X = Fierce Punch
		add_button_assignment(assignments, IPT_BUTTON4, { ITEM_ID_BUTTON1 });  // B = Quick Kick
		add_button_assignment(assignments, IPT_BUTTON6, { ITEM_ID_BUTTON6 });  // A = Fierce Kick

		// Medium Punch: L (or ZR)
		{
			input_seq b2_seq(input_code(DEVICE_CLASS_JOYSTICK, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, ITEM_ID_BUTTON2));
			b2_seq |= input_code(DEVICE_CLASS_JOYSTICK, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, ITEM_ID_BUTTON8);
			assignments.emplace_back(IPT_BUTTON2, SEQ_TYPE_STANDARD, b2_seq);
		}

		// Medium Kick: R (or ZL)
		{
			input_seq b5_seq(input_code(DEVICE_CLASS_JOYSTICK, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, ITEM_ID_BUTTON5));
			b5_seq |= input_code(DEVICE_CLASS_JOYSTICK, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, ITEM_ID_BUTTON7);
			assignments.emplace_back(IPT_BUTTON5, SEQ_TYPE_STANDARD, b5_seq);
		}

		add_button_assignment(assignments, IPT_BUTTON7, { ITEM_ID_BUTTON7 });  // ZL
		add_button_assignment(assignments, IPT_BUTTON8, { ITEM_ID_BUTTON8 });  // ZR
		add_button_assignment(assignments, IPT_BUTTON9, { ITEM_ID_BUTTON9 });  // Stick L
		add_button_assignment(assignments, IPT_BUTTON10, { ITEM_ID_BUTTON10 }); // Stick R

		if (m_player_index == 0)
		{
			add_button_assignment(assignments, IPT_START1, { ITEM_ID_START });     // Plus
			add_button_assignment(assignments, IPT_COIN1, { ITEM_ID_SELECT });     // Minus
		}
		else if (m_player_index == 1)
		{
			add_button_assignment(assignments, IPT_START2, { ITEM_ID_START });     // Plus
			add_button_assignment(assignments, IPT_COIN2, { ITEM_ID_SELECT });     // Minus
		}
		else
		{
			add_button_assignment(assignments, IPT_START, { ITEM_ID_START });      // Plus
			add_button_assignment(assignments, IPT_SELECT, { ITEM_ID_SELECT });    // Minus
		}

		// UI navigation
		add_button_assignment(assignments, IPT_UI_SELECT, { ITEM_ID_BUTTON1 });  // B / UI Select
		add_button_assignment(assignments, IPT_UI_BACK,   { ITEM_ID_BUTTON6 });  // A / UI Back (Menu back only, does NOT exit game!)
		add_button_assignment(assignments, IPT_UI_CLEAR,  { ITEM_ID_BUTTON3 });  // X / UI Clear (matching Guarazzo)
		add_button_assignment(assignments, IPT_UI_MENU,   { ITEM_ID_BUTTON10 }); // Stick R (R3) / UI Menu (doesn't interfere with Coin!)
		add_button_assignment(assignments, IPT_UI_PAUSE,  { ITEM_ID_BUTTON9 });  // Stick L (L3) / UI Pause

		// Quit to hbmenu: Plus + Minus combo
		assignments.emplace_back(
			IPT_UI_CANCEL,
			SEQ_TYPE_STANDARD,
			input_seq(
				input_code(DEVICE_CLASS_JOYSTICK, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, ITEM_ID_START),
				input_code(DEVICE_CLASS_JOYSTICK, 0, ITEM_CLASS_SWITCH, ITEM_MODIFIER_NONE, ITEM_ID_SELECT)));

		add_button_pair_assignment(assignments, IPT_UI_PREV_GROUP, IPT_UI_NEXT_GROUP, ITEM_ID_BUTTON2, ITEM_ID_BUTTON5);
		add_button_pair_assignment(assignments, IPT_UI_PAGE_UP, IPT_UI_PAGE_DOWN, ITEM_ID_BUTTON7, ITEM_ID_BUTTON8);

		add_assignment(assignments, IPT_JOYSTICKRIGHT_LEFT, SEQ_TYPE_STANDARD, ITEM_CLASS_SWITCH, ITEM_MODIFIER_LEFT, { ITEM_ID_RXAXIS });
		add_assignment(assignments, IPT_JOYSTICKRIGHT_RIGHT, SEQ_TYPE_STANDARD, ITEM_CLASS_SWITCH, ITEM_MODIFIER_RIGHT, { ITEM_ID_RXAXIS });
		add_assignment(assignments, IPT_JOYSTICKRIGHT_UP, SEQ_TYPE_STANDARD, ITEM_CLASS_SWITCH, ITEM_MODIFIER_UP, { ITEM_ID_RYAXIS });
		add_assignment(assignments, IPT_JOYSTICKRIGHT_DOWN, SEQ_TYPE_STANDARD, ITEM_CLASS_SWITCH, ITEM_MODIFIER_DOWN, { ITEM_ID_RYAXIS });

		device.set_default_assignments(std::move(assignments));
	}

private:
	struct button_def {
		std::uint64_t mask;
		input_item_id item_id;
		const char *name;
	};

	static constexpr button_def s_buttons[16] = {
		{ HidNpadButton_B,      ITEM_ID_BUTTON1,  "B" },
		{ HidNpadButton_L,      ITEM_ID_BUTTON2,  "L" },
		{ HidNpadButton_X,      ITEM_ID_BUTTON3,  "X" },
		{ HidNpadButton_Y,      ITEM_ID_BUTTON4,  "Y" },
		{ HidNpadButton_R,      ITEM_ID_BUTTON5,  "R" },
		{ HidNpadButton_A,      ITEM_ID_BUTTON6,  "A" },
		{ HidNpadButton_ZL,     ITEM_ID_BUTTON7,  "ZL" },
		{ HidNpadButton_ZR,     ITEM_ID_BUTTON8,  "ZR" },
		{ HidNpadButton_StickL, ITEM_ID_BUTTON9,  "Stick L" },
		{ HidNpadButton_StickR, ITEM_ID_BUTTON10, "Stick R" },
		{ HidNpadButton_Plus,   ITEM_ID_START,    "Plus" },
		{ HidNpadButton_Minus,  ITEM_ID_SELECT,   "Minus" },
		{ HidNpadButton_Up,     ITEM_ID_HAT1UP,   "D-Pad Up" },
		{ HidNpadButton_Down,   ITEM_ID_HAT1DOWN, "D-Pad Down" },
		{ HidNpadButton_Left,   ITEM_ID_HAT1LEFT, "D-Pad Left" },
		{ HidNpadButton_Right,  ITEM_ID_HAT1RIGHT, "D-Pad Right" },
	};

	PadState        m_pad;
	std::int32_t    m_axes[4];
	std::uint8_t    m_buttons[16];
	int             m_player_index;
};

class switch_joystick_module : public input_module_impl<switch_joystick_device, osd_common_t>
{
public:
	switch_joystick_module() :
		input_module_impl<switch_joystick_device, osd_common_t>(OSD_JOYSTICKINPUT_PROVIDER, "switch")
	{
	}

	virtual void input_init(running_machine &machine) override
	{
		input_module_impl<switch_joystick_device, osd_common_t>::input_init(machine);

		padConfigureInput(8, HidNpadStyleSet_NpadStandard);
		osd_printf_verbose("Switch: native input module (libnx pad API)\n");

		for (int i = 0; i < 8; ++i)
		{
			char name[32], id[32];
			std::snprintf(name, sizeof(name), "Switch Pad %d", i + 1);
			std::snprintf(id, sizeof(id), "switchpad%d", i + 1);
			switch_joystick_device &dev = create_device<switch_joystick_device>(
				DEVICE_CLASS_JOYSTICK,
				std::string(name),
				std::string(id),
				i);
			osd_printf_verbose("Switch: %s %s\n", dev.name().c_str(), dev.is_connected() ? "conectado" : "sin conectar");
		}
	}
};

#endif // defined(__SWITCH__)

} // anonymous namespace

} // namespace osd


MODULE_DEFINITION(KEYBOARDINPUT_SDL, osd::sdl_keyboard_module)
MODULE_DEFINITION(MOUSEINPUT_SDL, osd::sdl_mouse_module)
MODULE_DEFINITION(LIGHTGUNINPUT_SDL, osd::sdl_lightgun_module)
MODULE_DEFINITION(JOYSTICKINPUT_SDLJOY, osd::sdl_joystick_module)
MODULE_DEFINITION(JOYSTICKINPUT_SDLGAME, osd::sdl_game_controller_module)
#if defined(__SWITCH__)
MODULE_DEFINITION(JOYSTICKINPUT_SWITCH, osd::switch_joystick_module)
#endif

#endif // defined(OSD_SDL) && !defined(SDLMAME_SDL3)
