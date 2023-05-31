#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#include <string>

struct Insinfo {
	//std::string id;
	char id[21];
	double tick;
	int multiplier;
	double margin_rate;
	double commission_rate_open;
	double commission_rate_close_yesterday;
	double commission_rate_close_today;
	double commission_fixed_open;
	double commission_fixed_close_yesterday;
	double commission_fixed_close_today;
	int open_limit;
	int cancel_limit;
	int trade_limit;
	double preclose;
	double lowlimit;
	double highlimit;
	//std::string session;
};

struct TraderAccount {
    std::string server;
    std::string broker;
    std::string userid;
    std::string passwd;
    std::string investor;
    std::string appid;
    std::string authid;
};

// struct MdAccount {
//     string server;
//     string broker;
//     string userid;
//     string passwd;
//     string flow_path;
// };


#endif