import json
import sqlite3
import sys


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: inspect_eprj2.py <project.eprj2>")

    path = sys.argv[1]
    con = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    cur = con.cursor()

    tables = [r["name"] for r in cur.execute(
        "select name from sqlite_master where type='table' order by name"
    )]
    print("TABLES")
    for table in tables:
        count = cur.execute(f'select count(*) as c from "{table}"').fetchone()["c"]
        print(f"- {table}: {count}")

    print("\nSCHEMA")
    for row in cur.execute(
        "select name, sql from sqlite_master where type='table' order by name"
    ):
        print(f"\n[{row['name']}]\n{row['sql']}")

    for table in tables:
        print(f"\nSAMPLE {table}")
        rows = cur.execute(f'select * from "{table}" limit 5').fetchall()
        for row in rows:
            data = {}
            for key in row.keys():
                value = row[key]
                if isinstance(value, str) and len(value) > 220:
                    value = value[:220] + "...<truncated>"
                data[key] = value
            print(json.dumps(data, ensure_ascii=False, default=str))


if __name__ == "__main__":
    main()
