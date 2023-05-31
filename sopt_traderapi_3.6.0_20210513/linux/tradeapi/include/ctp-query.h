#ifndef CTPQUERY_H
#define CTPQUERY_H

#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <sys/time.h>
#include <map>
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <atomic>
#include "ThostFtdcTraderApi.h"
#include "common.h"
using namespace std;

class CFtdTrader : public CThostFtdcTraderSpi {
public:
    CFtdTrader(void);
    ~CFtdTrader(void){};
    void Init(string traderAddr, string brokerId, string usrId, string pwd, string investorId, string appId, string authId, string day);
    void LogonServer(void);
    void ReqQryInstrument(void);
    //void ReqQryCommissionRate(void);
    void ReqQryDepthMarketData(void);
    volatile bool m_Querying;
    //map<string, Insinfo> insinfo;
private:
    int ReqAuthenticate(void);
    int ReqUserLogin(CThostFtdcReqUserLoginField&);
    virtual void OnFrontConnected(void);
    virtual void OnFrontDisconnected(int nReason);
    virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    //virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
private:
    int m_nMsgSeqNum;
    string m_TraderAddr;
    string m_BrokerID;
    string m_UsrID;
    string m_Psw;
    string m_InvestorID;
    string m_AppID;
    string m_AuthID;
    string m_TradingDay;
    CThostFtdcTraderApi *m_pTraderApi;
    volatile bool m_LoginFlag;
    vector<CThostFtdcInstrumentField *> m_Instrument;
    vector<CThostFtdcDepthMarketDataField *> m_DepthMarketData;
};

#endif
