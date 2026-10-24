#ifndef SYSTEM3_EMBEDDED_H
#define SYSTEM3_EMBEDDED_H

#include <SDL.h>
#include <string>

struct system3_host {
	void* user;
	void (*present)(void*, const SDL_Surface*);
	int (*poll_event)(void*, SDL_Event*);
	int (*checkpoint)(void*);
	void (*log)(void*, int, const char*);
};

int system3_host_run(const system3_host*, const char* root, const char* saves, const char* font,
	const char* encoding, bool print_text);
void system3_host_checkpoint();
int system3_host_poll_event(SDL_Event*);
void system3_host_wait_event(SDL_Event*);
const Uint8* system3_host_keyboard();
Uint32 system3_host_mouse();
void system3_host_clear_input();
void system3_host_reset_cursor();
void system3_host_pause_audio(bool);
int system3_host_audio_peak();
void system3_host_delay(int milliseconds);
void system3_host_present();
void system3_host_resize(int width, int height);
std::string system3_host_path(const char* name, bool savedata = false, bool write = false);

#endif
