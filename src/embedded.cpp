// Native, in-process host. No SDL window, global event pump, cwd change or exit.
#include "embedded.h"
#include "ags.h"
#include "config.h"
#include "fileio.h"
#include "game_id.h"
#include "nact.h"
#include "texthook.h"
#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <filesystem>
#include <stdexcept>

SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
std::unique_ptr<NACT> g_nact;
uint32_t sdl_custom_event_type;

namespace {
const system3_host* host;
SDL_Surface* output;
Uint8 keys[SDL_NUM_SCANCODES];
Uint32 buttons;
std::filesystem::path game_root, save_root;
struct SessionStopped { };
std::string lower(std::string text)
{
	for (char& c : text)
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
	return text;
}
std::filesystem::path resolve(std::filesystem::path base, const std::filesystem::path& relative)
{
	for (const auto& part : relative) {
		if (part == ".")
			continue;
		auto next = base / part;
		if (!std::filesystem::exists(next) && std::filesystem::is_directory(base))
			for (const auto& entry : std::filesystem::directory_iterator(base))
				if (lower(entry.path().filename().u8string()) == lower(part.u8string())) {
					next = entry.path();
					break;
				}
		base = next;
	}
	return base;
}
}

std::string system3_host_path(const char* name, bool savedata, bool write)
{
	namespace fs = std::filesystem;
	std::string text(name ? name : "");
	std::replace(text.begin(), text.end(), '\\', '/');
	fs::path relative = fs::u8path(text).lexically_normal();
	if (relative.is_absolute() && !savedata)
		return relative.u8string();
	if (relative.is_absolute() || (!relative.empty() && *relative.begin() == ".."))
		relative = relative.filename();
	if (savedata) {
		auto file = resolve(save_root, relative);
		if (write) {
			fs::create_directories(file.parent_path());
			return file.u8string();
		}
		if (fs::is_regular_file(file))
			return file.u8string();
		// Read existing imported saves until this slot has an Aether save.
		return resolve(game_root, relative).u8string();
	}
	return resolve(game_root, relative).u8string();
}

int system3_host_run(const system3_host* hooks, const char* root, const char* saves,
	const char* font, const char* encoding, bool print_text)
{
	host = hooks;
	game_root = std::filesystem::u8path(root);
	save_root = std::filesystem::u8path(saves);
	system3_host_clear_input();
	system3_host_reset_cursor();
	int result = 0;
	try {
		char program[] = "system3";
		char* args[] = { program };
		Config config(1, args); // Preserve translated game id, encoding and font.
		config.save_dir = saves;
		config.debugger_mode = DebuggerMode::DISABLED;
		config.texthook_mode = print_text ? TexthookMode::PRINT : TexthookMode::NONE;
		if (config.font_file.empty())
			config.font_file = font;
		if (encoding && *encoding && strcmp(encoding, "auto"))
			config.encoding = encoding;
		GameId game(config);
		if (!game.is_valid())
			sys_error("Unknown System3 game; set game = <game-id> in system3.ini");
		std::string info = std::string("System3 game=") + game.name + " encoding=" + game.encoding;
		if (host->log)
			host->log(host->user, 1, info.c_str());
		struct ResetVm {
			~ResetVm() { g_nact.reset(); }
		} reset_vm;
		FILEIO::set_savedir(saves);
		texthook_set_mode(config.texthook_mode);
		texthook_set_suppression_list(config.texthook_suppressions.c_str());
		do {
			g_nact.reset(NACT::create(config, game));
			result = g_nact->mainloop();
			g_nact.reset();
		} while (result == NACT_RESTART);
	} catch (const SessionStopped&) {
		result = 0;
	} catch (const std::exception& error) {
		if (host->log)
			host->log(host->user, 0, error.what());
		result = 1;
	}
	g_nact.reset();
	texthook_set_mode(TexthookMode::NONE);
	texthook_set_suppression_list("");
	SDL_DestroyRenderer(g_renderer);
	g_renderer = nullptr;
	SDL_FreeSurface(output);
	output = nullptr;
	game_root.clear();
	save_root.clear();
	host = nullptr;
	return result;
}

void system3_host_checkpoint()
{
	if (host && host->checkpoint && host->checkpoint(host->user))
		throw SessionStopped {};
}
void system3_host_delay(int milliseconds)
{
	for (int remaining = milliseconds; remaining > 0;) {
		system3_host_checkpoint();
		int count = std::min(remaining, 5);
		SDL_Delay(count);
		remaining -= count;
	}
}
int system3_host_poll_event(SDL_Event* event)
{
	system3_host_checkpoint();
	if (!host || !host->poll_event || !host->poll_event(host->user, event))
		return 0;
	if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
		auto code = event->key.keysym.scancode;
		if (code > SDL_SCANCODE_UNKNOWN && code < SDL_NUM_SCANCODES)
			keys[code] = event->type == SDL_KEYDOWN;
	} else if (event->type == SDL_MOUSEBUTTONDOWN)
		buttons |= SDL_BUTTON(event->button.button);
	else if (event->type == SDL_MOUSEBUTTONUP)
		buttons &= ~SDL_BUTTON(event->button.button);
	return 1;
}
void system3_host_wait_event(SDL_Event* event)
{
	while (!system3_host_poll_event(event))
		system3_host_delay(5);
}
const Uint8* system3_host_keyboard() { return keys; }
Uint32 system3_host_mouse() { return buttons; }
void system3_host_clear_input()
{
	memset(keys, 0, sizeof(keys));
	buttons = 0;
}
void system3_host_resize(int width, int height)
{
	SDL_DestroyRenderer(g_renderer);
	SDL_FreeSurface(output);
	g_renderer = nullptr;
	output = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (!output || !(g_renderer = SDL_CreateSoftwareRenderer(output)))
		sys_error("Cannot create System3 renderer: %s", SDL_GetError());
}
void system3_host_present()
{
	if (host && host->present)
		host->present(host->user, output);
}
[[noreturn]] void sys_error(const char* format, ...)
{
	char text[2048];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	throw std::runtime_error(text);
}
void sys_warning(const char* format, ...)
{
	char text[2048];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	if (host && host->log)
		host->log(host->user, 1, text);
}
