"""Sinh sơ đồ draw.io cho báo cáo IoT Nhóm 16 và xuất PNG bằng draw.io Desktop.

Cách chạy (từ thư mục Bao_cao):
    python py/ve_so_do_drawio.py            # sinh .drawio và xuất PNG
    python py/ve_so_do_drawio.py --no-png   # chỉ sinh .drawio

Kết quả: so_do/hinh_x_y.drawio và hinh/hinh_x_y.png.
Chạy lại sẽ GHI ĐÈ file .drawio đã sửa tay.
Mở file .drawio bằng draw.io (Desktop hoặc app.diagrams.net) để chỉnh tay.
Căn cứ nội dung: src/main.cpp và node-red/flows.json (19/11/2026).
"""
import html
import subprocess
import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")
ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "so_do"
PNG_DIR = ROOT / "hinh"
OUT.mkdir(exist_ok=True)
DRAWIO = Path.home() / "AppData/Local/Programs/draw.io/draw.io.exe"

FONT = "fontFamily=Times New Roman;fontSize=13;"
BOX = "rounded=1;whiteSpace=wrap;html=1;arcSize=12;strokeColor=#333333;fillColor=#FFFFFF;" + FONT
NOTE = "text;html=1;whiteSpace=wrap;align=left;verticalAlign=top;" + FONT
EDGE = "edgeStyle=orthogonalEdgeStyle;rounded=1;html=1;endArrow=block;endFill=1;strokeColor=#333333;" + FONT + "fontSize=12;labelBackgroundColor=#FFFFFF;"


class Diagram:
    def __init__(self, name):
        self.name = name
        self.cells = []
        self.n = 0

    def _id(self):
        self.n += 1
        return f"c{self.n}"

    def v(self, label, x, y, w, h, style=BOX, parent="1", cid=None):
        cid = cid or self._id()
        self.cells.append(
            f'<mxCell id="{cid}" value="{html.escape(label, quote=True)}" style="{style}" vertex="1" parent="{parent}">'
            f'<mxGeometry x="{x}" y="{y}" width="{w}" height="{h}" as="geometry"/></mxCell>'
        )
        return cid

    def e(self, s, t, label="", style=EDGE, points=None, parent="1"):
        cid = self._id()
        pts = ""
        if points:
            pts = '<Array as="points">' + "".join(f'<mxPoint x="{x}" y="{y}"/>' for x, y in points) + "</Array>"
        self.cells.append(
            f'<mxCell id="{cid}" value="{html.escape(label, quote=True)}" style="{style}" edge="1" source="{s}" target="{t}" parent="{parent}">'
            f'<mxGeometry relative="1" as="geometry">{pts}</mxGeometry></mxCell>'
        )
        return cid

    def model(self):
        return (
            '<mxGraphModel adaptiveColors="auto" background="#FFFFFF"><root><mxCell id="0"/><mxCell id="1" parent="0"/>'
            + "".join(self.cells)
            + "</root></mxGraphModel>"
        )

    def save(self):
        path = OUT / f"{self.name}.drawio"
        path.write_text(f'<mxfile><diagram name="{self.name}">{self.model()}</diagram></mxfile>', encoding="utf-8")
        return path


def b(t):
    return f"<b>{t}</b>"


# ------------------------------------------------------------ Sơ đồ IoT
LANE = "swimlane;startSize=40;html=1;whiteSpace=wrap;fillColor=#F2F2F2;strokeColor=#333333;" + FONT + "fontStyle=1;"
ELL = "ellipse;whiteSpace=wrap;html=1;strokeColor=#333333;fillColor=#FFFFFF;" + FONT
LEFT = BOX + "align=left;spacingLeft=8;"


def pt(style, ex, ey, nx, ny):
    return style + f"exitX={ex};exitY={ey};exitDx=0;exitDy=0;entryX={nx};entryY={ny};entryDx=0;entryDy=0;"


