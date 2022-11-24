/*
Author - Hardik Suhag

Compilation Environment:
Use any linux distribution with GNU compiler on or after C++ 14 ( >= GCC 6.3)

To compile program use:-
$g++ assembler.cpp -o assembler
$./assembler COPY.txt

To view 'Intermediate File', 'Assembly listing' or 'Output Object Program' use:-
$cat intermediate_file.txt
$cat assembly_listing.txt
$cat output_object_program.txt

To view how the code works and its documentation, open the 'readme.txt' file:-
$cat readme.txt

*/

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <unordered_map>
using namespace std;

// assembler settings
#define SHOW_W_LINBL 0
#define INDENT "\t\t"
#define PRG_DFLTN "UNTITL"
#define IMD_FNAME "intermediate_file.txt"
#define ALS_FNAME "assembly_listing.txt"
#define OBJ_FNAME "output_object_program.txt"

// macros for type of operands
#define O_NOOPD "no operand"
#define O_LABEL "label"
#define O_INDXD "indexed"
#define O_DECIM "decimal"
#define O_HEXAD "hexadecimal"
#define O_CHRAR "character array"

// ERROR STATEMENTS =>
// file related errors
#define E_NOARG "No input file provided"
#define E_NOFIL "Can't open the input file, perhaps the name was wrong?"
#define E_NOIMD "Can't locate the intermediate assembly file, perhaps it was relocated or deleted?"

// pass - 1 => Errors in <START>, <END> statements
#define E_NOFST "The first instruction is not a START instruction, use the format '<label> START <address>'"
#define E_NOSTT "Program Empty, no start instruction found"
#define E_MPLST "More than one start directives in the program, ensure only one is present"
#define E_NOEND "Could not find END directive in program"

// pass - 1 => Mnemonic Errors
#define E_INVMN(mnemonic) ((string("Operation mnemonic <")+string(mnemonic))+string("> is not valid, maybe there was a typo"))
#define E_NOOPN "The instruction does not contain any operation mnemonic"
#define E_TWOMN "Two mnemonics specified in the line"

// pass - 1 => Operand/Label Errors
#define E_RSNOO(opn) (string("No operand specified in the <") + string(opn) + string("> assembler directive"))
#define E_MANYO "Two or more operands or operation mnemonics specified in line"
#define E_SALEX(symbol) (string("The symbol <") + string(symbol) + string("> already exists. Please remove multiple declarations"))
#define E_STOUT "String is too large. Maximum length of 30 bytes allowed in charcter arrays"
#define E_WGIDX "Incorrect usage of indexed addressing. Only the index regsiter X is permitted"
#define E_NALNM "The symbol is not an alphanumeric starting with a letter"
#define E_SMFMT "Invalid operand syntax"

// pass - 2 => syntax errors and structural errors
#define E_RWNDM "<RESW> and <RESB> only accept numeric (decimal) operands"
#define E_WDNDM "<WORD> assembler directive only accepts numeric (decimal) operands"
#define E_DTERR "Constants require data in decimal/hexadecimal/charater-array formats"
#define E_DTOUT "Constant's data is out of bounds. Word is 3 bytes long and Byte is 1 byte long"
#define E_RSUBO "The instruction <RSUB> expects no operand. One provided"
#define E_DNALW "Direct addressing is not allowed in Sic, you must specify label or variable names"
#define E_INVSY(symbol) ((string("No variable named <")+string(symbol))+string("> was declared in the program"))
#define E_ENDOP "Invalid operand for the <END> intruction. Please specify the label of the first instruction to execute"

// other errors
#define E_LOCLG "Starting address of program too large for the entire program to fit into memory, try reducing it in the START instruction"
#define E_INTRL "Syntax error, please correct syntax and try again."

// warning flags
bool fl_empty_lines;

// Warning Statements
#define W_LINBL "The program contains one or more non-empty blank lines"
#define W_NOSTL "Program name not specified in START instruction"
#define W_NOSTO "Program starting address not specified in START instruction"
#define W_NOENO "First executable instruction not specified, please specify operand in END directive"

// Declaration of the instruction class
class instruction;

// file streams
ifstream assembly_program;
ofstream intermediate_file_w;
ifstream intermediate_file_r;
ofstream assembly_listing;
ofstream object_program;

