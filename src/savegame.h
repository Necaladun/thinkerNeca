#pragma once

#include "main.h"

/*
Minor terrain: an independent tile feature, not an LM_* landmark. Stored
and saved separately from original landmarks. Yield/render hooks are future
batches.
*/
enum MinorLandmark : uint8_t {
    MINOR_NONE = 0,
    MINOR_JUNGLE = 1,
    MINOR_URANIUM = 2,
    MINOR_IRONSTONE = 3,
};

MinorLandmark minor_landmark_at(int x, int y);
bool minor_landmark_set(int x, int y, MinorLandmark type);
void minor_landmark_clear(int x, int y);
void minor_landmark_reset();

enum SaveLoadStatus {
    SAVE_LOAD_VALID = 0,
    SAVE_LOAD_NONE = 1,
    SAVE_LOAD_NOT = 2,
    SAVE_LOAD_OLD = 3,
    SAVE_LOAD_ERROR = 4,
    SAVE_LOAD_SECURITY = 5,
};

int __cdecl game_data(FILE* fp, int write_file);
int __cdecl game_io(MapWindow* state, FILE* fp);
int __cdecl encrypt_write(void* src_ptr, size_t len, size_t cnt, FILE* fp);
int __cdecl encrypt_read(void* dst_ptr, size_t len, size_t cnt, FILE* fp);
void __cdecl map_shutdown();
void __cdecl map_wipe();
int __cdecl map_init();
int __cdecl map_write(FILE* fp);
int __cdecl map_read(FILE* fp);
int __cdecl map_data(FILE* fp, int save_flag, int extended_flag);
int __cdecl header_check(char* dst, FILE* fp);
int __cdecl header_write(const char* src, FILE* fp);
int __cdecl mod_save_daemon(const char* filename);
int __cdecl mod_load_daemon(const char* filename, int flag);
int __cdecl mod_save_map_daemon(const char* filename);
int __cdecl mod_load_map_daemon(const char* filename);
void __cdecl mod_auto_save();

