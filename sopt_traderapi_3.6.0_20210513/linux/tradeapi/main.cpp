#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <sys/time.h>
#include <map>
#include <iostream>
#include <vector>
#include <thread>
#include "ctp-query.h"
#include "common.h"

void load_trader_account(const char *fname, TraderAccount &traderacc) {
    FILE *fp;

    if ((fp = fopen(fname, "r")) == NULL) {
        printf("fail to open %s", fname);
        exit(-1);
    }

    char buf[4096];

    while (fgets(buf, sizeof(buf), fp)) {
        traderacc.server = strtok(buf, ",\n");
        traderacc.broker = strtok(NULL, ",\n");
        traderacc.userid = strtok(NULL, ",\n");
        traderacc.passwd = strtok(NULL, ",\n");
        traderacc.investor = strtok(NULL, ",\n");
        traderacc.appid = strtok(NULL, ",\n");
        traderacc.authid = strtok(NULL, ",\n");
    }
    fclose(fp);
}

void get_trader(const char *fname, const char *day, CFtdTrader &trader)
{
    TraderAccount traderacc;
    load_trader_account(fname, traderacc);
    trader.Init(traderacc.server, traderacc.broker, traderacc.userid, traderacc.passwd, traderacc.investor, traderacc.appid, traderacc.authid, day);
    trader.LogonServer();
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("usage: %s TraderAccount.cfg TradingDay\n", argv[0]);
        exit(-1);
    }
    CFtdTrader trader;
    get_trader(argv[1], argv[2], trader);
    trader.ReqQryInstrument();
    //trader.ReqQryCommissionRate();
    trader.ReqQryDepthMarketData();
    while (trader.m_Querying) {
        sleep(1);
    }
    return 0;
}

