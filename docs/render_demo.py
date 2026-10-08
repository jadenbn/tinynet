"""Draw a README GIF from movement-demo.cpp's actual network/simulation trace."""
import csv
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

WIDTH, HEIGHT = 800, 500
BACKGROUND = '#101923'
BLUE, ORANGE, RED = '#57baff', '#ffb45c', '#ff6f7e'
SCALE, CENTER_X, CENTER_Z = 16, 0, 0
FONT = '/System/Library/Fonts/Supplemental/Arial.ttf'
font = ImageFont.truetype(FONT, 16) if Path(FONT).exists() else ImageFont.load_default()
small = ImageFont.truetype(FONT, 13) if Path(FONT).exists() else font
large = ImageFont.truetype(FONT, 25) if Path(FONT).exists() else font


def point(x, z):
    return 305 + (x-CENTER_X)*SCALE, 265 - (z-CENTER_Z)*SCALE


def circle(draw, p, color, radius=10, outline=False):
    x, y = p
    bounds = (x-radius, y-radius, x+radius, y+radius)
    draw.ellipse(bounds, outline=color, width=2) if outline else draw.ellipse(bounds, fill=color)


def player(draw, row, prefix, color):
    x, y = point(row[prefix+'x'], row[prefix+'z'])
    qx, qy, qz, qw = (row[prefix+k] for k in ('qx', 'qy', 'qz', 'qw'))
    # Rotate (0,0,1) using the recorded unit quaternion.
    forward_x = 2*(qx*qz + qw*qy)
    forward_z = 1-2*(qx*qx + qy*qy)
    circle(draw, (x, y), color)
    draw.line((x, y, x+forward_x*24, y-forward_z*24), fill=color, width=3)


def main():
    global SCALE, CENTER_X, CENTER_Z
    if len(sys.argv) != 3:
        raise SystemExit('usage: render_demo.py trace.csv output.gif')
    with open(sys.argv[1], newline='') as source:
        rows = [{key: float(value) for key, value in row.items()}
                for row in csv.DictReader(source)]
    assert len(rows) > 100, 'demo trace did not capture enough frames'
    xs = [row[key] for row in rows for key in ('px', 'ax', 'rx', 'rawx')]
    zs = [row[key] for row in rows for key in ('pz', 'az', 'rz', 'rawz')]
    CENTER_X, CENTER_Z = (min(xs)+max(xs))/2, (min(zs)+max(zs))/2
    SCALE = min(560/(max(xs)-min(xs)+4), 310/(max(zs)-min(zs)+4))
    frames = []
    trail_local, trail_remote = [], []
    for row in rows:
        image = Image.new('RGB', (WIDTH, HEIGHT), BACKGROUND)
        draw = ImageDraw.Draw(image)
        for x in range(11, 601, 23):
            draw.line((x, 85, x, 445), fill='#223141')
        for y in range(82, 445, 23):
            draw.line((10, y, 600, y), fill='#223141')
        trail_local.append(point(row['px'], row['pz']))
        trail_remote.append(point(row['rx'], row['rz']))
        if len(trail_local) > 1:
            draw.line(trail_local[-45:], fill='#25506d', width=2)
            draw.line(trail_remote[-45:], fill='#674b32', width=2)
        circle(draw, point(row['ax'], row['az']), RED, 15, True)
        circle(draw, point(row['rawx'], row['rawz']), '#927a59', 15, True)
        player(draw, row, 'p', BLUE)
        player(draw, row, 'r', ORANGE)
        draw.rectangle((0, 0, WIDTH, 80), fill=BACKGROUND)
        draw.text((20, 15), 'tinynet / movement over UDP', font=large, fill='white')
        draw.text((20, 49), '60Hz simulation | 50ms each way | 15ms jitter | 5% loss | 2% duplicates',
                  font=small, fill='#a5bacd')
        draw.rectangle((610, 90, 790, 440), fill='#182534')
        lines = [('LOCAL PLAYER', BLUE), ('Immediate prediction', 'white'),
                 ('Red ring: server update', RED), ('', 'white'),
                 ('REMOTE PLAYER', ORANGE), ('100ms interpolation', 'white'),
                 ('Faint ring: raw update', '#a5bacd'), ('', 'white'),
                 (f"RTT: {row['rtt']:.0f}ms", 'white'),
                 (f"Unacked inputs: {row['pending']:.0f}", 'white'), ('', 'white'),
                 ('Snapshot / 2 players', '#a5bacd'), (f"{row['rawbytes']:.0f} -> {row['packedbytes']:.0f} bytes", 'white'),
                 (f"{100*(1-row['packedbytes']/row['rawbytes']):.1f}% smaller", BLUE)]
        for index, (text, color) in enumerate(lines):
            draw.text((620, 105+index*22), text, font=small, fill=color)
        label = 'Inputs released: friction brings players to rest' if row['time'] >= 10 else 'Scripted inputs through the real client/server code'
        draw.rectangle((0, 450, WIDTH, HEIGHT), fill=BACKGROUND)
        draw.text((20, 464), label, font=font, fill='#a5bacd')
        if not frames:
            palette = image.convert('P', palette=Image.Palette.ADAPTIVE, colors=64)
        frames.append(image.quantize(palette=palette, dither=Image.Dither.NONE))
    # GIF stores durations in hundredths of a second: 30+30+40ms = 30fps.
    duration = [30 if i % 3 != 2 else 40 for i in range(len(frames))]
    frames[0].save(sys.argv[2], save_all=True, append_images=frames[1:],
                   duration=duration, loop=0, optimize=True, disposal=1)
    with Image.open(sys.argv[2]) as gif:
        assert gif.n_frames > 100 and gif.size == (WIDTH, HEIGHT)
    print(f'Saved {len(frames)} frames to {sys.argv[2]}')


if __name__ == '__main__':
    main()
