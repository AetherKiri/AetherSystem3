// Keep upstream's standalone ABI while isolating embedded engine symbols.
#ifdef SYSTEM3_EMBEDDED
#define AGS AetherSystem3AGS
#define NACT AetherSystem3NACT
#define MAKO AetherSystem3MAKO
#define FILEIO AetherSystem3FILEIO
#define Config AetherSystem3Config
#define Strings AetherSystem3Strings
#define Encoding AetherSystem3Encoding
#define SjisEncoding AetherSystem3SjisEncoding
#define Utf8Encoding AetherSystem3Utf8Encoding
#define GameId AetherSystem3GameId
#define Scenario AetherSystem3Scenario
#define Dri AetherSystem3Dri
#define MsgSkip AetherSystem3MsgSkip
#define CG AetherSystem3CG
#define sys_error system3_sys_error
#define sys_warning system3_sys_warning
#define g_window system3_window
#define g_renderer system3_renderer
#define g_nact system3_nact
#define sdl_custom_event_type system3_custom_event_type
#define open_file system3_open_file
#define open_resource system3_open_resource
#define ags_setAntialiasedStringMode system3_set_antialias
#define texthook_set_mode system3_texthook_set_mode
#define texthook_set_suppression_list system3_texthook_set_suppression_list
#define texthook_character system3_texthook_character
#define texthook_newline system3_texthook_newline
#define texthook_nextpage system3_texthook_nextpage
#define texthook_keywait system3_texthook_keywait
#endif
