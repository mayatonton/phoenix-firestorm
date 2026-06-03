# AYAstorm r41 UBO Codegen — shared error type
# Spec: docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md §4.3.1.6
#       docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md §5.2.1.6 / §5.4.1.4 / §9.4
"""Shared CodegenError + error-format helpers.

All build-time-fail conditions (= GLSL parse failure / std140 mismatch /
SPIR-V reflection mismatch / perfect hash collision) raise CodegenError,
formatted per chapter 08 §9.4. main.py catches it at the top level and
exits with the appropriate non-zero code.
"""

from __future__ import annotations

from typing import Optional

LOG_PREFIX = "[codegen_ubo]"


class CodegenError(Exception):
    """Build-time-fail exception. Always carries a §9.4-format message."""


def format_error(
    summary: str,
    *,
    glsl_file: Optional[str] = None,
    line: Optional[int] = None,
    block: Optional[str] = None,
    member: Optional[str] = None,
    reason: Optional[str] = None,
    action: Optional[str] = None,
    extra_lines: Optional[list] = None,
) -> str:
    """Render a §9.4 build-error message.

    Caller passes only the fields that apply; missing ones are suppressed.
    """
    out = [f"{LOG_PREFIX} ERROR: {summary}"]
    if glsl_file is not None:
        loc = f"{glsl_file}:{line}" if line is not None else glsl_file
        out.append(f"  GLSL file: {loc}")
    if block is not None:
        suffix = f", member: {member}" if member is not None else ""
        out.append(f"  Block: {block}{suffix}")
    if extra_lines:
        out.extend(f"  {ln}" for ln in extra_lines)
    if reason is not None:
        out.append(f"  Reason: {reason}")
    if action is not None:
        out.append(f"  Action: {action}")
    return "\n".join(out)
