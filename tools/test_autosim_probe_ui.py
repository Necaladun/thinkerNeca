#!/usr/bin/env python3
"""Structural contract for AUTO-002 probe UI classification.

This checks source routing, not native SMAC/X runtime behaviour.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
probe = (ROOT / "src/probe.cpp").read_text(encoding="utf-8")
action = (ROOT / "src/veh_action.cpp").read_text(encoding="utf-8")

def require(condition, message):
    if not condition:
        raise AssertionError(message)

require("NetMsg_pop(NetMsg," not in probe,
        "probe.cpp still has raw return-ignored NetMsg_pop output")
require(probe.count("autosim_notice_netmsg(NetMsg,") == 17,
        "unexpected count of individually audited NetMsg probe result sites")
for label in ("SPYFOUND", "SPYLOST"):
    require(f'autosim_notice_popp("modmenu", "{label}"' in probe,
            f"{label} is not routed through passive notice policy")
require(probe.count("probe_notice_popp2(StrBuffer, infil_img);") == 2,
        "genetic/mind control result popup is unclassified")
require(probe.count("probe_notice_portrait(") == 6,
        "post-operation portrait popup is unclassified")
require('telemetry_record_ui_notice("probe_intelligence"' in probe,
        "intelligence report requires a guarded observer presentation")
require("report_intel(tgt_fc_id);" in probe,
        "native report_intel must remain for normal games")
for label in ("PACTFRAMEEXCUSE", "FRAMEEXCUSE"):
    require(probe.count(f'choice = popp_2("{label}"') == 2,
            f"{label} must continue to return gameplay choices")
for label in ("PROBEPACTTEAM", "PROBEPACT", "ENEMYPROBE2"):
    require(f'"{label}"' in action, f"missing classified {label} path")
require(action.count('"use_unobserved_ai_path"') == 2,
        "AI observer choice prompts must use preexisting nonobserved path")
for label in ("GOTMYPROBE", "GOTYOURPROBE"):
    require(f'autosim_notice_netmsg(NetMsg, "{label}"' in action,
            f"{label} must not block as an information message")
require('choice = popp(ScriptFile, "ENEMYPROBE"' in action,
        "actual human probe decision path must remain")
print("AUTO-002 probe source contract passed (not a Windows runtime test)")
