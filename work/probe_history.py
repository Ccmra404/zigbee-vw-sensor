import base64
import binascii
import gzip
import json
import sqlite3
import sys
import zlib


def printable_preview(data: bytes, limit: int = 160) -> str:
    return "".join(chr(b) if 32 <= b < 127 else "." for b in data[:limit])


def main() -> None:
    path = sys.argv[1]
    con = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    hist = con.execute("select * from history_data limit 1").fetchone()
    branch = con.execute(
        "select * from project_history_dbe15db3cab0494fb5e6bf69aaf5cf6a limit 1"
    ).fetchone()

    text = hist["dataStr"]
    print(f"history uuid: {hist['uuid']}")
    print(f"history key: {branch['key']}")
    print(f"dataStr chars: {len(text)}")
    raw = base64.b64decode(text)
    print(f"base64 bytes: {len(raw)}")
    print(f"first 32 hex: {binascii.hexlify(raw[:32]).decode()}")
    print(f"ascii preview: {printable_preview(raw)}")

    candidates = [("raw", raw)]
    for name, func in (
        ("zlib", zlib.decompress),
        ("gzip", gzip.decompress),
    ):
        try:
            out = func(raw)
            candidates.append((name, out))
            print(f"{name} ok: {len(out)} bytes {printable_preview(out)}")
        except Exception as exc:
            print(f"{name} no: {type(exc).__name__}: {exc}")

    for name, data in candidates:
        try:
            obj = json.loads(data.decode("utf-8"))
            print(f"{name} json ok")
            print(json.dumps(obj, ensure_ascii=False, indent=2)[:2000])
        except Exception as exc:
            print(f"{name} json no: {type(exc).__name__}: {exc}")


if __name__ == "__main__":
    main()
