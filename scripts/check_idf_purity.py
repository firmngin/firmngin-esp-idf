#!/usr/bin/env python3
"""IDF purity gate. Exit 1 when Arduino-only APIs appear outside allowed fences."""
import io
import pathlib
import re
import sys

API = re.compile(
    r"("
    r"\bSerial\.[a-z]"                          # Serial.print/printf/...
    r"|\bWiFi\."                                # WiFi.status() etc.
    r"|\bWiFiClient|WiFiClientSecure"
    r"|\bHTTPClient\b"
    r"|\bPreferences\b"
    r"|\blittleFS\.|\bLittleFS\."
    r"|\bmillis\(|\bdelay\(|\byield\(\)|\bESP\.restart"
    r"|#include\s*<(Arduino|WiFi|WiFiClientSecure|HTTPClient|Preferences|LittleFS|Update)\.h>"
    r"|\.(toInt|toFloat|indexOf|lastIndexOf|substring|startsWith|endsWith|charAt"
    r"|equalsIgnoreCase|trim|toUpperCase|toLowerCase)\("
    r")"
)

ALLOWED_FILES = {
    # the compat layer itself, and the vendored IDF component
    "fng_compat.h",
    "esp_websocket_client_vendored.h",
    "esp_websocket_client_vendored.c",
}


def fenced_by_arduino(lines):
    """Yield (line_number, in_arduino_block) with fence tracking."""
    depth = 0
    for number, line in enumerate(lines, 1):
        opens = len(re.findall(r"#\s*if\s+defined\(\s*ARDUINO\s*\)|#\s*ifdef\s+ARDUINO", line))
        closes = line.startswith("#endif")
        depth += opens
        in_block = depth > 0
        if closes and depth > 0:
            depth -= 1
        yield number, in_block


def strip_comment(line):
    # drop trailing // comments (naive but sufficient: no // inside C strings here)
    line = re.sub(r"//.*$", "", line)
    # drop one-line /* */ pairs
    line = re.sub(r"/\*.*?\*/", "", line)
    return line


def run(srcdir):
    src = pathlib.Path(srcdir)
    findings = []
    for path in sorted(src.iterdir()):
        if path.suffix not in (".h", ".hpp", ".c", ".cpp"):
            continue
        if path.name in ALLOWED_FILES:
            continue
        lines = io.StringIO(path.read_text(errors="replace")).read().split("\n")
        for number, in_block in fenced_by_arduino(lines):
            if in_block:
                continue
            code = strip_comment(lines[number - 1])
            if API.search(code):
                findings.append(f"{path.name}:{number}: {code.strip()[:150]}")
    if findings:
        print("FAILED purity check - Arduino-only APIs found outside allowed fences:")
        for f in findings[:40]:
            print("  " + f)
        if len(findings) > 40:
            print(f"  ... and {len(findings) - 40} more")
        return 1
    print("OK: core is Arduino-API-free outside the compat layer and ARDUINO fences.")
    return 0


if __name__ == "__main__":
    sys.exit(run(sys.argv[1] if len(sys.argv) > 1 else "src"))
