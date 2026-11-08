#include "telemetry.h"
#include "main.h"

#include <iomanip>
#include <map>
#include <sstream>

// TEL-001 is an independent, opt-in file writer. Keep all game-state reads in
// telemetry_record_completed_turn() and never evaluate game RNG or AI functions.
namespace {

FILE* turns_file = NULL;
FILE* bases_file = NULL;
FILE* ui_notices_file = NULL;
FILE* orbit_file = NULL;
int orbit_event_seq = 0;
int orbit_attack_seq = 0;
// Once the first satellite appears, retain snapshots even after all are lost.
bool orbit_snapshots_started = false;
int council_meeting_seq = 0;
int council_calling_faction = 0;
int council_active_meeting = 0;
const CouncWindow* council_active_window = NULL;
bool council_result_written = false;
int project_event_seq = 0;
struct ProjectQueueState {
    int faction = 0;
    int facility = 0;
    int x = 0;
    int y = 0;
    std::string base_name;
};
std::map<std::pair<int,int>, ProjectQueueState> project_queues;
FILE* council_file = NULL;
FILE* projects_file = NULL;
FILE* timings_file = NULL;
FILE* checkpoints_file = NULL;
// TEL-LOG-02 streams are standalone; none are presentation diagnostics.
FILE* production_file = NULL;
FILE* units_file = NULL;
FILE* combat_file = NULL;
FILE* research_file = NULL;
int tel02_event_seq = 0;
int tel02_vehicle_next = 0;
std::vector<int> tel02_vehicle_ids;
std::vector<int> tel02_research_targets;
std::string campaign_dir;
std::string session_dir;
std::string campaign_id;
std::string campaign_display_name;
std::string campaign_created_utc;
std::string last_loaded_save;
int last_committed_turn = -1;
int last_checkpoint_turn = -1;
int initial_observer = 0;
bool terminal_result_recorded = false;
bool recording = false;
bool resumed_linked_campaign = false;
// Monotonic process-local duration for THIS execution session. DWORD tick
// subtraction is wrap-safe for any single active interval under 49 days.
unsigned long long simulation_elapsed_completed_ms = 0;
DWORD simulation_active_start_tick = 0;
bool simulation_clock_active = false;
bool simulation_clock_started = false;

std::string utc_stamp_ms() {
    SYSTEMTIME time = {};
    GetSystemTime(&time);
    char buffer[40] = {};
    snprintf(buffer, sizeof(buffer), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        (unsigned)time.wYear, (unsigned)time.wMonth, (unsigned)time.wDay,
        (unsigned)time.wHour, (unsigned)time.wMinute,
        (unsigned)time.wSecond, (unsigned)time.wMilliseconds);
    return buffer;
}

std::string utc_stamp() {
    SYSTEMTIME time;
    GetSystemTime(&time);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%04u%02u%02uT%02u%02u%02uZ",
        (unsigned)time.wYear, (unsigned)time.wMonth, (unsigned)time.wDay,
        (unsigned)time.wHour, (unsigned)time.wMinute, (unsigned)time.wSecond);
    return buffer;
}

std::string readable_campaign_default() {
    const int owner = MapWin ? MapWin->cOwner : 0;
    const char* observed = owner > 0 && owner < MaxPlayerNum
        ? MFactions[owner].formal_name_faction : "All-AI";
    char value[176] = {};
    snprintf(value, sizeof(value), "AI Sim - %s - MY%d",
        observed ? observed : "All-AI", *CurrentMissionYear);
    return value;
}

std::string folder_slug(const char* name) {
    std::string result;
    bool separator = false;
    if (!name) name = "";
    for (const unsigned char* p = (const unsigned char*)name; *p; ++p) {
        const unsigned char c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9') || c == '_' || c == '-') {
            if (separator && !result.empty() && result.back() != '-') result += '-';
            separator = false;
            result += (char)c;
        } else {
            separator = true;
        }
        if (result.size() >= 40) break;
    }
    while (!result.empty() && result.back() == '-') result.pop_back();
    return result.empty() ? "All-AI" : result;
}

bool valid_campaign_id(const std::string& name) {
    if (name.empty() || name.size() >= 110) return false;
    for (unsigned char c : name) {
        if (!(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z')
        && !(c >= '0' && c <= '9') && c != '-' && c != '_') return false;
    }
    return true;
}

std::string json_string(const char* text) {
    // Faction and base names originate in the game's ANSI codepage, not UTF-8.
    std::string utf8;
    if (text) {
        int wide_len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
        if (wide_len > 0) {
            std::vector<wchar_t> wide((size_t)wide_len);
            if (MultiByteToWideChar(CP_ACP, 0, text, -1, wide.data(), wide_len)) {
                int byte_len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1,
                    NULL, 0, NULL, NULL);
                if (byte_len > 0) {
                    std::vector<char> bytes((size_t)byte_len);
                    if (WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1,
                        bytes.data(), byte_len, NULL, NULL)) {
                        utf8.assign(bytes.data(), (size_t)byte_len - 1);
                    }
                }
            }
        }
    }
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : utf8) {
        if (ch == '"' || ch == '\\') {
            out << '\\' << (char)ch;
        } else if (ch < 0x20) {
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                << (unsigned)ch << std::dec;
        } else {
            out << (char)ch;
        }
    }
    out << '"';
    return out.str();
}

bool make_dir(const std::string& path, bool allow_existing) {
    if (CreateDirectoryA(path.c_str(), NULL)) {
        return true;
    }
    if (!allow_existing || GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }
    DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES
        && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

// An explicit link is written ONLY by Simulation Control named saves.
// FNV-1a+length binds it to the exact save bytes (noncryptographic); a later
// native Save Game overwrite cannot silently inherit stale campaign identity.
std::string file_fingerprint(const char* path);

bool read_named_save_link(const std::string& save_path, std::string* id,
    std::string* created, std::string* display,
    int* saved_turns, int* saved_notices, int* saved_fast) {
    std::vector<std::string> candidates(1, save_path);
    if (save_path.find_first_of("\\/") == std::string::npos)
        candidates.push_back("saves/" + save_path);
    for (const std::string& candidate : candidates) {
        FILE* fp = fopen((candidate + ".autosim-campaign").c_str(), "rb");
        if (!fp) continue;
        char version[64] = {}, linked_id[128] = {}, stamp[64] = {},
            label[160] = {}, fingerprint[180] = {};
        const bool ok = fgets(version, sizeof(version), fp)
            && fgets(linked_id, sizeof(linked_id), fp)
            && fgets(stamp, sizeof(stamp), fp)
            && fgets(label, sizeof(label), fp)
            && fgets(fingerprint, sizeof(fingerprint), fp);
        // Optional v2-compatible sixth line: a simulation operator profile.
        // Older linked saves without the line remain readable.
        char options[128] = {};
        if (ok) fgets(options, sizeof(options), fp);
        fclose(fp);
        if (!ok) continue;
        auto remove_newline = [](char* value) {
            size_t n = strlen(value);
            while (n && (value[n - 1] == '\n' || value[n - 1] == '\r'))
                value[--n] = '\0';
        };
        remove_newline(version); remove_newline(linked_id);
        remove_newline(stamp); remove_newline(label); remove_newline(fingerprint);
        if (strcmp(version, "thinkerNeca-campaign-v2")
        || !valid_campaign_id(linked_id) || strlen(stamp) != 16
        || !label[0] || strncmp(fingerprint, "{\"present\":true,", 16)
        || file_fingerprint(candidate.c_str()) != fingerprint) continue;
        const std::string root = std::string("campaigns/") + linked_id;
        DWORD attrs = GetFileAttributesA((root + "/campaign.json").c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        attrs = GetFileAttributesA((root + "/sessions").c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        *id = linked_id; *created = stamp; *display = label;
        int turns = 0, notices = -1, fast = -1;
        char trailing = 0;
        if (sscanf(options, "autosim-v1 %d %d %d %c",
            &turns, &notices, &fast, &trailing) == 3
        && turns >= 1 && turns <= 100000
        && (notices == 0 || notices == 1)
        && (fast == 0 || fast == 1)) {
            if (saved_turns) *saved_turns = turns;
            if (saved_notices) *saved_notices = notices;
            if (saved_fast) *saved_fast = fast;
        }
        return true;
    }
    return false;
}

bool atomic_write(const std::string& path, const std::string& contents) {
    std::string pending = path + ".tmp";
    FILE* file = fopen(pending.c_str(), "wb");
    if (!file) {
        return false;
    }
    bool ok = fwrite(contents.data(), 1, contents.size(), file) == contents.size();
    if (fclose(file)) {
        ok = false;
    }
    if (ok) {
        ok = MoveFileExA(pending.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    }
    if (!ok) {
        DeleteFileA(pending.c_str());
    }
    return ok;
}

// A noncryptographic fingerprint for detecting different installed game files.
// In particular, hashes are not purported to prove identical game behaviour.
std::string file_fingerprint(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        return "{\"present\":false}";
    }
    uint64_t hash = UINT64_C(14695981039346656037);
    uint64_t length = 0;
    unsigned char buffer[8192];
    size_t bytes = 0;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytes; ++i) {
            hash = (hash ^ buffer[i]) * UINT64_C(1099511628211);
        }
        length += bytes;
    }
    const bool ok = !ferror(file);
    fclose(file);
    if (!ok) {
        return "{\"present\":true,\"read_error\":true}";
    }
    std::ostringstream out;
    out << "{\"present\":true,\"bytes\":" << length
        << ",\"fnv1a64\":\"" << std::hex << std::setw(16)
        << std::setfill('0') << hash << "\"}";
    return out.str();
}

bool append_line(FILE* file, const std::string& line) {
    return fwrite(line.data(), 1, line.size(), file) == line.size()
        && fputc('\n', file) != EOF && fflush(file) == 0;
}

std::string status_json(const char* state) {
    std::ostringstream out;
    out << "{\"schema\":1,\"state\":" << json_string(state)
        << ",\"last_committed_turn\":" << last_committed_turn
        << ",\"simulation_elapsed_ms\":";
    if (simulation_clock_started) out << telemetry_run_elapsed_ms();
    else out << "null";
    out << ",\"simulation_time_basis\":\"active_ai_wall_time_excludes_operator_pauses\""
        << ",\"updated_utc\":" << json_string(utc_stamp().c_str()) << "}\n";
    return out.str();
}

bool write_campaign_metadata() {
    if (campaign_dir.empty()) return true;
    std::ostringstream out;
    out << "{\"schema\":1,\"campaign_id\":"
        << json_string(campaign_id.c_str())
        << ",\"display_name\":" << json_string(campaign_display_name.c_str())
        << ",\"created_utc\":" << json_string(campaign_created_utc.c_str())
        << ",\"lineage\":\"explicit_or_new\"}\n";
    return atomic_write(campaign_dir + "/campaign.json", out.str());
}

void close_files() {
    if (turns_file) {
        fclose(turns_file);
        turns_file = NULL;
    }
    if (bases_file) {
        fclose(bases_file);
        bases_file = NULL;
    }
    if (ui_notices_file) {
        fclose(ui_notices_file);
        ui_notices_file = NULL;
    }
    if (orbit_file) {
        fclose(orbit_file);
        orbit_file = NULL;
    }
    if (council_file) {
        fclose(council_file);
        council_file = NULL;
    }
    if (projects_file) {
        fclose(projects_file);
        projects_file = NULL;
    }
    if (timings_file) {
        fclose(timings_file);
        timings_file = NULL;
    }
    if (checkpoints_file) {
        fclose(checkpoints_file);
        checkpoints_file = NULL;
    }
    if (production_file) { fclose(production_file); production_file = NULL; }
    if (units_file) { fclose(units_file); units_file = NULL; }
    if (combat_file) { fclose(combat_file); combat_file = NULL; }
    if (research_file) { fclose(research_file); research_file = NULL; }
}

void recorder_failure() {
    recording = false;
    close_files();
    if (!session_dir.empty()) {
        atomic_write(session_dir + "/status.json", status_json("io_error"));
    }
    OutputDebugStringA("thinkerNeca TEL-001: recording disabled after a file error\n");
}

} // namespace

