"""Dựng file Word báo cáo đồ án IoT Nhóm 16 từ mẫu Word và các tệp Markdown trong md/.

Cách chạy (từ thư mục Bao_cao):
    python3.13.exe py/build_bao_cao_iot.py

- Mẫu:   mau_word/Mau_SportHub.docx (báo cáo đồ án môn khác, chỉ lấy style, bìa, khung phần đầu)
- Nguồn: md/00_PHAN_DAU.md, md/01_*.md … md/06_*.md, ảnh hinh/hinh_x_y.png
- Kết quả: BaoCao_DoAn_IoT_Nhom16.docx (bị ghi đè mỗi lần chạy: sửa Markdown, không sửa tay file Word)

Cú pháp Markdown được hỗ trợ: # / ## / ### (Heading 1–3), đoạn văn, "- " gạch đầu dòng,
"BANG: <chú thích>" + bảng Markdown, "HINH: <chú thích>" (chèn ô chờ hình + chú thích),
"CODE:" ... "ENDCODE", "REF: " (tài liệu tham khảo), "@@API_TABLE".
Mở file Word, bấm Ctrl+A rồi F9 để cập nhật mục lục, danh mục hình và bảng.
"""
import copy
import json
import re
import sys
from pathlib import Path

import docx
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt
from docx.table import Table
from docx.text.paragraph import Paragraph

sys.stdout.reconfigure(encoding="utf-8")
ROOT = Path(__file__).resolve().parent.parent
# Chữ cần tìm trong mẫu SportHub để thay
OLD_LECTURER = "ThS. Nguyễn Quang Huy"
TEMPLATE = ROOT / "mau_word" / "Mau_SportHub.docx"
MD_DIR = ROOT / "md"
OUT = ROOT / "BaoCao_DoAn_IoT_Nhom16.docx"

TITLE = "HỆ THỐNG GIÁM SÁT VÀ CẢNH BÁO QUÁ TẢI ĐIỆN NĂNG THÔNG MINH"
COURSE = "Học phần: Internet of Things"
LECTURER = "ThS. Võ Hoàng Hải"
# (MSSV, họ tên); người đầu tiên ký ô "Nhóm trưởng" ở lời cam đoan
STUDENTS = [
    ("2001230490", "Lê Đức Lương"),
    ("2001230486", "Trần Lê Đức Trí"),
]
MEMBERS = [f"{n}. {mssv} – {name}" for n, (mssv, name) in enumerate(STUDENTS, 1)]

doc = docx.Document(str(TEMPLATE))
body = doc.element.body


def P(el):
    return Paragraph(el, doc)


def els():
    return list(body.iterchildren())


def find_index(pred, start=0):
    for i, el in enumerate(els()):
        if i >= start and pred(el):
            return i
    raise ValueError("không tìm thấy phần tử")


def is_par(el, text=None, style=None):
    if el.tag != qn("w:p"):
        return False
    p = P(el)
    if text is not None and p.text.strip() != text:
        return False
    if style is not None and p.style.name != style:
        return False
    return True


def set_text(el, text):
    """Đặt chữ cho đoạn, giữ định dạng của run đầu tiên."""
    p = P(el)
    runs = p.runs
    if not runs:
        p.add_run(text)
        return
    runs[0].text = text
    for r in runs[1:]:
        r._r.getparent().remove(r._r)


def strip_runs(el):
    for child in list(el):
        if child.tag != qn("w:pPr"):
            el.remove(child)


def remove(el):
    el.getparent().remove(el)


