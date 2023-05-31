#!/usr/bin/bash
Hour=`date +%H`
time=19
day=`date +%u`
friday=5
if test ${Hour} -gt ${time}; then
	if test ${day} -eq ${friday}; then
		DT=`date -d "+3 day" +%Y%m%d`
	else
		DT=`date -d "+1 day" +%Y%m%d`
	fi
else
	DT=`date +%Y%m%d`
fi
ulimit -c unlimited
cd ~/ctp_query_stock_option
export PATH=$PATH:~/ctp_query_stock_option/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/.local/lib
killall ctp-query-stock-option
ctp-query-stock-option TraderAccount_option.cfg ${DT}-option &> query-ctp-option.log
cp -r ${DT}-option /mnt/nas-3/OPTION/ctp_option/${DT}
ctp-query-stock-option TraderAccount_spot.cfg ${DT}-spot &> query-ctp-spot.log
cp -r ${DT}-spot /mnt/nas-3/OPTION/ctp_spot/${DT}