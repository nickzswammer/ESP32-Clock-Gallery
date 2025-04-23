import os
from xbm_to_bin import xbm_to_bin_with_header  # assumes xbm_to_bin.py is in same folder

INPUT_DIR = "input_bin"
OUTPUT_DIR = "output_bin"

# Make sure output directory exists
os.makedirs(OUTPUT_DIR, exist_ok=True)

for filename in os.listdir(INPUT_DIR):
    if filename.endswith(".xbm"):
        in_path = os.path.join(INPUT_DIR, filename)
        out_name = os.path.splitext(filename)[0] + ".bin"
        out_path = os.path.join(OUTPUT_DIR, out_name)
        print(f"Converting {filename} → {out_name}")
        xbm_to_bin_with_header(in_path, out_path)
