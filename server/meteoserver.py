import sqlite3
from datetime import datetime as dt
from flask import Flask
from flask import render_template
app = Flask(__name__)
DBPATH = "/home/pi/Programming/meteo/pi/test.db"

def get_latest():
    conn = sqlite3.connect(DBPATH)
    cur = conn.cursor()
    cur.execute("""
       select * from meteo order by timestamp desc limit 1;
    """)
    row = cur.fetchall()[0]
    conn.close()
    ts = float(row[0])
    datetime = dt.fromtimestamp(ts).strftime("%d %b %H:%M:%S")
    temp = row[1]
    humidity = row[2]
    pressure = int(row[3] / 1.33322387415)
    light = row[4]
    out = """
<!DOCTYPE html>
<html>
<head>
	<title>Meteo measurements</title>
	<meta http-equiv="refresh" content="30" />
</head>
<body>
        The last recording: {0} <br>
        Temperature: {1}&#176; <br>
        Humidity: {2}% <br>
        Pressure: {3} mmHg <br>
        Light: {4} <br>
</body>
    """.format(datetime, temp, humidity, pressure, light)
    
    return render_template("measurements.html", date=datetime, temperature=temp, humidity=humidity, pressure=pressure, light=light)

@app.route('/')
def index():
    return get_latest()
