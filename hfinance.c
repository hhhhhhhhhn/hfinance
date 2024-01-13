#include "hlib/core.h"
#include "hlib/hstring.h"
#include "hlib/hparse.h"
#include "hlib/hhashmap.h"
#include "hlib/hflag.h"

typedef struct {
	u32 year;
	u32 month;
	u32 day;
} Date;

typedef struct {
	Date date;
	str description;
} Transaction;

Date parse_date(str view) {
	view = str_trim(view);
	str day_str = str_split_char(&view, '/');
	str month_str = str_split_char(&view, '/');
	str year_str = view;

	u64 day;
	u64 month;
	u64 year;
	if (!hparse_u64(day_str, &day)) printf("Failed to parse date '%.*s'", (int)view.len, view.data);
	if (!hparse_u64(month_str, &month)) printf("Failed to parse date '%.*s'", (int)view.len, view.data);
	if (!hparse_u64(year_str, &year)) printf("Failed to parse date '%.*s'", (int)view.len, view.data);

	return (Date){
		.day = (u32)day,
		.month = (u32)month,
		.year = (u32)year,
	};
}

const Date MAX_DATE = {~0, ~0, ~0};
const Date MIN_DATE = {0, 0, 0};

void print_accounts(HHashMap* accounts) {
	usize index = 0;
	str* account;
	i64* balance;
	while(hhashmap_next(accounts, &account, &balance, &index)) {
		printf("%.*s: %ld\n", (int)account->len, account->data, *balance);
	}
}

// Transforms "expenses.misc.rent" into "expenses.misc"
str get_parent_account(str account) {
	for(i64 index = account.len-1; index >= 0; index--) {
		if (account.data[index] == '.') {
			return str_slice(account, 0, index);
		}
	}
	return str_slice(account, account.len, account.len);
}

void process_transaction(HHashMap* accounts, str view) {
	i64 total = 0;

	while(view.len > 0) {
		str transaction_str = str_trim(str_split_char(&view, '\n'));
		str account = str_trim(str_split_char(&transaction_str, ':'));
		str amount_str  = str_trim(transaction_str);

		i64 amount;
		if (!hparse_i64(amount_str, &amount)) panicf("Invalid amount: '%.*s'", (int)amount_str.len, amount_str.data);

		while (account.len > 0) {
			if(hhashmap_get(accounts, &account) == NULL) {
				i64 zero = 0;
				hhashmap_set(accounts, &account, &zero);
			}
			i64* balance = (i64*)hhashmap_get(accounts, &account);
			*balance += amount;

			account = get_parent_account(account);
		}

		total += amount; // Total
	}

	if(total != 0) panicf("Transaction sum does not add to 0, but instead %ld", total);
}


i32 cmp_dates(Date a, Date b) {
	if(a.year < b.year) return -1;
	if(a.year > b.year) return 1;
	if(a.month < b.month) return -1;
	if(a.month > b.month) return 1;
	if(a.day < b.day) return -1;
	if(a.day > b.day) return 1;
	return 0;
}

void process_hfinance_file(str view, Date start_date, Date end_date) {
	HHashMap accounts = hhashmap_new(sizeof(str), sizeof(i64), HKEYTYPE_STR);
	Date last_date = {0};
	Transaction last_transaction;

	while(view.len > 0) {
		str transaction_str = str_split_str(&view, STR("\n\n"));
		transaction_str = str_trim(transaction_str);

		if (transaction_str.len == 0) continue;

		str header_str = str_split_char(&transaction_str, '\n');
		str date_str = str_split_char(&header_str, ':');
		str description = header_str;

		last_transaction = (Transaction){
			.date = parse_date(date_str),
			.description = str_trim(description)
		};
		if(cmp_dates(last_transaction.date, last_date) < 0) panic("Dates not cronological");
		last_date = last_transaction.date;

		if(cmp_dates(last_transaction.date, start_date) < 0) continue; // TODO: Optimize
		if(cmp_dates(last_transaction.date, end_date) > 0) break;

		process_transaction(&accounts, transaction_str);
	}
	print_accounts(&accounts);
	hhashmap_free(&accounts);
}

i32 main(int argc, char** argv) {
	str* start_str = hflag_str('s', "start", "Start of period (d/m/y)", STR("None"));
	str* end_str = hflag_str('e', "end", "End of period (d/m/y)", STR("None"));
	hflag_parse(&argc, &argv);

	Date start;
	Date end;
	if (str_eq(*start_str, STR("None"))) {
		start = MIN_DATE;
	} else {
		start = parse_date(*start_str);
	}

	if (str_eq(*end_str, STR("None"))) {
		end = MAX_DATE;
	} else {
		end = parse_date(*end_str);
	}

	strbResult result = strb_from_file(stdin);
	if (!result.ok) {
		panic("Failed to read from stdin");
	}

	str view = str_from_strb(&result.builder);

	process_hfinance_file(view, start, end);

	strb_free(&result.builder);
	return 0;
}
