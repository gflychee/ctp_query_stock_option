#include "ctp-query.h"

CFtdTrader::CFtdTrader(void) {
    m_pTraderApi = NULL;
    m_nMsgSeqNum = 0;
    m_Querying = false;
    m_LoginFlag = false;
}

void CFtdTrader::Init(string traderAddr, string brokerId, string usrId, string pwd, string investorId, string appId, string authId, string day)
{
    m_TraderAddr = traderAddr;
    m_BrokerID = brokerId;
    m_UsrID = usrId;
    m_Psw = pwd;
    m_InvestorID = investorId;
    m_AppID = appId;
    m_AuthID = authId;
    m_TradingDay = day;
    mkdir("account", 0700);
    mkdir(("account/" + m_UsrID).c_str(), 0700);
    mkdir(day.c_str(), 0700);
    printf("Init Done\n");
    fflush(stdout);
}

void CFtdTrader::LogonServer(void)
{
    m_pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi(("account/" + m_UsrID + "/").c_str());
    m_pTraderApi->RegisterSpi(this);
    m_pTraderApi->SubscribePublicTopic(THOST_TERT_QUICK);
    m_pTraderApi->RegisterFront((char *)m_TraderAddr.c_str());
    m_pTraderApi->Init();
}

void CFtdTrader::OnFrontConnected()
{
    int ret = ReqAuthenticate();
    printf("OnFrontConnected and ReqAuthenticate ret = %d\n", ret);
    fflush(stdout);
    if (ret != 0) {
        exit(-1);
    }
}

void CFtdTrader::OnFrontDisconnected(int nReason)
{
    printf("OnFrontDisconnected nReason: %d\n", nReason);
    //m_LoginFlag.store(false);
    fflush(stdout);
    m_LoginFlag = false;
    exit(-1);
}

int CFtdTrader::ReqAuthenticate()
{
    CThostFtdcReqAuthenticateField req;
    memset(&req, 0, sizeof(req));
    snprintf(req.UserID, sizeof(req.UserID), "%s", m_UsrID.c_str());
    snprintf(req.BrokerID, sizeof(req.BrokerID), "%s", m_BrokerID.c_str());
    strncpy(req.AppID, m_AppID.c_str(), sizeof(req.AppID));
    strncpy(req.AuthCode, m_AuthID.c_str(), sizeof(req.AuthCode));
    printf("ReqAuthenticate: %s %s\n", req.AppID, req.AuthCode);
    fflush(stdout);
    return m_pTraderApi->ReqAuthenticate(&req, ++m_nMsgSeqNum);
}

void CFtdTrader::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        printf("OnRspAuthenticate errorCode: %d, errorMsg: %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        fflush(stdout);
        exit(-1);
    }
    printf("OnRspAuthenticate and ReqUserLogin: %s\n", m_UsrID.c_str());
    fflush(stdout);
    CThostFtdcReqUserLoginField req;

    memset(&req, 0, sizeof(req));
    ReqUserLogin(req);
}

int CFtdTrader::ReqUserLogin(CThostFtdcReqUserLoginField &req)
{
    strcpy(req.BrokerID, m_BrokerID.c_str());
    strcpy(req.UserID, m_UsrID.c_str());
    strcpy(req.Password, m_Psw.c_str());
    int iResult = m_pTraderApi->ReqUserLogin(&req, ++m_nMsgSeqNum);
    return iResult;
}

void CFtdTrader::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (!((pRspInfo) && (pRspInfo->ErrorID != 0))) {
        m_LoginFlag = true;
        printf("User Login Successed: %s\n", m_UsrID.c_str());
        fflush(stdout);
    } else {
        printf("User login failed: %s, errorCode: %d, errorMsg: %s\n", m_UsrID.c_str(), pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        fflush(stdout);
        exit(-1);
    }
}

void CFtdTrader::ReqQryInstrument(void)
{
    int wait_time = 1;
    while (!m_LoginFlag && wait_time <= 10) {
    printf("wait %ds\n", wait_time);
    fflush(stdout);
    wait_time++;
        sleep(1);
    }
    if (!m_LoginFlag) {
        exit(-1);
    }
    m_Querying = true;
    
    CThostFtdcQryInstrumentField req;
    memset(&req, 0, sizeof(req));
    m_pTraderApi->ReqQryInstrument(&req, ++m_nMsgSeqNum);
}

