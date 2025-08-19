from flask import Flask, render_template, url_for, request, redirect, send_from_directory
import os
from PIL import Image
import zipfile # <-- Import zipfile
from datetime import datetime # <-- Import datetime

app = Flask(__name__)

# --- CONFIGURATION ---
RAW_UPLOAD_FOLDER = 'uploaded_files'
PROCESSED_FOLDER = 'processed_files'
ZIPPED_FOLDER = 'zipped_files' # <-- Add a folder for zip files
os.makedirs(RAW_UPLOAD_FOLDER, exist_ok=True)
os.makedirs(PROCESSED_FOLDER, exist_ok=True)
os.makedirs(ZIPPED_FOLDER, exist_ok=True) # <-- Create the directory
WIDTH = 400
HEIGHT = 300
# ---------------------

def convert_image_to_bin(filename):
    """(This function remains the same as before)"""
    input_path = os.path.join(RAW_UPLOAD_FOLDER, filename)
    bin_filename = os.path.splitext(filename)[0] + '.bin'
    output_path = os.path.join(PROCESSED_FOLDER, bin_filename)
    img = Image.open(input_path).convert('L')
    img.thumbnail((WIDTH, HEIGHT), Image.LANCZOS)
    canvas = Image.new('L', (WIDTH, HEIGHT), 255)
    paste_x = (WIDTH - img.width) // 2
    paste_y = (HEIGHT - img.height) // 2
    canvas.paste(img, (paste_x, paste_y))
    dithered = canvas.convert('1', dither=Image.FLOYDSTEINBERG)
    with open(output_path, 'wb') as f:
        f.write(WIDTH.to_bytes(2, byteorder='little'))
        f.write(HEIGHT.to_bytes(2, byteorder='little'))
        byte = 0
        bit_count = 0
        for y in range(HEIGHT):
            for x in range(WIDTH):
                pixel = dithered.getpixel((x, y))
                bit = 1 if pixel == 0 else 0
                byte |= (bit << bit_count)
                bit_count += 1
                if bit_count == 8:
                    f.write(bytes([byte]))
                    byte = 0
                    bit_count = 0
        if bit_count > 0:
            f.write(bytes([byte]))
    return bin_filename

@app.route('/')
def upload_page():
    return render_template("index.html")

@app.route('/success', methods=['POST'])
def success():
    if request.method == 'POST':
        uploaded_files = request.files.getlist('file')
        processed_filenames = []
        for f in uploaded_files:
            if f and f.filename:
                save_path = os.path.join(RAW_UPLOAD_FOLDER, f.filename)
                f.save(save_path)
                bin_filename = convert_image_to_bin(f.filename)
                processed_filenames.append(bin_filename)
        
        # --- ZIP FILE CREATION ---
        if not processed_filenames:
            return redirect(url_for('upload_page')) # Redirect if no files were processed

        # 1. Create a unique filename for the zip archive
        timestamp = int(datetime.now().timestamp())
        zip_filename = f'converted_images_{timestamp}.zip'
        zip_path = os.path.join(ZIPPED_FOLDER, zip_filename)

        # 2. Create the zip file and add the processed .bin files to it
        with zipfile.ZipFile(zip_path, 'w') as zipf:
            for bin_file in processed_filenames:
                file_path = os.path.join(PROCESSED_FOLDER, bin_file)
                # arcname=bin_file ensures the path inside the zip is clean
                zipf.write(file_path, arcname=bin_file)
        
        # 3. Render a new page with a link to download the zip file
        return render_template('download_zip.html', zip_filename=zip_filename)

    return redirect(url_for('upload_page'))

@app.route('/download_zip/<filename>')
def download_zip_file(filename):
    """New route to handle downloading the final zip file."""
    return send_from_directory(ZIPPED_FOLDER, filename, as_attachment=True)

# This route is no longer needed if you zip everything, but can be kept
@app.route('/download/<filename>')
def download_file(filename):
    return send_from_directory(PROCESSED_FOLDER, filename, as_attachment=True)


if __name__ == "__main__":
    app.run(debug=True)