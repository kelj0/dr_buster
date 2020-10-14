from flask import Flask

app = Flask(__name__)

@app.route('/')
@app.route('/100')
@app.route('/770')
@app.route('/623')
@app.route('/303')
@app.route('/904')
@app.route('/703')
@app.route('/604')
@app.route('/504')
@app.route('/130')
@app.route('/120')
@app.route('/900')
@app.route('/800')
@app.route('/700')
@app.route('/600')
@app.route('/500')
@app.route('/400')
@app.route('/300')
@app.route('/200')
@app.route('/105')
@app.route('/104')
@app.route('/103')
@app.route('/102')
def index():
    return 'Hello, World!'
