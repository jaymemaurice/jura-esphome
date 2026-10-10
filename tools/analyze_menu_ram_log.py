#!/usr/bin/env python3
"""Analyze v22.19.3 MENU_RAM logs off-device.

Reconstructs each 256-byte menu snapshot from the COFFEE_READY baseline plus
per-state MENU_RAM_PAGE diffs. With repeated samples, it ranks stable bytes that
discriminate each menu state from the others. This intentionally runs off the
ESP8266 so discovery itself has no growing state database.
"""
from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

BASE_RE = re.compile(r"MENU_RAM_BASELINE_PAGE: page=0x([0-9A-Fa-f]{2}) data=([0-9A-Fa-f]{32})")
BEGIN_RE = re.compile(r"MENU_RAM_CAPTURE_BEGIN: .*?state=([^ ]+) ")
PAGE_RE = re.compile(r"MENU_RAM_PAGE: .*?state=([^ ]+) page=0x([0-9A-Fa-f]{2}) changes=([^\r\n]+)")
STATE_RE = re.compile(r"MENU_RAM_STATE: .*?state=([^ ]+) .*?changes=(\d+)")
VALUE_PAGE_RE = re.compile(r"MENU_VALUE_RAM_PAGE: .*?state=([^ ]+) phase=(PLUS|RESTORE) page=0x([0-9A-Fa-f]{2}) changes=([^\r\n]+)")
VALUE_STATE_RE = re.compile(r"MENU_VALUE_RAM_STATE: .*?state=([^ ]+) phase=(PLUS|RESTORE) changes=(\d+)")
CHANGE_RE = re.compile(r"([0-9A-Fa-f]{2}):([0-9A-Fa-f]{2})>([0-9A-Fa-f]{2})")
OBS_RE = re.compile(r'OPERATOR_OBSERVATION: operation=([^ ]+) step=(\d+) state=([^ ]+) note="(.*)"')


def parse_logs_extended(paths: list[Path]):
    samples: dict[str, list[bytes]] = defaultdict(list)
    value_samples: dict[str, dict[str, list[bytes]]] = defaultdict(lambda: defaultdict(list))
    baselines: list[bytes] = []
    for path in paths:
        baseline = bytearray(256)
        baseline_pages = set()
        current_state = None
        current = None
        latest_state_snapshot: dict[str, bytes] = {}
        probe_state = None
        probe_phase = None
        probe_current = None
        for line in path.read_text(errors="replace").splitlines():
            m = BASE_RE.search(line)
            if m:
                page = int(m.group(1), 16)
                data = bytes.fromhex(m.group(2))
                baseline[page:page + 16] = data
                baseline_pages.add(page)
                if len(baseline_pages) == 16:
                    baselines.append(bytes(baseline))
                continue
            m = BEGIN_RE.search(line)
            if m and len(baseline_pages) == 16:
                current_state = m.group(1)
                current = bytearray(baseline)
                continue
            m = PAGE_RE.search(line)
            if m and current is not None:
                state = m.group(1)
                if state != current_state:
                    continue
                for cm in CHANGE_RE.finditer(m.group(3)):
                    offset = int(cm.group(1), 16)
                    current[offset] = int(cm.group(3), 16)
                continue
            m = STATE_RE.search(line)
            if m and current is not None and m.group(1) == current_state:
                snapshot = bytes(current)
                samples[current_state].append(snapshot)
                latest_state_snapshot[current_state] = snapshot
                current_state = None
                current = None
                continue
            m = VALUE_PAGE_RE.search(line)
            if m:
                state, phase = m.group(1), m.group(2)
                baseline_state = latest_state_snapshot.get(state)
                if baseline_state is None:
                    continue
                if probe_state != state or probe_phase != phase or probe_current is None:
                    probe_state, probe_phase = state, phase
                    probe_current = bytearray(baseline_state)
                for cm in CHANGE_RE.finditer(m.group(4)):
                    offset = int(cm.group(1), 16)
                    probe_current[offset] = int(cm.group(3), 16)
                continue
            m = VALUE_STATE_RE.search(line)
            if m and probe_current is not None and m.group(1) == probe_state and m.group(2) == probe_phase:
                value_samples[probe_state][probe_phase].append(bytes(probe_current))
                probe_state = None
                probe_phase = None
                probe_current = None
    return samples, baselines, value_samples


def parse_logs(paths: list[Path]):
    """Backward-compatible v22.19 API: return menu-state samples and baselines.

    Use parse_logs_extended() when editable-value probe samples are needed.
    """
    samples, baselines, _ = parse_logs_extended(paths)
    return samples, baselines



def parse_observations(paths: list[Path]):
    """Return operator-entered notes without changing the legacy parser APIs."""
    observations = []
    for path in paths:
        for line in path.read_text(errors="replace").splitlines():
            m = OBS_RE.search(line)
            if not m:
                continue
            observations.append({
                "operation": m.group(1),
                "step": int(m.group(2)),
                "state": m.group(3),
                "note": m.group(4),
                "source": str(path),
            })
    return observations