void telemetry_run_clock_set_active(bool active) {
    if (simulation_clock_active == active) return;
    const DWORD now = GetTickCount();
    if (active) {
        simulation_clock_started = true;
        simulation_active_start_tick = now;
    } else {
        simulation_elapsed_completed_ms +=
            (unsigned long long)(DWORD)(now - simulation_active_start_tick);
    }
    simulation_clock_active = active;
}

bool telemetry_run_clock_started() {
    return simulation_clock_started;
}

unsigned long long telemetry_run_elapsed_ms() {
    return simulation_elapsed_completed_ms +
        (simulation_clock_active
            ? (unsigned long long)(DWORD)(GetTickCount() - simulation_active_start_tick)
            : 0);
}

static bool telemetry_orbit_snapshot(const char* phase);

void telemetry_begin_session(const char* source_save) {
    telemetry_end_session();
    simulation_elapsed_completed_ms = 0;
    simulation_active_start_tick = 0;
    simulation_clock_active = false;
    simulation_clock_started = false;
    last_committed_turn = -1;
    last_checkpoint_turn = -1;
    orbit_event_seq = 0;
    orbit_attack_seq = 0;
    orbit_snapshots_started = false;
    tel02_event_seq = 0;
    tel02_vehicle_next = 0;
    tel02_vehicle_ids.clear();
    tel02_research_targets.clear();
    council_meeting_seq = 0;
    council_calling_faction = 0;
    council_active_meeting = 0;
    council_active_window = NULL;
    council_result_written = false;
    project_event_seq = 0;
    project_queues.clear();
    initial_observer = MapWin ? MapWin->cOwner : 0;
    terminal_result_recorded = false;
    campaign_dir.clear();
    session_dir.clear();
    campaign_id.clear();
    campaign_created_utc.clear();
    campaign_display_name = autosim_launcher_active
        && !autosim_launch_campaign_name.empty()
        ? autosim_launch_campaign_name : readable_campaign_default();
    resumed_linked_campaign = false;
    if (!conf.telemetry_enabled || *MultiplayerActive || *PbemActive) {
        return;
    }
    if (!make_dir("campaigns", true)) {
        recorder_failure();
        return;
    }

    const std::string stamp = utc_stamp();
    const std::string source = (source_save && source_save[0])
        ? source_save : last_loaded_save;
    bool joined = false;
    std::string linked_id, linked_stamp, linked_name;
    int linked_turns = 0, linked_notices = -1, linked_fast = -1;
    if (!source.empty()
    && read_named_save_link(source, &linked_id, &linked_stamp, &linked_name,
        &linked_turns, &linked_notices, &linked_fast)) {
        const std::string prompt = "Resume telemetry in existing campaign?\n\n"
            + linked_name + "\n\nSave: " + source
            + "\n\nNo: record as a new campaign.";
        if (MessageBoxA(*phWnd, prompt.c_str(), "thinkerNeca Campaign",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1) == IDYES) {
            campaign_id = linked_id;
            campaign_created_utc = linked_stamp;
            campaign_display_name = linked_name;
            campaign_dir = "campaigns/" + campaign_id;
            joined = true;
            resumed_linked_campaign = true;
            // Only apply an explicit operator profile after the campaign
            // relink has been accepted. Session identity stays unchanged.
            // Explicit pre-game launcher settings override older settings
            // saved in the sidecar of a resumed campaign.
            if (linked_turns > 0 && !autosim_launcher_active) {
                conf.autosim_manual_turns = linked_turns;
                conf.autosim_suppress_notices = linked_notices;
                conf.autosim_fast_combat = linked_fast;
            }
        }
    }
    if (!joined) {
        SYSTEMTIME local = {};
        GetLocalTime(&local);
        char date_time[32] = {};
        snprintf(date_time, sizeof(date_time),
            "%04u%02u%02u-%02u%02u%02u",
            (unsigned)local.wYear, (unsigned)local.wMonth,
            (unsigned)local.wDay, (unsigned)local.wHour,
            (unsigned)local.wMinute, (unsigned)local.wSecond);
        // Name is the operator's selected campaign display label, not
        // the initial camera faction. Stable ID is never renamed later.
        std::ostringstream prefix;
        prefix << folder_slug(campaign_display_name.c_str()) << "-" << date_time;
        for (int attempt = 0; attempt < 1000; ++attempt) {
            campaign_id = prefix.str() + (attempt ? "-" + std::to_string(attempt + 1) : "");
            campaign_dir = "campaigns/" + campaign_id;
            if (make_dir(campaign_dir, false)) break;
            campaign_dir.clear();
        }
        campaign_created_utc = stamp;
    }
    if (campaign_dir.empty() || !make_dir(campaign_dir + "/sessions", joined)) {
        recorder_failure();
        return;
    }
    if (!joined && !write_campaign_metadata()) {
        recorder_failure();
        return;
    }
    // Existing sessions stay immutable. After a save/reload, create the
    // next available session-NNN; Save & Continue within one process remains
    // in the SAME session with explicit segment markers.
    std::string session_id;
    for (int candidate = 1; candidate <= 999; ++candidate) {
        char number[32] = {};
        snprintf(number, sizeof(number), "session-%03d", candidate);
        session_id = number;
        session_dir = campaign_dir + "/sessions/" + session_id;
        if (make_dir(session_dir, false)) break;
        session_dir.clear();
    }
    if (session_dir.empty()) {
        recorder_failure();
        return;
    }

    std::ostringstream manifest;
    manifest << "{\"schema\":1,\"session_id\":" << json_string(session_id.c_str())
             << ",\"campaign_id\":" << json_string(campaign_id.c_str())
             << ",\"started_utc\":" << json_string(stamp.c_str())
             << ",\"starting_turn\":" << *CurrentTurn
             << ",\"source_save\":";
    if (!source.empty()) {
        manifest << json_string(source.c_str());
    } else {
        manifest << "null";
    }
    manifest << ",\"autosim_manual_turns\":" << conf.autosim_manual_turns;
    manifest << ",\"archive_interval\":" << conf.autosim_archive_interval;
    manifest << ",\"timing_enabled\":" << (conf.autosim_timing_enabled ? "true" : "false");
    manifest << ",\"autosim_unlimited_expansion\":"
             << (conf.autosim_unlimited_expansion ? "true" : "false");
    manifest << ",\"autosim_suppress_notices\":" << conf.autosim_suppress_notices;
    manifest << ",\"mod_version\":" << json_string(MOD_VERSION)
             << ",\"mod_date\":" << json_string(MOD_DATE)
             << ",\"build_kind\":";
#ifdef BUILD_DEBUG
    manifest << "\"debug\"";
#elif defined(BUILD_REL)
    manifest << "\"release\"";
#else
    manifest << "\"develop\"";
#endif
    manifest << ",\"difficulty\":" << *DiffLevel
             << ",\"rules\":" << *GameRules
             << ",\"map_seed\":" << *MapRandomSeed
             << ",\"map_width\":" << *MapAreaX
             << ",\"map_height\":" << *MapAreaY
             << ",\"config_files\":{\"thinker.ini\":" << file_fingerprint("thinker.ini")
             << ",\"thinker_user.ini\":" << file_fingerprint("thinker_user.ini")
             << ",\"alphax.txt\":" << file_fingerprint("alphax.txt")
             << ",\"terranx.exe\":" << file_fingerprint("terranx.exe")
             << ",\"thinker.dll\":" << file_fingerprint("thinker.dll")
              << "},\"automation_launch\":"
              << (autosim_launcher_active ? "true" : "false")
              << ",\"automation_profile\":"
              << (autosim_launch_profile_path.empty()
                  ? "{\"present\":false}"
                  : file_fingerprint(autosim_launch_profile_path.c_str()))
              << ",\"factions\":[";
    for (int fc = 1; fc < MaxPlayerNum; ++fc) {
        if (fc > 1) manifest << ',';
        manifest << "{\"id\":" << fc << ",\"name\":"
                 << json_string(MFactions[fc].formal_name_faction)
                 << ",\"filename\":" << json_string(MFactions[fc].filename)
                 << ",\"search_key\":" << json_string(MFactions[fc].search_key) << "}";
    }
    manifest << "]}\n";
    if (!atomic_write(session_dir + "/manifest.json", manifest.str())) {
        recorder_failure();
        return;
    }
    turns_file = fopen((session_dir + "/turns.jsonl").c_str(), "wb");
    bases_file = fopen((session_dir + "/bases.jsonl").c_str(), "wb");
    ui_notices_file = fopen((session_dir + "/ui_notices.jsonl").c_str(), "wb");
    orbit_file = fopen((session_dir + "/orbit.jsonl").c_str(), "wb");
    council_file = fopen((session_dir + "/council.jsonl").c_str(), "wb");
    projects_file = fopen((session_dir + "/projects.jsonl").c_str(), "wb");
    timings_file = fopen((session_dir + "/timings.jsonl").c_str(), "wb");
    checkpoints_file = fopen((session_dir + "/checkpoints.jsonl").c_str(), "wb");
    production_file = fopen((session_dir + "/production.jsonl").c_str(), "wb");
    units_file = fopen((session_dir + "/units.jsonl").c_str(), "wb");
    combat_file = fopen((session_dir + "/combat.jsonl").c_str(), "wb");
    research_file = fopen((session_dir + "/research.jsonl").c_str(), "wb");
    if (!turns_file || !bases_file || !ui_notices_file || !orbit_file
    || !council_file || !projects_file || !timings_file || !checkpoints_file
    || !production_file || !units_file || !combat_file || !research_file) {
        recorder_failure();
        return;
    }
    recording = true;
    telemetry_unit_baseline();
    if (!recording) return;
    tel02_research_targets.resize(MaxPlayerNum, -1);
    for (int fc = 1; fc < MaxPlayerNum; ++fc)
        tel02_research_targets[fc] = Factions[fc].tech_research_id;
    // Existing queues are a baseline, not falsely dated project starts.
    for (int i = 0; i < *BaseCount; ++i) {
        const BASE& b = Bases[i];
        const int facility = -b.queue_items[0];
        if (b.faction_id > 0 && b.faction_id < MaxPlayerNum
        && facility >= SP_ID_First && facility <= SP_ID_Last
        && SecretProjects[facility - SP_ID_First] == SP_Unbuilt) {
            ProjectQueueState state;
            state.faction = b.faction_id;
            state.facility = facility;
            state.x = b.x;
            state.y = b.y;
            state.base_name = b.name;
            project_queues[std::make_pair((int)b.x, (int)b.y)] = state;
        }
    }
    if (!telemetry_orbit_snapshot("session_start")) return;
    if (!atomic_write(session_dir + "/status.json", status_json("recording"))) {
        recorder_failure();
    }
}

