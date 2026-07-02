import gzip
import json
import sqlite3
import sys
from pathlib import Path

from cryptography.hazmat.primitives.ciphers.aead import AESGCM


def main() -> None:
    project_path = sys.argv[1]
    output_path = Path(sys.argv[2])

    con = sqlite3.connect(f"file:{project_path}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row

    branch = con.execute(
        "select branch_uuid from projects limit 1"
    ).fetchone()["branch_uuid"]
    table = "project_history_" + branch.replace("-", "_")
    hist = con.execute(f'select * from "{table}" order by id desc limit 1').fetchone()
    data = con.execute(
        "select dataStr from history_data where uuid=?", (hist["uuid"],)
    ).fetchone()["dataStr"]

    key = bytes.fromhex(hist["key"])
    iv = bytes.fromhex(hist["uuid"])
    encrypted = __import__("base64").b64decode(data)
    plaintext_gzip = AESGCM(key).decrypt(iv, encrypted, None)
    plaintext = gzip.decompress(plaintext_gzip).decode("utf-8")

    try:
        parsed = json.loads(plaintext)
        output_path.write_text(json.dumps(parsed, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"json ok: {output_path}")
        print(f"top-level type: {type(parsed).__name__}")
        if isinstance(parsed, dict):
            print("keys:", ", ".join(parsed.keys()))
    except Exception:
        output_path.write_text(plaintext, encoding="utf-8")
        print(f"text ok: {output_path}")
        print(plaintext[:1000])


if __name__ == "__main__":
    main()
