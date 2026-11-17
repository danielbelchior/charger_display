# PlatformIO extra script: compiles the firmware version into FIRMWARE_VERSION.
# Order: the FIRMWARE_VERSION environment variable (set by the CI release
# build), then `git describe` (e.g. v2026.11.17.1, or v2026.11.17.1-3-gabc1234
# after a tag, with -dirty for uncommitted changes), then "dev".
import os
import re
import subprocess

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

ALLOWED = re.compile(r"^[A-Za-z0-9._+-]{1,40}$")


def git_version(project_dir):
    try:
        out = subprocess.run(
            ["git", "describe", "--tags", "--match", "v*", "--always", "--dirty"],
            cwd=project_dir,
            capture_output=True,
            text=True,
            timeout=10,
            check=True,
        ).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return None
    return out or None


def resolve_version(project_dir):
    for candidate in (os.environ.get("FIRMWARE_VERSION", "").strip(), git_version(project_dir)):
        if candidate and ALLOWED.match(candidate):
            return candidate
    return "dev"


version = resolve_version(env.subst("$PROJECT_DIR"))  # noqa: F821
print("Firmware version: " + version)
env.Append(CPPDEFINES=[("FIRMWARE_VERSION", env.StringifyMacro(version))])  # noqa: F821
