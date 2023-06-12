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
    #return "20230605"
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
    #return "20230602"
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

Cffex_DepthMarketData_path: Path = HOME.joinpath("ctp-query-insinfo", today()+"-day", "DepthMarketData.csv")
Cffex_Instrument_path: Path = HOME.joinpath("ctp-query-insinfo", today()+"-day", "Instrument.csv")

# etf50_path: Path = ROOT.joinpath("mnt", "nas-3", "ProcessedData", "cffex_sh_snap", "guojun", "etf_full_feather", yesterday(), "SH510050-"+yesterday()+".feather")
# etf300_path: Path = ROOT.joinpath("mnt", "nas-3", "ProcessedData", "cffex_sh_snap", "guojun", "etf_full_feather", yesterday(), "SH510300-"+yesterday()+".feather")
# etf500_path: Path = ROOT.joinpath("mnt", "nas-3", "ProcessedData", "cffex_sh_snap", "guojun", "etf_full_feather", yesterday(), "SH510500-"+yesterday()+".feather")

if __name__ == '__main__':
    try:
        msg = []
        DT = today()
        yester = yesterday()
        print(f"yesterday:{yester},today:{DT}")
        insinfo = pd.DataFrame(columns=columnname)
        Stock_Option_DepthMarketData = pd.read_csv(Stock_Option_DepthMarketData_path,dtype={1:str,4:np.float64,20:str,44:np.float64},encoding="gbk")
        Stock_Option_Instrument = pd.read_csv(Stock_Option_Instrument_path,encoding="gbk")
        Cffex_DepthMarketData = pd.read_csv(Cffex_DepthMarketData_path,encoding="gbk")
        Cffex_Instrument = pd.read_csv(Cffex_Instrument_path,encoding="gbk")
        out_file = "/mnt/nas-3/OPTION/sse/insinfo/" + DT + "/"
        if not os.path.exists(out_file):
            os.mkdir(out_file)

        i = 0

        df_etf = Stock_Option_DepthMarketData.loc[(Stock_Option_DepthMarketData['InstrumentID']=='510050') | (Stock_Option_DepthMarketData['InstrumentID']=='510300') | (Stock_Option_DepthMarketData['InstrumentID']=='510500')]
        for row in df_etf.itertuples():
            ins = "SH" + getattr(row,'InstrumentID')
            insinfo.loc[i] = [ins,0.001,1,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-1,-1,-1,getattr(row,'PreClosePrice'),getattr(row,'LowerLimitPrice'),getattr(row,'UpperLimitPrice'),'-',99999999,0.0,'-']
            i = i + 1
        
        df_cffex=Cffex_Instrument.loc[(Cffex_Instrument['ExchangeID']=='CFFEX') & (Cffex_Instrument['InstrumentID'].str.contains('IC') | Cffex_Instrument['InstrumentID'].str.contains('IH') | Cffex_Instrument['InstrumentID'].str.contains('IF'))]
        result = df_cffex.merge(Cffex_DepthMarketData, on='InstrumentID')
        
        for row in result.itertuples():
            ins = getattr(row,'InstrumentID')
            #print(type(getattr(row,'OptionsType')))
            underlying = '-'
            call_put = '-'
            Strike = 0.0
            commission_rate_open = 0.000023
            commission_rate_close_yesterday = 0.000023
            commission_rate_close_today = 0.00023
            commission_fixed_open = 0.0
            commission_fixed_close_yesterday = 0.0
            commission_fixed_close_today = 0.0

            if getattr(row,'ProductID') == 'IF':
                underlying = 'SH510300'
            elif getattr(row,'ProductID') == 'IC':
                underlying = 'SH510500'
            elif getattr(row,'ProductID') == 'IH':
                underlying = 'SH510050'
            else:
                underlying = '-'

            expiredate = int(getattr(row,'ExpireDate'))

            insinfo.loc[i] = [ins,getattr(row,'PriceTick'),getattr(row,'VolumeMultiple'),0.0,commission_rate_open,commission_rate_close_yesterday,commission_rate_close_today,commission_fixed_open,commission_fixed_close_yesterday,commission_fixed_close_today,-1,-1,-1,getattr(row,'PreClosePrice'),getattr(row,'LowerLimitPrice'),getattr(row,'UpperLimitPrice'),call_put,expiredate,Strike,underlying]
            i = i + 1

        df_stock=Stock_Option_Instrument.loc[(Stock_Option_Instrument['ProductID']=='ETF_O') & (Stock_Option_Instrument['UnderlyingInstrID'].str.contains('510050') | Stock_Option_Instrument['UnderlyingInstrID'].str.contains('510300') | Stock_Option_Instrument['UnderlyingInstrID'].str.contains('510500'))]
        result = df_stock.merge(Stock_Option_DepthMarketData, on='InstrumentID')
        for row in result.itertuples():
            ins = getattr(row,'InstrumentID')
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
    
            if getattr(row,'UnderlyingInstrID') == '510050':
                underlying = 'SH510050'
            elif getattr(row,'UnderlyingInstrID') == '510300':
                underlying = 'SH510300'
            elif getattr(row,'UnderlyingInstrID') == '510500':
                underlying = 'SH510500'
            else:
                underlying = '-'

            expiredate = int(getattr(row,'ExpireDate'))
            
            insinfo.loc[i] = [ins,getattr(row,'PriceTick'),getattr(row,'VolumeMultiple'),0.0,commission_rate_open,commission_rate_close_yesterday,commission_rate_close_today,commission_fixed_open,commission_fixed_close_yesterday,commission_fixed_close_today,-1,-1,-1,getattr(row,'PreClosePrice'),getattr(row,'LowerLimitPrice'),getattr(row,'UpperLimitPrice'),call_put,expiredate,Strike,underlying]
            i = i + 1

        insinfo.to_csv(out_file + "insinfo.csv",index=False)
        msg.append(f"{DT}'s etf option insinfo.csv has been generated.")
        send_wechat_bot_msg(msg, mention_list_1)
    except:
        msg.append(f"error in {__file__}\n{traceback.format_exc()}")
        send_wechat_bot_msg(msg, mention_list_1)


