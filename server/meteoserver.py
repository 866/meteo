import pymysql
from datetime import datetime as dt
from flask import Flask
from flask import render_template
app = Flask(__name__)
host = '192.168.0.103'
def get_latest():
    conn = pymysql.connect(host, 'meteopi', 'ipoetem', 'home')
    cur = conn.cursor()
    cur.execute("""
       select * 
       from meteo 
       where timestamp=(
          select max(timestamp) 
          from meteo) 
       limit 1;
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

@app.route('/reboot')
def reboot():
    import os
    os.system("( sleep 5 ; reboot ) &")
    return "Reboot in 5 seconds..."

@app.route('/shutdown')
def shutdown():
    import os
    os.system("( sleep 5 ; shutdown -P 0 ) &")
    return "Shutdown in 5 seconds..."