# ---------------------------------------------------------------- Bìa (2 bản)
def fix_cover(start):
    idx = find_index(lambda e: is_par(e, "ĐỒ ÁN MÔN HỌC"), start)
    e = els()
    set_text(e[idx + 2], TITLE)
    i = find_index(lambda x: P(x).text.startswith("Học phần:") if x.tag == qn("w:p") else False, idx)
    set_text(els()[i], COURSE)
    i = find_index(lambda x: is_par(x, OLD_LECTURER), idx)
    set_text(els()[i], LECTURER)
    i = find_index(lambda x: is_par(x, "SINH VIÊN THỰC HIỆN:"), idx)
    member_pars = []
    k = i + 1
    while els()[k].tag == qn("w:p") and re.match(r"^\d+\. ", P(els()[k]).text.strip()):
        member_pars.append(els()[k])
        k += 1
    last = member_pars[-1]
    while len(member_pars) < len(MEMBERS):
        new = copy.deepcopy(last)
        last.addnext(new)
        member_pars.append(new)
        last = new
    for el in member_pars[len(MEMBERS):]:  # mẫu có 5 thành viên, nhóm có 2
        remove(el)
    for el, text in zip(member_pars, MEMBERS):
        set_text(el, text)
    return i + 1 + len(MEMBERS)


nxt = fix_cover(0)
fix_cover(nxt)

# ------------------------------------------------- Nhận xét giảng viên hướng dẫn
i = find_index(lambda e: is_par(e, "NHẬN XÉT CỦA GIẢNG VIÊN"))
j = find_index(lambda e: e.tag == qn("w:p") and P(e).text.startswith("TP. Hồ Chí Minh, ngày"), i)
set_text(els()[j], "TP. Hồ Chí Minh, ngày ..... tháng ..... năm 2026")
set_text(els()[j + 1], "Giảng viên")
# bỏ đoạn chứa ảnh chữ ký và thay tên giảng viên
k = j + 2
while not is_par(els()[k], OLD_LECTURER):
    if els()[k].find(".//" + qn("w:drawing")) is not None:
        remove(els()[k])
        continue
    k += 1
set_text(els()[k], LECTURER)


# ------------------------------------------------------- Đọc phần đầu Markdown
front = (MD_DIR / "00_PHAN_DAU.md").read_text(encoding="utf-8")
blocks = {}
for m in re.finditer(r"@@(\w+)\n(.*?)(?=\n@@|\Z)", front, re.S):
    blocks[m.group(1)] = [ln.strip() for ln in m.group(2).strip().split("\n") if ln.strip()]


def replace_section(heading, next_heading, lines, keep_tail=0):
    """Thay các đoạn giữa heading và next_heading bằng lines (giữ keep_tail phần tử cuối)."""
    s = find_index(lambda e: is_par(e, heading))
    t = find_index(lambda e: is_par(e, next_heading), s)
    body_els = els()[s + 1 : t]
    proto = next(e for e in body_els if e.tag == qn("w:p") and len(P(e).text) > 40)
    tail = body_els[len(body_els) - keep_tail :] if keep_tail else []
    anchor = els()[s]
    for el in body_els:
        if el not in tail:
            remove(el)
    for text in lines:
        new = copy.deepcopy(proto)
        set_text(new, text)
        anchor.addnext(new)
        anchor = new


# Lời cam đoan: giữ dòng ngày, bảng chữ ký và các đoạn trống phía sau
s = find_index(lambda e: is_par(e, "LỜI CAM ĐOAN"))
t = find_index(lambda e: is_par(e, "TÓM TẮT"), s)
seg = els()[s + 1 : t]
date_idx = next(n for n, e in enumerate(seg) if e.tag == qn("w:p") and P(e).text.startswith("TP. Hồ Chí Minh"))
set_text(seg[date_idx], "TP. Hồ Chí Minh, ngày ..... tháng ..... năm 2026")
sign = next(e for e in seg if e.tag == qn("w:tbl"))
for drawing in list(sign.iter(qn("w:drawing"))):  # bỏ ảnh chữ ký của mẫu
    run = drawing.getparent()
    run.getparent().remove(run)
sign_tbl = Table(sign, doc)
row0 = sign_tbl.rows[0]._tr
need_rows = (len(STUDENTS) + 2) // 3  # mỗi hàng 3 ô ký
for r in sign_tbl.rows[need_rows:]:
    sign.remove(r._tr)