// Dedicated orbital event ledger. These functions only read already-computed
// game state and never call native AI routines or advance game RNG.
static std::ostringstream orbit_record(const char* event) {
    std::ostringstream out;
    out << "{\"schema\":1,\"seq\":" << ++orbit_event_seq
        << ",\"event\":" << json_string(event)
        << ",\"utc\":" << json_string(utc_stamp_ms().c_str())
        << ",\"turn\":" << *CurrentTurn
        << ",\"mission_year\":" << *CurrentMissionYear
        << ",\"processing_faction\":" << *CurrentFaction;
    return out;
}

static bool orbit_append(const std::string& line) {
    if (!recording || !orbit_file) return true;
    if (!append_line(orbit_file, line)) {
        recorder_failure();
        return false;
    }
    return true;
}

static const char* orbit_target_name(int target_type) {
    switch (target_type) {
    case 0: return "nutrient";
    case 1: return "mineral";
    case 2: return "energy";
    case 3: return "odp";
    case 4: return "geosync";
    default: return "unknown";
    }
}

static int orbit_geosync_at(int base_id) {
    return base_id >= 0 && base_id < *BaseCount
        && has_fac_built(FAC_GEOSYNC_SURVEY_POD, base_id) ? 1 : 0;
}

static bool telemetry_orbit_snapshot(const char* phase) {
    if (!recording || !orbit_file) return true;
    if (!orbit_snapshots_started) {
        // Avoid a large per-turn all-zero ledger before orbital development.
        // Only actual satellites activate it; ground-based GSP/Flechette
        // defences alone do not. Resumed late-game SAVs qualify immediately.
        for (int i = 1; i < MaxPlayerNum; ++i) {
            const Faction& f = Factions[i];
            if (f.satellites_nutrient > 0 || f.satellites_mineral > 0
            || f.satellites_energy > 0 || f.satellites_ODP > 0) {
                orbit_snapshots_started = true;
                break;
            }
        }
        if (!orbit_snapshots_started) return true;
    }
    int geosync[MaxPlayerNum] = {};
    int flechette[MaxPlayerNum] = {};
    for (int i = 0; i < *BaseCount; ++i) {
        const int faction = Bases[i].faction_id;
        if (faction > 0 && faction < MaxPlayerNum) {
            geosync[faction] += has_fac_built(FAC_GEOSYNC_SURVEY_POD, i) ? 1 : 0;
            flechette[faction] += has_fac_built(FAC_FLECHETTE_DEFENSE_SYS, i) ? 1 : 0;
        }
    }
    std::ostringstream out = orbit_record("inventory_snapshot");
    out << ",\"phase\":" << json_string(phase) << ",\"factions\":[";
    for (int i = 1; i < MaxPlayerNum; ++i) {
        if (i > 1) out << ',';
        const Faction& f = Factions[i];
        out << "{\"id\":" << i
            << ",\"nutrient\":" << f.satellites_nutrient
            << ",\"mineral\":" << f.satellites_mineral
            << ",\"energy\":" << f.satellites_energy
            << ",\"odp\":" << f.satellites_ODP
            << ",\"odp_deployed\":" << f.ODP_deployed
            << ",\"geosync\":" << geosync[i]
            << ",\"flechette\":" << flechette[i] << "}";
    }
    out << "]}";
    return orbit_append(out.str());
}

void telemetry_orbit_launch(int faction_id, int base_id, int facility_id, int before_count) {
    if (!recording || !orbit_file || faction_id <= 0 || faction_id >= MaxPlayerNum) return;
    int after_count = before_count;
    switch (facility_id) {
    case FAC_SKY_HYDRO_LAB: after_count = Factions[faction_id].satellites_nutrient; break;
    case FAC_NESSUS_MINING_STATION: after_count = Factions[faction_id].satellites_mineral; break;
    case FAC_ORBITAL_POWER_TRANS: after_count = Factions[faction_id].satellites_energy; break;
    case FAC_ORBITAL_DEFENSE_POD: after_count = Factions[faction_id].satellites_ODP; break;
    default: return;
    }
    std::ostringstream out = orbit_record("launch");
    out << ",\"faction\":" << faction_id
        << ",\"base_id\":" << base_id
        << ",\"facility_id\":" << facility_id
        << ",\"type\":" << json_string(
            facility_id == FAC_SKY_HYDRO_LAB ? "nutrient" :
            facility_id == FAC_NESSUS_MINING_STATION ? "mineral" :
            facility_id == FAC_ORBITAL_POWER_TRANS ? "energy" : "odp")
        << ",\"before\":" << before_count << ",\"after\":" << after_count;
    if (base_id >= 0 && base_id < *BaseCount) {
        const BASE& base = Bases[base_id];
        out << ",\"base_name\":" << json_string(base.name)
            << ",\"x\":" << base.x << ",\"y\":" << base.y;
    }
    out << "}";
    if (!orbit_append(out.str())) return;
    // Include a baseline at the actual first launch, even when a victory or
    // interruption occurs before the next completed-turn snapshot.
    if (!orbit_snapshots_started)
        telemetry_orbit_snapshot("first_launch");
}

OrbitAttackTrace telemetry_orbit_begin_attack(int attacker, int defender,
    int target_type, int base_id, int reserved_odp) {
    OrbitAttackTrace trace;
    if (!recording || !orbit_file) return trace;
    if (attacker <= 0 || attacker >= MaxPlayerNum
        || defender <= 0 || defender >= MaxPlayerNum) return trace;
    const Faction& atk = Factions[attacker];
    const Faction& def = Factions[defender];
    trace.id = ++orbit_attack_seq;
    trace.attacker_odp = atk.satellites_ODP;
    trace.attacker_deployed = atk.ODP_deployed;
    trace.target_nutrient = def.satellites_nutrient;
    trace.target_mineral = def.satellites_mineral;
    trace.target_energy = def.satellites_energy;
    trace.target_odp = def.satellites_ODP;
    trace.target_geosync = orbit_geosync_at(base_id);
    std::ostringstream out = orbit_record("attack_decision");
    out << ",\"attack_id\":" << trace.id
        << ",\"source\":\"enemy_strategy\""
        << ",\"attacker\":" << attacker << ",\"defender\":" << defender
        << ",\"target_type\":" << target_type
        << ",\"target_name\":" << json_string(orbit_target_name(target_type))
        << ",\"target_base_id\":" << base_id
        << ",\"reserve_required\":" << reserved_odp
        << ",\"attacker_odp\":" << trace.attacker_odp
        << ",\"attacker_deployed\":" << trace.attacker_deployed
        << ",\"defender_odp\":" << trace.target_odp
        << ",\"diplomatic_flags\":" << (unsigned)atk.diplo_status[defender]
        << ",\"defender_before\":{\"nutrient\":" << trace.target_nutrient
        << ",\"mineral\":" << trace.target_mineral
        << ",\"energy\":" << trace.target_energy
        << ",\"odp\":" << trace.target_odp
        << ",\"geosync\":" << trace.target_geosync << "}}";
    orbit_append(out.str());
    return trace;
}