# ------------------------------------------------------------ Hình 3.1
def hinh_3_1():
    d = Diagram("hinh_3_1")
    xs = [20, 320, 620, 920]
    titles = ["Tầng 1: Cảm biến và thiết bị biên", "Tầng 2: Mạng và truyền thông",
              "Tầng 3: Xử lý trung gian", "Tầng 4: Ứng dụng"]
    lanes = [d.v(t, x, 20, 240, 420, LANE) for t, x in zip(titles, xs)]
    pot = d.v("Biến trở<br>(tải giả lập)", 30, 60, 180, 50, parent=lanes[0])
    esp = d.v(b("ESP32") + "<br>Lọc trung bình 8 mẫu<br>Máy trạng thái bảo vệ", 30, 170, 180, 90, parent=lanes[0])
    act = d.v("Relay, LED đỏ, còi", 30, 330, 180, 50, parent=lanes[0])
    d.e(pot, esp, "ADC (GPIO 34)")
    d.e(esp, act, "GPIO 25, 26, 33")
    d.v("Wi-Fi, bản tin JSON", 30, 70, 180, 40, NOTE + "align=center;", parent=lanes[1])
    brk = d.v(b("Broker MQTT") + "<br>EMQX Cloud, cổng 8883<br>TLS + tài khoản", 30, 170, 180, 90, parent=lanes[1])
    nr = d.v(b("Node-RED (Docker)") + "<br>• Tách U, I, P<br>• Kiểm tra quá tải<br>• Tính tiền điện EVN<br>• Ghi log CSV mỗi 30 s",
             30, 130, 180, 170, LEFT, parent=lanes[2])
    dash = d.v(b("Node-RED Dashboard") + "<br>Đồng hồ, biểu đồ,<br>trạng thái, nút điều khiển", 30, 170, 180, 90, parent=lanes[3])
    usr = d.v("Người dùng", 30, 330, 180, 50, parent=lanes[3])
    d.e(esp, brk, "telemetry,<br>alarm, status", pt(EDGE, 1, 0.3, 0, 0.3))
    d.e(brk, esp, "control", pt(EDGE, 0, 0.75, 1, 0.75))
    d.e(brk, nr, "telemetry", pt(EDGE, 1, 0.3, 0, 0.4))
    d.e(nr, brk, "control", pt(EDGE, 0, 0.6, 1, 0.75))
    d.e(nr, dash, "dữ liệu", pt(EDGE, 1, 0.4, 0, 0.3))
    d.e(dash, nr, "lệnh relay", pt(EDGE, 0, 0.75, 1, 0.6))
    d.e(usr, dash, "xem, bấm nút", EDGE + "startArrow=block;startFill=1;")
    return d


# ------------------------------------------------------------ Hình 3.3
def hinh_3_3():
    d = Diagram("hinh_3_3")
    start = d.v("", 140, 20, 24, 24, "ellipse;html=1;fillColor=#333333;strokeColor=#333333;")
    n = d.v(b("NORMAL") + "<br>LED tắt, còi tắt<br>Relay đóng", 40, 110, 230, 100, ELL)
    w = d.v(b("WARNING") + "<br>LED sáng, còi tắt<br>Relay vẫn đóng, đếm giờ", 400, 110, 230, 100, ELL)
    c = d.v(b("CRITICAL") + "<br>LED sáng, còi 1.500 Hz<br>Relay ngắt, gửi alarm", 900, 110, 230, 100, ELL)
    d.e(start, n, "Khởi động")
    d.e(n, w, "P > 1.200 W", pt(EDGE, 0.95, 0.3, 0.05, 0.3))
    d.e(w, n, "P < 1.100 W", pt(EDGE, 0.05, 0.7, 0.95, 0.7))
    d.e(w, c, "P > 1.200 W<br>liên tục ≥ 3 s")
    d.e(c, n, "P < 1.100 W (đóng lại relay)", pt(EDGE, 0.5, 1, 0.5, 1), points=[(1015, 280), (155, 280)])
    return d


# ------------------------------------------------------------ Sơ đồ cho slide (chữ to, 16:9)
SF = "fontFamily=Arial;fontSize=20;"
SBOX = "rounded=1;whiteSpace=wrap;html=1;arcSize=10;strokeColor=#2072B7;strokeWidth=2;fillColor=#FFFFFF;" + SF
SLANE = "swimlane;startSize=56;html=1;whiteSpace=wrap;strokeColor=#2072B7;strokeWidth=2;" + SF + "fontSize=22;fontStyle=1;"
SEDGE = "edgeStyle=orthogonalEdgeStyle;rounded=1;html=1;endArrow=block;endFill=1;strokeColor=#2072B7;strokeWidth=2;" + SF + "fontSize=18;labelBackgroundColor=#FFFFFF;"