for _ in range(need_rows - len(sign_tbl.rows)):
    sign.append(copy.deepcopy(row0))
cells = [c for r in sign_tbl.rows for c in r.cells]
for n, cell in enumerate(cells):
    if n < len(STUDENTS):
        set_text(cell.paragraphs[0]._p, "Nhóm trưởng" if n == 0 else "Thành viên")
        for par in cell.paragraphs[2:]:
            if par.text.strip():
                set_text(par._p, STUDENTS[n][1])
    else:  # ô thừa ở hàng cuối
        for par in cell.paragraphs:
            if par.text.strip():
                set_text(par._p, "")
proto = seg[3]  # đoạn văn dài "Tôi tên là ..."
anchor = els()[s]
for el in seg[:date_idx]:
    remove(el)
for text in blocks["LOI_CAM_DOAN"]:
    new = copy.deepcopy(proto)
    set_text(new, text)
    anchor.addnext(new)
    anchor = new

replace_section("TÓM TẮT", "LỜI CẢM ƠN", blocks["TOM_TAT"])
replace_section("LỜI CẢM ƠN", "MỤC LỤC", blocks["LOI_CAM_ON"])


# ------------------------------------------------ Mục lục và danh mục tự động
def field_paragraph(instr, placeholder):
    p = OxmlElement("w:p")
    # Đoạn chứa dấu kết thúc trường rất thấp để không tràn sang tạo trang trống
    ppr = OxmlElement("w:pPr")
    sp = OxmlElement("w:spacing")
    for k, v in (("w:before", "0"), ("w:after", "0"), ("w:line", "20"), ("w:lineRule", "exact")):
        sp.set(qn(k), v)
    ppr.append(sp)
    p.append(ppr)
    for kind, text in (("begin", None), ("instr", instr), ("separate", None), ("text", placeholder), ("end", None)):
        r = OxmlElement("w:r")
        if kind in ("begin", "separate", "end"):
            fc = OxmlElement("w:fldChar")
            fc.set(qn("w:fldCharType"), kind)
            if kind == "begin":
                fc.set(qn("w:dirty"), "true")
            r.append(fc)
        elif kind == "instr":
            it = OxmlElement("w:instrText")
            it.set(qn("xml:space"), "preserve")
            it.text = f" {instr} "
            r.append(it)
        else:
            t = OxmlElement("w:t")
            t.text = text
            r.append(t)
        p.append(r)
    return p


def replace_list(heading, next_pred, instr, placeholder):
    s = find_index(lambda e: is_par(e, heading))
    t = find_index(next_pred, s + 1)
    seg = els()[s + 1 : t]
    for el in seg:
        if el.find(".//" + qn("w:sectPr")) is not None:
            strip_runs(el)  # giữ ngắt section
        else:
            remove(el)
    els()[s].addnext(field_paragraph(instr, placeholder))


hint = "Nhấn chuột phải → Update Field (hoặc Ctrl+A rồi F9) để cập nhật."
replace_list("MỤC LỤC", lambda e: is_par(e, "DANH MỤC CÁC KÝ HIỆU VÀ CHỮ VIẾT TẮT"), r'TOC \o "1-3" \h \z \u', hint)
replace_list("DANH MỤC HÌNH", lambda e: is_par(e, "DANH MỤC BẢNG"), r'TOC \h \z \t "Image Caption,1"', hint)
replace_list(
    "DANH MỤC BẢNG",
    lambda e: e.tag == qn("w:p") and P(e).style.name == "Heading 1" and P(e).text.startswith("CHƯƠNG 1"),
    r'TOC \h \z \t "Table Caption,1"',
    hint,
)

# ------------------------------------------------------- Bảng chữ viết tắt
s = find_index(lambda e: is_par(e, "DANH MỤC CÁC KÝ HIỆU VÀ CHỮ VIẾT TẮT"))
abbr_tbl = Table(els()[s + 1], doc)
rows = [ln for ln in blocks["VIET_TAT"] if ln.startswith("|") and not set(ln) <= set("|- ")]
data = [[c.strip() for c in ln.strip("|").split("|")] for ln in rows[1:]]
proto_row = abbr_tbl.rows[1]._tr
for r in abbr_tbl.rows[1:]:
    r._tr.getparent().remove(r._tr)
