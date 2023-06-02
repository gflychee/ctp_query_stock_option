#!/usr/bin/python3
import pandas as pd
import os
from time import *
import csv
import numpy as np
import sys
from itertools import islice
from datetime import datetime
from pathlib import Path
import requests
from hft_monitor import *
import traceback
import pymssql

#obj=pymssql.connect(host="dbs.cfi",user="public_data",password="public_data",database="WDDB")

#cur=obj.cursor()
#cur.execute("SELECT S_INFO_WINDCODE as windcode, S_DQ_CLOSE as 'close' from dbo.CINDEXFUTURESEODPRICES")

columnname = ['id','tick','multiplier','margin_rate','commission_rate_open','commission_rate_close_yesterday',
'commission_rate_close_today','commission_fixed_open','commission_fixed_close_yesterday','commission_fixed_close_today',
'open_limit','cancel_limit','trade_limit','preclose','lowlimit','highlimit','call_put','expiration','strike','underlying']
HOME: Path = Path("~").expanduser()
mention_list_1 = ["maitri"]

ROOT: Path = Path("/").expanduser()

def today() -> str:
    #return "20221116"
    now: datetime = datetime.now()
    date: str = now.strftime("%Y%m%d")
    find_next: bool = (now.hour >= 20)
    cal_path: Path = HOME.joinpath("ChinaTradingDates.txt")
    with open(cal_path) as fin:
        for line in fin:
            line = line.strip()
            if not line:
                continue
            if line < date or (find_next and line == date):
                continue
            return line

def yesterday() -> str:
    #return "20221115"
    now: datetime = datetime.now()
    date: str = now.strftime("%Y%m%d")
    cal_path: Path = HOME.joinpath("ChinaTradingDates.txt")
    yes = ""
    with open(cal_path) as fin:
        for line in fin:
            line = line.strip()
            if not line:
                continue
            if line == date:
                return yes
            else :
                yes = line


Stock_Option_DepthMarketData_path: Path = HOME.joinpath("ctp_query_stock_option", today()+"-option", "DepthMarketData.csv")
Stock_Option_Instrument_path: Path = HOME.joinpath("ctp_query_stock_option", today()+"-option", "Instrument.csv")

Stock_Spot_DepthMarketData_path: Path = HOME.joinpath("ctp_query_stock_option", today()+"-spot", "DepthMarketData.csv")
Stock_Spot_Instrument_path: Path = HOME.joinpath("ctp_query_stock_option", today()+"-spot", "Instrument.csv")

Cffex_DepthMarketData_path: Path = HOME.joinpath("ctp-query-insinfo", today()+"-day", "DepthMarketData.csv")
Cffex_Instrument_path: Path = HOME.joinpath("ctp-query-insinfo", today()+"-day", "Instrument.csv")

etf50_path: Path = ROOT.joinpath("mnt", "nas-3", "ProcessedData", "cffex_sh_snap", "guojun", "etf_full_feather", yesterday(), "SH510050-"+yesterday()+".feather")
etf300_path: Path = ROOT.joinpath("mnt", "nas-3", "ProcessedData", "cffex_sh_snap", "guojun", "etf_full_feather", yesterday(), "SH510300-"+yesterday()+".feather")
etf500_path: Path = ROOT.joinpath("mnt", "nas-3", "ProcessedData", "cffex_sh_snap", "guojun", "etf_full_feather", yesterday(), "SH510500-"+yesterday()+".feather")