def slide_2_luong():
    d = Diagram("slide_2_luong")
    lp = d.v("Luồng bảo vệ – ưu tiên cao, chạy mỗi 10 ms", 20, 20, 520, 520, SLANE + "fillColor=#DCEBF7;")
    ln = d.v("Luồng mạng – vòng lặp chính", 940, 20, 520, 520, SLANE + "fillColor=#FDEBD8;")
    a1 = d.v(b("Đọc biến trở (ADC)") + "<br>lọc trung bình 8 mẫu, tính P, I, E", 40, 80, 440, 100, SBOX, parent=lp)
    a2 = d.v(b("Máy trạng thái") + "<br>NORMAL → WARNING → CRITICAL", 40, 230, 440, 100, SBOX, parent=lp)
    a3 = d.v(b("LED, còi, relay") + "<br>ngắt tải đúng 3 s sau khi quá tải", 40, 380, 440, 100, SBOX, parent=lp)
    d.e(a1, a2, style=SEDGE)
    d.e(a2, a3, style=SEDGE)
    b1 = d.v(b("Kết nối Wi-Fi, MQTT") + "<br>thử lại mỗi 5 s (có thể đứng ~5 s)", 40, 80, 440, 100, SBOX, parent=ln)
    b2 = d.v(b("Gửi dữ liệu") + "<br>telemetry 2 s, alarm, nhận control", 40, 230, 440, 100, SBOX, parent=ln)
    b3 = d.v(b("In nhật ký Serial") + "<br>kèm thời điểm chuyển trạng thái", 40, 380, 440, 100, SBOX, parent=ln)
    d.e(b1, b2, style=SEDGE)
    d.e(b2, b3, style=SEDGE)
    f = d.v(b("Biến cờ dùng chung") + '<br><font style="font-size:17px">lệnh relay_on, relay_off, reset'
            "<br>cờ cảnh báo, nhật ký FSM</font>",
            600, 220, 300, 130, SBOX + "fillColor=#FFF7CC;strokeColor=#B8860B;dashed=1;")
    d.e(f, a2, "", pt(SEDGE, 0, 0.3, 1, 0.3))
    d.e(a2, f, "", pt(SEDGE, 1, 0.75, 0, 0.75))
    d.e(b2, f, "", pt(SEDGE, 0, 0.3, 1, 0.3))
    d.e(f, b2, "", pt(SEDGE, 1, 0.75, 0, 0.75))
    d.v(b("Mất broker:") + " chỉ luồng mạng chậm lại, luồng bảo vệ vẫn ngắt đúng 3 s "
        "(T5: WARNING t = 19.304 ms → CRITICAL t = 22.304 ms)",
        20, 560, 1440, 60, NOTE + SF + "fontSize=20;align=center;verticalAlign=middle;")
    return d


def slide_4_chuc_nang():
    d = Diagram("slide_4_chuc_nang")
    items = [
        ("1. Đo và lọc", "Biến trở thay cảm biến<br>Lọc trung bình 8 mẫu<br>Tính P, I, điện năng E"),
        ("2. Bảo vệ tại chỗ", "3 mức NORMAL, WARNING,<br>CRITICAL<br>Ngắt relay sau 3 s quá tải"),
        ("3. Truyền MQTT", "telemetry mỗi 2 s<br>alarm, status (LWT)<br>control nhận lệnh"),
        ("4. Giám sát, điều khiển", "Dashboard Node-RED<br>Tiền điện, log CSV<br>Nút cắt / bật nguồn"),
    ]
    ids = []
    for k, (t, s) in enumerate(items):
        x = 20 + k * 360
        ids.append(d.v(f'<font style="font-size:24px"><b>{t}</b></font><br><br>{s}', x, 20, 300, 200,
                       SBOX + "verticalAlign=top;spacingTop=18;" + ("fillColor=#DCEBF7;" if k == 1 else "")))
    for a, c in zip(ids, ids[1:]):
        d.e(a, c, style=SEDGE)
    return d


DIAGRAMS = [hinh_3_1, hinh_3_3]
SLIDE_DIAGRAMS = [slide_2_luong, slide_4_chuc_nang]

if __name__ == "__main__":
    if "--slide" in sys.argv:
        DIAGRAMS = SLIDE_DIAGRAMS
    for fn in DIAGRAMS:
        dg = fn()
        path = dg.save()
        print("Đã ghi", path.name)
        if "--no-png" not in sys.argv:
            png = PNG_DIR / (path.stem + ".png")
            subprocess.run([str(DRAWIO), "-x", "-f", "png", "-s", "2", "-b", "10", "-o", str(png), str(path)],
                           check=True, capture_output=True, timeout=180)
            print("  → PNG", png.name)