for vals in data:
    tr = copy.deepcopy(proto_row)
    abbr_tbl._tbl.append(tr)
    for cell_el, v in zip(tr.findall(qn("w:tc")), vals):
        par = cell_el.find(qn("w:p"))
        for extra in cell_el.findall(qn("w:p"))[1:]:
            cell_el.remove(extra)
        set_text(par, v)

# ------------------------------------------------- Xóa toàn bộ nội dung cũ
start = find_index(lambda e: e.tag == qn("w:p") and P(e).style.name == "Heading 1" and P(e).text.startswith("CHƯƠNG 1"))
proto_table = None
proto_img_par = None
for el in els()[start:]:
    if el.tag == qn("w:tbl") and proto_table is None:
        tb = Table(el, doc)
        if tb.style is not None and tb.style.name == "Table" and len(tb.rows) > 2:
            proto_table = copy.deepcopy(el)
for el in els()[start:]:
    if el.tag == qn("w:sectPr"):
        continue
    remove(el)
final_sect = body.find(qn("w:sectPr"))


# ------------------------------------------------------------ Hàm thêm nội dung
def add_par(text="", style="Normal", align=None, indent_first=None, bold_prefix=None):
    p = OxmlElement("w:p")
    final_sect.addprevious(p)
    par = Paragraph(p, doc)
    par.style = doc.styles[style]
    if align is not None:
        par.alignment = align
    if indent_first is not None:
        par.paragraph_format.first_line_indent = indent_first
    if text:
        add_runs(par, text)
    return par


def add_runs(par, text):
    parts = re.split(r"(\*\*[^*]+\*\*)", text)
    for part in parts:
        if not part:
            continue
        if part.startswith("**") and part.endswith("**"):
            par.add_run(part[2:-2]).bold = True
        else:
            par.add_run(part)


proto_tblPr = proto_table.find(qn("w:tblPr")) if proto_table is not None else None
proto_rows = proto_table.findall(qn("w:tr")) if proto_table is not None else []


def cell_proto(row_idx):
    tc = proto_rows[row_idx].find(qn("w:tc"))
    p = tc.find(qn("w:p"))
    ppr = p.find(qn("w:pPr"))
    r = p.find(qn("w:r"))
    rpr = r.find(qn("w:rPr")) if r is not None else None
    tcpr = tc.find(qn("w:tcPr"))
    return ppr, rpr, tcpr


