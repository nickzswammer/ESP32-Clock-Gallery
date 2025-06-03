import os
import re
import sys

# includes header data of width and height
def xbm_to_bin_with_header(xbm_path, bin_path):
    with open(xbm_path, 'r') as f:
        xbm_data = f.read()

    width = int(re.search(r'#define\s+\w+_width\s+(\d+)', xbm_data).group(1))
    height = int(re.search(r'#define\s+\w+_height\s+(\d+)', xbm_data).group(1))
    hex_data = re.findall(r'0x[0-9a-fA-F]{2}', xbm_data)
    pixel_data = bytes(int(x, 16) for x in hex_data)

    with open(bin_path, 'wb') as f:
        f.write(width.to_bytes(2, 'little'))
        f.write(height.to_bytes(2, 'little'))
        f.write(pixel_data)

    print(f"Saved {bin_path} ({width}x{height}, {len(pixel_data)} bytes)")

def main():
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

if __name__ == "__main__":
    main()