// data structures
unordered_map <string, int> OPTAB;
unordered_map <string, int> SYMTAB;
unordered_map <string, int> DIRECTIVES;

// global program information variables
int program_starting_address;
int first_executable_instruction;
int length_of_program;
string program_name;

// global variables
string input_file_name;
vector<instruction> program;

// prints errors to standard output
void error(string error_string, string* err_ins=NULL, int ln = 0){
	cout << "Error: " << error_string << "\n";
	if(err_ins){
		cout << "line-" << ln << ">\t" << *err_ins << "\n";
	}
	exit(0);
}

// prints warnings to standard output
void warning(string warning_string, string* err_ins=NULL, int ln=0){
	cout << "Warning: " << warning_string << "\n";
	if(err_ins){
		cout << "line-" << ln << ">\t" << *err_ins << "\n";
	}
}

// for conversion of a number to hexadecimal string of specified size
string to_hex(int num, int size){
	string q = "";
	while(num){
		int rem = num%16;
		num/=16;
		if(rem<10) q.push_back('0'+rem);
		else q.push_back(rem-10+'A');
	}
	while(q.size()<size) q.push_back('0');
	reverse(q.begin(), q.end());
	return q;
}

// definition of the instruction class
class instruction{
public:
	string instruction_text; // source code text of instruction
	int source_line_number; // line number of instruction in source code

	string label;
	string mnemonic;
	string operand;

	bool has_location; // only time an instruction has location but not size in memory is START
	int location; // location of ins. in RAM
	int size_in_memory; // size of ins. in RAM
	bool has_object_code; // if the ins. has object code in RAM
	string object_code; // object code of the ins. (set later)

	bool is_comment; // if the line is a comment
	string comment; // text of the comment
	string type_of_operand; /* specifies the type of operand in line. can be - 
		O_NOOPD => "no operand"
		O_LABEL => "label"
		O_INDXD => "indexed"
		O_DECIM => "decimal"
		O_HEXAD => "hexadecimal"
		O_CHRAR => "character array"
	*/

	bool is_blank; // if its a blank line