void telemetry_orbit_end_attack(const OrbitAttackTrace& trace, int attacker,
    int defender, int target_type, int base_id) {
    if (trace.id < 0 || !recording || !orbit_file) return;
    const Faction& atk = Factions[attacker];
    const Faction& def = Factions[defender];
    int destroyed = 0;
    switch (target_type) {
    case 0: destroyed = trace.target_nutrient - def.satellites_nutrient; break;
    case 1: destroyed = trace.target_mineral - def.satellites_mineral; break;
    case 2: destroyed = trace.target_energy - def.satellites_energy; break;
    case 3: destroyed = trace.target_odp - def.satellites_ODP; break;
    case 4: destroyed = trace.target_geosync - orbit_geosync_at(base_id); break;
    }
    std::ostringstream out = orbit_record("attack_result");
    out << ",\"attack_id\":" << trace.id
        << ",\"attacker\":" << attacker << ",\"defender\":" << defender
        << ",\"target_type\":" << target_type
        << ",\"target_name\":" << json_string(orbit_target_name(target_type))
        << ",\"target_base_id\":" << base_id
        << ",\"result\":" << json_string(destroyed > 0 ? "target_destroyed" :
            (atk.satellites_ODP < trace.attacker_odp ? "attacker_odp_lost" :
            "no_confirmed_inventory_delta"))
        << ",\"result_basis\":\"inventory_delta\""
        << ",\"target_destroyed_count\":" << destroyed
        << ",\"attacker_odp_before\":" << trace.attacker_odp
        << ",\"attacker_odp_after\":" << atk.satellites_ODP
        << ",\"attacker_deployed_before\":" << trace.attacker_deployed
        << ",\"attacker_deployed_after\":" << atk.ODP_deployed
        << ",\"defender_after\":{\"nutrient\":" << def.satellites_nutrient
        << ",\"mineral\":" << def.satellites_mineral
        << ",\"energy\":" << def.satellites_energy
        << ",\"odp\":" << def.satellites_ODP
        << ",\"geosync\":" << orbit_geosync_at(base_id) << "}}";
    orbit_append(out.str());
}

void telemetry_orbit_environment_loss(int faction_id, const char* cause,
    int lost_mineral, int lost_energy, int lost_odp) {
    if (!recording || !orbit_file
        || (!lost_mineral && !lost_energy && !lost_odp)) return;
    std::ostringstream out = orbit_record("environmental_loss");
    out << ",\"faction\":" << faction_id
        << ",\"cause\":" << json_string(cause)
        << ",\"mineral_lost\":" << lost_mineral
        << ",\"energy_lost\":" << lost_energy
        << ",\"odp_lost\":" << lost_odp << "}";
    orbit_append(out.str());
}

void telemetry_record_ui_notice(const char* source, const char* label, const char* action) {
    // UI telemetry is a separate, append-only stream and does not modify
    // committed faction/base snapshots. No RNG or game decision functions.
    if (!recording || !ui_notices_file) {
        return;
    }
    std::ostringstream event;
    event << "{\"schema\":1,\"utc\":" << json_string(utc_stamp_ms().c_str())
          << ",\"turn\":" << *CurrentTurn
          << ",\"processing_faction\":" << *CurrentFaction
          << ",\"source\":" << json_string(source)
          << ",\"label\":" << json_string(label)
          << ",\"action\":" << json_string(action) << "}";
    if (!append_line(ui_notices_file, event.str())) {
        recorder_failure();
    }
}

// Automation-only Council presentation diagnostics. There is deliberately NO
// routine call_council/CouncWin_init stream and no Council UI information in
// council.jsonl. Native suppression hooks and decisions remain unchanged.
void telemetry_record_council_event(const char* phase, int actor, const char* detail) {
    if (!phase || !recording) return;
    if (strcmp(phase, "modal_fallback")
        && strcmp(phase, "human_only_offer_omitted")
        && strcmp(phase, "notice_suppressed")
        && strcmp(phase, "vote_offer_native_exit")) return;
    std::ostringstream note;
    note << "actor=" << actor << " " << (detail ? detail : "");
    telemetry_record_ui_notice("council", phase, note.str().c_str());
}

void telemetry_council_call_enter(int faction_id) {
    council_calling_faction = recording ? faction_id : 0;
}
void telemetry_council_call_leave() {
    council_calling_faction = 0;
}

static const char* council_proposal_name(int proposal) {
    switch (proposal) {
    case PROP_ELECT_PLANETARY_GOVERNOR: return "planetary_governor";
    case PROP_UNITE_SUPREME_LEADER: return "supreme_leader";
    case PROP_SALVAGE_UNITY_CORE: return "unity_core";
    case PROP_GLOBAL_TRADE_PACT: return "global_trade_pact";
    case PROP_REPEAL_GLOBAL_TRADE_PACT: return "repeal_global_trade_pact";
    case PROP_LAUNCH_SOLAR_SHADE: return "launch_solar_shade";
    case PROP_INCREASE_SOLAR_SHADE: return "increase_solar_shade";
    case PROP_MELT_POLAR_CAPS: return "melt_polar_caps";
    case PROP_REPEAL_UN_CHARTER: return "repeal_un_charter";
    case PROP_REINSTATE_UN_CHARTER: return "reinstate_un_charter";
    default: return "unrecognised";
    }
}

static std::ostringstream council_record(const char* type) {
    std::ostringstream row;
    row << "{\"schema\":1,\"event\":" << json_string(type)
        << ",\"utc\":" << json_string(utc_stamp_ms().c_str())
        << ",\"turn\":" << *CurrentTurn
        << ",\"mission_year\":" << *CurrentMissionYear
        << ",\"processing_faction\":" << *CurrentFaction
        << ",\"meeting_id\":" << json_string(
            (std::string(telemetry_get_session_id()) + "-council-" + std::to_string(council_active_meeting)).c_str());
    return row;
}
static void council_append(const std::string& value) {
    if (recording && council_file && !append_line(council_file, value))
        recorder_failure();
}

void telemetry_council_initialized(const CouncWindow* win, int arg1, int arg2) {
    if (!recording || !council_file || !win) return;
    const int proposal = win->proposal_type;
    if (proposal < PROP_ELECT_PLANETARY_GOVERNOR
    || proposal > PROP_REINSTATE_UN_CHARTER) return;
    ++council_meeting_seq;
    council_active_meeting = council_meeting_seq;
    council_active_window = win;
    council_result_written = false;
    std::ostringstream proposal_row = council_record("proposal");
    proposal_row << ",\"proposal_id\":" << proposal
        << ",\"proposal_name\":" << json_string(council_proposal_name(proposal))
        << ",\"candidate_faction\":" << win->candidate_faction_id
        // The call_council() argument is a verified caller, NOT necessarily
        // the sponsor of each motion. Do not manufacture a proposer.
        << ",\"convening_faction\":";
    if (council_calling_faction > 0 && council_calling_faction < MaxPlayerNum)
        proposal_row << council_calling_faction;
    else proposal_row << "null";
    proposal_row << ",\"proposer_faction\":null"
        << ",\"native_init_arg1\":" << arg1
        << ",\"native_init_arg2\":" << arg2
        << ",\"native_vote_mask\":" << (unsigned)win->vote_faction_mask
        << "}";
    council_append(proposal_row.str());
    if (!recording) return;
    std::ostringstream ballot_row = council_record("ballots");
    ballot_row << ",\"proposal_id\":" << proposal
        << ",\"native_vote_mask\":" << (unsigned)win->vote_faction_mask
        << ",\"ballots\":[";
    for (int fc = 1; fc < MaxPlayerNum; ++fc) {
        if (fc > 1) ballot_row << ',';
        ballot_row << "{\"faction\":" << fc
            // Native raw vote codes (including -3) are not yet decoded;
            // native_vote_weight_raw may be zero or modified in Council.
            << ",\"choice_raw\":" << win->faction_vote[fc]
            << ",\"weight_raw\":" << win->faction_vote_count[fc]
            << ",\"alive\":" << (is_alive(fc) ? "true" : "false")
            << ",\"human\":" << (is_human(fc) ? "true" : "false")
            << ",\"reason\":null,\"reason_status\":"
            << json_string(is_human(fc) ? "human_choice_not_instrumented"
                : "native_ai_reason_not_instrumented") << "}";
    }
    ballot_row << "]}";
    council_append(ballot_row.str());
}

void telemetry_council_resolved(const CouncWindow* win, int native_result,
    int previous_passes, int previous_governor, unsigned previous_game_state) {
    if (!recording || !council_file || !win
    || council_active_window != win || council_result_written
    || !council_active_meeting) return;
    // 0 is still incomplete, not proof of a failed motion.
    if (!win->proposal_outcome) return;
    const int proposal = win->proposal_type;
    if (proposal < PROP_ELECT_PLANETARY_GOVERNOR
    || proposal > PROP_REINSTATE_UN_CHARTER) return;
    council_result_written = true;
    std::ostringstream result = council_record("resolution");
    result << ",\"proposal_id\":" << proposal
        << ",\"proposal_name\":" << json_string(council_proposal_name(proposal))
        << ",\"outcome_raw\":" << win->proposal_outcome
        << ",\"native_modal_or_resolution_return\":" << native_result
        << ",\"candidate_faction\":" << win->candidate_faction_id
        << ",\"governor_before\":" << previous_governor
        << ",\"governor_after\":" << *GovernorFaction
        << ",\"pass_count_before\":" << previous_passes
        << ",\"pass_count_after\":" << ProposalPassCount[proposal]
        << ",\"game_state_before\":" << previous_game_state
        << ",\"game_state_after\":" << (unsigned)*GameState
        << ",\"victory_type\":" << *GameVictoryType
        << ",\"native_vote_mask\":" << (unsigned)win->vote_faction_mask
        << ",\"interpretation\":\"native_codes_not_decoded\"}";
    council_append(result.str());
}


