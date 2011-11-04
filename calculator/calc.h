#pragma once

const int NAME_LEN_MAX = 30;
const int EXPR_LEN_MAX = 255;
const int ERR_LEN_MAX = 255;

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctype.h>
#include <cmath>
#include <vector>

using namespace std;

namespace calc {
	double factorial(double value);
	double sign(double value);
	void toupper(char upper[], const char str[]);

	class Error {
	public:
		Error(const int row, const int col, const int id, ...);

		int get_row() {return err_row;} // Returns the row of the error
		int get_col() {return err_col;} // Returns the column of the error
		int get_id() {return err_id;}   // Returns the id of the error
		char* get_msg() {return msg;}   // Returns a pointer to the error msg

	private:
		int err_row;    // row where the error occured
		int err_col;    // column (position) where the error occured
		int err_id;     // id of the error
		char msg[255];

		char* msgdesc(const int id);
	};

	class Variablelist {
	public:
		bool exist(const char* name);
		bool add(const char* name, double value);
		bool del(const char* name);

		bool get_value(const char* name, double* value);
		bool get_value(const int id, double* value);
		int  get_id(const char* name);
		bool set_value(const char* name, const double value);

	private:
		struct VAR
		{
			char name[NAME_LEN_MAX+1];
			double value;
		};

		vector<VAR> var;
	};

	class Parser
	{
		// public functions
	public:
		Parser();
		char* parse(const char expr[]);

		// enumerations
	private:

		enum TOKENTYPE {NOTHING = -1, DELIMETER, NUMBER, VARIABLE, FUNCTION, UNKNOWN};

		enum OPERATOR_ID {AND, OR, BITSHIFTLEFT, BITSHIFTRIGHT,                 // level 2
			EQUAL, UNEQUAL, SMALLER, LARGER, SMALLEREQ, LARGEREQ,    // level 3
			PLUS, MINUS,                     // level 4
			MULTIPLY, DIVIDE, MODULUS, XOR,  // level 5
			POW,                             // level 6
			FACTORIAL};                      // level 7

		// data
	private:
		char expr[EXPR_LEN_MAX+1];    // holds the expression
		char* e;                      // points to a character in expr

		char token[NAME_LEN_MAX+1];   // holds the token
		TOKENTYPE token_type;         // type of the token

		double ans;                   // holds the result of the expression
		char ans_str[255];            // holds a string containing the result 
		// of the expression

		Variablelist user_var;        // list with variables defined by user

		// private functions
	private:
		void getToken();

		double parse_level1();
		double parse_level2();
		double parse_level3();
		double parse_level4();
		double parse_level5();
		double parse_level6();
		double parse_level7();
		double parse_level8();
		double parse_level9();
		double parse_level10();
		double parse_number();

		int get_operator_id(const char op_name[]);
		double eval_operator(const int op_id, const double &lhs, const double &rhs);
		double eval_function(const char fn_name[], const double &value);
		double eval_variable(const char var_name[]);

		int row();
		int col();
	};
}