	// constructor of instruction class
	instruction(string line, int source_line_number){

		// set default values for member variables
		this->source_line_number = source_line_number; instruction_text = line; 
		label = ""; mnemonic = ""; operand = ""; 
		has_object_code = false; has_location = false; location = -1; size_in_memory = 0; object_code = "";
		is_comment = false; comment = ""; type_of_operand = O_NOOPD;
		is_blank = 0;
		int line_len = line.size();

		// check if comment
		for(int i = 0; i<line_len; i++){ // O(n) loop
			if(isspace(line[i])) continue;
			if(line[i] != '.') break;
			for(int j = i+1; j<line_len; j++) comment.push_back(line[j]);
			is_comment = 1;
			break;
		}
		if(is_comment) return;

		// parse the line
		vector<string> words = {""};
		for(int i = 0; i<line_len; i++){
			if(isspace(line[i])){
				if(words.back().size()==0) continue;
				words.push_back("");
			}else{
				words.back().push_back(line[i]);
			}
		}
		while(words.size() && words.back().size()==0) words.pop_back();

		if(words.size()==0){ // blank case
			is_blank = 1;
			fl_empty_lines = true;
			return;
		}else if(words.size()==1){
			// it can only be a command (operation or directive)
			if(DIRECTIVES.count(words[0])){
				mnemonic = words[0];
			}else if(OPTAB.count(words[0])){
				mnemonic = words[0];
			}else{
				error(E_INVMN(words[0]), &(this->instruction_text), source_line_number);
			}
		}else if(words.size()==2){
			int keyword_matches = (DIRECTIVES.count(words[0]) || OPTAB.count(words[0])); 
			keyword_matches *= 10;
			keyword_matches += (DIRECTIVES.count(words[1]) || OPTAB.count(words[1]));
			if(keyword_matches == 0) error(E_NOOPN, &(this->instruction_text), source_line_number);
			else if(keyword_matches == 11) error(E_TWOMN, &(this->instruction_text), source_line_number);
			else if(keyword_matches == 10){
				mnemonic = words[0];
				operand = words[1];
			}else if(keyword_matches == 1){
				label = words[0];
				mnemonic = words[1];
			}else{
				error(E_INTRL, &(this->instruction_text), source_line_number);
			}
		}else if(words.size()==3){
			label = words[0];
			mnemonic = words[1];
			operand = words[2];
			if(!(OPTAB.count(mnemonic) || DIRECTIVES.count(mnemonic))) 
				error(E_INVMN(mnemonic), &(this->instruction_text), source_line_number);
		}else{
			error(E_MANYO, &(this->instruction_text), source_line_number);
		}

		
		// sanitizing label
		if(label.size() && !is_label_format(label)) error(E_NALNM , &(this->instruction_text), source_line_number);

		// sanitizing operand and setting type_of_operand
		if(operand.size()){
			int cnt_commas = 0;
			int cnt_quotes = 0;
			for(int j = 0; j<operand.size(); j++) cnt_commas += (operand[j] == ',');
			for(int j = 0; j<operand.size(); j++) cnt_quotes += (operand[j]=='\'');

			// first check for comma, then check for quotes, then check for starting letter, then alphanumeric for label
			if(cnt_commas){ // comma case
				int len = operand.size();

				if(cnt_commas>1) error(E_SMFMT, &(this->instruction_text), source_line_number);
				if(operand.size()<2 || !(operand[len-2]==',' && operand[len-1]=='X'))
					error(E_WGIDX, &(this->instruction_text), source_line_number);
				if(!is_label_format(operand.substr(0,len-2))) error(E_NALNM, &(this->instruction_text), source_line_number);

				type_of_operand = O_INDXD;
			}else if(cnt_quotes){ // quotes case
				int len = operand.size();

				if(len<4 || (!(operand[1]=='\'' && operand[len-1]=='\''))) 
					error(E_SMFMT, &(this->instruction_text), source_line_number);

				if(operand[0]=='X') { // Hexadecimal case
					type_of_operand = O_HEXAD;
					for(char dig: operand) 
						if(!(isdigit(dig) || ('A'<=dig || dig<='F') )) 
							error(E_SMFMT, &(this->instruction_text), source_line_number);
				}
				else if(operand[0]=='C') { // Character string case
					if(cnt_quotes>2) error(E_SMFMT, &(this->instruction_text), source_line_number);
					for(char x: operand) if(x>255) error(E_SMFMT, &(this->instruction_text), source_line_number);
					if(operand.size() > 33) error(E_STOUT ,&(this->instruction_text), source_line_number);
					type_of_operand = O_CHRAR;
				} else {
					error(E_SMFMT, &(this->instruction_text), source_line_number);
				}
			}else if(isdigit(operand[0])){ // decimal case
				for(char dig: operand) if(!isdigit(dig)) error(E_NALNM, &(this->instruction_text), source_line_number);
				type_of_operand = O_DECIM;
			}else{ // label case
				if(!is_label_format(operand)) error(E_NALNM, &(this->instruction_text), source_line_number);
				type_of_operand = O_LABEL;
			}
		}

		// finding the value of instruction's size in memory
		if(OPTAB.count(mnemonic)){
			size_in_memory = 3;
		}else{
			if(mnemonic == "START" || mnemonic == "END") return;
			if(operand.size()==0) error(E_RSNOO(mnemonic), &(this->instruction_text), source_line_number);
			if(mnemonic == "RESW") {
				if(type_of_operand == O_HEXAD){
					string data_hex = operand.substr(2,operand.size()-3);
					size_in_memory = 3 * stoi(data_hex, NULL, 16);
				} else 
					size_in_memory = stoi(operand)*3;
			} else if(mnemonic == "RESB") {
				if(type_of_operand == O_HEXAD){
					string data_hex = operand.substr(2,operand.size()-3);
					size_in_memory = stoi(data_hex, NULL, 16);
				} else 
					size_in_memory = stoi(operand);
			} else if(mnemonic == "WORD") size_in_memory = 3;
			else if(mnemonic == "BYTE"){ // byte case
				if(type_of_operand == O_HEXAD){
					size_in_memory = 1;
				}else if(type_of_operand == O_CHRAR){
					size_in_memory = operand.size() - 3;
				}else{ // default should be decimal, structural data sanity checks will done in pass-2
					size_in_memory = 1;
				}
			}else{
				error(E_INTRL, &(this->instruction_text), source_line_number);
			}
		}
	}
	
	bool is_label_format(string str){ // if str is alphanumeric starting with a letter or not
		if(str.size()==0) error(E_INTRL);
		if(!isalpha(str[0])) return 0;
		for(int j = 1; j<str.size(); j++) if(!isalnum(str[j])) return 0;
		return 1;
	}