def add_table(header, rows, widths_cm=None):
    ncol = len(header)
    t = doc.add_table(rows=1 + len(rows), cols=ncol)
    final_sect.addprevious(t._tbl)
    if proto_tblPr is not None:
        old = t._tbl.find(qn("w:tblPr"))
        t._tbl.replace(old, copy.deepcopy(proto_tblPr))
    total = 15.0
    if widths_cm is None:
        lens = [max(len(header[c]), *(len(r[c]) for r in rows)) if rows else len(header[c]) for c in range(ncol)]
        lens = [min(max(l, 6), 60) for l in lens]
        widths_cm = [total * l / sum(lens) for l in lens]
        # cột không hẹp hơn từ dài nhất (tránh gãy chữ như "WARNIN G")
        longest = [max(len(wd) for v in [header[c]] + [r[c] for r in rows] for wd in (v.split() or [""])) for c in range(ncol)]
        mins = [max(1.7, 0.25 * min(lw, 24) + 0.6) for lw in longest]
        fixed = set()
        for _ in range(ncol):  # cột dưới mức tối thiểu được giữ cố định, phần còn lại chia theo tỉ lệ
            free = [c for c in range(ncol) if c not in fixed]
            room = total - sum(mins[c] for c in fixed)
            base = sum(lens[c] for c in free)
            widths_cm = [mins[c] if c in fixed else room * lens[c] / base for c in range(ncol)]
            low = {c for c in free if widths_cm[c] < mins[c]}
            if not low:
                break
            fixed |= low
    grid = t._tbl.find(qn("w:tblGrid"))
    for gc, w in zip(grid.findall(qn("w:gridCol")), widths_cm):
        gc.set(qn("w:w"), str(int(Cm(w).twips)))
    hdr = cell_proto(0) if proto_rows else (None, None, None)
    bod = cell_proto(1) if len(proto_rows) > 1 else hdr
    for ri, vals in enumerate([header] + rows):
        ppr, rpr, tcpr = hdr if ri == 0 else bod
        for ci, v in enumerate(vals):
            cell = t.rows[ri].cells[ci]
            cell.width = Cm(widths_cm[ci])
            if tcpr is not None:
                newtcpr = copy.deepcopy(tcpr)
                for x in newtcpr.findall(qn("w:tcW")) + newtcpr.findall(qn("w:gridSpan")) + newtcpr.findall(qn("w:vMerge")):
                    newtcpr.remove(x)
                cur = cell._tc.find(qn("w:tcPr"))
                w = cur.find(qn("w:tcW")) if cur is not None else None
                if cur is not None:
                    cell._tc.remove(cur)
                if w is not None:
                    newtcpr.insert(0, w)
                cell._tc.insert(0, newtcpr)
            par = cell.paragraphs[0]
            if ppr is not None:
                old = par._p.find(qn("w:pPr"))
                if old is not None:
                    par._p.remove(old)
                newppr = copy.deepcopy(ppr)
                for kn in newppr.findall(qn("w:keepNext")):  # mẫu giữ cả bảng liền khối → bảng dài nhảy trang
                    newppr.remove(kn)
                par._p.insert(0, newppr)
            run = par.add_run(v)
            if rpr is not None:
                run._r.insert(0, copy.deepcopy(rpr))
            if ri == 0:
                run.bold = True
    # hàng tiêu đề lặp lại khi bảng sang trang; hàng không bị cắt ngang trang
    for ri, row in enumerate(t.rows):
        trpr = row._tr.get_or_add_trPr()
        cs = OxmlElement("w:cantSplit")
        trpr.append(cs)
        if ri == 0:
            hd = OxmlElement("w:tblHeader")
            trpr.append(hd)
            for cell in row.cells:
                for par in cell.paragraphs:
                    par.paragraph_format.keep_with_next = True
    return t


def parse_md_table(lines):
    rows = [[c.strip() for c in ln.strip().strip("|").split("|")] for ln in lines if not re.match(r"^\|\s*-", ln)]
    return rows[0], rows[1:]


def add_code(lines):
    for ln in lines:
        par = add_par(style="Normal", align=WD_ALIGN_PARAGRAPH.LEFT, indent_first=Cm(0))
        pf = par.paragraph_format
        pf.space_before = Pt(0)
        pf.space_after = Pt(0)
        pf.line_spacing = 1.0
        pf.left_indent = Cm(0.5)
        run = par.add_run(ln if ln else " ")
        run.font.name = "Consolas"
        run._r.get_or_add_rPr().get_or_add_rFonts().set(qn("w:cs"), "Consolas")
        run.font.size = Pt(10)


def api_table():
    data = json.loads((MD_DIR / "api_routes.json").read_text(encoding="utf-8"))
    rows = []
    for r in data:
        method = r["method"].replace("|HEAD", "")
        uri = "/" + r["uri"]
        action = r["action"].split("\\")[-1].replace("Controller@", " @ ")
        rows.append([method, uri, action])
    rows.sort(key=lambda x: x[1])
    cap = add_par("Bảng A.1: Danh sách route API (php artisan route:list --path=api)", style="Table Caption")
    cap.paragraph_format.keep_with_next = True
    add_table(["Phương thức", "Đường dẫn", "Controller @ hàm"], rows, [2.4, 7.3, 5.3])