def stable_values(samples: dict[str, list[bytes]]):
    stable: dict[str, dict[int, int]] = {}
    for state, ss in samples.items():
        vals = {}
        if not ss:
            stable[state] = vals
            continue
        for offset in range(256):
            v = ss[0][offset]
            if all(s[offset] == v for s in ss[1:]):
                vals[offset] = v
        stable[state] = vals
    return stable


def rank_candidates(samples: dict[str, list[bytes]], stable: dict[str, dict[int, int]]):
    states = sorted(samples)
    result = {}
    for state in states:
        candidates = []
        target = stable[state]
        for offset, value in target.items():
            distinguished = 0
            comparable = 0
            for other in states:
                if other == state or offset not in stable[other]:
                    continue
                comparable += 1
                if stable[other][offset] != value:
                    distinguished += 1
            if distinguished:
                candidates.append({
                    "offset": offset,
                    "value": value,
                    "distinguished": distinguished,
                    "comparable": comparable,
                })
        candidates.sort(key=lambda x: (-x["distinguished"], -x["comparable"], x["offset"]))
        result[state] = candidates
    return result


def greedy_signature(state: str, states: list[str], stable: dict[str, dict[int, int]], ranked, max_terms=4):
    remaining = {s for s in states if s != state}
    terms = []
    for cand in ranked[state]:
        off, val = cand["offset"], cand["value"]
        eliminated = {o for o in remaining if off in stable[o] and stable[o][off] != val}
        if not eliminated:
            continue
        terms.append({"offset": off, "mask": 0xFF, "value": val, "eliminates": len(eliminated)})
        remaining -= eliminated
        if not remaining or len(terms) >= max_terms:
            break
    return terms, sorted(remaining)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logs", nargs="+", type=Path)
    ap.add_argument("--json", type=Path, help="write machine-readable analysis")
    ap.add_argument("--top", type=int, default=8, help="candidate bytes to print per state")
    args = ap.parse_args()

    samples, baselines, value_samples = parse_logs_extended(args.logs)
    if not samples:
        raise SystemExit("No complete v22.19 MENU_RAM state captures found")
    stable = stable_values(samples)
    ranked = rank_candidates(samples, stable)
    states = sorted(samples)

    observations = parse_observations(args.logs)
    report = {"states": {}, "baseline_count": len(baselines), "value_probes": {}, "observations": observations}
    for state in states:
        terms, unresolved = greedy_signature(state, states, stable, ranked)
        report["states"][state] = {
            "samples": len(samples[state]),
            "stable_bytes": len(stable[state]),
            "signature_terms": terms,
            "unresolved_states": unresolved,
            "top_candidates": ranked[state][:args.top],
        }
        confidence = "REPEATED" if len(samples[state]) >= 2 else "SINGLE-SAMPLE"
        print(f"\n{state}: samples={len(samples[state])} ({confidence}) stable_bytes={len(stable[state])}")
        if terms:
            print("  greedy signature:", ", ".join(
                f"0x{t['offset']:02X}=0x{t['value']:02X}" for t in terms))
        else:
            print("  greedy signature: none yet")
        if unresolved:
            print(f"  still not distinguished from {len(unresolved)} state(s)")
        for c in ranked[state][:args.top]:
            print(f"  candidate 0x{c['offset']:02X}=0x{c['value']:02X} "
                  f"distinguishes {c['distinguished']}/{c['comparable']} comparable states")


    if value_samples:
        print("\nEditable-value probes:")
        for state in sorted(value_samples):
            plus_list = value_samples[state].get("PLUS", [])
            restore_list = value_samples[state].get("RESTORE", [])
            base_list = samples.get(state, [])
            count = min(len(plus_list), len(restore_list), len(base_list))
            entries = []
            for i in range(count):
                base, plus, restore = base_list[-count + i], plus_list[i], restore_list[i]
                plus_changes = [off for off in range(256) if plus[off] != base[off]]
                restored = [off for off in plus_changes if restore[off] == base[off]]
                entries.append({
                    "plus_changed_offsets": plus_changes,
                    "restored_offsets": restored,
                    "restore_remaining_offsets": [off for off in range(256) if restore[off] != base[off]],
                })
            report["value_probes"][state] = {"captures": entries}
            if entries:
                common = set(entries[0]["restored_offsets"])
                for entry in entries[1:]:
                    common &= set(entry["restored_offsets"])
                print(f"  {state}: captures={len(entries)} common +1->restore candidates=" +
                      (", ".join(f"0x{x:02X}" for x in sorted(common)) if common else "none yet"))

    if observations:
        print("\nOperator observations:")
        for obs in observations:
            note = obs["note"] if obs["note"] else "<blank>"
            print(f'  {obs["operation"]} step={obs["step"]} state={obs["state"]}: {note}')

    if args.json:
        args.json.write_text(json.dumps(report, indent=2) + "\n")
        print(f"\nWrote {args.json}")


if __name__ == "__main__":
    main()