	string to_string(){ // converts an instruction back to its string, including location
		string indent = INDENT;
		if(is_blank) return("\n"); // blank line case
		else if(is_comment){ // comment case
			string q = ".";
			for(char x: indent) q.push_back(x);
			for(char x: comment) q.push_back(x);
			q.push_back('\n');
			return q;
			//intermediate_file_w << "." << INDENT << comment << "\n";
		}else{
			string loc_str = "";
			if(has_location && location>=0) {// to account for START case haslocation=T,location=F
				if(location>=(2<<15)) error(E_LOCLG);
				loc_str = to_hex(location,4); // convert the location to a 4 digit hex string
			}
			else loc_str = "None";
			string q = "";
			for(char x: string("Loc-")) q.push_back(x);
			for(char x: loc_str) q.push_back(x);
			for(char x: indent) q.push_back(x);
			for(char x: label) q.push_back(x);
			for(char x: indent) q.push_back(x);
			for(char x: mnemonic) q.push_back(x);
			for(char x: indent) q.push_back(x);
			for(char x: operand) q.push_back(x);
			q.push_back('\n');
			return q;
			//intermediate_file_w << loc_str << INDENT << label << INDENT << mnemonic << INDENT << operand << "\n";
		}
		error(E_INTRL);
		return("");
	}
};

// the text record class
class text_record{
public:
	int total_length;
	int starting_address;
	vector<string> records; // to be separated by carets
	text_record(int str_addr){
		starting_address = str_addr;
		records = {};
		total_length = 0;
	}
	void insert(string object_code){
		total_length += object_code.size();
		records.push_back(object_code);
	}
	string to_string(){
		string q = "T";

		// 6 bytes of hexadecimal starting address
		int data_decimal = starting_address;
		string data_hex = "";
		while(data_decimal){
			int next_dig = data_decimal%16;
			char next_dig_char;
			if(next_dig<10) next_dig_char = '0' + next_dig;
			else next_dig_char = 'A' + (next_dig-10);
			data_hex.push_back(next_dig_char);
			data_decimal/=16;
		}
		while(data_hex.size()<6) data_hex.push_back('0');
		reverse(data_hex.begin(), data_hex.end());
		q.push_back('^');
		for(char x: data_hex) q.push_back(x);

		// 2 bytes of hexadecimal line lenght in bytes
		data_decimal = total_length/2;
		data_hex = "";
		while(data_decimal){
			int next_dig = data_decimal%16;
			char next_dig_char;
			if(next_dig<10) next_dig_char = '0' + next_dig;
			else next_dig_char = 'A' + (next_dig-10);
			data_hex.push_back(next_dig_char);
			data_decimal/=16;
		}
		while(data_hex.size()<2) data_hex.push_back('0');
		reverse(data_hex.begin(), data_hex.end());
		q.push_back('^');
		for(char x: data_hex) q.push_back(x);

		for(string object_code: records){
			q.push_back('^');
			for(char x: object_code) q.push_back(x);
		}

		return(q);

	}
};

