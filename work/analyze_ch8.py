import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

from parse_history_docs import load_docs


KEYWORDS = re.compile(
    r"(VCC|GND|PGND|VIN|VBAT|BAT|5V|3V3|3\.3|RESET|RST|BOOT|SWD|UART|TX|RX|ZIGBEE|ANT|RF|LDO|MT3608|TP4057|XC62|LGS5148|SP3485|VM101)",
    re.I,
)


def fmt(v):
    if v is None:
        return ""
    return str(v)


def main() -> None:
    docs = load_docs(Path(sys.argv[1]))
    ch8 = next(
        d for d in docs
        if d["head"].get("docType") == "PCB"
        and d["head"].get("uuid") == "cd9fecffdae84514ae92653cd7f403ed"
        and len(d["records"]) > 10
    )

    attrs = defaultdict(dict)
    comps = {}
    nets = {}
    pad_nets = []

    for head, body in ch8["records"]:
        typ = head.get("type")
        rid = head.get("id")
        if typ == "COMPONENT":
            comps[rid] = body
        elif typ == "ATTR":
            attrs[body.get("parentId") or body.get("partId")][body.get("key")] = body.get("value")
        elif typ == "NET":
            nets[rid] = body
        elif typ == "PAD_NET":
            pad_nets.append((head, body))

    print("CH8 record counts")
    print(Counter(head.get("type") for head, _ in ch8["records"]).most_common())

    print("\nComponents")
    rows = []
    for cid, comp in comps.items():
        a = attrs.get(cid, {})
        text = " ".join(fmt(x) for x in [cid, comp.get("title"), comp.get("device"), comp.get("package"), *a.values()])
        rows.append((cid, comp, a, bool(KEYWORDS.search(text))))
    for cid, comp, a, marked in sorted(rows, key=lambda r: (not r[3], fmt(r[2].get("Designator") or r[1].get("designator") or r[0]))):
        designator = a.get("Designator") or a.get("Prefix") or comp.get("designator") or cid
        name = a.get("Name") or a.get("Value") or comp.get("title") or comp.get("device") or ""
        supplier = a.get("Supplier Part") or a.get("LCSC Part") or a.get("Part Number") or ""
        package = a.get("Package") or a.get("Footprint") or comp.get("package") or ""
        if marked or len(rows) < 200:
            print(f"- {designator}: {name} | {package} | {supplier}")

    print("\nNets")
    for nid, net in sorted(nets.items(), key=lambda kv: fmt(kv[1].get("netName") or kv[1].get("name"))):
        name = net.get("netName") or net.get("name") or nid
        if KEYWORDS.search(name):
            print(f"- {name}: id={nid}")

    print("\nPad nets matching keywords")
    for head, body in pad_nets:
        text = json.dumps(body, ensure_ascii=False)
        if KEYWORDS.search(text):
            print(json.dumps({"id": head.get("id"), **body}, ensure_ascii=False)[:500])


if __name__ == "__main__":
    main()