void CFtdTrader::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
        printf("OnRspQryInstrument failed: %s\n", pRspInfo->ErrorMsg);
        fflush(stdout);
        m_Querying = false;
    } else {
        if (pInstrument == NULL) {
            printf("Investor: %s  pInstrument = NULL \n", m_UsrID.c_str());
            fflush(stdout);
            m_Querying = false;
            return;
        }
        m_Instrument.push_back(new CThostFtdcInstrumentField (*pInstrument));
        
        if (bIsLast == true) {
            FILE *outfile;
            outfile = fopen((m_TradingDay + "/Instrument.csv").c_str(), "w");
            if(outfile == NULL) {
                perror("open file error");
                exit(-1);
            }
            fprintf(outfile, "InstrumentID,ExchangeID,InstrumentName,ExchangeInstID,ProductID,ProductClass,DeliveryYear,DeliveryMonth,MaxMarketOrderVolume,MinMarketOrderVolume,\
MaxLimitOrderVolume,MinLimitOrderVolume,VolumeMultiple,PriceTick,CreateDate,OpenDate,ExpireDate,StartDelivDate,EndDelivDate,InstLifePhase,IsTrading,PositionType,PositionDateType,\
LongMarginRatio,ShortMarginRatio,MaxMarginSideAlgorithm,UnderlyingInstrID,StrikePrice,OptionsType,UnderlyingMultiple,CombinationType,MinBuyVolume,MinSellVolume,InstrumentCode\n");
            for (auto &item : m_Instrument) {
                fprintf(outfile, "%s,%s,%s,%s,%s,%c,%d,%d,%d,%d,%d,%d,%d,%f,%s,%s,%s,%s,%s,%c,%d,%c,%c,%f,%f,%c,%s,%f,%c,%f,%c,%d,%d,%s\n", \
                    item->InstrumentID, item->ExchangeID, item->InstrumentName, item->ExchangeInstID, item->ProductID, item->ProductClass, item->DeliveryYear, item->DeliveryMonth, \
                    item->MaxMarketOrderVolume, item->MinMarketOrderVolume, item->MaxLimitOrderVolume, item->MinLimitOrderVolume, item->VolumeMultiple, item->PriceTick, \
                    item->CreateDate, item->OpenDate, item->ExpireDate, item->StartDelivDate, item->EndDelivDate, item->InstLifePhase, item->IsTrading, item->PositionType, \
                    item->PositionDateType, item->LongMarginRatio, item->ShortMarginRatio, item->MaxMarginSideAlgorithm, item->UnderlyingInstrID, item->StrikePrice, item->OptionsType, \
                    item->UnderlyingMultiple, item->CombinationType, item->MinBuyVolume, item->MinSellVolume, item->InstrumentCode);
                fflush(outfile);
            }
            fclose(outfile);
            m_Querying = false;
        }           
    }
}
/*
void CFtdTrader::ReqQryCommissionRate(void)
{
    while (m_Querying) {
        sleep(1);
    }
    CThostFtdcQryInstrumentCommissionRateField req;
    memset(&req, 0, sizeof(req));
    for (auto &item : insinfo) {
        strcpy(req.BrokerID, m_BrokerID.c_str());
        strcpy(req.InvestorID, m_InvestorID.c_str());
        while (m_Querying) {
            sleep(1);
        }
        strcpy(req.InstrumentID, item.first.c_str());
        printf("ReqQryCommissionRate req.InstrumentID: %s\n", req.InstrumentID);
        fflush(stdout);
        m_Querying = true;
        int result = 0;
        do {
            if (result) {
                sleep(1);
            }
            result = m_pTraderApi->ReqQryInstrumentCommissionRate(&req, ++m_nMsgSeqNum);
            if (result != 0) {
                printf("result: %d\n", result);
                fflush(stdout);
            }
        } while(result); 
    }
}

void CFtdTrader::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
        printf("OnRspQryInstrumentCommissionRate failed: %s\n", pRspInfo->ErrorMsg);
        fflush(stdout);
        m_Querying = false;
    } else {
        if (pInstrumentCommissionRate == NULL) {
            printf("Investor: %s  pInstrumentCommissionRate = NULL \n", m_UsrID.c_str());
            fflush(stdout);
            m_Querying = false;
            return;
        }
        auto iter = insinfo.find(pInstrumentCommissionRate->InstrumentID);
        if (iter != insinfo.end()) {
            insinfo[pInstrumentCommissionRate->InstrumentID].commission_rate_open = pInstrumentCommissionRate->OpenRatioByMoney;
            insinfo[pInstrumentCommissionRate->InstrumentID].commission_rate_close_yesterday = pInstrumentCommissionRate->CloseRatioByMoney;
            insinfo[pInstrumentCommissionRate->InstrumentID].commission_rate_close_today = pInstrumentCommissionRate->CloseTodayRatioByMoney;
            insinfo[pInstrumentCommissionRate->InstrumentID].commission_fixed_open = pInstrumentCommissionRate->OpenRatioByVolume;
            insinfo[pInstrumentCommissionRate->InstrumentID].commission_fixed_close_yesterday = pInstrumentCommissionRate->CloseRatioByVolume;
            insinfo[pInstrumentCommissionRate->InstrumentID].commission_fixed_close_today = pInstrumentCommissionRate->CloseTodayRatioByVolume;
        }
        if (bIsLast == true) {
            m_Querying = false;
        }
    }
}
*/
void CFtdTrader::ReqQryDepthMarketData()
{
    while (m_Querying) {
        sleep(1);
    }

    CThostFtdcQryDepthMarketDataField req;
    memset(&req, 0, sizeof(req));

   m_Querying = true;
   int result = 0;
   do {
       if (result) {
           sleep(1);
       }
       result = m_pTraderApi->ReqQryDepthMarketData(&req, ++m_nMsgSeqNum);
       if (result != 0) {
           printf("result: %d\n", result);
           fflush(stdout);
       }
   } while(result);
}