void pass_1(){
	assembly_program.open(input_file_name);
	intermediate_file_w.open(IMD_FNAME);
	if(!assembly_program.is_open()) error(E_NOFIL);

	// main code of the first pass
	// reading and parsing the file
	program = {};
	int source_line_number = 1;
	while(!assembly_program.eof()){
		string line;
		getline(assembly_program, line);
		instruction ins = instruction(line, source_line_number);
		source_line_number++;
		program.push_back(ins);
	}

	// deal with start instruction
	int line_number = 0;
	for(line_number = 0; line_number<program.size() ; line_number++){
		if(program[line_number].mnemonic=="START") break;
		if(program[line_number].mnemonic!="") 
			error(E_NOFST, &(program[line_number].instruction_text), program[line_number].source_line_number);
	}
	if(line_number==program.size()) error(E_NOSTT);
	instruction& start_ins = program[line_number];
	if(start_ins.label==""){
		warning(W_NOSTL);
		start_ins.label=PRG_DFLTN;
	}
	if(start_ins.operand==""){
		warning(W_NOSTO, &(start_ins.instruction_text), start_ins.source_line_number);
		start_ins.operand="0";
	}
	program_starting_address = stoi(start_ins.operand, NULL, 16);
	program_name = start_ins.label;

	// deal with end instruction
	for(line_number=0; line_number<program.size();line_number++) if(program[line_number].mnemonic=="END") break;
	if(line_number==program.size()) error(E_NOEND);
	instruction& end_ins = program[line_number];
	if(end_ins.operand==""){
		warning(W_NOENO, &(end_ins.instruction_text), end_ins.source_line_number);
		end_ins.operand=start_ins.label;
	}

	// set the location of every instruction
	int location_counter = program_starting_address;
	for(instruction &ins : program){
		if(ins.mnemonic=="START" && &ins!=&start_ins) error(E_MPLST, &(ins.instruction_text), ins.source_line_number);
		if(ins.mnemonic=="END") break;
		if(ins.size_in_memory || ins.mnemonic=="START"){
			ins.location = location_counter;
			ins.has_location = true;
			if(ins.label.size()){
				if(SYMTAB.count(ins.label)) error(E_SALEX(ins.label), &(ins.instruction_text), ins.source_line_number);
				SYMTAB.insert({ins.label, ins.location});
			}
			location_counter += ins.size_in_memory;
		}
	}

	// set the value of program length
	length_of_program = location_counter - program_starting_address;

	// writing back to the intermediate file
	for(instruction& ins: program){
		intermediate_file_w << ins.to_string();
	}
	
	assembly_program.close();
	intermediate_file_w.close();
	return;
}

