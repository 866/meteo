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
    cur.execute("""
       select value, timestamp 
       from home 
       where timestamp=(
          select max(timestamp) 
          from home
          where sensor=3
          ) and sensor=3
       limit 1;
    """)
    ht = cur.fetchall()[0]
    
    cur.execute("""
       select value, timestamp 
       from home 
       where timestamp=(
          select max(timestamp) 
          from home
          where sensor=2
          ) and sensor=2
       limit 1;
    """)
    hh = cur.fetchall()[0]
    
    cur.execute("""
       select value, timestamp 
       from home 
       where timestamp=(
          select max(timestamp) 
          from home
          where sensor=1
          ) and sensor=1
       limit 1;
    """)
    hm = cur.fetchall()[0]
    
    home_temp = int(ht[0])
    home_humi = int(hh[0])
    home_moisture = int(hm[0])
    home_datetime = dt.fromtimestamp(ht[1]).strftime("%d %b %H:%M:%S")
    conn.close()
    ts = float(row[0])
    datetime = dt.fromtimestamp(ts).strftime("%d %b %H:%M:%S")
    temp = row[1]
    humidity = row[2]
    pressure = int(row[3] / 1.33322387415)
    light = row[4]
    
    return render_template("measurements.html", date=datetime, temperature=temp, humidity=humidity, pressure=pressure, light=light,
                           home_temp=home_temp, home_dt=home_datetime, home_humi=home_humi, home_moisture=home_moisture)

def get_series(series):
    conn = pymysql.connect(host='192.168.0.103',
                                 user='meteopi',
                                 password='ipoetem',
                                 db='home')
    data = pd.read_sql(f"""select from_unixtime(timestamp) as date, {series} from meteo
            where {series} is not null and {series} < 1000000
            order by timestamp desc limit 5000;""", conn)
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