void CFtdTrader::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
        printf("OnRspQryDepthMarketData failed: %s\n", pRspInfo->ErrorMsg);
        fflush(stdout);
        m_Querying = false;
    } else {
        if (pDepthMarketData == NULL) {
            printf("Investor: %s  pDepthMarketData = NULL \n", m_UsrID.c_str());
            fflush(stdout);
            m_Querying = false;
            return;
        }
        m_DepthMarketData.push_back(new CThostFtdcDepthMarketDataField(*pDepthMarketData));
        if (bIsLast == true) {
            FILE *outfile;
            outfile = fopen((m_TradingDay + "/DepthMarketData.csv").c_str(), "w");
            if(outfile == NULL) {
                perror("open file error");
                exit(-1);
            }
            fprintf(outfile, "TradingDay,InstrumentID,ExchangeID,ExchangeInstID,LastPrice,PreSettlementPrice,PreClosePrice,PreOpenInterest,OpenPrice,HighestPrice,LowestPrice,Volume,\
Turnover,OpenInterest,ClosePrice,SettlementPrice,UpperLimitPrice,LowerLimitPrice,PreDelta,CurrDelta,UpdateTime,UpdateMillisec,BidPrice1,BidVolume1,AskPrice1,AskVolume1,BidPrice2,\
BidVolume2,AskPrice2,AskVolume2,BidPrice3,BidVolume3,AskPrice3,AskVolume3,BidPrice4,BidVolume4,AskPrice4,AskVolume4,BidPrice5,BidVolume5,AskPrice5,AskVolume5,AveragePrice,\
ActionDay,CircuitRefPrice\n");
            for (auto &item : m_DepthMarketData) {
                fprintf(outfile, "%s,%s,%s,%s,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%f,%f,%f,%f,%f,%s,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%d,%f,%s,%f\n",\
                    item->TradingDay, item->InstrumentID, item->ExchangeID, item->ExchangeInstID, item->LastPrice, item->PreSettlementPrice, item->PreClosePrice, item->PreOpenInterest, \
                    item->OpenPrice, item->HighestPrice, item->LowestPrice, item->Volume, item->Turnover, item->OpenInterest, item->ClosePrice, item->SettlementPrice, \
                    item->UpperLimitPrice, item->LowerLimitPrice, item->PreDelta, item->CurrDelta, item->UpdateTime, item->UpdateMillisec, item->BidPrice1, item->BidVolume1, \
                    item->AskPrice1, item->AskVolume1, item->BidPrice2, item->BidVolume2, item->AskPrice2, item->AskVolume2, item->BidPrice3, item->BidVolume3, item->AskPrice3, \
                    item->AskVolume3, item->BidPrice4, item->BidVolume4, item->AskPrice4, item->AskVolume4, item->BidPrice5, item->BidVolume5, item->AskPrice5, item->AskVolume5, \
                    item->AveragePrice, item->ActionDay, item->CircuitRefPrice);
                fflush(outfile);
            }
            fclose(outfile);
            m_Querying = false;
        }
    }
}
