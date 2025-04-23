import re
import sys

# includes header data of width and height

def xbm_to_bin_with_header(xbm_path, bin_path):
    import re

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


# Example usage
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python xbm_to_bin.py input.xbm output.bin")
    else:
        xbm_to_bin_with_header(sys.argv[1], sys.argv[2])
