from flask import Flask, Response
import subprocess

app = Flask(__name__)

RADIOS = {
    "franceinter": "http://direct.franceinter.fr/live/franceinter-midfi.mp3",
    "fip": "http://direct.fipradio.fr/live/fip-midfi.mp3",
    "disco": "http://sc8.1.fm:8100/;"
}

def stream_to_pcm(url):
    cmd = [
        "ffmpeg",
        "-hide_banner",
        "-loglevel", "error",
        "-i", url,
        "-f", "s16le",
        "-acodec", "pcm_s16le",
        "-ac", "1",
        "-ar", "22050",
        "-"
    ]

    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)

    try:
        while True:
            data = process.stdout.read(1024)
            if not data:
                break
            yield data
    finally:
        process.kill()

@app.route("/<radio>.raw")
def radio_stream(radio):
    if radio not in RADIOS:
        return "Radio inconnue", 404

    return Response(
        stream_to_pcm(RADIOS[radio]),
        mimetype="application/octet-stream"
    )

@app.route("/")
def home():
    return "Radio proxy OK"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)