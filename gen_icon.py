from PIL import Image, ImageDraw
import math

# 生成一个"风扇 + 扳手"主题的 ICO 图标
SIZES = [16, 24, 32, 48, 64, 128, 256]


def draw_wrench(draw, size, cx, cy, scale, color, outline=None):
    """在 (cx, cy) 处绘制一个倾斜的扳手，scale 为相对于 size 的比例。"""
    s = size * scale
    lw = max(1, int(s * 0.14))      # 扳手杆宽度
    head_r = int(s * 0.28)          # 扳手头部半径
    handle_len = int(s * 0.78)
    angle = math.radians(-45)       # 从左上到右下倾斜

    # 头部圆心位置（向扳手顶端偏移）
    hx = cx + math.cos(angle) * handle_len * 0.45
    hy = cy + math.sin(angle) * handle_len * 0.45

    # 扳手头部：C 形开口
    draw.ellipse(
        [hx - head_r, hy - head_r, hx + head_r, hy + head_r],
        fill=color,
        outline=outline,
        width=max(1, size // 80)
    )
    # 开口缺口（用背景色画一个楔形小三角覆盖）
    gap_w = int(head_r * 0.55)
    gap_h = int(head_r * 0.9)
    points = [
        (hx, hy),
        (hx + gap_w, hy - gap_h),
        (hx - gap_w * 0.3, hy - gap_h * 1.2),
    ]
    # 缺口用透明色擦除，这里传入擦除色由上层决定

    # 扳手杆：从头部圆心向另一方向延伸的圆角矩形
    ux = math.cos(angle)
    uy = math.sin(angle)
    px, py = -uy, ux  # 垂直单位向量

    # 杆的四个端点
    x0 = hx + px * lw * 0.5
    y0 = hy + py * lw * 0.5
    x1 = hx - px * lw * 0.5
    y1 = hy - py * lw * 0.5
    x2 = cx - ux * handle_len * 0.55 - px * lw * 0.35
    y2 = cy - uy * handle_len * 0.55 - py * lw * 0.35
    x3 = cx - ux * handle_len * 0.55 + px * lw * 0.35
    y3 = cy - uy * handle_len * 0.55 + py * lw * 0.35
    draw.polygon([(x0, y0), (x1, y1), (x2, y2), (x3, y3)], fill=color)

    # 给杆画外轮廓线
    if outline:
        draw.line([(x0, y0), (x1, y1)], fill=outline, width=max(1, size // 120))
        draw.line([(x1, y1), (x2, y2)], fill=outline, width=max(1, size // 120))
        draw.line([(x2, y2), (x3, y3)], fill=outline, width=max(1, size // 120))
        draw.line([(x3, y3), (x0, y0)], fill=outline, width=max(1, size // 120))

    return points  # 返回缺口坐标，供上层擦除


def draw_fan(size):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    cx, cy = size // 2, size // 2
    pad = int(size * 0.08)
    outer_r = size // 2 - pad
    inner_r = int(outer_r * 0.22)
    blade_len = outer_r - inner_r
    blade_w = int(blade_len * 0.45)

    # 配色
    bg_color = (35, 42, 58, 255)
    blade_color = (0, 184, 217, 255)
    center_color = (230, 245, 255, 255)
    shadow_color = (0, 130, 155, 255)
    wrench_color = (255, 186, 80, 255)   # 暖橙色扳手，突出工具感
    wrench_outline = (200, 130, 40, 255)
    erase_color = bg_color

    # 背景圆
    draw.ellipse([pad, pad, size - pad, size - pad], fill=bg_color)

    # 四个风扇叶片
    for i in range(4):
        angle = math.radians(i * 90)
        x0 = cx + math.cos(angle) * inner_r
        y0 = cy + math.sin(angle) * inner_r
        x1 = cx + math.cos(angle) * outer_r * 0.88
        y1 = cy + math.sin(angle) * outer_r * 0.88
        perp = math.radians(i * 90 + 90)
        wx = math.cos(perp) * blade_w * 0.5
        wy = math.sin(perp) * blade_w * 0.5

        polygon = [
            (x0 + wx, y0 + wy),
            (x1 + wx * 1.4, y1 + wy * 1.4),
            (x1 - wx * 0.2, y1 - wy * 0.2),
            (x0 - wx, y0 - wy),
        ]
        draw.polygon(polygon, fill=blade_color)
        draw.line([(x0, y0), (x1, y1)], fill=shadow_color, width=max(1, size // 64))

    # 在中心绘制扳手（覆盖在叶片汇合处）
    gap_points = draw_wrench(draw, size, cx, cy, 0.42, wrench_color, wrench_outline)

    # 用背景色擦除扳手头部缺口，形成 C 形开口
    draw.polygon(gap_points, fill=erase_color)

    # 中心小圆点（表现螺母/轴心）
    dot_r = max(2, size // 32)
    draw.ellipse(
        [cx - dot_r, cy - dot_r, cx + dot_r, cy + dot_r],
        fill=wrench_outline
    )

    return img


images = [draw_fan(sz) for sz in SIZES]

images[0].save(
    "icon.ico",
    format="ICO",
    sizes=[(im.width, im.height) for im in images],
    append_images=images[1:]
)
print("icon.ico generated with sizes:", SIZES)
