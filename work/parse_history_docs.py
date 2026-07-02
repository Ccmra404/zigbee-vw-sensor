import json
import sys
from collections import Counter
from pathlib import Path


def parse_line(line: str):
    if "||" not in line:
        return None
    a, b = line.rstrip("\n").split("||", 1)
    if b.endswith("|"):
        b = b[:-1]
    try:
        return json.loads(a), json.loads(b)
    except Exception:
        return None


def load_docs(path: Path):
    docs = []
    current = None
    for raw in path.read_text(encoding="utf-8").splitlines():
        parsed = parse_line(raw)
        if not parsed:
            continue
        head, body = parsed
        typ = head.get("type")
        if typ == "DOCHEAD":
            current = {"head": body, "records": [], "meta": {}}
            docs.append(current)
        elif current is not None:
            current["records"].append((head, body))
            if typ == "META":
                current["meta"] = body
    return docs


def main() -> None:
    docs = load_docs(Path(sys.argv[1]))
    print(f"docs: {len(docs)}")
    for i, doc in enumerate(docs):
        counts = Counter(head.get("type") for head, _ in doc["records"])
        title = doc["meta"].get("title") or doc["meta"].get("name") or ""
        print(
            f"{i:03d} {doc['head'].get('docType')} uuid={doc['head'].get('uuid')} "
            f"title={title} records={len(doc['records'])} "
            f"top={dict(counts.most_common(8))}"
        )


if __name__ == "__main__":
    main()
