import base64
import json
import sqlite3
import sys
from pathlib import Path


def main() -> None:
    project_path = sys.argv[1]
    out_dir = Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)

    con = sqlite3.connect(f"file:{project_path}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    branch = con.execute("select branch_uuid from projects limit 1").fetchone()["branch_uuid"]
    table = "project_history_" + branch.replace("-", "_")
    hist = con.execute(f'select * from "{table}" order by id desc limit 1').fetchone()
    row = con.execute("select dataStr from history_data where uuid=?", (hist["uuid"],)).fetchone()

    (out_dir / "history.bin").write_bytes(base64.b64decode(row["dataStr"]))
    (out_dir / "history_meta.json").write_text(
        json.dumps({"uuid": hist["uuid"], "key": hist["key"], "branch": branch}, indent=2),
        encoding="utf-8",
    )
    print(json.dumps({"uuid": hist["uuid"], "key": hist["key"], "branch": branch}, ensure_ascii=False))


if __name__ == "__main__":
    main()
