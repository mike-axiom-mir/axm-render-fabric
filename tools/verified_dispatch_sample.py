#!/usr/bin/env python3
"""Repeat one verified AXM external dispatch without turning timings into rankings.

Each sample is accepted only after axm-render-external reports a fully verified dispatch.
Identity/evidence fields must remain stable across the sample set; only parent-observed
steady-clock timings are summarized.
"""

from __future__ import annotations

import argparse
import statistics
import subprocess
import sys
from pathlib import Path
from typing import Dict, List

TIMING_FIELDS = (
    "capability_process_elapsed_us",
    "render_process_elapsed_us",
    "receipt_verification_elapsed_us",
    "external_dispatch_elapsed_us",
)

CONTINUITY_FIELDS = (
    "renderer_process_resolution",
    "renderer_process_canonical_path",
    "renderer_process_digest64",
    "renderer_process_continuity",
    "renderer",
    "renderer_version",
    "backend",
    "capabilities_digest64",
    "scene_source_digest64",
    "request_source_digest64",
    "frame_pixels_digest64",
    "output_file_digest64",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Repeat one axm-render-external job and summarize verified steady-clock "
            "timing observations without comparing or ranking renderers."
        )
    )
    parser.add_argument("--harness", required=True, help="path to axm-render-external")
    parser.add_argument("--renderer", required=True, help="selected renderer executable")
    parser.add_argument("--request", required=True, help="AXM_RENDER_REQUEST v1 file")
    parser.add_argument("--work-dir", required=True, help="directory for per-sample receipts/capabilities")
    parser.add_argument("--samples", type=int, default=5, help="verified dispatch count (default: 5)")
    parser.add_argument(
        "--expect-capabilities",
        help="optional previously inspected AXM_RENDER_CAPABILITIES v1 manifest",
    )
    args = parser.parse_args()
    if args.samples < 1 or args.samples > 100:
        parser.error("--samples must be between 1 and 100")
    return args


def parse_key_values(stdout: str) -> Dict[str, str]:
    values: Dict[str, str] = {}
    for raw_line in stdout.splitlines():
        if "=" not in raw_line:
            continue
        key, value = raw_line.split("=", 1)
        key = key.strip()
        if not key:
            continue
        # Child renderers may emit some of the same informational keys before the
        # harness prints its final verified values. The final occurrence is authoritative.
        values[key] = value.strip()
    return values


def require_field(values: Dict[str, str], field: str, sample_index: int) -> str:
    if field not in values:
        raise RuntimeError(f"sample {sample_index}: missing required field {field}")
    return values[field]


def require_nonnegative_integer(values: Dict[str, str], field: str, sample_index: int) -> int:
    text = require_field(values, field, sample_index)
    if not text.isdigit():
        raise RuntimeError(f"sample {sample_index}: {field} is not a non-negative integer: {text!r}")
    return int(text)


def format_median(values: List[int]) -> str:
    value = statistics.median(values)
    if isinstance(value, int) or float(value).is_integer():
        return str(int(value))
    return format(value, ".1f")


def main() -> int:
    args = parse_args()

    harness = str(Path(args.harness).resolve())
    renderer = str(Path(args.renderer).resolve())
    request = str(Path(args.request).resolve())
    expected_capabilities = (
        str(Path(args.expect_capabilities).resolve()) if args.expect_capabilities else None
    )
    work_dir = Path(args.work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    timing_samples: Dict[str, List[int]] = {field: [] for field in TIMING_FIELDS}
    stable_values: Dict[str, str] | None = None

    for sample_index in range(1, args.samples + 1):
        receipt = work_dir / f"sample-{sample_index:03d}.axmreceipt"
        capabilities = work_dir / f"sample-{sample_index:03d}.axmcaps"

        command = [
            harness,
            "--renderer",
            renderer,
            "--request",
            request,
            "--receipt",
            str(receipt),
            "--capabilities",
            str(capabilities),
        ]
        if expected_capabilities:
            command.extend(["--expect-capabilities", expected_capabilities])

        completed = subprocess.run(command, text=True, capture_output=True, check=False)
        if completed.returncode != 0:
            raise RuntimeError(
                f"sample {sample_index}: verified dispatch failed with status "
                f"{completed.returncode}\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
            )

        values = parse_key_values(completed.stdout)
        if require_field(values, "external_process_dispatch", sample_index) != "PASS":
            raise RuntimeError(f"sample {sample_index}: external_process_dispatch was not PASS")
        if require_field(values, "timing_clock", sample_index) != "steady":
            raise RuntimeError(f"sample {sample_index}: timing_clock was not steady")

        current_stable = {
            field: require_field(values, field, sample_index) for field in CONTINUITY_FIELDS
        }
        if stable_values is None:
            stable_values = current_stable
        else:
            for field in CONTINUITY_FIELDS:
                if current_stable[field] != stable_values[field]:
                    raise RuntimeError(
                        f"sample {sample_index}: continuity field drifted: {field}: "
                        f"expected {stable_values[field]!r}, observed {current_stable[field]!r}"
                    )

        for field in TIMING_FIELDS:
            timing_samples[field].append(
                require_nonnegative_integer(values, field, sample_index)
            )

    assert stable_values is not None

    print("AXM_VERIFIED_DISPATCH_SAMPLES 1")
    print(f"sample_count={args.samples}")
    print("timing_clock=steady")
    for field in CONTINUITY_FIELDS:
        print(f"{field}={stable_values[field]}")
    for field in TIMING_FIELDS:
        values = timing_samples[field]
        print(f"{field}_samples={','.join(str(value) for value in values)}")
        print(f"{field}_min={min(values)}")
        print(f"{field}_median={format_median(values)}")
        print(f"{field}_max={max(values)}")
    print("verified_dispatch_sample_continuity=PASS")
    print("verified_dispatch_sampling=PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # compact CLI failure boundary
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
