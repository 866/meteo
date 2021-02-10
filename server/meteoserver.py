import pymysql
from datetime import datetime as dt
from flask import Flask
from flask import render_template


import plotly
import plotly.graph_objs as go
import json
import pandas as pd 

app = Flask(__name__)
host = '192.168.0.100'


wind_query = """
    with percentiles as (     
        select value, PERCENT_RANK() OVER (ORDER BY value) as percentile 
        from home where sensor=7  and 
        timestamp > (select max(timestamp) from home where sensor=7) - 3*60*60 and 
        value < 100),
    a as (
        select value, 0.25 as percentile 
        from percentiles 
        order by abs(0.25-percentile) 
        limit 1
    ),
    b as (
        select value, 0.75 as percentile 
        from percentiles order by abs(0.75-percentile) 
        limit 1
    ),
    c as (
        select value, 1 as percentile 
        from percentiles order by abs(1-percentile) 
        limit 1
    )

    select * from a
    union
    select * from b
    union
    select * from c;
"""

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

    cur.execute(wind_query)
    rows = cur.fetchall()
    wind_speeds = rows[0][0], rows[1][0], rows[2][0]
    ts = float(row[0])
    args = {"wind_speeds": wind_speeds, 'home_temp': int(ht[0]), 'home_humi': int(hh[0]),
            'home_moisture': int(hm[0]), 'home_dt': dt.fromtimestamp(ht[1]).strftime("%d %b %H:%M:%S"),
            'date': dt.fromtimestamp(ts).strftime("%d %b %H:%M:%S"), 'temperature': row[1], 'humidity': row[2],
            'pressure': int(row[3] / 1.33322387415), 'light': row[4]}
    conn.close()
    return render_template("measurements.html", **args)


def get_series(series):
    conn = pymysql.connect(host=host,
                         user='meteopi',
                         password='ipoetem',
                         db='home')
    if series == "windspeed":
        data = pd.read_sql(f"""select from_unixtime(timestamp) as date, 
                value as windspeed from home
                where sensor=7 and value <= 100 and value >= 0
                order by timestamp desc limit 5000;""", conn)
    elif series.startswith("homesensor"):
        sens_num = series.split("_")[1]
        data = pd.read_sql(f"""select from_unixtime(timestamp) as date, 
                value as {series} from home
                where sensor={sens_num} and value < 2000 and value >= 0
                order by timestamp desc limit 5000;""", conn)
    else:
        data = pd.read_sql(f"""select from_unixtime(timestamp) as date, 
                {series} from meteo
                where {series} is not null and {series} < 1000000 and 
                {series} > -50
                order by timestamp desc limit 5000;""", conn)
    conn.close()
    if series == "pressure":
        data["pressure"] = (data["pressure"] / 1.33322387415).astype(int) 
    return data


def create_plot(series):
    data = get_series(series)
    data = [
        go.Scatter(
            x=data['date'],
            y=data[series]
        )
    ]
    graphJSON = json.dumps(data, cls=plotly.utils.PlotlyJSONEncoder)
    return graphJSON


@app.route('/graphs/<string:series>')
def graph_series(series):
    if series not in ["temperature", "light", "pressure", "humidity",
                      "windspeed"] and not series.startswith("homesensor"):
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

