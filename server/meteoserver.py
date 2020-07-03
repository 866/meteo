import pymysql
from datetime import datetime as dt
from flask import Flask
from flask import render_template


import plotly
import plotly.graph_objs as go
import json
import pandas as pd 

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

def get_series(series):
    conn = pymysql.connect(host='192.168.0.103',
                                 user='meteopi',
                                 password='ipoetem',
                                 db='home')
    data = pd.read_sql(f"select from_unixtime(timestamp) as date, {series} from meteo order by timestamp desc limit 5000;", conn)
    conn.close()
    if series == "pressure":
        data["pressure"] = (data["pressure"] / 1.33322387415).astype(int) 
    return data
    
def create_plot(series):
    data = get_series(series)
    data = [
        go.Scatter(
            x=data['date'], # assign x as the dataframe column 'x'
            y=data[series]
        )
    ]
    graphJSON = json.dumps(data, cls=plotly.utils.PlotlyJSONEncoder)
    return graphJSON

@app.route('/graphs/<string:series>')
def graph_series(series):
    if series not in ["temperature", "light", "pressure", "humidity"]:
        return "There is no such graph"
    bar = create_plot(series)
    return render_template('series.html', plot=bar, series=series)

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

