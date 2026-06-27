#!/usr/bin/env python3
"""Download Hey habit SVGs and rasterize to 20x20 PNGs (black + white)."""

import json
import re
import subprocess
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "resources" / "images" / "habits"
CSS_URL = "https://app.hey.com/assets/web/calendar/habits-356520a4.css"
SIZE = 20
ICON_FIT = 16  # max dimension inside SIZE x SIZE canvas
USER_AGENT = "Mozilla/5.0 (compatible; hey-watchface/1.0)"


def http_get(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read()


def fetch_icons_from_css():
    css = http_get(CSS_URL).decode("utf-8")
    pattern = re.compile(
        r'btn--habit--([a-z]+)::before\s*\{\s*background-image:\s*url\("(/assets/icons/habits/[^"]+)"\)'
    )
    icons = {}
    for slug, path in pattern.findall(css):
        icons[slug] = f"https://app.hey.com{path}"
    return icons


def center_on_canvas(png_path: Path, canvas: int = SIZE, max_content: int = ICON_FIT):
  """Place rasterized content on a fixed canvas, scaled to max_content."""
  from PIL import Image

  img = Image.open(png_path).convert("RGBA")
  bbox = img.getbbox()
  out = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
  if bbox:
    cropped = img.crop(bbox)
    bw, bh = cropped.size
    scale = min(1.0, max_content / max(bw, bh))
    nw = max(1, int(bw * scale))
    nh = max(1, int(bh * scale))
    if nw != bw or nh != bh:
      cropped = cropped.resize((nw, nh), Image.LANCZOS)
    ox = (canvas - nw) // 2
    oy = (canvas - nh) // 2
    out.paste(cropped, (ox, oy), cropped)
  out.save(png_path)


def rasterize(svg_path: Path, png_path: Path, color: str):
  svg = svg_path.read_text()
  svg = re.sub(r'fill="[^"]*"', f'fill="{color}"', svg)
  svg = svg.replace('fill="#000"', f'fill="{color}"')
  if 'fill=' not in svg and '<path' in svg:
    svg = svg.replace("<path", f'<path fill="{color}"', 1)
  tmp = png_path.with_suffix(".tmp.svg")
  tmp.write_text(svg)
  subprocess.run(
      [
          "npx", "--yes", "@resvg/resvg-js-cli",
          str(tmp), str(png_path),
          "--fit-width", str(ICON_FIT),
          "--background", "rgba(0,0,0,0)",
      ],
      check=True,
      cwd=ROOT,
      stdout=subprocess.DEVNULL,
      stderr=subprocess.DEVNULL,
  )
  tmp.unlink(missing_ok=True)
  pebbleize_png(png_path)
  center_on_canvas(png_path)


def pebbleize_png(png_path: Path):
  """Flatten to crisp alpha-masked PNGs Pebble draws correctly."""
  from PIL import Image

  is_white = png_path.stem.endswith("_w")
  ink = (255, 255, 255, 255) if is_white else (35, 28, 51, 255)
  img = Image.open(png_path).convert("RGBA")
  out = Image.new("RGBA", img.size, (0, 0, 0, 0))
  px = img.load()
  dst = out.load()
  for y in range(img.height):
    for x in range(img.width):
      r, g, b, a = px[x, y]
      if a > 32:
        dst[x, y] = ink
  out.save(png_path)


def pad_all_icons():
  """Re-center existing PNGs on the 20x20 canvas (no re-download)."""
  paths = sorted(OUT.glob("*.png"))
  total = len(paths)
  for i, png_path in enumerate(paths, 1):
    print(f"[{i}/{total}] center {png_path.name}", flush=True)
    pebbleize_png(png_path)
    center_on_canvas(png_path)
  print(f"Centered {total} icons in {OUT}")


def sync_package_json(media):
  """Update package.json pebble.resources.media from generated icon list."""
  pkg_path = ROOT / "package.json"
  pkg = json.loads(pkg_path.read_text())
  pkg["pebble"]["resources"]["media"] = media
  pkg_path.write_text(json.dumps(pkg, indent=2) + "\n")
  print(f"Updated {pkg_path.relative_to(ROOT)}")


def process_slug(slug: str, url: str):
  svg_file = OUT / f"{slug}.svg"
  svg_file.write_bytes(http_get(url))
  black_png = OUT / f"{slug}.png"
  white_png = OUT / f"{slug}_w.png"
  rasterize(svg_file, black_png, "#23202E")
  rasterize(svg_file, white_png, "#FFFFFF")
  svg_file.unlink(missing_ok=True)


def finalize_outputs(slugs: list[str]):
  media = []
  entries = []
  for slug in slugs:
    upper = slug.upper()
    media.append({
        "type": "png",
        "name": f"HABIT_ICON_{upper}",
        "file": f"images/habits/{slug}.png",
    })
    media.append({
        "type": "png",
        "name": f"HABIT_ICON_{upper}_W",
        "file": f"images/habits/{slug}_w.png",
    })
    entries.append((slug, upper))

  default_svg = OUT / "default.svg"
  default_svg.write_text(
      '<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">'
      '<circle cx="12" cy="12" r="8" fill="#23202E"/></svg>'
  )
  rasterize(default_svg, OUT / "default.png", "#23202E")
  default_svg.write_text(
      '<svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">'
      '<circle cx="12" cy="12" r="8" fill="#FFFFFF"/></svg>'
  )
  rasterize(default_svg, OUT / "default_w.png", "#FFFFFF")
  default_svg.unlink(missing_ok=True)
  media.append({"type": "png", "name": "HABIT_ICON_DEFAULT", "file": "images/habits/default.png"})
  media.append({"type": "png", "name": "HABIT_ICON_DEFAULT_W", "file": "images/habits/default_w.png"})

  (ROOT / "scripts" / "habit_icon_resources.json").write_text(json.dumps(media, indent=2))
  sync_package_json(media)
  (ROOT / "src" / "c" / "habit_icons.gen.c").write_text(
      "#include <pebble.h>\n\n"
      "typedef struct {\n"
      "  const char *slug;\n"
      "  uint32_t black;\n"
      "  uint32_t white;\n"
      "} HabitIconResource;\n\n"
      "static const HabitIconResource s_habit_icon_resources[] = {\n"
      + "".join(
          f'  {{"{slug}", RESOURCE_ID_HABIT_ICON_{upper}, RESOURCE_ID_HABIT_ICON_{upper}_W}},\n'
          for slug, upper in entries
      )
      + '  {"default", RESOURCE_ID_HABIT_ICON_DEFAULT, RESOURCE_ID_HABIT_ICON_DEFAULT_W},\n'
      "};\n\n"
      f"#define HABIT_ICON_RESOURCE_COUNT {len(entries) + 1}\n\n"
      "uint32_t habit_icon_resource_for_slug(const char *slug, bool white) {\n"
      "  for (int i = 0; i < HABIT_ICON_RESOURCE_COUNT; i++) {\n"
      "    if (strcmp(slug, s_habit_icon_resources[i].slug) == 0) {\n"
      "      return white ? s_habit_icon_resources[i].white : s_habit_icon_resources[i].black;\n"
      "    }\n"
      "  }\n"
      "  return white ? RESOURCE_ID_HABIT_ICON_DEFAULT_W : RESOURCE_ID_HABIT_ICON_DEFAULT;\n"
      "}\n"
  )

  print(f"Generated {len(entries)} habit icon pairs in {OUT}")
  validate_key_icons()


def main(only=None):
  OUT.mkdir(parents=True, exist_ok=True)
  icons = fetch_icons_from_css()
  if not icons:
    print("No icons found in Hey CSS", file=sys.stderr)
    sys.exit(1)

  slugs = sorted(icons.keys())
  if only:
    missing = [slug for slug in only if slug not in icons]
    if missing:
      print(f"Unknown slugs: {', '.join(missing)}", file=sys.stderr)
      sys.exit(1)
    slugs_to_process = only
  else:
    slugs_to_process = slugs

  total = len(slugs_to_process)
  for index, slug in enumerate(slugs_to_process, 1):
    print(f"[{index}/{total}] {slug}", flush=True)
    process_slug(slug, icons[slug])

  finalize_outputs(sorted(icons.keys()))


def validate_key_icons():
  from PIL import Image

  for slug in ("read", "heart", "weights", "fruit"):
    path = OUT / f"{slug}.png"
    if not path.exists():
      print(f"validate {slug}: missing", flush=True)
      continue
    im = Image.open(path)
    print(f"validate {slug}: size={im.size} bbox={im.getbbox()}", flush=True)

if __name__ == "__main__":
  if len(sys.argv) > 1 and sys.argv[1] == "--pad-only":
    pad_all_icons()
  elif len(sys.argv) > 1 and sys.argv[1] == "--only":
    only_slugs = [s.strip() for s in sys.argv[2].split(",") if s.strip()]
    if not only_slugs:
      print("Usage: generate_habit_icons.py --only slug1,slug2,...", file=sys.stderr)
      sys.exit(1)
    main(only=only_slugs)
  else:
    main()
