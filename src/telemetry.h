#pragma once

// Optional, read-only campaign telemetry. Never uses Thinker's debug.txt stream.
void telemetry_begin_session(const char* source_save);
void telemetry_record_completed_turn();
void telemetry_end_session();
