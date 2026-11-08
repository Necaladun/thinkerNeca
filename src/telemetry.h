#pragma once

// Optional campaign telemetry. Game observations remain read-only; changing
// campaign labels/links only modifies recorder metadata and sidecar files.
void telemetry_begin_session(const char* source_save);
void telemetry_record_completed_turn();
// Wall-clock + monotonic timer for actual native engine phase timings.
// No game RNG, AI calls or turn/state modification.
struct TelemetryPhaseStamp {
    unsigned long long started_ticks = 0;
    char started_utc[40] = {};
};
TelemetryPhaseStamp telemetry_phase_begin();
void telemetry_phase_end(const TelemetryPhaseStamp& stamp, const char* phase, int faction);
// Optional full-cycle game-state checkpoint stored in the active campaign archive.
// Does not replace native autosaves or run during Ironman RNG-mutating saves.
void telemetry_archive_completed_turn();
// Preserve operator-created native SAVs beside telemetry without replacing them.
void telemetry_archive_named_save(const char* saved_game_path);
// Terminal result is recorded once at the exact faction/phase boundary,
// without pretending an incomplete turn was a completed cycle.
void telemetry_record_terminal_result(const char* save_path, const char* save_status);
void telemetry_resume_after_victory();
// TEL-ORBIT basic event stream. Keep capture read-only; future AI decision
// traces can extend the schema without changing the native combat sequence.
struct OrbitAttackTrace {
    int id = -1;
    int attacker_odp = 0;
    int attacker_deployed = 0;
    int target_nutrient = 0;
    int target_mineral = 0;
    int target_energy = 0;
    int target_odp = 0;
    int target_geosync = 0;
};
void telemetry_orbit_launch(int faction_id, int base_id, int facility_id, int before_count);
OrbitAttackTrace telemetry_orbit_begin_attack(int attacker, int defender,
    int target_type, int base_id, int reserved_odp);
void telemetry_orbit_end_attack(const OrbitAttackTrace& trace, int attacker,
    int defender, int target_type, int base_id);
void telemetry_orbit_environment_loss(int faction_id, const char* cause,
    int lost_mineral, int lost_energy, int lost_odp);
void telemetry_record_ui_notice(const char* source, const char* label, const char* action);
// Dedicated session Council trace, flushed before native modal processing.
void telemetry_record_council_event(const char* phase, int actor, const char* detail);
void telemetry_end_session();

// Human-friendly campaign labels are separate from stable folder IDs. A save
// made by the Simulation Control window includes an explicit opt-in campaign
// link; loading it offers to start a NEW session within the existing campaign.
const char* telemetry_get_campaign_name();
const char* telemetry_get_campaign_id();
const char* telemetry_get_session_id();
// True only after the operator explicitly rejoined a fingerprint-linked save.
bool telemetry_resumed_linked_campaign();
bool telemetry_set_campaign_name(const char* name);
bool telemetry_record_named_save(const char* save_path);
void telemetry_note_loaded_save(const char* source_path);
