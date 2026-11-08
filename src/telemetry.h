#pragma once

// Optional campaign telemetry. Game observations remain read-only; changing
// campaign labels/links only modifies recorder metadata and sidecar files.
void telemetry_begin_session(const char* source_save);
void telemetry_record_completed_turn();
void telemetry_record_ui_notice(const char* source, const char* label, const char* action);
void telemetry_end_session();

// Human-friendly campaign labels are separate from stable folder IDs. A save
// made by the Simulation Control window includes an explicit opt-in campaign
// link; loading it offers to start a NEW session within the existing campaign.
const char* telemetry_get_campaign_name();
bool telemetry_set_campaign_name(const char* name);
bool telemetry_record_named_save(const char* save_path);
void telemetry_note_loaded_save(const char* source_path);
