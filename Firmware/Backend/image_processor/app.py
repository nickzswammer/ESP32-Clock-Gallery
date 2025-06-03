# Run Using "flask --app app run --debug"

from flask import Flask, render_template, url_for, request, redirect, send_from_directory
import os
from PIL import Image

app = Flask(__name__)

RAW_UPLOAD_FOLDER = 'uploaded_files'
os.makedirs(RAW_UPLOAD_FOLDER, exist_ok=True)
PROCESSED_FOLDER = 'processed_files'
os.makedirs(PROCESSED_FOLDER, exist_ok=True)

@app.route('/')
def upload_page():
    return render_template("index.html")

@app.route('/success', methods = ['POST'])
def success():
    if request.method == 'POST':
        f = request.files['file']
        save_path = os.path.join(RAW_UPLOAD_FOLDER, f.filename)
        f.save(save_path)
        return redirect(url_for('process_image', filename=f.filename))

from PIL import Image, ImageOps
import os

@app.route('/process/<filename>')
def process_image(filename):
    WIDTH = 400
    HEIGHT = 300

    input_path = os.path.join(RAW_UPLOAD_FOLDER, filename)
    bin_filename = os.path.splitext(filename)[0] + '.bin'
    output_path = os.path.join(PROCESSED_FOLDER, bin_filename)

    # Load and scale image
    img = Image.open(input_path).convert('L')  # Convert to grayscale first
    img.thumbnail((WIDTH, HEIGHT), Image.LANCZOS)

    # Create white canvas and center image
    canvas = Image.new('L', (WIDTH, HEIGHT), 255)
    paste_x = (WIDTH - img.width) // 2
    paste_y = (HEIGHT - img.height) // 2
    canvas.paste(img, (paste_x, paste_y))

    # Apply dithering to simulate GIMP behavior
    dithered = canvas.convert('1', dither=Image.FLOYDSTEINBERG)
    dithered.save("debug_dithered.png")

    with open(output_path, 'wb') as f:
        # Write width and height in little-endian (2 bytes each)
        f.write(WIDTH.to_bytes(2, byteorder='little'))
        f.write(HEIGHT.to_bytes(2, byteorder='little'))

        # Pack bits row-major, LSB-first, with no per-row padding
        byte = 0
        bit_count = 0

        for y in range(HEIGHT):
            for x in range(WIDTH):
                pixel = dithered.getpixel((x, y))
                bit = 1 if pixel == 0 else 0  # 1 = black, 0 = white
                byte |= (bit << bit_count)   # LSB-first
                bit_count += 1

                if bit_count == 8:
                    f.write(bytes([byte]))
                    byte = 0
                    bit_count = 0

        # Pad final byte if needed
        if bit_count > 0:
            byte |= (0xFF << bit_count) & 0xFF  # Fill unused bits with 1s (white)
            f.write(bytes([byte]))


    return f"""
    <h2>Conversion complete with dithering!</h2>
    <a href="{url_for('download_file', filename=bin_filename)}" download>Download {bin_filename}</a>
    """


@app.route('/download/<filename>')
def download_file(filename):
    return send_from_directory(PROCESSED_FOLDER, filename, as_attachment=True)


if __name__ == "__main__":
    app.run(debug=True)