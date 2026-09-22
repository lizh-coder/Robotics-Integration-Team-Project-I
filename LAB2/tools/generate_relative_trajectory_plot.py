from pathlib import Path
from math import cos, pi, sin, sqrt

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(r"D:\02_Academic\大三\机器人小组项目")
OUT = ROOT / "LAB2" / "tools" / "relative_trajectory_plane.png"


def main():
    width, height = 1364, 880
    left, top, right, bottom = 160, 115, 1230, 735
    image = Image.new("RGB", (width, height), "white")
    draw = ImageDraw.Draw(image)
    font_path = r"C:\Windows\Fonts\arial.ttf"
    title_font = ImageFont.truetype(font_path, 34)
    body_font = ImageFont.truetype(font_path, 25)
    small_font = ImageFont.truetype(font_path, 22)
    draw.rectangle((left, top, right, bottom), fill="#F2F2F2", outline="#777777", width=2)
    draw.text((265, 24), "Relative AV2 trajectory on the plane z - 2y = 1", fill="#111111", font=title_font)

    cx, cy = (left + right) / 2, (top + bottom) / 2
    scale = 245
    for grid in [-1.0, -0.5, 0.0, 0.5, 1.0]:
        x = cx + grid * scale
        draw.line((x, top, x, bottom), fill="#D0D0D0", width=1)
        y = cy - grid * scale
        draw.line((left, y, right, y), fill="#D0D0D0", width=1)
    draw.line((left, cy, right, cy), fill="#777777", width=2)
    draw.line((cx, top, cx, bottom), fill="#777777", width=2)

    points = []
    for i in range(601):
        t = 2 * pi * i / 600
        x = 0.5 * sin(2 * t)
        y = sqrt(5) / 2 * cos(2 * t)
        points.append((cx + x * scale, cy - y * scale))
    draw.line(points, fill="#111111", width=6, joint="curve")
    draw.ellipse((cx - 9, cy - 9, cx + 9, cy + 9), fill="#555555")

    draw.line((cx, cy, cx + 0.5 * scale, cy), fill="#555555", width=3)
    draw.polygon([(cx + 0.5 * scale, cy), (cx + 0.5 * scale - 16, cy - 8), (cx + 0.5 * scale - 16, cy + 8)], fill="#555555")
    draw.text((cx + 55, cy + 30), "semi-axis = 0.5", fill="#333333", font=small_font)
    draw.line((cx, cy, cx, cy - sqrt(5) / 2 * scale), fill="#555555", width=3)
    draw.polygon([(cx, cy - sqrt(5) / 2 * scale), (cx - 8, cy - sqrt(5) / 2 * scale + 16), (cx + 8, cy - sqrt(5) / 2 * scale + 16)], fill="#555555")
    draw.text((cx + 30, top + 32), "semi-axis = sqrt(5)/2", fill="#333333", font=small_font)
    draw.text((right - 380, bottom + 32), "x_p = 0.5 sin(2t)", fill="#222222", font=body_font)
    draw.text((28, 78), "y_p = sqrt(5)/2 cos(2t)", fill="#222222", font=body_font)
    draw.text((cx + 15, cy + 15), "center p", fill="#333333", font=small_font)
    image.save(OUT)


if __name__ == "__main__":
    main()