void pass_2(){
	intermediate_file_r.open(IMD_FNAME);
	assembly_listing.open(ALS_FNAME);
	object_program.open(OBJ_FNAME);
	if(!intermediate_file_r.is_open()) error(E_NOIMD);

	// main code
	for(int line_number = 0; line_number < program.size() ; line_number++){
		// checking for the operand structure
		// all operations require a single label like structure(or a comma 
		// type structure) except RSUB (requires nothing)
		// assembler directives START and END completely sanitized, although multiple ENDs 
		// can exist in which case the first END is sanitized
		// other 4 asm directives BYTE, WORD, RESB, RESW have data types that are
		// completely identified but not sanitized
		// ** individual sanitization done, only structural 
		instruction& ins = program[line_number];
		if(ins.mnemonic == "START" || (ins.is_comment || ins.is_blank)) continue;
		if(ins.mnemonic == "END") {
			if(ins.type_of_operand != O_LABEL)
				error(E_ENDOP, &(ins.instruction_text), ins.source_line_number);
			if(!SYMTAB.count(ins.operand))
					error(E_INVSY(ins.operand), &(ins.instruction_text), ins.source_line_number);
			first_executable_instruction = SYMTAB[ins.operand];
			break;
		}

		if(ins.mnemonic == "RESW" || ins.mnemonic == "RESB"){
			if(ins.type_of_operand!=O_DECIM && ins.type_of_operand!=O_HEXAD) // structural sanity check
				error(E_RWNDM, &(ins.instruction_text), ins.source_line_number);
		}else if(DIRECTIVES.count(ins.mnemonic)){
			// either word or byte
			string otyp = ins.type_of_operand;
			if(ins.mnemonic=="WORD" && otyp!=O_DECIM) // structural sanity check
				error(E_WDNDM, &(ins.instruction_text), ins.source_line_number);
			if((otyp!=O_DECIM) && (otyp!=O_HEXAD && otyp!=O_CHRAR)) // structural sanity check
				error(E_DTERR, &(ins.instruction_text), ins.source_line_number);
			
			if(otyp==O_DECIM){
				int max_data_size;
				if(ins.mnemonic=="WORD") max_data_size = 3; else max_data_size = 1;
				int data_decimal = stoi(ins.operand);
				if(data_decimal > ((1<<(8*max_data_size))-1)) 
					error(E_DTOUT, &(ins.instruction_text), ins.source_line_number);
				string data_hex = "";
				while(data_decimal){
					int next_dig = data_decimal%16;
					char next_dig_char;
					if(next_dig<10) next_dig_char = '0' + next_dig;
					else next_dig_char = 'A' + (next_dig-10);
					data_hex.push_back(next_dig_char);
					data_decimal/=16;
				}

				while(data_hex.size() < (2*max_data_size)) data_hex.push_back('0');
				
				reverse(data_hex.begin(),data_hex.end());
				ins.has_object_code = 1;
				ins.object_code = data_hex;
			}else if(otyp == O_HEXAD){
				string data_hex = "";
				for(int i=(ins.operand.size()-2); i>=2; i--) data_hex.push_back(ins.operand[i]);
				if(data_hex.size()%2) data_hex.push_back('0');
				reverse(data_hex.begin(), data_hex.end());
				if(data_hex.size()!=2)
					error(E_DTOUT, &(ins.instruction_text), ins.source_line_number);
				ins.has_object_code = 1;
				ins.object_code = data_hex;
			}else if(otyp == O_CHRAR){
				for(int i = 2; i<(ins.operand.size()-1); i++){
					int data_decimal = ins.operand[i];
					string data_hex = "";
					while(data_decimal){
						int next_dig = data_decimal%16;
						char next_dig_char;
						if(next_dig<10) next_dig_char = '0' + next_dig;
						else next_dig_char = 'A' + (next_dig-10);
						data_hex.push_back(next_dig_char);
						data_decimal/=16;
					}
					if(data_hex.size()%2) data_hex.push_back('0');
					reverse(data_hex.begin(), data_hex.end());
					ins.object_code.push_back(data_hex[0]);
					ins.object_code.push_back(data_hex[1]);
					ins.has_object_code = 1;
				}
			}else{
				// should never reach here as sanitization alreaady done while parsing the line
				error(E_DTERR, &(ins.instruction_text), ins.source_line_number);
			}
		}else{
			if(ins.mnemonic=="RSUB"){
				if(ins.operand.size())  // structural sanity check
					error(E_RSUBO, &(ins.instruction_text), ins.source_line_number);
				ins.object_code = "4C0000";
				ins.has_object_code = 1;
			}else{ // all operations other than RSUB expect a memory address 'm' (as label or buffer,x)
				if(ins.type_of_operand!=O_LABEL && ins.type_of_operand!=O_INDXD) // structural sanity check
					error(E_DNALW, &(ins.instruction_text), ins.source_line_number);
				int data_decimal = OPTAB[ins.mnemonic] << 16;
				string final_opd = ins.operand;
				if(ins.type_of_operand==O_INDXD) {
					final_opd = final_opd.substr(0,(final_opd.size()-2));
					data_decimal += (1<<15);
				}
				if(!SYMTAB.count(final_opd))
					error(E_INVSY(final_opd), &(ins.instruction_text), ins.source_line_number);
				data_decimal += SYMTAB[final_opd];
				string data_hex = "";
				while(data_decimal){
					int next_dig = data_decimal%16;
					char next_dig_char;
					if(next_dig<10) next_dig_char = '0' + next_dig;
					else next_dig_char = 'A' + (next_dig-10);
					data_hex.push_back(next_dig_char);
					data_decimal/=16;
				}
				if(data_hex.size()>6) error(E_INTRL);
				while(data_hex.size()<6) data_hex.push_back('0');
				reverse(data_hex.begin(), data_hex.end());
				ins.object_code = data_hex;
				ins.has_object_code = 1;
			}
		}
	}

	// writing to the final file
	assembly_listing << "Obj" << INDENT << "Location\n\n";
	for(instruction &ins: program){
		assembly_listing << ins.object_code << INDENT << ins.to_string();
	}

	// generating the object code records
	string header_record = "";
	bool insert_new = 1;
	vector<text_record> text_records = {};
	string end_record = "";
	for(instruction &ins: program){
		if(ins.is_comment || ins.is_blank) continue;
		if(ins.mnemonic == "START"){// write header record
			header_record.push_back('H');

			header_record.push_back('^');
			for(int i = 0; i<6; i++) {
				if(i<program_name.size()) header_record.push_back(program_name[i]);
				else header_record.push_back(' ');
			}

			// 6 bytes of hexadecimal starting address
			string data_hex = "";
			int data_decimal = program_starting_address;
			while(data_decimal){
				int next_dig = data_decimal%16;
				char next_dig_char;
				if(next_dig<10) next_dig_char = '0' + next_dig;
				else next_dig_char = 'A' + (next_dig-10);
				data_hex.push_back(next_dig_char);
				data_decimal/=16;
			}
			while(data_hex.size()<6) data_hex.push_back('0');
			reverse(data_hex.begin(), data_hex.end());
			header_record.push_back('^');
			for(char x: data_hex) header_record.push_back(x);

			// 6 bytes of hexadecimal length of program
			data_hex = "";
			data_decimal = length_of_program;
			while(data_decimal){
				int next_dig = data_decimal%16;
				char next_dig_char;
				if(next_dig<10) next_dig_char = '0' + next_dig;
				else next_dig_char = 'A' + (next_dig-10);
				data_hex.push_back(next_dig_char);
				data_decimal/=16;
			}
			while(data_hex.size()<6) data_hex.push_back('0');
			reverse(data_hex.begin(), data_hex.end());
			header_record.push_back('^');
			for(char x: data_hex) header_record.push_back(x);

			continue;
		}
		if(ins.mnemonic == "END"){// write end record
			end_record.push_back('E');
			end_record.push_back('^');
			// 6 bytes of hexadecimal first executable instruction
			string data_hex = "";
			int data_decimal = first_executable_instruction;
			while(data_decimal){
				int next_dig = data_decimal%16;
				char next_dig_char;
				if(next_dig<10) next_dig_char = '0' + next_dig;
				else next_dig_char = 'A' + (next_dig-10);
				data_hex.push_back(next_dig_char);
				data_decimal/=16;
			}
			while(data_hex.size()<6) data_hex.push_back('0');
			reverse(data_hex.begin(), data_hex.end());
			for(char x: data_hex) end_record.push_back(x);

			break;
		}
		// time to write to text records
		int location = ins.location;
		string object_code = ins.object_code;
		if(ins.mnemonic == "RESW" || ins.mnemonic == "RESB"){
			insert_new = true;
			continue;
		}
		if(insert_new){
			text_records.push_back(text_record(ins.location));
			text_records.back().insert(ins.object_code);
			insert_new = false;
			continue;
		}
		if((text_records.back().total_length + ins.object_code.size()) > 60){
			text_records.push_back(text_record(ins.location));
			text_records.back().insert(ins.object_code);
		}else{
			text_records.back().insert(ins.object_code);
		}
	}

	// writing to the object file
	object_program << header_record << "\n";
	for(text_record tr: text_records){
		object_program << tr.to_string() << "\n";
	}
	object_program << end_record << "\n";

	// closing streams
	intermediate_file_r.close();
	assembly_listing.close();
	object_program.close();
	return;
}