// Secret Project state is tracked by map coordinates and faction, NOT by
// base-slot index (native base deletion compacts and reuses those slots).
// These events describe production transitions, not the AI's motivation.
static int project_facility_on_queue(const BASE& base) {
    const int facility = -base.queue_items[0];
    return base.faction_id > 0 && base.faction_id < MaxPlayerNum
        && facility >= SP_ID_First && facility <= SP_ID_Last
        && SecretProjects[facility - SP_ID_First] == SP_Unbuilt
        ? facility : 0;
}
static ProjectQueueState project_state(const BASE& base, int facility) {
    ProjectQueueState state;
    state.faction = base.faction_id;
    state.facility = facility;
    state.x = base.x;
    state.y = base.y;
    state.base_name = base.name;
    return state;
}
static void project_append(const char* event, const ProjectQueueState& state,
    int previous_facility, int next_facility, const char* source,
    const char* reason) {
    if (!recording || !projects_file || !state.facility) return;
    std::ostringstream out;
    out << "{\"schema\":1,\"event\":\"" << event << "\""
        << ",\"event_id\":" << json_string(
            (std::string(telemetry_get_session_id()) + "-project-"
                + std::to_string(++project_event_seq)).c_str())
        << ",\"utc\":" << json_string(utc_stamp_ms().c_str())
        << ",\"turn\":" << *CurrentTurn
        << ",\"mission_year\":" << *CurrentMissionYear
        << ",\"processing_faction\":" << *CurrentFaction
        << ",\"faction\":" << state.faction
        << ",\"project_id\":" << state.facility - SP_ID_First
        << ",\"facility_id\":" << state.facility
        << ",\"project_name\":" << json_string(Facility[state.facility].name)
        << ",\"base_name\":" << json_string(state.base_name.c_str())
        << ",\"base_x\":" << state.x << ",\"base_y\":" << state.y
        << ",\"previous_facility_id\":";
    if (previous_facility) out << previous_facility;
    else out << "null";
    out << ",\"next_facility_id\":";
    if (next_facility) out << next_facility;
    else out << "null";
    out << ",\"next_project_name\":";
    if (next_facility >= SP_ID_First && next_facility <= SP_ID_Last)
        out << json_string(Facility[next_facility].name);
    else out << "null";
    out << ",\"source\":" << json_string(source)
        << ",\"change_cause\":" << json_string(reason)
        << ",\"ai_reason\":null,\"reason_status\":\"not_instrumented\"}";
    if (!append_line(projects_file, out.str())) recorder_failure();
}
static void project_transition(const ProjectQueueState* previous,
    const ProjectQueueState* next, const char* source) {
    if (previous && next && previous->faction == next->faction
    && previous->facility == next->facility) return;
    if (previous && next && previous->faction != next->faction) {
        project_append("interrupted", *previous, previous->facility, 0,
            source, "base_owner_changed");
        project_append("started", *next, 0, next->facility,
            source, "new_owner_queue");
    } else if (previous && next) {
        // A switch is one event, not a second new project start.
        project_append("changed", *previous, previous->facility, next->facility,
            source, "production_queue_changed");
    } else if (previous) {
        const char* cause = SecretProjects[previous->facility - SP_ID_First] != SP_Unbuilt
            ? "project_no_longer_available" : "production_queue_replaced_or_base_lost";
        project_append("abandoned", *previous, previous->facility, 0, source, cause);
    } else if (next) {
        project_append("started", *next, 0, next->facility,
            source, "production_queue_selected");
    }
}
void telemetry_project_queue_changed(int base_id, int previous_item, int next_item) {
    if (!recording || !projects_file || base_id < 0 || base_id >= *BaseCount
    || previous_item == next_item) return;
    const BASE& base = Bases[base_id];
    const std::pair<int,int> key(base.x, base.y);
    const auto old = project_queues.find(key);
    ProjectQueueState previous;
    const ProjectQueueState* p = NULL;
    if (old != project_queues.end()) {
        previous = old->second;
        p = &previous;
    } else {
        const int old_facility = -previous_item;
        if (old_facility >= SP_ID_First && old_facility <= SP_ID_Last
        && SecretProjects[old_facility - SP_ID_First] == SP_Unbuilt) {
            previous = project_state(base, old_facility);
            p = &previous;
        }
    }
    const int facility = project_facility_on_queue(base);
    ProjectQueueState next = project_state(base, facility);
    project_transition(p, facility ? &next : NULL, "mod_base_change");
    if (!recording) return;
    if (facility) project_queues[key] = next;
    else project_queues.erase(key);
}
void telemetry_project_completed(int base_id, int facility_id) {
    if (!recording || !projects_file || base_id < 0 || base_id >= *BaseCount
    || facility_id < SP_ID_First || facility_id > SP_ID_Last) return;
    const BASE& base = Bases[base_id];
    const ProjectQueueState completed = project_state(base, facility_id);
    project_append("completed", completed, facility_id, 0,
        "mod_base_production", "native_secret_projects_assignment");
    project_queues.erase(std::make_pair((int)base.x, (int)base.y));
}
static void telemetry_project_reconcile_completed_turn() {
    if (!recording || !projects_file) return;
    std::map<std::pair<int,int>, ProjectQueueState> current;
    std::map<std::pair<int,int>, int> present_bases;
    for (int i = 0; i < *BaseCount; ++i) {
        const BASE& b = Bases[i];
        const std::pair<int,int> key(b.x, b.y);
        present_bases[key] = b.faction_id;
        const int facility = project_facility_on_queue(b);
        if (!facility) continue;
        current[key] = project_state(b, facility);
        const auto old = project_queues.find(key);
        project_transition(old == project_queues.end() ? NULL : &old->second,
            &current[key], "completed_turn_reconciliation");
        if (!recording) return;
    }
    for (const auto& previous : project_queues) {
        if (current.find(previous.first) == current.end()) {
            const auto city = present_bases.find(previous.first);
            const bool lost = city == present_bases.end();
            const bool captured = !lost && city->second != previous.second.faction;
            const char* reason = lost ? "base_removed" :
                captured ? "base_captured" :
                SecretProjects[previous.second.facility - SP_ID_First] != SP_Unbuilt
                ? "project_completed_elsewhere_or_unavailable" : "production_queue_replaced";
            project_append(lost || captured ? "interrupted" : "abandoned",
                previous.second, previous.second.facility, 0,
                "completed_turn_reconciliation", reason);
            if (!recording) return;
        }
    }
    project_queues.swap(current);
}

TelemetryPhaseStamp telemetry_phase_begin() {
    TelemetryPhaseStamp stamp;
    if (!recording || !timings_file || !conf.autosim_timing_enabled) return stamp;
    // Tick count measures actual elapsed wall time, including any blocking
    // UI/disk calls within the timed phase. It is not the CPU time of AI code.
    stamp.started_ticks = (unsigned long long)GetTickCount();
    SYSTEMTIME utc = {};
    GetSystemTime(&utc);
    snprintf(stamp.started_utc, sizeof(stamp.started_utc),
        "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        (unsigned)utc.wYear, (unsigned)utc.wMonth, (unsigned)utc.wDay,
        (unsigned)utc.wHour, (unsigned)utc.wMinute,
        (unsigned)utc.wSecond, (unsigned)utc.wMilliseconds);
    return stamp;
}

void telemetry_phase_end(const TelemetryPhaseStamp& stamp,
    const char* phase, int faction) {
    if (!recording || !timings_file || !stamp.started_utc[0]) return;
    const DWORD elapsed = GetTickCount() - (DWORD)stamp.started_ticks;
    std::ostringstream event;
    event << "{\"schema\":1,\"phase\":" << json_string(phase)
          << ",\"faction\":" << faction
          << ",\"turn\":" << *CurrentTurn
          << ",\"mission_year\":" << *CurrentMissionYear
          << ",\"observer\":" << (MapWin ? MapWin->cOwner : 0)
          << ",\"started_utc\":" << json_string(stamp.started_utc)
          << ",\"ended_utc\":" << json_string(utc_stamp_ms().c_str())
          << ",\"elapsed_ms\":" << (unsigned long)elapsed << "}";
    if (!append_line(timings_file, event.str())) recorder_failure();
}

// Only called at the same full-faction-cycle checkpoint at which turn/base
// telemetry is committed. Never enter the native SAV serializer mid-AI action,
// diplomacy callback, or after a game-ending control transition.
void telemetry_archive_completed_turn() {
    if (!recording || !checkpoints_file || !autosim_session_active()
    || conf.autosim_archive_interval <= 0 || *CurrentTurn < 1
    || *CurrentTurn != last_committed_turn
    || *CurrentTurn == last_checkpoint_turn
    || *CurrentTurn % conf.autosim_archive_interval
    || (*GameState & STATE_GAME_DONE))
        return;

    last_checkpoint_turn = *CurrentTurn;
    const TelemetryPhaseStamp timer = telemetry_phase_begin();
    const std::string folder = session_dir + "/saves";
    char filename[96] = {};
    snprintf(filename, sizeof(filename), "checkpoint-T%05d-MY%d.sav",
        *CurrentTurn, *CurrentMissionYear);
    const std::string target = folder + "/" + filename;
    const std::string temp = folder + "/.pending-" + filename;
    const char* outcome = "saved";
    if (*GameRules & RULES_IRONMAN) {
        // mod_save_daemon calls game_srand for Ironman: do not perturb AI RNG.
        outcome = "skipped_ironman_rng";
    } else if (!make_dir(folder, true)) {
        outcome = "failed_create_directory";
    } else if (GetFileAttributesA(target.c_str()) != INVALID_FILE_ATTRIBUTES) {
        outcome = "skipped_existing_checkpoint";
    } else {
        DeleteFileA(temp.c_str());
        if (mod_save_daemon(temp.c_str())) {
            outcome = "failed_serializer";
        } else {
            WIN32_FILE_ATTRIBUTE_DATA info = {};
            if (!GetFileAttributesExA(temp.c_str(), GetFileExInfoStandard, &info)
            || (info.nFileSizeHigh == 0 && info.nFileSizeLow < 4096)) {
                outcome = "failed_small_or_missing_save";
            } else if (!MoveFileExA(temp.c_str(), target.c_str(),
                MOVEFILE_WRITE_THROUGH)) {
                outcome = "failed_commit";
            } else if (!telemetry_record_named_save(target.c_str())) {
                // The actual SAV is valid enough for a file-level copy,
                // but report the missing campaign-link metadata distinctly.
                outcome = "saved_link_failed";
            }
        }
        DeleteFileA(temp.c_str());
    }
    std::ostringstream event;
    const bool saved = !strcmp(outcome, "saved") || !strcmp(outcome, "saved_link_failed");
    event << "{\"schema\":1,\"utc\":" << json_string(utc_stamp_ms().c_str())
          << ",\"turn\":" << *CurrentTurn
          << ",\"mission_year\":" << *CurrentMissionYear
          << ",\"kind\":\"completed_cycle\""
          << ",\"status\":" << json_string(outcome)
          << ",\"path\":" << (saved ? json_string(target.c_str()) : "null")
          << ",\"fingerprint\":"
          << (saved ? file_fingerprint(target.c_str()) : "null")
          << "}";
    if (!append_line(checkpoints_file, event.str())) recorder_failure();
    telemetry_phase_end(timer, "archive_save", 0);
}

