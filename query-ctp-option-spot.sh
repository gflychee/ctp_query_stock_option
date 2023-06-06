#!/usr/bin/bash
DT=`date +%Y%m%d`
ulimit -c unlimited
cd ~/ctp_query_stock_option
export PATH=$PATH:~/ctp_query_stock_option/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/ctp_query_stock_option/lib
killall ctp-query-stock-option
ctp-query-stock-option TraderAccount_option.cfg ${DT}-option &> query-ctp-option.log
cp -r ${DT}-option /mnt/nas-3/OPTION/ctp_option/${DT}
