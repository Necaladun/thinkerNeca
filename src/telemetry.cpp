#include "telemetry.h"
#include "main.h"

#include <iomanip>
#include <sstream>

// TEL-001 is an independent, opt-in file writer. Keep all game-state reads in
// telemetry_record_completed_turn() and never evaluate game RNG or AI functions.
namespace {

FILE* turns_file = NULL;
FILE* bases_file = NULL;
FILE* ui_notices_file = NULL;
FILE* council_file = NULL;
FILE* timings_file = NULL;
FILE* checkpoints_file = NULL;
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
    if (council_file) {
        fclose(council_file);
        council_file = NULL;
    }
    if (timings_file) {
        fclose(timings_file);
        timings_file = NULL;
    }
    if (checkpoints_file) {
        fclose(checkpoints_file);
        checkpoints_file = NULL;
    }
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

void telemetry_begin_session(const char* source_save) {
    telemetry_end_session();
    last_committed_turn = -1;
    last_checkpoint_turn = -1;
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
        char day[16] = {};
        snprintf(day, sizeof(day), "%04u%02u%02u", (unsigned)local.wYear,
            (unsigned)local.wMonth, (unsigned)local.wDay);
        std::ostringstream prefix;
        prefix << "AI-Sim-" << folder_slug(MFactions[MapWin->cOwner].formal_name_faction)
               << "-MY" << *CurrentMissionYear << "-" << day;
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
    council_file = fopen((session_dir + "/council.jsonl").c_str(), "wb");
    timings_file = fopen((session_dir + "/timings.jsonl").c_str(), "wb");
    checkpoints_file = fopen((session_dir + "/checkpoints.jsonl").c_str(), "wb");
    if (!turns_file || !bases_file || !ui_notices_file || !council_file
    || !timings_file || !checkpoints_file) {
        recorder_failure();
        return;
    }
    recording = true;
    if (!atomic_write(session_dir + "/status.json", status_json("recording"))) {
        recorder_failure();
    }
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

// Dedicated flushed Council trace for diagnosing stalls and election parity.
// This only reads existing game state; it never evaluates votes or RNG.
void telemetry_record_council_event(const char* phase, int actor, const char* detail) {
    if (!recording || !council_file) return;
    const int owner = MapWin ? MapWin->cOwner : 0;
    std::ostringstream event;
    event << "{\"schema\":1,\"utc\":" << json_string(utc_stamp_ms().c_str())
          << ",\"turn\":" << *CurrentTurn
          << ",\"mission_year\":" << *CurrentMissionYear
          << ",\"processing_faction\":" << *CurrentFaction
          << ",\"actor\":" << actor
          << ",\"observer\":" << owner
          << ",\"human_mask\":" << (unsigned)FactionStatus[0]
          << ",\"alive_mask\":" << (unsigned)FactionStatus[1]
          << ",\"pending\":" << *CouncilSessionPending
          << ",\"governor\":" << *GovernorFaction
          << ",\"phase\":" << json_string(phase)
          << ",\"detail\":" << json_string(detail)
          << ",\"observer_proposal\":" << ((owner > 0 && owner < MaxPlayerNum) ? CouncilProposal[owner] : -1)
          << ",\"observer_vote_state\":" << ((owner > 0 && owner < MaxPlayerNum) ? CouncilVoteState[owner] : -1)
          << ",\"energy\":[";
    for (int i = 0; i < MaxPlayerNum; ++i) {
        if (i) event << ',';
        event << Factions[i].energy_credits;
    }
    event << "]}";
    if (!append_line(council_file, event.str())) recorder_failure();
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
          << ",\"kind\":\"completed_cycle\"
          << ",\"status\":" << json_string(outcome)
          << ",\"path\":" << (saved ? json_string(target.c_str()) : "null")
          << ",\"fingerprint\":"
          << (saved ? file_fingerprint(target.c_str()) : "null")
          << "}";
    if (!append_line(checkpoints_file, event.str())) recorder_failure();
    telemetry_phase_end(timer, "archive_save", 0);
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
    last_committed_turn = turn;
    if (!atomic_write(session_dir + "/status.json", status_json("recording"))) {
        recorder_failure();
    }
}

// AUTO-OBS: terminal state is not a completed full faction cycle. In
// particular, a Council or production victory may fire during one AI's
// upkeep. No score/vote calculators, RNG or native UI are invoked here.
void telemetry_record_terminal_result(const char* save_path, const char* save_status) {
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
    if (!recording) return;
    terminal_result_recorded = false;
    // result.json remains as the most recent victory; victories.jsonl is the
    // durable complete history. The active session itself is recording again.
    if (!atomic_write(session_dir + "/status.json", status_json("recording"))) {
        recorder_failure();
    }
}

void telemetry_end_session() {
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
