import datetime as dt
import json
import sqlite3
import sys


def main() -> None:
    path = sys.argv[1]
    con = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    row = con.execute(
        "select structure from project_structures order by id desc limit 1"
    ).fetchone()
    structure = json.loads(row["structure"])
    print(json.dumps(structure, ensure_ascii=False, indent=2))
    print("\nSUMMARY")
    for kind in ("boards", "schematics", "sheets", "pcbs"):
        items = structure.get(kind, {})
        print(f"{kind}: {len(items)}")
        for item in items.values():
            ts = item.get("updateTime")
            when = ""
            if isinstance(ts, int):
                when = " " + dt.datetime.fromtimestamp(ts / 1000).isoformat(sep=" ")
            name = item.get("title") or item.get("name") or item.get("uuid")
            print(f"- {name} ({item.get('uuid')}){when}")


if __name__ == "__main__":
    main()