# ------------------------------------------------------------ Dựng nội dung
for md in sorted(MD_DIR.glob("0[1-9]_*.md")):
    lines = md.read_text(encoding="utf-8").split("\n")
    i = 0
    while i < len(lines):
        ln = lines[i].rstrip()
        if not ln.strip() or ln.startswith("<!--"):
            i += 1
            continue
        if ln.startswith("# "):
            par = add_par(ln[2:].strip(), style="Heading 1")
        elif ln.startswith("## "):
            add_par(ln[3:].strip(), style="Heading 2")
        elif ln.startswith("### "):
            add_par(ln[4:].strip(), style="Heading 3")
        elif ln.startswith("BANG:"):
            add_par(ln[5:].strip(), style="Table Caption").paragraph_format.keep_with_next = True
            j = i + 1
            tbl = []
            while j < len(lines) and lines[j].strip().startswith("|"):
                tbl.append(lines[j])
                j += 1
            header, rows = parse_md_table(tbl)
            add_table(header, rows)
            add_par("", style="Normal")
            i = j
            continue
        elif ln.startswith("HINH:"):
            cap = ln[5:].strip()
            num = re.match(r"Hình (\d+)\.(\d+)", cap)
            img = ROOT / "hinh" / f"hinh_{num.group(1)}_{num.group(2)}.png" if num else None
            if img is not None and img.exists():
                # Ảnh sơ đồ sinh bởi py/ve_so_do_drawio.py
                ph = add_par(align=WD_ALIGN_PARAGRAPH.CENTER, indent_first=Cm(0))
                from PIL import Image as _Img
                with _Img.open(img) as im:
                    ratio = im.height / im.width
                width = min(15.0, 20.0 / ratio, im.width / 60)
                ph.add_run().add_picture(str(img), width=Cm(width))
            else:
                ph = add_par(f"[CHÈN HÌNH: {cap.split(':', 1)[1].strip()}]", align=WD_ALIGN_PARAGRAPH.CENTER, indent_first=Cm(0))
                ph.runs[0].italic = True
            ph.paragraph_format.keep_with_next = True
            add_par(cap, style="Image Caption")
        elif ln.strip() == "CODE:":
            j = i + 1
            code = []
            while j < len(lines) and lines[j].strip() != "ENDCODE":
                code.append(lines[j].rstrip())
                j += 1
            add_code(code)
            i = j + 1
            continue
        elif ln.startswith("REF:"):
            par = add_par(ln[4:].strip(), align=WD_ALIGN_PARAGRAPH.LEFT, indent_first=Cm(-0.8))
            par.paragraph_format.left_indent = Cm(0.8)
        elif ln.strip() == "@@API_TABLE":
            api_table()
        elif ln.startswith("- "):
            add_par("– " + ln[2:].strip())
        else:
            add_par(ln.strip())
        i += 1

# Word hỏi cập nhật mục lục khi mở file
settings = doc.settings.element
if settings.find(qn("w:updateFields")) is None:
    uf = OxmlElement("w:updateFields")
    uf.set(qn("w:val"), "true")
    settings.append(uf)

# Xóa ảnh của mẫu không còn được tham chiếu (hình khóa luận cũ, chữ ký) khỏi gói .docx
used = {b.get(qn("r:embed")) for b in body.iter(qn("a:blip"))}
part = doc.part
for rid, rel in list(part.rels.items()):
    if rel.reltype.endswith("/image") and rid not in used:
        part.drop_rel(rid)
        if rid in part.rels:
            del part.rels[rid]

doc.core_properties.author = "Nhóm 16"
doc.core_properties.title = "Báo cáo đồ án IoT Nhóm 16"
doc.core_properties.comments = ""
doc.core_properties.last_modified_by = "Nhóm 16"
doc.core_properties.revision = 1
import datetime as _dt
doc.core_properties.created = doc.core_properties.modified = _dt.datetime.now()
doc.core_properties.last_printed = _dt.datetime.now()
doc.save(str(OUT))
print("Đã tạo:", OUT)