void telemetry_archive_named_save(const char* saved_game_path) {
    if (!recording || !checkpoints_file || !saved_game_path || !saved_game_path[0])
        return;
    const std::string folder = session_dir + "/saves";
    const char* name = strrchr(saved_game_path, '/');
    const char* other = strrchr(saved_game_path, '\\');
    if (!name || (other && other > name)) name = other;
    name = name ? name + 1 : saved_game_path;
    // Save names are validated upstream by autosim_save_named, and never
    // interpreted as paths here. Only copy its existing basename.
    if (strstr(name, "..") || strchr(name, '/') || strchr(name, '\\'))
        return;

    char prefix[80] = {};
    snprintf(prefix, sizeof(prefix), "named-T%05d-MY%d-%lu-",
        *CurrentTurn, *CurrentMissionYear, (unsigned long)GetTickCount());
    const std::string destination = folder + "/" + prefix + name;
    const TelemetryPhaseStamp timer = telemetry_phase_begin();
    const char* result = "copied";
    if (!make_dir(folder, true)) {
        result = "failed_directory";
    } else if (!CopyFileA(saved_game_path, destination.c_str(), TRUE)) {
        result = "failed_copy";
    } else {
        const std::string sidecar = std::string(saved_game_path) + ".autosim-campaign";
        if (GetFileAttributesA(sidecar.c_str()) != INVALID_FILE_ATTRIBUTES) {
            const std::string target_sidecar = destination + ".autosim-campaign";
            if (!CopyFileA(sidecar.c_str(), target_sidecar.c_str(), TRUE))
                result = "copied_sidecar_failed";
        }
    }
    const bool copied = !strcmp(result, "copied") || !strcmp(result, "copied_sidecar_failed");
    std::ostringstream event;
    event << "{\"schema\":1,\"utc\":" << json_string(utc_stamp_ms().c_str())
          << ",\"turn\":" << *CurrentTurn
          << ",\"mission_year\":" << *CurrentMissionYear
          << ",\"kind\":\"operator_named\",\"status\":" << json_string(result)
          << ",\"source\":" << json_string(saved_game_path)
          << ",\"path\":" << (copied ? json_string(destination.c_str()) : "null")
          << ",\"fingerprint\":"
          << (copied ? file_fingerprint(destination.c_str()) : "null") << "}";
    if (!append_line(checkpoints_file, event.str())) recorder_failure();
    telemetry_phase_end(timer, "named_save_archive_copy", 0);
}

void telemetry_record_completed_turn() {
    if (!recording || *CurrentTurn <= last_committed_turn) {
        return;
    }
    const int turn = *CurrentTurn;
    int bases[MaxPlayerNum] = {};
    int population[MaxPlayerNum] = {};
    int units[MaxPlayerNum] = {};
    int native_units = 0;
    for (int i = 0; i < *BaseCount; ++i) {
        const BASE& base = Bases[i];
        if (base.faction_id > 0 && base.faction_id < MaxPlayerNum) {
            ++bases[base.faction_id];
            population[base.faction_id] += base.pop_size;
        }
    }
    for (int i = 0; i < *VehCount; ++i) {
        const int fc = Vehs[i].faction_id;
        if (fc == 0) {
            ++native_units;
        } else if (fc > 0 && fc < MaxPlayerNum) {
            ++units[fc];
        }
    }

    std::ostringstream turn_line;
    turn_line << "{\"schema\":1,\"phase\":\"completed_faction_cycle\""
              << ",\"turn\":" << turn
              << ",\"mission_year\":" << *CurrentMissionYear
              << ",\"world\":{\"map_width\":" << *MapAreaX
              << ",\"map_height\":" << *MapAreaY
              << ",\"sea_level\":" << *MapSeaLevel
              << ",\"base_count\":" << *BaseCount
              << ",\"vehicle_count\":" << *VehCount
              << ",\"native_vehicle_count\":" << native_units << "}"
              << ",\"factions\":[";
    for (int fc = 1; fc < MaxPlayerNum; ++fc) {
        if (fc > 1) turn_line << ',';
        const Faction& f = Factions[fc];
        turn_line << "{\"id\":" << fc
                  << ",\"name\":" << json_string(MFactions[fc].formal_name_faction)
                  << ",\"alive\":" << (is_alive(fc) ? "true" : "false")
                  << ",\"human\":" << (is_human(fc) ? "true" : "false")
                  << ",\"bases\":" << bases[fc]
                  << ",\"population\":" << population[fc]
                  << ",\"units\":" << units[fc]
                  << ",\"credits\":" << f.energy_credits
                  << ",\"energy_surplus\":" << f.energy_surplus_total
                  << ",\"nutrient_surplus\":" << f.nutrient_surplus_total
                  << ",\"labs\":" << f.labs_total
                  << ",\"maintenance\":" << f.facility_maint_total
                  << ",\"commerce\":" << f.turn_commerce_income
                  << ",\"research_target\":" << f.tech_research_id
                  << ",\"research_progress\":" << f.tech_accumulated
                  << ",\"research_cost_stored\":" << f.tech_cost
                  << ",\"se\":{\"politics\":" << f.SE_Politics
                  << ",\"economics\":" << f.SE_Economics
                  << ",\"values\":" << f.SE_Values
                  << ",\"future\":" << f.SE_Future
                  << ",\"economy\":" << f.SE_economy
                  << ",\"efficiency\":" << f.SE_effic
                  << ",\"support\":" << f.SE_support
                  << ",\"morale\":" << f.SE_morale
                  << ",\"police\":" << f.SE_police
                  << ",\"growth\":" << f.SE_growth
                  << ",\"planet\":" << f.SE_planet
                  << ",\"probe\":" << f.SE_probe
                  << ",\"industry\":" << f.SE_industry
                  << ",\"research\":" << f.SE_research << "}}";
    }
    turn_line << "]}";

    std::ostringstream base_line;
    base_line << "{\"schema\":1,\"phase\":\"completed_faction_cycle\""
              << ",\"turn\":" << turn << ",\"bases\":[";
    for (int i = 0; i < *BaseCount; ++i) {
        if (i) base_line << ',';
        const BASE& b = Bases[i];
        base_line << "{\"slot\":" << i
                  << ",\"faction\":" << (int)b.faction_id
                  << ",\"name\":" << json_string(b.name)
                  << ",\"x\":" << b.x << ",\"y\":" << b.y
                  << ",\"population\":" << (int)b.pop_size
                  << ",\"nutrient_intake\":" << b.nutrient_intake
                  << ",\"mineral_intake\":" << b.mineral_intake
                  << ",\"energy_intake\":" << b.energy_intake
                  << ",\"nutrient_surplus\":" << b.nutrient_surplus
                  << ",\"mineral_surplus\":" << b.mineral_surplus
                  << ",\"energy_surplus\":" << b.energy_surplus
                  << ",\"labs\":" << b.labs_total
                  << ",\"economy\":" << b.economy_total
                  << ",\"psych\":" << b.psych_total
                  << ",\"eco_damage\":" << b.eco_damage
                  << ",\"production_item\":" << b.queue_items[0]
                  << ",\"mineral_stock\":" << b.minerals_accumulated
                  << ",\"rioting\":" << ((b.state_flags & BSTATE_DRONE_RIOTS_ACTIVE) ? "true" : "false")
                  << ",\"golden_age\":" << ((b.state_flags & BSTATE_GOLDEN_AGE_ACTIVE) ? "true" : "false")
                  << "}";
    }
    base_line << "]}";

    // Only publish a committed turn in status.json after both data streams
    // have been flushed; partial lines/turns can then be diagnosed by readers.
    if (!append_line(turns_file, turn_line.str())
        || !append_line(bases_file, base_line.str())) {
        recorder_failure();
        return;
    }
    if (!telemetry_orbit_snapshot("completed_turn")) return;
    telemetry_project_reconcile_completed_turn();
    if (!recording) return;
    telemetry_research_reconcile();
    if (!recording) return;
    last_committed_turn = turn;
    if (!atomic_write(session_dir + "/status.json", status_json("recording"))) {
        recorder_failure();
    }
}

