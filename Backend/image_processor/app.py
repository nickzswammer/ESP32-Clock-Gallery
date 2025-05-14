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

@app.route('/process/<filename>')
def process_image(filename):
    WIDTH = 400
    HEIGHT = 300

    input_path = os.path.join(RAW_UPLOAD_FOLDER, filename)
    bin_filename = os.path.splitext(filename)[0] + '.bin'
    output_path = os.path.join(PROCESSED_FOLDER, bin_filename)

    img = Image.open(input_path).convert('1')

    img.thumbnail((WIDTH, HEIGHT), Image.LANCZOS)
    
    canvas = Image.new('1', (WIDTH, HEIGHT), 1)
    paste_x = (WIDTH - img.width) // 2
    paste_y = (HEIGHT - img.height) // 2
    canvas.paste(img, (paste_x, paste_y))

    with open(output_path, 'wb') as f:
        for y in range(HEIGHT):
            byte = 0
            bit_count = 0
            for x in range(WIDTH):
                pixel = canvas.getpixel((x, y))
                bit = 0 if pixel == 0 else 1
                byte = (byte << 1) | bit
                bit_count += 1
                if bit_count == 8:
                    f.write(bytes([byte]))
                    byte = 0
                    bit_count = 0
            if bit_count > 0:
                byte <<= (8 - bit_count)
                f.write(bytes([byte]))

    # Provide download link to user
    return f"""
    <h2>Conversion complete!</h2>
    <p>Download your .bin file:</p>
    <a href="{url_for('download_file', filename=bin_filename)}" download>Download {bin_filename}</a>
    """

@app.route('/download/<filename>')
def download_file(filename):
    return send_from_directory(PROCESSED_FOLDER, filename, as_attachment=True)


if __name__ == "__main__":
    app.run(debug=True)