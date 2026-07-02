import json
import re
import sys
from collections import defaultdict
from pathlib import Path

from parse_history_docs import load_docs

POWER_NETS = {
    "GND", "PGND", "24VT5V5", "BATT5V", "5V5", "P5V", "LDO5V",
    "5V", "+3V3", "P3.3V", "3.3V", "VCC", "VZIGBEE",
    "UART1_RX", "UART1_TX", "UART2RX", "UART2TX",
}
IMPORTANT = re.compile(
    r"(U\d+|CN\d+|J\d+|F\d+|D\d+|L\d+|Q\d+|SWD1|LED\d+|C\d+|R\d+)$"
)


def maps(docs, doc_type):
    result = {}
    for doc in docs:
        if doc["head"].get("docType") == doc_type and doc["meta"].get("title"):
            result[doc["head"].get("uuid")] = doc["meta"].get("title")
    return result


def main() -> None:
    docs = load_docs(Path(sys.argv[1]))
    dev_names = maps(docs, "DEVICE")
    fp_names = maps(docs, "FOOTPRINT")
    ch8 = next(
        d for d in docs
        if d["head"].get("docType") == "PCB"
        and d["head"].get("uuid") == "cd9fecffdae84514ae92653cd7f403ed"
        and len(d["records"]) > 10
    )

    comps = {}
    attrs = defaultdict(dict)
    pads = defaultdict(list)
    for head, body in ch8["records"]:
        typ = head.get("type")
        if typ == "COMPONENT":
            comps[head["id"]] = body
            attrs[head["id"]].update(body.get("attrs") or {})
        elif typ == "ATTR":
            attrs[body.get("parentId")][body.get("key")] = body.get("value")
        elif typ == "PAD_NET":
            try:
                _, comp_id, pad_no, _ = json.loads(head["id"])
            except Exception:
                continue
            pads[comp_id].append((pad_no, body.get("padNet")))

    rows = []
    for cid, body in comps.items():
        a = attrs[cid]
        ref = a.get("Designator", cid)
        dev_uuid = a.get("Device", "")
        fp_uuid = a.get("Footprint", "")
        dev = dev_names.get(dev_uuid, dev_uuid)
        fp = fp_names.get(fp_uuid, fp_uuid)
        value = a.get("Value") or a.get("Manufacturer Part") or ""
        nets = sorted(set(n for _, n in pads[cid] if n))
        rows.append((ref, cid, value, dev, fp, pads[cid], nets))

    print("Power / bring-up related components")
    for ref, cid, value, dev, fp, pad_list, nets in sorted(rows):
        text = " ".join([ref, value, dev, fp, " ".join(nets)])
        if any(n in POWER_NETS for n in nets) or re.search(r"(XC62|MT3608|TP4057|Zigbee|LGS5148|SP3485|SWD|DC|BAT|USB|LDO|LED|CN|J)", text, re.I):
            pins = " ".join(f"{p}:{n}" for p, n in pad_list)
            print(f"{ref:5} {value:10} {dev:28} {fp:32} {pins}")

    print("\nPower nets and attached refs")
    net_to_refs = defaultdict(list)
    for ref, cid, value, dev, fp, pad_list, nets in rows:
        for pad, net in pad_list:
            if net in POWER_NETS:
                net_to_refs[net].append(f"{ref}.{pad}")
    for net in sorted(net_to_refs):
        print(f"{net:8} {', '.join(net_to_refs[net])}")


if __name__ == "__main__":
    main()