// AUTO-OBS: terminal state is not a completed full faction cycle. In
// particular, a Council or production victory may fire during one AI's
// upkeep. No score/vote calculators, RNG or native UI are invoked here.
void telemetry_record_terminal_result(const char* save_path, const char* save_status) {
    // Freeze BEFORE the post-victory control window, screenshots, saves
    // and operator deliberation. Later re-writes of result.json reuse it.
    telemetry_run_clock_set_active(false);
    if (!recording) return;
    // A later operator-requested save updates result.json for the SAME win,
    // but only the initial win emits an append-only history entry.
    const bool new_victory = !terminal_result_recorded;

    const int victory = *GameVictoryType;
    const char* reason = "unknown";
    switch (victory) {
    case VIC_UNIFY_SOLO:
    case VIC_UNIFY_COOP: reason = "conquest"; break;
    case VIC_DIPLOMATIC_SOLO:
    case VIC_DIPLOMATIC_COOP:
    case VIC_DIPLOMATIC_LOSS: reason = "diplomatic"; break;
    case VIC_ECONOMIC_SOLO:
    case VIC_ECONOMIC_COOP:
    case VIC_ECONOMIC_LOSS: reason = "economic"; break;
    case VIC_TRANSCEND_PLR:
    case VIC_TRANSCEND_UNK:
    case VIC_TRANSCEND_LOSS: reason = "transcendence"; break;
    case VIC_ALIEN_SOLO:
    case VIC_ALIEN_COOP:
    case VIC_ALIEN_LOSS: reason = "alien"; break;
    case VIC_TIME_LIMIT: reason = "time_limit"; break;
    case VIC_SUDDEN_DEATH: reason = "scenario_sudden_death"; break;
    case VIC_LOST_CAPTURE:
    case VIC_LOST_REMOVE: reason = "unexpected_observer_defeat"; break;
    default: break;
    }

    int alive_count = 0;
    int sole_survivor = 0;
    int base_counts[MaxPlayerNum] = {};
    int unit_counts[MaxPlayerNum] = {};
    for (int i = 0; i < *BaseCount; ++i) {
        const int faction = Bases[i].faction_id;
        if (faction >= 0 && faction < MaxPlayerNum) ++base_counts[faction];
    }
    for (int i = 0; i < *VehCount; ++i) {
        const int faction = Vehs[i].faction_id;
        if (faction >= 0 && faction < MaxPlayerNum) ++unit_counts[faction];
    }
    for (int i = 1; i < MaxPlayerNum; ++i) {
        if (is_alive(i)) { ++alive_count; sole_survivor = i; }
    }
    if (victory == VIC_NONE && alive_count == 0) reason = "extinction";

    std::vector<int> winners;
    const char* winner_basis = "unresolved";
    if (victory == VIC_UNIFY_SOLO || victory == VIC_UNIFY_COOP) {
        if (alive_count == 1) {
            winners.push_back(sole_survivor);
            winner_basis = "sole_living_faction";
        } else {
            // Cooperative/surrender victory can involve multiple living
            // factions; do not guess the leader from the camera or ranking.
            winner_basis = "coalition_unresolved";
        }
    } else if (victory == VIC_TRANSCEND_PLR || victory == VIC_TRANSCEND_UNK
        || victory == VIC_TRANSCEND_LOSS) {
        for (int i = 1; i < MaxPlayerNum; ++i) {
            if (Monuments[i].events[MON_WINNING_TRANS].achieved) winners.push_back(i);
        }
        if (!winners.empty()) winner_basis = "transcendence_monument";
    } else {
        unsigned win_flag = 0;
        if (victory == VIC_ECONOMIC_SOLO || victory == VIC_ECONOMIC_COOP
            || victory == VIC_ECONOMIC_LOSS) win_flag = PFLAG_UNK_40000;
        if (victory == VIC_ALIEN_SOLO || victory == VIC_ALIEN_COOP
            || victory == VIC_ALIEN_LOSS) win_flag = PFLAG_UNK_100000;
        if (victory == VIC_DIPLOMATIC_SOLO || victory == VIC_DIPLOMATIC_COOP
            || victory == VIC_DIPLOMATIC_LOSS) win_flag = PFLAG_UNK_20000;
        if (win_flag) {
            for (int i = 1; i < MaxPlayerNum; ++i) {
                if ((unsigned)Factions[i].player_flags & win_flag) winners.push_back(i);
            }
            if (!winners.empty()) winner_basis = "native_winner_flags";
        }
    }

    const char* status = victory == VIC_TIME_LIMIT ? "time_limit"
        : victory == VIC_NONE && alive_count == 0 ? "extinction"
        : victory == VIC_LOST_CAPTURE || victory == VIC_LOST_REMOVE
            ? "unexpected_observer_defeat"
        : victory == VIC_NONE ? "unknown_termination" : "victory";

    std::ostringstream result;
    result << "{\"schema\":1,\"status\":" << json_string(status)
           << ",\"reason\":" << json_string(reason)
           << ",\"victory_type_raw\":" << victory
           << ",\"game_state_raw\":" << (unsigned)*GameState
           << ",\"turn\":" << *CurrentTurn
           << ",\"mission_year\":" << *CurrentMissionYear
           << ",\"last_committed_turn\":" << last_committed_turn
           << ",\"simulation_elapsed_ms\":";
    if (simulation_clock_started) result << telemetry_run_elapsed_ms();
    else result << "null";
    result << ",\"simulation_time_basis\":\"active_ai_wall_time_excludes_operator_pauses\""
           << ",\"partial_cycle\":" << (*CurrentTurn > last_committed_turn ? "true" : "false")
           << ",\"initial_observer\":" << initial_observer
           << ",\"final_observer\":" << (MapWin ? MapWin->cOwner : 0)
           << ",\"human_mask\":" << (unsigned)FactionStatus[0]
           << ",\"alive_mask\":" << (unsigned)FactionStatus[1]
           << ",\"alive_count\":" << alive_count
           << ",\"winner_basis\":" << json_string(winner_basis)
           << ",\"primary_winner_factions\":[";
    for (size_t j = 0; j < winners.size(); ++j) {
        if (j) result << ',';
        result << winners[j];
    }
    result << "],\"terminal_save\":{\"status\":" << json_string(save_status)
           << ",\"path\":";
    if (save_path && save_path[0]) result << json_string(save_path);
    else result << "null";
    result << "},\"factions\":[";
    for (int i = 1; i < MaxPlayerNum; ++i) {
        if (i > 1) result << ',';
        const Faction& faction = Factions[i];
        result << "{\"id\":" << i
               << ",\"name\":" << json_string(MFactions[i].formal_name_faction)
               << ",\"alive\":" << (is_alive(i) ? "true" : "false")
               << ",\"bases\":" << base_counts[i]
               << ",\"units\":" << unit_counts[i]
               << ",\"credits\":" << faction.energy_credits
               << ",\"ranking\":" << faction.ranking
               << ",\"tech_ranking\":" << faction.tech_ranking
               << ",\"player_flags\":" << (unsigned)faction.player_flags
               << ",\"transcendence_event\":"
               << (Monuments[i].events[MON_WINNING_TRANS].achieved ? "true" : "false")
               << "}";
    }
    result << "]}";
    if (atomic_write(session_dir + "/result.json", result.str())) {
        if (new_victory) {
            // Persist the achievement before the operator decides whether
            // to continue playing. Each new post-victory win gets one event.
            FILE* history = fopen((session_dir + "/victories.jsonl").c_str(), "ab");
            if (history) {
                if (!append_line(history, result.str()))
                    OutputDebugStringA("thinkerNeca AUTO-OBS: victory history append failed\n");
                fclose(history);
            } else {
                OutputDebugStringA("thinkerNeca AUTO-OBS: victory history open failed\n");
            }
        }
        terminal_result_recorded = true;
        if (!atomic_write(session_dir + "/status.json", status_json("terminal"))) {
            OutputDebugStringA("thinkerNeca AUTO-OBS: terminal status write failed\n");
        }
    } else {
        OutputDebugStringA("thinkerNeca AUTO-OBS: terminal result write failed\n");
    }
}

void telemetry_resume_after_victory() {
    // An explicit continuation resumes only from the operator's choice;
    // the preceding victory-control pause never enters the duration.
    telemetry_run_clock_set_active(true);
    if (!recording) return;
    terminal_result_recorded = false;
    // result.json remains as the most recent victory; victories.jsonl is the
    // durable complete history. The active session itself is recording again.
    if (!atomic_write(session_dir + "/status.json", status_json("recording"))) {
        recorder_failure();
    }
}


