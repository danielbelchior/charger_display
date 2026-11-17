# PlatformIO extra script for [env:ota]: passes the OTA password to espota so
# it only has to live in main/secrets.h. The OTA_PASSWORD environment variable
# takes precedence.
import os
import re

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

DEFINE = re.compile(r'^\s*#\s*define\s+OTA_PASSWORD\s+"((?:[^"\\]|\\.)*)"', re.MULTILINE)


def password_from_secrets(path):
    try:
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
    except OSError:
        return None
    # Drop comments so a commented-out template line is not picked up.
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"^\s*//[^\n]*", "", text, flags=re.MULTILINE)
    match = DEFINE.search(text)
    if not match:
        return None
    return re.sub(r"\\(.)", r"\1", match.group(1))


password = os.environ.get("OTA_PASSWORD")
if password is None:
    password = password_from_secrets(os.path.join(env.subst("$PROJECT_DIR"), "main", "secrets.h"))  # noqa: F821

if password:
    env.Append(UPLOADERFLAGS=["--auth=" + password])  # noqa: F821