void populate_OPTAB(){
	OPTAB = {
		{"LDA", 0x00}, {"LDX", 0x04}, {"LDL", 0x08},
		{"STA", 0x0c}, {"STX", 0x10}, {"STL", 0x14}, 
		{"LDCH", 0x50}, {"STCH", 0x54}, 
		{"ADD", 0x18}, {"SUB", 0x1c}, {"MUL", 0x20}, {"DIV", 24},
		{"COMP", 0x28}, 
		{"J", 0x3c}, {"JLT", 0x38}, {"JEQ", 0x30}, {"JGT", 0x34},
		{"JSUB", 0x48}, {"RSUB", 0x4c},
		{"TIX", 0x2c},
		{"TD", 0xe0}, {"RD", 0xd8}, {"WD", 0xdc}
	};
}

void populate_DIRECTIVES(){
	DIRECTIVES = {
		{"START", 0}, {"END", 0}, {"BYTE", 0}, {"WORD", 0}, {"RESB", 0}, {"RESW", 0}
	};
}

void populate_SYMTAB(){
	SYMTAB = {};
}

int main(int argc, char** args){

	// finding the program's name from cmd line argument
	if(argc < 2) error(E_NOARG);
	input_file_name = args[1];
	program_starting_address = 0;
	program_name = PRG_DFLTN;

	// setting flags
	fl_empty_lines = false;

	// populates tables
	populate_OPTAB();
	populate_SYMTAB();
	populate_DIRECTIVES();

	// Assembling starts here
	pass_1();
	pass_2();

	// show warning of empty lines if flag is ON
	if(fl_empty_lines && SHOW_W_LINBL) warning(W_LINBL);

	// ending notes of the assembler
	cout << "Code assembled successfully\n";
	cout << "Intermediate File Written to file\t\t:" << IMD_FNAME << "\n";
	cout << "Assembly Listing written to file \t\t:" << ALS_FNAME << "\n";
	cout << "Object Code written to file      \t\t:" << OBJ_FNAME << "\n";

	return 0;
}
