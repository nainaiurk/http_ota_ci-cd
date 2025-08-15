import os
from flask import Flask, request, send_from_directory, render_template

app = Flask(__name__)

UPLOAD_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'uploads')
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        if 'file' not in request.files:
            return "No file part"
        file = request.files['file']
        if file.filename == '':
            return "No selected file"
        if file and file.filename.endswith('.bin'):
            file.save(os.path.join(app.config['UPLOAD_FOLDER'], 'firmware.bin'))
            return "File uploaded successfully!"
    return render_template('index.html')

@app.route('/firmware.bin')
def serve_firmware():
    return send_from_directory(app.config['UPLOAD_FOLDER'], 'firmware.bin', as_attachment=True)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