// TEL-LOG-02. Emit only source-observed mutations; no RNG, AI recalculation,
// native UI or game-state writes. All four streams share a session event order.
static std::ostringstream tel02_row(const char* event) {
    std::ostringstream out;
    out << "{\"schema\":1,\"event\":" << json_string(event)
        << ",\"event_id\":" << json_string(
            (session_dir.empty() ? std::string("unknown") :
                std::string(telemetry_get_session_id()) + "-evt-"
                + std::to_string(++tel02_event_seq)).c_str())
        << ",\"utc\":" << json_string(utc_stamp_ms().c_str())
        << ",\"turn\":" << *CurrentTurn
        << ",\"mission_year\":" << *CurrentMissionYear
        << ",\"processing_faction\":" << *CurrentFaction;
    return out;
}
static void tel02_append(FILE* stream, const std::string& row) {
    if (recording && stream && !append_line(stream, row)) recorder_failure();
}
static int tel02_vehicle_serial(int slot) {
    return slot >= 0 && (size_t)slot < tel02_vehicle_ids.size()
        ? tel02_vehicle_ids[(size_t)slot] : 0;
}
static void tel02_add_vehicle(std::ostringstream& out, int slot, const char* prefix) {
    out << ",\"" << prefix << "_slot\":" << slot
        << ",\"" << prefix << "_instance\":";
    const int serial = tel02_vehicle_serial(slot);
    if (!serial) out << "null";
    else out << json_string(
        (std::string(telemetry_get_session_id()) + "-vehicle-"
            + std::to_string(serial)).c_str());
}
static void tel02_vehicle_event(const char* event, int slot, const char* cause,
    int old_owner = -1) {
    if (!recording || !units_file || slot < 0 || slot >= *VehCount) return;
    const VEH& veh = Vehs[slot];
    const int uid = veh.unit_id;
    std::ostringstream out = tel02_row(event);
    tel02_add_vehicle(out, slot, "vehicle");
    out << ",\"faction\":" << veh.faction_id
        << ",\"unit_id\":" << uid
        << ",\"unit_name\":" << json_string(
            uid >= 0 && uid < MaxProtoNum ? Units[uid].name : "")
        << ",\"x\":" << veh.x << ",\"y\":" << veh.y
        << ",\"morale\":" << veh.morale
        << ",\"damage_taken\":" << veh.damage_taken
        << ",\"home_base_slot\":" << veh.home_base_id
        << ",\"cause\":" << json_string(cause);
    if (old_owner >= 0) out << ",\"previous_faction\":" << old_owner;
    out << ",\"cause_status\":\"source_observed_or_unspecified\"}";
    tel02_append(units_file, out.str());
}
void telemetry_unit_baseline() {
    if (!recording || !units_file) return;
    tel02_vehicle_ids.clear();
    tel02_vehicle_next = 0;
    for (int i = 0; i < *VehCount && recording; ++i) {
        tel02_vehicle_ids.push_back(++tel02_vehicle_next);
        tel02_vehicle_event("session_baseline", i, "existing_at_session_start");
    }
}
void telemetry_unit_created(int slot) {
    if (!recording || slot < 0 || slot >= *VehCount) return;
    if ((size_t)slot > tel02_vehicle_ids.size())
        tel02_vehicle_ids.resize((size_t)slot, 0);
    if ((size_t)slot == tel02_vehicle_ids.size())
        tel02_vehicle_ids.push_back(++tel02_vehicle_next);
    else tel02_vehicle_ids[(size_t)slot] = ++tel02_vehicle_next;
    tel02_vehicle_event("created", slot, "veh_init");
}
void telemetry_unit_destroyed(int slot) {
    if (!recording || slot < 0 || slot >= *VehCount) return;
    tel02_vehicle_event("removed", slot, "veh_kill_reason_unknown");
    if ((size_t)slot < tel02_vehicle_ids.size())
        tel02_vehicle_ids.erase(tel02_vehicle_ids.begin() + slot);
}
void telemetry_unit_owner_changed(int slot, int old_owner, int new_owner, const char* cause) {
    if (!recording || slot < 0 || slot >= *VehCount) return;
    // At the hook, the native owner has already been assigned.
    if (Vehs[slot].faction_id != new_owner || old_owner == new_owner) return;
    tel02_vehicle_event("owner_changed", slot, cause, old_owner);
}
void telemetry_production_queue_changed(int base_id, int old_item, int new_item,
    int minerals_after) {
    if (!recording || !production_file || base_id < 0 || base_id >= *BaseCount
        || old_item == new_item) return;
    const BASE& b = Bases[base_id];
    std::ostringstream out = tel02_row("queue_changed");
    out << ",\"faction\":" << b.faction_id
        << ",\"base_slot\":" << base_id << ",\"base_name\":" << json_string(b.name)
        << ",\"base_x\":" << b.x << ",\"base_y\":" << b.y
        << ",\"previous_item\":" << old_item << ",\"new_item\":" << new_item
        << ",\"minerals_after\":" << minerals_after
        << ",\"source\":\"mod_base_change\",\"ai_reason\":null"
        << ",\"reason_status\":\"not_instrumented\"}";
    tel02_append(production_file, out.str());
}
void telemetry_production_unit(int base_id, int unit_id, int slot, int cost) {
    if (!recording || !production_file || base_id < 0 || base_id >= *BaseCount
        || slot < 0 || slot >= *VehCount) return;
    const BASE& b = Bases[base_id];
    std::ostringstream out = tel02_row("unit_completed");
    out << ",\"faction\":" << b.faction_id
        << ",\"base_slot\":" << base_id << ",\"base_name\":" << json_string(b.name)
        << ",\"base_x\":" << b.x << ",\"base_y\":" << b.y
        << ",\"unit_id\":" << unit_id
        << ",\"unit_name\":" << json_string(Units[unit_id].name)
        << ",\"mineral_cost\":" << cost
        << ",\"minerals_after\":" << b.minerals_accumulated
        << ",\"actual_morale\":" << Vehs[slot].morale;
    tel02_add_vehicle(out, slot, "vehicle");
    out << ",\"source\":\"mod_base_production\"}";
    tel02_append(production_file, out.str());
}
void telemetry_production_facility(int base_id, int facility_id, int cost) {
    if (!recording || !production_file || base_id < 0 || base_id >= *BaseCount
        || facility_id <= 0 || facility_id > FAC_EMPTY_SP_64) return;
    const BASE& b = Bases[base_id];
    const bool satellite = facility_id >= FAC_SKY_HYDRO_LAB
        && facility_id <= FAC_ORBITAL_DEFENSE_POD;
    const char* event = facility_id >= SP_ID_First ? "project_completed"
        : satellite ? "satellite_completed" : "facility_completed";
    std::ostringstream out = tel02_row(event);
    out << ",\"faction\":" << b.faction_id
        << ",\"base_slot\":" << base_id << ",\"base_name\":" << json_string(b.name)
        << ",\"base_x\":" << b.x << ",\"base_y\":" << b.y
        << ",\"facility_id\":" << facility_id
        << ",\"facility_name\":" << json_string(Facility[facility_id].name)
        << ",\"mineral_cost\":" << cost
        << ",\"minerals_after\":" << b.minerals_accumulated
        << ",\"source\":\"mod_base_production\"}";
    tel02_append(production_file, out.str());
}
void telemetry_research_acquired(int faction, int tech_id, int source,
    bool already_owned, int is_share) {
    if (!recording || !research_file || faction <= 0 || faction >= MaxPlayerNum
        || tech_id < 0 || tech_id >= Tech_ID_Commlink) return;
    std::ostringstream out = tel02_row("tech_acquired");
    out << ",\"faction\":" << faction
        << ",\"tech_id\":" << tech_id
        << ",\"tech_name\":" << json_string(Tech[tech_id].name)
        << ",\"source_faction\":" << source
        << ",\"source_status\":\"native_tech_achieved_argument\""
        << ",\"shared\":" << (is_share ? "true" : "false")
        << ",\"previously_owned\":" << (already_owned ? "true" : "false")
        << ",\"acquisition_kind\":" << json_string(
            source > 0 ? "other_faction" : source == 0 ? "research_or_free"
            : source == -1 ? "planetary_datalinks" : source == -2
            ? "techshare" : "native_internal")
        << "}";
    tel02_append(research_file, out.str());
}
void telemetry_research_reconcile() {
    if (!recording || !research_file
        || tel02_research_targets.size() != MaxPlayerNum) return;
    for (int faction = 1; faction < MaxPlayerNum && recording; ++faction) {
        const int current = Factions[faction].tech_research_id;
        if (current == tel02_research_targets[faction]) continue;
        std::ostringstream out = tel02_row("target_observed");
        out << ",\"faction\":" << faction
            << ",\"previous_target\":" << tel02_research_targets[faction]
            << ",\"new_target\":" << current
            << ",\"timing_basis\":\"completed_faction_cycle\""
            << ",\"ai_reason\":null,\"reason_status\":\"not_instrumented\"}";
        tel02_append(research_file, out.str());
        tel02_research_targets[faction] = current;
    }
}
void telemetry_combat_exchange(int attacker, int defender, const char* mode,
    int type_flags, int attack_strength, int defend_strength,
    int attacker_hp_before, int defender_hp_before,
    int attacker_hp_after, int defender_hp_after,
    bool retreated, bool marine_detached) {
    if (!recording || !combat_file || attacker < 0 || defender < 0
        || attacker >= *VehCount || defender >= *VehCount) return;
    const VEH& atk = Vehs[attacker];
    const VEH& def = Vehs[defender];
    std::ostringstream out = tel02_row("hp_exchange");
    out << ",\"mode\":" << json_string(mode)
        << ",\"combat_type_flags\":" << type_flags
        << ",\"attacker_faction\":" << atk.faction_id
        << ",\"defender_faction\":" << def.faction_id
        << ",\"attacker_unit_id\":" << atk.unit_id
        << ",\"defender_unit_id\":" << def.unit_id
        << ",\"attacker_reactor\":" << atk.reactor_type()
        << ",\"defender_reactor\":" << def.reactor_type()
        << ",\"target_x\":" << def.x << ",\"target_y\":" << def.y
        << ",\"offense_final_raw\":" << attack_strength
        << ",\"defense_final_raw\":" << defend_strength
        << ",\"attacker_hp_before\":" << attacker_hp_before
        << ",\"attacker_hp_after\":" << attacker_hp_after
        << ",\"defender_hp_before\":" << defender_hp_before
        << ",\"defender_hp_after\":" << defender_hp_after
        << ",\"retreated\":" << (retreated ? "true" : "false")
        << ",\"marine_detachment\":" << (marine_detached ? "true" : "false")
        << ",\"result_scope\":\"hp_exchange_before_postcombat_cleanup\"";
    tel02_add_vehicle(out, attacker, "attacker");
    tel02_add_vehicle(out, defender, "defender");
    out << "}";
    tel02_append(combat_file, out.str());
}

void telemetry_end_session() {
    telemetry_run_clock_set_active(false);
    if (recording) {
        if (!atomic_write(session_dir + "/status.json",
            status_json(terminal_result_recorded ? "terminal" : "closed"))) {
            OutputDebugStringA("thinkerNeca TEL-001: failed to finalise status.json\n");
        }
    }
    recording = false;
    close_files();
}


bool telemetry_resumed_linked_campaign() {
    return resumed_linked_campaign;
}

const char* telemetry_get_campaign_id() {
    return campaign_id.c_str();
}

const char* telemetry_get_session_id() {
    // Keep the filesystem identity immutable across display-name edits.
    static std::string session_id;
    const size_t slash = session_dir.find_last_of("/\\\\");
    session_id = session_dir.empty() ? "" :
        session_dir.substr(slash == std::string::npos ? 0 : slash + 1);
    return session_id.c_str();
}

const char* telemetry_get_campaign_name() {
    if (campaign_display_name.empty())
        campaign_display_name = readable_campaign_default();
    return campaign_display_name.c_str();
}

bool telemetry_set_campaign_name(const char* name) {
    if (!name || !name[0] || strlen(name) > 88) return false;
    for (const unsigned char* p = (const unsigned char*)name; *p; ++p)
        if (*p < 32 || *p == 127) return false;
    const std::string previous = campaign_display_name;
    campaign_display_name = name;
    if (!write_campaign_metadata()) {
        campaign_display_name = previous;
        return false;
    }
    return true;
}

bool telemetry_record_named_save(const char* save_path) {
    if (!save_path || !save_path[0]) return false;
    const std::string path = std::string(save_path) + ".autosim-campaign";
    if (campaign_id.empty()) {
        // Telemetry disabled: do not leave a prior save's stale campaign link
        // attached after the native serializer has overwritten its bytes.
        DeleteFileA(path.c_str());
        return true;
    }
    const std::string fingerprint = file_fingerprint(save_path);
    if (fingerprint.find("{\"present\":true,") != 0
    || fingerprint.find("read_error") != std::string::npos) {
        DeleteFileA(path.c_str());
        return false;
    }
    char saved_options[96] = {};
    snprintf(saved_options, sizeof(saved_options), "autosim-v1 %d %d %d\n",
        conf.autosim_manual_turns,
        conf.autosim_suppress_notices ? 1 : 0,
        conf.autosim_fast_combat ? 1 : 0);
    const std::string contents = "thinkerNeca-campaign-v2\n" + campaign_id
        + "\n" + campaign_created_utc + "\n" + campaign_display_name
        + "\n" + fingerprint + "\n" + saved_options;
    if (!atomic_write(path, contents)) {
        DeleteFileA(path.c_str());
        OutputDebugStringA("thinkerNeca TEL-001: failed to write named-save campaign link\n");
        return false;
    }
    return true;
}

void telemetry_note_loaded_save(const char* source_path) {
    last_loaded_save = source_path ? source_path : "";
}
