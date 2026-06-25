import sys
import zipfile
import xml.etree.ElementTree as ET

NS = {
    "w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
}


def text_of(el):
    parts = []
    for node in el.iter():
        tag = node.tag.rsplit("}", 1)[-1]
        if tag in {"t", "delText"} and node.text:
            parts.append(node.text)
        elif tag == "tab":
            parts.append("\t")
        elif tag in {"br", "cr"}:
            parts.append("\n")
    return "".join(parts).strip()


def para_style(p):
    ppr = p.find("w:pPr", NS)
    if ppr is None:
        return ""
    ps = ppr.find("w:pStyle", NS)
    if ps is None:
        return ""
    return ps.attrib.get(f"{{{NS['w']}}}val", "")


def main(path, full_tables=False):
    with zipfile.ZipFile(path) as z:
        xml = z.read("word/document.xml")
    root = ET.fromstring(xml)
    body = root.find("w:body", NS)
    if body is None:
        return
    i = 0
    for child in list(body):
        name = child.tag.rsplit("}", 1)[-1]
        if name == "p":
            txt = text_of(child)
            if txt:
                style = para_style(child)
                marker = "P"
                if "Heading" in style or "Judul" in style or style.startswith("BAB") or style.startswith("TOC"):
                    marker = "H"
                print(f"{i:04d}\t{marker}\t{style}\t{txt}")
                i += 1
        elif name == "tbl":
            rows = []
            for tr in child.findall(".//w:tr", NS):
                cells = []
                for tc in tr.findall("./w:tc", NS):
                    cells.append(" / ".join(t for t in [text_of(p) for p in tc.findall('.//w:p', NS)] if t))
                if any(cells):
                    rows.append(" | ".join(cells))
            if rows:
                shown = rows if full_tables else rows[:8]
                print(f"{i:04d}\tTABLE\t\t" + " || ".join(shown))
                if len(rows) > 8 and not full_tables:
                    print(f"{i:04d}\tTABLE_CONT\t\t... {len(rows)-8} more rows")
                i += 1


if __name__ == "__main__":
    main(sys.argv[1], "--full-tables" in sys.argv[2:])