if __name__ == '__main__':
    try:
        msg = []
        DT = today()
        yester = yesterday()
        print(f"yesterday:{yester},today:{DT}")
        insinfo = pd.DataFrame(columns=columnname)
        Stock_Option_DepthMarketData = pd.read_csv(Stock_Option_DepthMarketData_path,encoding="gbk")
        Stock_Option_Instrument = pd.read_csv(Stock_Option_Instrument_path,encoding="gbk")
        Stock_Spot_DepthMarketData = pd.read_csv(Stock_Spot_DepthMarketData_path,encoding="gbk")
        Stock_Spot_Instrument = pd.read_csv(Stock_Spot_Instrument_path,encoding="gbk")
        Cffex_DepthMarketData = pd.read_csv(Cffex_DepthMarketData_path,encoding="gbk")
        Cffex_Instrument = pd.read_csv(Cffex_Instrument_path,encoding="gbk")
        out_file = "/mnt/nas-3/OPTION/sse/insinfo/" + DT + "/"
        if not os.path.exists(out_file):
            os.mkdir(out_file)
    
        etf50 = pd.read_feather(etf50_path)
        etf300 = pd.read_feather(etf300_path)
        etf500 = pd.read_feather(etf500_path)
        if etf50.iloc[-1][-1] == 0.0 or etf300.iloc[-1][-1] == 0.0 or etf500.iloc[-1][-1] == 0.0:
            msg.append(f"{yester} etf50 or etf300 or etf500 close price = 0.0.")
            send_wechat_bot_msg(msg, mention_list_1)

        insinfo.loc[0] = ['SH510050',0.001,1,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-1,-1,-1,etf50.iloc[-1][-1],0.0,0.0,'-',99999999,0.0,'-']
        insinfo.loc[1] = ['SH510300',0.001,1,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-1,-1,-1,etf300.iloc[-1][-1],0.0,0.0,'-',99999999,0.0,'-']
        insinfo.loc[2] = ['SH510500',0.001,1,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-1,-1,-1,etf500.iloc[-1][-1],0.0,0.0,'-',99999999,0.0,'-']
        
        df_cffex=Cffex_Instrument.loc[(Cffex_Instrument['ExchangeID']=='CFFEX') & (Cffex_Instrument['InstrumentID'].str.contains('IC') | Cffex_Instrument['InstrumentID'].str.contains('IH') | Cffex_Instrument['InstrumentID'].str.contains('IF'))]
        
        i = 3
        for row in df_cffex.itertuples():
            ins = getattr(row,'InstrumentID')
            #print(type(getattr(row,'OptionsType')))
            underlying = '-'
            call_put = '-'
            Strike = 0.0
            commission_rate_open = 0.0
            commission_rate_close_yesterday = 0.0
            commission_rate_close_today = 0.0
            commission_fixed_open = 0.0
            commission_fixed_close_yesterday = 0.0
            commission_fixed_close_today = 0.0

            if getattr(row,'OptionsType') == 1:
                call_put = 'C'
                Strike = getattr(row,'StrikePrice')
            elif getattr(row,'OptionsType') == 2:
                call_put = 'P'
                Strike = getattr(row,'StrikePrice')
            else:
                call_put = '-'
    
            if getattr(row,'ProductID') == 'IF' or getattr(row,'ProductID') == 'IO':
                underlying = 'SH000300'
            elif getattr(row,'ProductID') == 'IM' or getattr(row,'ProductID') == 'MO':
                underlying = 'SH000852'
            elif getattr(row,'ProductID') == 'IH' or getattr(row,'ProductID') == 'HO':
                underlying = 'SH000016'
            else:
                underlying = '-'

            if getattr(row,'ProductID') == 'IF' or getattr(row,'ProductID') == 'IM' or getattr(row,'ProductID') == 'IH':
                commission_rate_open = 0.000023
                commission_rate_close_yesterday = 0.000023
                commission_rate_close_today = 0.00023
                commission_fixed_open = 0.0
                commission_fixed_close_yesterday = 0.0
                commission_fixed_close_today = 0.0

            if getattr(row,'ProductID') == 'IO' or getattr(row,'ProductID') == 'MO' or getattr(row,'ProductID') == 'HO':
                commission_rate_open = 0.0
                commission_rate_close_yesterday = 0.0
                commission_rate_close_today = 0.0
                commission_fixed_open = 15.0
                commission_fixed_close_yesterday = 15.0
                commission_fixed_close_today = 15.0

            expiredate = int(getattr(row,'ExpireDate'))
            
            for j in range(0,len(DepthMarketData['InstrumentID'])):
                if ins == DepthMarketData['InstrumentID'][j]:
                    insinfo.loc[i] = [ins,getattr(row,'PriceTick'),getattr(row,'VolumeMultiple'),0.0,commission_rate_open,commission_rate_close_yesterday,commission_rate_close_today,commission_fixed_open,commission_fixed_close_yesterday,commission_fixed_close_today,-1,-1,-1,DepthMarketData['PreClosePrice'][j],DepthMarketData['LowerLimitPrice'][j],DepthMarketData['UpperLimitPrice'][j],call_put,expiredate,Strike,underlying]
                    i = i + 1
                    #print(i,ins)
                    break
        insinfo.to_csv(out_file + "insinfo_preclose.csv",index=False)
        msg.append(f"{DT}'s option insinfo_preclose.csv has been generated.")
        send_wechat_bot_msg(msg, mention_list_1)
    except:
        msg.append(f"error in {__file__}\n{traceback.format_exc()}")
        send_wechat_bot_msg(msg, mention_list_1)


