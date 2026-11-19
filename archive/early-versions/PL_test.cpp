#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iomanip>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <memory>
using namespace std;

// ========================
// Token 與節點結構定義
// ========================
struct Node;
class Environment;

// Token 的類型枚舉
enum TokenType {
    LEFT_PAREN, RIGHT_PAREN, INT, FLOAT, STRING, DOT, NIL, T, QUOTE, SYMBOL, 
    END_OF_FILE, LET, LAMBDA, VERBOSE, VERBOSE_QUERY
};

// 表示一個詞彙單位
struct Token {
	TokenType type;
	string value;
	int line = 1, column = 1;
};

/***
 * 記錄變數與對應的值
 */
class Environment {
	map<string, Node*> table;
    bool verboseMode = true;
    Environment* parent;
public:
    Environment(Environment* p = nullptr) : parent(p) {}
    Environment(const Environment& other);
	// 定義一個符號對應到某個值
	void define(const string& symbol, Node* value);

	// 查詢某個符號對應的值
	Node* lookup(const string& symbol);

	// 清空所有定義的符號
	void clear();

	// 檢查某個符號是否為內建 primitive
	bool isPrimitive(const string& symbol);
    bool isVerbose() const { return verboseMode; }
    void setVerbose(bool mode) { verboseMode = mode; }
};

// S-expression 的節點結構
struct Node {
	bool isAtom;
	Token atom;
	Node* left;
	Node* right;
    Environment* env; 

    Node(bool isAtom_, const Token& atom_, Node* left_, Node* right_)
        : isAtom(isAtom_), atom(atom_), left(left_), right(right_), env(nullptr) {}
	Node() : isAtom(false), left(nullptr), right(nullptr), env(nullptr) {}
    Node(const Node& other);
    ~Node();
};

Node::Node(const Node& other) : 
    isAtom(other.isAtom), atom(other.atom),
    left(other.left ? new Node(*other.left) : nullptr),
    right(other.right ? new Node(*other.right) : nullptr),
    env(other.env ? new Environment(*other.env) : nullptr) {}

Node::~Node() {
    delete left;
    delete right;
    delete env;
}

// ========================
// 字元分析器：負責掃描輸入並產生 token
// ========================
class LexError : public runtime_error {
public:
	int line;
	int column;
	string symbol;

	LexError(const string& msg, int ln = 0, int col = 0, const string& sym = "")
		: runtime_error(msg), line(ln), column(col), symbol(sym) {}
};

class LexicalAnalyer {
public:
	// 回傳下一個 Token
	Token GetNextToken();

	// 重置掃描器的狀態
	void reset();

	// 吃掉當前行剩下的輸入
	void eat();

	// 設定是否在未結束的 S-exp 內部
	void SetUnfinishedSexp(bool state);

private:
	// 從 cin 取得下一字元，並記錄行列資訊
	char GetNextChar();

	// 將最近一個字元還原回輸入流
	void UngetChar();

	char last_char = '\0';
	int last_line = 0, last_column = 0;
	int line = 1, column = 1;
	bool eof_flag = false;
	bool in_unfinished_sexp = false;
};

void LexicalAnalyer::SetUnfinishedSexp(bool state) {
    in_unfinished_sexp = state;
}

Token LexicalAnalyer::GetNextToken() {
    try {
        char c;
        while (true) {
            c = GetNextChar();
            if (eof_flag) {
                Token eofToken;
                eofToken.type = END_OF_FILE;
                eofToken.line = line;
                eofToken.column = column;
                return eofToken;
            }
            
            if (in_unfinished_sexp && c == '\n') {
                continue;
            }
        
            if (c == ';') {
                while (c != '\n' && !eof_flag) c = GetNextChar();
                continue;
            }

            if (!isspace(c)) {
                break;
            }
        }

        Token token;
        token.line = last_line;
        token.column = last_column;

        if (c == '(') {
            token.type = LEFT_PAREN;
            token.value = "(";
        } 
        else if (c == ')') {
            token.type = RIGHT_PAREN;
            token.value = ")";
        } 
        else if (c == '\'') {
            token.type = QUOTE;
            token.value = "'";
        } 
        else if (c == '.') {
            token.value = ".";
            char next = GetNextChar();
            bool isFloat = false;
            if (isdigit(next)) {
                isFloat = true;
                token.value += next;
                next = GetNextChar();
                while (!isspace(next) && next != '(' && next != ')' && next != '\0' && next != '\n') {
                    if (!isdigit(next)) {
                        isFloat = false;
                    } else {
                        isFloat = true;
                    }
                    token.value += next;
                    next = GetNextChar();
                }
                UngetChar();
                token.type = isFloat ? FLOAT : SYMBOL;
            } else if (isspace(next) || next == '(' || next == ')' || next == '\0' || next == '\n' || next == '"') {
                UngetChar();
                token.type = DOT;
            } else {
                token.value += next;
                next = GetNextChar();
                while (!isspace(next) && next != '(' && next != ')' && next != '\0' && next != '\n') {
                    token.value += next;
                    next = GetNextChar();
                }
                UngetChar();
                token.type = SYMBOL;
            } 
        } 
        else if (c == '"') {
            token.type = STRING;
            token.value = "\"";
            bool escape = false;
            while (true) {
                char ch = GetNextChar();
                if (ch == '\0') {
                    line = 1;
                    throw LexError("ERROR (no closing quote) : END-OF-FILE encountered at Line " + 
                                  to_string(last_line) + " Column " + to_string(last_column), 
                                  last_line, last_column, "\"");
                }
                if (ch == '\n') {
                    line = 1;
                    throw LexError("ERROR (no closing quote) : END-OF-LINE encountered at Line " + 
                                  to_string(last_line) + " Column " + to_string(last_column),
                                  last_line, last_column, "\"");
                }
                if (escape) {
                    if (ch == 'n') token.value += '\n';
                    else if (ch == 't') token.value += '\t';
                    else if (ch == '\\') token.value += '\\';
                    else if (ch == '"') token.value += '\"';
                    else {
                        token.value += '\\';
                        token.value += ch;
                    }
                    escape = false;
                } 
                else if (ch == '\\') {
                    escape = true;
                } 
                else if (ch == '"') {
                    token.value += "\"";
                    break;
                } 
                else {
                    token.value += ch;
                }
            }
        } 
        else if (isdigit(c) || c == '+' || c == '-') {
            token.value = c;
            bool isFloat = false, isSymbol = false;
            bool hasDigit = isdigit(c); // 如果首字元就是數字
        
            char ch = GetNextChar();
        
            if (isspace(ch) || ch == '\0' || ch == '(' || ch == ')' || ch == '\n') {
                UngetChar();
                token.type = (hasDigit ? INT : SYMBOL);
                return token;
            }
        
            while (!isspace(ch) && ch != '(' && ch != ')' && ch != '\'' && ch != '"' && ch != '\0' && ch != '\n') {
                if (isdigit(ch)) {
                    hasDigit = true;
                    token.value += ch;
                }
                else if (ch == '.') {
                    if (isFloat) isSymbol = true; // 第二次出現 .
                    else isFloat = true;
                    token.value += ch;
                }
                else {
                    isSymbol = true;
                    token.value += ch;
                }
                ch = GetNextChar();
            }
            UngetChar();
        
            if (!hasDigit) token.type = SYMBOL;
            else token.type = (isSymbol ? SYMBOL : (isFloat ? FLOAT : INT));
        }
        else {
            token.value = c;
            char ch = GetNextChar();
            while (!isspace(ch) && ch != '(' && ch != ')' && ch != '\'' && ch != '"' && ch != '\0' && ch != ';') {
                token.value += ch;
                ch = GetNextChar();
            }
            UngetChar();
            if (token.value == "nil" || token.value == "#f") {
                token.type = NIL;
            } else if (token.value == "t" || token.value == "#t") {
                token.type = T;
            } else if (token.value == "let") {          // 新增 LET
                token.type = LET;
            } else if (token.value == "lambda") {      // 新增 LAMBDA
                token.type = LAMBDA;
            } else if (token.value == "verbose") {     // 新增 VERBOSE
                token.type = VERBOSE;
            } else if (token.value == "verbose?") {    // 新增 VERBOSE_QUERY
                token.type = VERBOSE_QUERY;             // 需在 enum 添加此類型
            } else {
                token.type = SYMBOL;
            }
        }

        return token;
    } 
    catch (LexError& err) {
        throw;
    }
}

void LexicalAnalyer::reset() {
    char ch;
    int current_line = line; // 記錄目前行數
    bool space = false; // 記錄是否有空白字元
    // 只處理當前行中的註解與空白
    while (true) {
        ch = cin.peek();

        if (ch == EOF || ch == '\n') {
            cin.get();
            // cout << "cin.peek()是endl" << endl;
            line = 1;
            column = 1; // 重設行列
            break;
        }
        if (line != current_line) break; // 防止跨行誤刪

        if (ch == ';') {
            // 吃掉整行註解直到 \n 或 EOF
            while (cin.peek() != '\n' && cin.peek() != EOF) {
                cin.get();
            }
            cin.get();  // 吃掉換行符號
            line = 1;
            column = 1; // 重設行列
            break;
        } 
        else if (isspace(ch)) {
            if (!space) {
                column = 2;
            } else {
                column++;
            }
            space = true; // 記錄有空白字元
            cin.get(); // 吃掉空白
        } 
        else {
            // cout << "重設行列" << endl;
            // 重設行列
            if (!space)  {
                // 如果沒有空白字元，則行數歸1，列數歸1
                // cout << "沒有空白字元" << endl;
                column = 1;
            } else {
                // cout << "有空白字元" << endl;
            }
            line = 1;
            break; // 不是註解也不是空白，結束
        }
    }
}

void LexicalAnalyer::eat() {
    char ch;
    do {
        ch = cin.get();
    } while (ch != '\n' && ch != EOF);
    UngetChar();
    line = 0;
}

char LexicalAnalyer::GetNextChar() {
    last_char = cin.get(); // 目前的字元
        last_line = line;      // 目前讀到的行
        last_column = column;  // 目前讀到的欄位
        // cout << "目前的行: "<<last_line << "目前的字元: "<< last_char<<endl;
        if (last_char == EOF) {
            eof_flag = true;
            return EOF;
        }

    if (last_char == '\n') {
        line++;
        // cout << "讀到換行符號" << endl;
        column = 1;
    } else {
        column++;
        // cout << "下個字元column++"<<column << endl;
    }

    return last_char;
}

void LexicalAnalyer::UngetChar() {
    if (last_char != '\n') { 
        cin.unget();
        column--;
    } else {
        cin.unget();
        line--;
        column = last_column;
    }
}

// ========================
// 語法分析器：負責組合 token 成 S-expression 樹
// ========================
class Parser {
public:
	// 建構 S-expression 語法樹
	Node* Parse();

	// 印出 S-expression 結果，格式化顯示
	void PrettyPrint(Node* node, int indent = 0);

	// 判斷是否為精準的 (exit) 指令
	bool IsExactExit(Node* node);

	// 重置分析器狀態
	void ResetLexer();
	
    string PrettyPrintToString(Node* node, int indent = 0) {
        if (!node) return "nil";
    
        ostringstream out;
    
        // 原子處理
        if (node->isAtom) {
            if (node->atom.type == FLOAT) {
                try {
                    // 轉成 double 判斷是否合法
                    stod(node->atom.value);
                    out << fixed << setprecision(3) << stof(node->atom.value);
                } catch (...) {
                    out << node->atom.value;
                }
            } else if (node->atom.type == T) {
                out << "#t";
            } else if (node->atom.type == NIL) {
                out << "nil";
            } else if (node->atom.type == STRING) {
                out << node->atom.value;
            } else if (node->atom.type == INT && node->atom.value[0] == '+') {
                out << node->atom.value.substr(1);
            } else {
                out << node->atom.value;
            }
            return out.str();
        }
    
        // 處理 (LIST 或 DOTTED PAIR)
        out << "( ";
        Node* curr = node;
        bool first = true;
    
        while (curr) {
            if (!first) out << string(indent + 2, ' ');
            out << PrettyPrintToString(curr->left, indent + 2) << "\n";
    
            if (curr->right && curr->right->isAtom && curr->right->atom.type != NIL) {
                out << string(indent + 2, ' ') << ".\n";
                out << string(indent + 2, ' ') << PrettyPrintToString(curr->right, indent + 2) << "\n";
                break;
            }
            curr = curr->right;
            if (!curr) break;
            first = false;
        }
    
        out << string(indent, ' ') << ")";
        return out.str();
    }     
    
private:
	// 取得下一個 token
	Token GetToken();

	// 將 token 還原給下一次讀取
	void UngetToken(Token token);

	// 建構列表型 S-expression
	Node* ParseList();

	// 建立一個原子節點
	Node* CreateAtomNode(string value, TokenType type);

	LexicalAnalyer lexer;
	Token lookahead;
	bool hasLookahead = false;
	bool in_unfinished_sexp = false;
};

Token Parser::GetToken() {
    if (hasLookahead) {
        hasLookahead = false;
        return lookahead;
    }

    Token token = lexer.GetNextToken();

    if (token.type == LEFT_PAREN) {
        lexer.SetUnfinishedSexp(true);
    } 

    else if (token.type == RIGHT_PAREN) {
        lexer.SetUnfinishedSexp(false);
    }

    return token;
}

void Parser::UngetToken(Token token) {
    lookahead = token;
    hasLookahead = true;
}

void Parser::ResetLexer() {
    if (!hasLookahead) {
        lexer.reset();
    }
}

Node* Parser::Parse() {
    try {
        Token token = GetToken();
        if (token.type == LEFT_PAREN) return ParseList();
        if (token.type == QUOTE) {
            Node* quote = new Node();
            quote->left = CreateAtomNode("quote", SYMBOL);
            quote->right = new Node();
            quote->right->left = Parse();
            quote->right->right = nullptr;
            return quote;
        }
        if (token.type == DOT || token.type == RIGHT_PAREN) {
            // lexer.eat();
            throw LexError("ERROR (unexpected token) : atom or '(' expected when token at Line " + 
                          to_string(token.line) + " Column " + to_string(token.column) + 
                          " is >>" + token.value + "<<", 
                          token.line, token.column, token.value);
        }
        if (token.type == END_OF_FILE) {
            throw LexError("ERROR (no more input) : END-OF-FILE encountered", 
                          token.line, token.column, "EOF");
        }

        Node* node = new Node();
        node->isAtom = true;
        node->atom = token;
        return node;
    } 
    catch (LexError& err) {
      
        string errmsg = err.what();
    
        // 情況3：Unexpected Token 錯誤
        if (errmsg.find("unexpected token") != string::npos) {
            lexer.eat();
        }
        // no closing quote 錯誤不處理
        else if (errmsg.find("no closing quote") != string::npos) {
            // 保留原有行列資訊
        }
        
        throw;
    }
}

void Parser::PrettyPrint(Node* node, int indent) {

    if (!node) {
        cout << "nil" << endl;
        return;
    }

    if (node->isAtom) {
        if (node->atom.type == FLOAT) {
            try {
                cout << fixed << setprecision(3) << stof(node->atom.value) << endl; 
            }
            catch (...) { 
                cout << node->atom.value << endl; 
            }
        } 
        else if (node->atom.type == T) cout << "#t" << endl;
        else if (node->atom.type == NIL) cout << "nil" << endl;
        else if (node->atom.type == STRING) cout << node->atom.value << endl;
        else if (node->atom.type == INT && node->atom.value[0] == '+') {
            cout << node->atom.value.substr(1) << endl;
        }
        else cout << node->atom.value << endl;
        return;
    }
    // cout << "indent: "<<indent<<endl;
    for (int i = max(indent - 1,2); i < indent - 1; ++i) cout << " ";
    cout << "( ";


    Node* curr = node;
    bool first = true;
    int new_indent = indent + 2;

    while (curr) {
        if (!first) {
            for (int i = 0; i < new_indent; ++i) cout << " ";
        }

        PrettyPrint(curr->left, new_indent);

        if (curr->right && curr->right->isAtom) {
            if (curr->right->atom.type == NIL) {
                break;
            }

            for (int i = 0; i < new_indent; ++i) cout << " ";
            cout << "." << endl;

            for (int i = 0; i < new_indent; ++i) cout << " ";
            PrettyPrint(curr->right, new_indent);
            break;
        }
        else if (curr->right != nullptr &&
                !curr->right->isAtom &&
                curr->right->left == nullptr &&
                curr->right->right == nullptr) {
            for (int i = 0; i < indent + 2; ++i) cout << " ";
            PrettyPrint(curr->right, new_indent);
            break;
        }

        curr = curr->right;
        first = false;
    }

    for (int i = 0; i < indent; ++i) cout << " ";
    cout << ")" << endl;
}

bool Parser::IsExactExit(Node* node) {
    // 這棵樹的樣子應該是：
    // (exit) → node is list, left is "exit", right is nullptr or NIL
    if (!node || node->isAtom) return false;

    // 左側是 atom "exit"
    if (!node->left || !node->left->isAtom) return false;
    if (node->left->atom.value != "exit") return false;

    // 右側是 NIL（也就是沒有多餘參數）
    if (!node->right) return true;

    if (node->right->isAtom && 
        (node->right->atom.type == NIL || node->right->atom.value == "nil"))
        return true;

    // 有其它東西 → 非精確 (exit)
    return false;
}

Node* Parser::ParseList() {
    Token token = GetToken();
    if (token.type == RIGHT_PAREN) {
        in_unfinished_sexp = false;
        Node* nilNode = new Node();
        nilNode->isAtom = true;
        nilNode->atom.type = NIL;
        nilNode->atom.value = "nil";
        return nilNode;
    }

    if (token.type == DOT) {
        // lexer.eat();
        throw LexError("ERROR (unexpected token) : atom or '(' expected when token at Line " + 
                      to_string(token.line) + " Column " + to_string(token.column) + 
                      " is >>.<<", 
                      token.line, token.column, ".");
    }
    UngetToken(token);

    Node* node = new Node();
    node->left = Parse();

    Token next = GetToken();

    if (next.type == DOT) {
        node->right = Parse();
        Token rp = GetToken();
        if (rp.type != RIGHT_PAREN) {
            lexer.eat();
            throw LexError("ERROR (unexpected token) : ')' expected when token at Line " + 
                          to_string(rp.line) + " Column " + to_string(rp.column) + 
                          " is >>" + rp.value + "<<", 
                          rp.line, rp.column, rp.value);
        }
    } 
    else if (next.type == RIGHT_PAREN) {
        node->right = nullptr;
    } 
    else {
        UngetToken(next);
        node->right = ParseList();
    }

    return node;
}

Node* Parser::CreateAtomNode(string value, TokenType type) {
    Node* node = new Node();
    node->isAtom = true;
    node->atom.type = type;
    node->atom.value = value;
    return node;
}

/****
 * 負責將語法樹轉為實際語義值
 */
class EvalError : public runtime_error {
    public:
        EvalError(const string& msg) : runtime_error(msg) {}
};

Environment::Environment(const Environment& other) {
    verboseMode = other.verboseMode;
    for (auto& pair : other.table) {
        table[pair.first] = new Node(*pair.second);
    }
    parent = other.parent;
}

void Environment::define(const string& symbol, Node* value) {
    if (isPrimitive(symbol)) {
        throw runtime_error("Cannot redefine primitive: " + symbol);
    }
    table[symbol] = value;
}

Node* Environment::lookup(const string& symbol) {
    auto it = table.find(symbol);
    if (it != table.end()) return it->second;
    if (parent) return parent->lookup(symbol);
    if (isPrimitive(symbol)) {
        Node* node = new Node();
        node->isAtom = true;
        node->atom.type = SYMBOL;
        node->atom.value = "#<procedure " + symbol + ">";
        return node;
    }
    throw runtime_error("ERROR (unbound symbol): " + symbol);
}

void Environment::clear() {
    table.clear(); // 清空所有定義的符號
}

// 檢查某個符號是否為內建 primitive（不可重新定義）
bool Environment::isPrimitive(const string& symbol) {
    static vector<string> primitives = {
        "cons", "list", "car", "cdr", "quote", "define", "if", "cond", "begin",
        "+", "-", "*", "/", ">", "<", "=", ">=", "<=",
        "atom?", "pair?", "list?", "null?", "integer?", "real?", "number?",
        "string?", "boolean?", "symbol?",
        "eqv?", "equal?", "string=?", "string<?", "string>?",
        "string-append",
        "and", "or", "not", "clean-environment",
        "exit", "let", "lambda", "verbose", "verbose?"
    };
    return find(primitives.begin(), primitives.end(), symbol) != primitives.end();
}

class Evaluator {
	Environment& env;
    int currentDepth = 0;  // 深度
public:
	Evaluator(Environment& e) : env(e) {}
	// 評估一個語法節點，並回傳結果節點
	Node* Eval(Node* expr);

private:
    // 處理 quote 特殊形式
    Node* handleQuote(Node* expr); 
    // 處理 define 特殊形式
    Node* handleDefine(Node* expr);
    // 建構 pair 或 list
    Node* handleCons(Node* expr);  
    // 建立 list
    Node* handleList(Node* expr);  
    // 取出左邊 (car)
    Node* handleCar(Node* expr);
    // 取出右邊 (cdr)
    Node* handleCdr(Node* expr);
    // 條件選擇 (if)
    Node* handleIf(Node* expr);
    // 多條件選擇 (cond)
    Node* handleCond(Node* expr);
    // 執行多個表達式並回傳最後一個結果 (begin)
    Node* handleBegin(Node* expr);
    // 各種 predicate
    Node* handlePredicate(Node* expr, const string& type);
    // 數學運算 (+, -, *, /)
    Node* handleArithmetic(Node* expr, const string& op);
    // 比較運算 (>, <, =, >=, <=)
    Node* handleComparison(Node* expr, const string& op);
    // 字串操作 string-append、string>? 等
    Node* handleStringCompare(Node* expr, const string& op);
    // 邏輯操作 not, and, or
    Node* handleLogic(Node* expr, const string& op);
    // 遞迴比對整個結構與值
    bool equalRecursive(Node* a, Node* b);
    // 判斷物件是否等價 (eqv? equal?)
    Node* handleEquality(Node* expr, const string& op);
    // 建立原子節點
    Node* makeAtom(string val, TokenType type);
    // 轉為錯誤顯示用字串
    string PrintRaw(Node* node);

    Node* handleLet(Node* expr);
    Node* handleLambda(Node* expr);
    Node* handleVerbose(Node* expr);
    Node* handleVerboseQuery(Node* expr);
    
    // 輔助函數
    Node* applyLambda(Node* lambda, Node* args);
    void checkReturnValue(Node* result, Node* expr);
};

Node* Evaluator::Eval(Node* expr) {
    // 記錄當前深度，呼叫時增加深度，結束時減少
    struct DepthTmp {
        Evaluator& evaluator;
        DepthTmp(Evaluator& e) : evaluator(e) { 
            evaluator.currentDepth++; 
        }
        ~DepthTmp() { 
            evaluator.currentDepth--; 
        }
    } guard(*this);

	if (!expr) return nullptr;

	// 如果是原子，處理 symbol lookup
	if (expr->isAtom) {
		if (expr->atom.type == SYMBOL) {
			try {
				return env.lookup(expr->atom.value);
			} catch (EvalError& e) {
				throw;
			}
		}
		return expr;
	}

	// 處理最外層是 clean-environment 的特殊情況
	if (expr->left && expr->left->isAtom && expr->left->atom.value == "clean-environment") {
		int argCount = 0;
		Node* tmp = expr->right;
		while (tmp) {
			argCount++;
			tmp = tmp->right;
		}
		if (argCount == 0) {
			env.clear();
			return makeAtom("environment cleaned", STRING);
		} else {
			throw EvalError("ERROR (level of CLEAN-ENVIRONMENT)");
		}
	}

	// 解析opSymbol
	string opSymbol = "";
	try {
		Node* resolved = Eval(expr->left);
		if (resolved && resolved->isAtom &&
			resolved->atom.type == SYMBOL &&
			resolved->atom.value.find("#<procedure ") == 0) {
			size_t start = resolved->atom.value.find(" ") + 1;
			size_t end = resolved->atom.value.find(">");
			opSymbol = resolved->atom.value.substr(start, end - start);
		}
	} catch (...) {
		// 忽略例外
	}

	// 檢查參數數量錯誤
	if (opSymbol == "cons") {
		int argCount = 0;
		Node* curr = expr->right;
		while (curr) {
			argCount++;
			curr = curr->right;
		}
		if (argCount != 2) {
			throw EvalError("ERROR (incorrect number of arguments) : cons");
		}
	}

    // 不是quote
    // 數量正確檢查有沒有define、clean-environment不在第一層
    if (opSymbol != "quote") {
        Node* curr = expr->right;
        while (curr) {
            if (curr->left && !curr->left->isAtom) {
                Node* sub = curr->left;
                if (sub->left && sub->left->isAtom) {
                    string innerOp = sub->left->atom.value;
                    if (innerOp == "define") {
                        throw EvalError("ERROR (level of DEFINE)");
                    }
                    if (innerOp == "clean-environment") {
                        throw EvalError("ERROR (level of CLEAN-ENVIRONMENT)");
                    }
                }
            }
            curr = curr->right;
        }
    }

    // 先檢查是否非function
    if (expr->right && expr->right->isAtom && expr->right->atom.type != NIL) {
        // 判斷這是一個improper list 或 dotted pair
        Parser printer;
        string printed = printer.PrettyPrintToString(expr, 0);
        throw EvalError("ERROR (non-list) : " + printed);
    }

	// 評估運算子
	Node* func = Eval(expr->left);
	if (!func || !func->isAtom || func->atom.type != SYMBOL ||
		func->atom.value.find("#<procedure") == string::npos) {
		throw EvalError("ERROR (attempt to apply non-function) : " +
			(func ? PrintRaw(func) : "invalid operator"));
	}

	// 提取運算子名稱
	string op = func->atom.value;
	if (op.rfind("#<procedure ", 0) == 0 && op.back() == '>') {
		op = op.substr(12, op.length() - 13);
	}

	// 分派特殊形式
	if (op == "quote") return handleQuote(expr);
	if (op == "define") return handleDefine(expr);
	if (op == "cons") return handleCons(expr);
	if (op == "list") return handleList(expr);
	if (op == "car") return handleCar(expr);
	if (op == "cdr") return handleCdr(expr);
	if (op == "if") return handleIf(expr);
	if (op == "cond") return handleCond(expr);
	if (op == "begin") return handleBegin(expr);
	if (op == "not" || op == "and" || op == "or") return handleLogic(expr, op);
	if (op == "eqv?" || op == "equal?") return handleEquality(expr, op);
	if (op == "+" || op == "-" || op == "*" || op == "/") return handleArithmetic(expr, op);
	if (op == ">" || op == "<" || op == ">=" || op == "<=" || op == "=") return handleComparison(expr, op);
	if (op == "string-append" || op == "string>?" || op == "string<?" || op == "string=?") return handleStringCompare(expr, op);
	if (op.find("?") != string::npos) return handlePredicate(expr, op);
	if (op == "clean-environment") {
		env.clear();
		return makeAtom("environment cleaned", STRING);
	}
    if (op == "exit") {
        // 檢查是否在第一層
        if (currentDepth != 1) {  
            throw EvalError("ERROR (level of EXIT)");
        }
        // 檢查參數數量
        if (expr->right) {
            throw EvalError("ERROR (incorrect number of arguments) : exit");
        } else {
            cout << "Thanks for using OurScheme!" << endl;
            exit(0);
        }
    }
    if (op == "let") return handleLet(expr);
    if (op == "lambda") return handleLambda(expr);
    if (op == "verbose") return handleVerbose(expr);
    if (op == "verbose?") return handleVerboseQuery(expr);
	throw EvalError("ERROR (attempt to apply non-function) : " + op);
}

Node* Evaluator::handleCons(Node* expr) {
    if (!expr->right || !expr->right->right || expr->right->right->right) {
        throw EvalError("ERROR (incorrect number of arguments) : cons");
    }
    Node* left = Eval(expr->right->left);
    Node* right = Eval(expr->right->right->left);
    Node* pair = new Node();
    pair->left = left;
    pair->right = right;
    return pair;
}

Node* Evaluator::handleList(Node* expr) {
    Node* args = expr->right;
    if (!args) return makeAtom("nil", NIL);

    // 檢查是否為 dotted pair
	Node* temp = expr->right;
	while (temp && !temp->isAtom) {
		if (temp->right && temp->right->isAtom && temp->right->atom.type != NIL) {
			Parser printer;
			string printed = printer.PrettyPrintToString(expr, 0);
			throw EvalError("ERROR (non-list) : " + printed);
		}
		temp = temp->right;
	}

    Node* head = new Node();
    Node* curr = head;
    while (args) {
        curr->left = Eval(args->left);
        if (!args->right) break;
        curr->right = new Node();
        curr = curr->right;
        args = args->right;
    }
    return head;
}

Node* Evaluator::handleCar(Node* expr) {
    if (!expr->right || expr->right->right) {
        throw EvalError("ERROR (incorrect number of arguments) : car");
    }
    Node* pair = Eval(expr->right->left);
    if (!pair || pair->isAtom || !pair->left) {
        throw EvalError("ERROR (car with incorrect argument type) : " + PrintRaw(pair));
    }
    return pair->left;
}

Node* Evaluator::handleCdr(Node* expr) {
    if (!expr->right || expr->right->right) {
        throw EvalError("ERROR (incorrect number of arguments) : cdr");
    }
    Node* pair = Eval(expr->right->left);
    if (!pair || pair->isAtom) {
        throw EvalError("ERROR (cdr with incorrect argument type) : " + PrintRaw(pair));
    }
    return pair->right ? pair->right : makeAtom("nil", NIL);
}

Node* Evaluator::handleIf(Node* expr) {
    // 檢查參數數量
    int argCount = 0;
    Node* tmp = expr->right;
    while (tmp) {
        argCount++;
        tmp = tmp->right;
    }

    // 必須有 2 或 3 個參數
    if (argCount < 2 || argCount > 3) {
        throw EvalError("ERROR (incorrect number of arguments) : if");
    }

    // 提取條件和分支
    Node* conditionPart = expr->right->left;
    Node* thenPart = expr->right->right->left;
    Node* elsePart = (argCount == 3) ? expr->right->right->right->left : nullptr;

    // 評估測試條件 (嚴格檢查返回值)
    Node* testResult = Eval(conditionPart);
    
    // 新增：檢查條件是否未返回值
    if (!testResult) {
        throw EvalError("ERROR (unbound test-condition) : " + PrintRaw(conditionPart));
    }

    // 新增：檢查條件返回值類型是否合法
    if (!testResult->isAtom || 
       (testResult->atom.type != NIL && testResult->atom.type != T)) {
        throw EvalError("ERROR (non-boolean test) : " + PrintRaw(conditionPart));
    }

    // 根據條件結果執行分支
    if (testResult->atom.type != NIL) { // 條件為真
        Node* result = Eval(thenPart);
        if (!result) {
            throw EvalError("ERROR (no return value) : " + PrintRaw(thenPart));
        }
        return result;
    } 
    else { // 條件為假
        if (elsePart) {
            Node* result = Eval(elsePart);
            if (!result) {
                throw EvalError("ERROR (no return value) : " + PrintRaw(elsePart));
            }
            return result;
        }
        // 新增：沒有 else 分支時拋出錯誤
        throw EvalError("ERROR (no return value) : " + PrintRaw(expr));
    }
}

Node* Evaluator::handleCond(Node* expr) {
    Node* clauses = expr->right;
    
    // 檢查是否有至少一個子句
    if (!clauses) {
        string printed = PrintRaw(expr);
        throw EvalError("ERROR (COND format) : " + printed);
    }

    // 檢查所有子句是否都是合法列表
    Node* curr = clauses;
    while (curr) {
        // 子句必須是列表
        if (!curr->left || curr->left->isAtom) {
            string printed = PrintRaw(expr);
            throw EvalError("ERROR (COND format) : " + printed);
        }
        
        Node* clause = curr->left;
        
        // 每個子句必須有條件和至少一個結果除非是最後一個 else 子句
        bool is_legal_else = (clause->left && clause->left->isAtom && 
                             clause->left->atom.value == "else" && 
                             curr->right == nullptr);
        
        if (!is_legal_else && (!clause->left || !clause->right)) {
            string printed = PrintRaw(expr);
            throw EvalError("ERROR (COND format) : " + printed);
        }
        
        curr = curr->right;
    }

    while (clauses) {
        Node* clause = clauses->left;
        Node* condition = clause->left;
        Node* actions = clause->right;
        
        // 處理合法的else子句
        if (condition->isAtom && condition->atom.type == SYMBOL && 
            condition->atom.value == "else" && clauses->right == nullptr) {
            
            if (!actions) {
                return makeAtom("#t", T);
            }
            
            Node* result = nullptr;
            while (actions) {
                result = Eval(actions->left);
                actions = actions->right;
            }
            return result;
        }
        
        Node* testResult = Eval(condition);
        if (!testResult->isAtom || testResult->atom.type != NIL) {
            if (!actions) {
                string printed = PrintRaw(expr);
                throw EvalError("ERROR (COND format) : " + printed);
            }
            
            Node* result = nullptr;
            while (actions) {
                result = Eval(actions->left);
                actions = actions->right;
            }
            return result;
        }
        
        clauses = clauses->right;
    }
    
    string printed = PrintRaw(expr);
    throw EvalError("ERROR (no return value) : " + printed);
}

Node* Evaluator::handleBegin(Node* expr) {
    Node* curr = expr->right;
    Node* result = nullptr;
    if (!curr) throw EvalError("ERROR (incorrect number of arguments) : begin");
    while (curr) {
        result = Eval(curr->left);
        curr = curr->right;
    }
    return result;
}

Node* Evaluator::handlePredicate(Node* expr, const string& type) {
    if (!expr->right || expr->right->right) {
        throw EvalError("ERROR (incorrect number of arguments) : " + type);
    }
    Node* arg = Eval(expr->right->left);
    bool result = false;
    if (type == "atom?") result = arg->isAtom;
    else if (type == "pair?") result = !arg->isAtom;
    else if (type == "null?") result = arg->isAtom && arg->atom.type == NIL;
    else if (type == "symbol?") result = arg->isAtom && arg->atom.type == SYMBOL;
    else if (type == "boolean?") result = arg->isAtom && (arg->atom.type == T || arg->atom.type == NIL);
    else if (type == "string?") result = arg->isAtom && arg->atom.type == STRING;
    else if (type == "number?") result = arg->isAtom && (arg->atom.type == INT || arg->atom.type == FLOAT);
    else if (type == "integer?") result = arg->isAtom && arg->atom.type == INT;
    else if (type == "real?") result = arg->isAtom && (arg->atom.type == INT || arg->atom.type == FLOAT);
    else throw EvalError("ERROR (unknown predicate) : " + type);
    return makeAtom(result ? "#t" : "nil", result ? T : NIL);
}

Node* Evaluator::handleArithmetic(Node* expr, const string& op) {
	Node* curr = expr->right;
	if (!curr || !curr->right) throw EvalError("ERROR (incorrect number of arguments) : " + op);

	bool allInt = true;
	bool first = true;
	int intResult = 0;
	double floatResult = 0;

	while (curr) {
		Node* val = Eval(curr->left);

		// 不是 INT 或 FLOAT 就拋出型別錯誤
		if (val->atom.type != INT && val->atom.type != FLOAT) {
			throw EvalError("ERROR (" + op + " with incorrect argument type) : " + PrintRaw(val));
		}

		string sval = val->atom.value;
		double num;
		try {
			num = stod(sval);
		} catch (...) {
			throw EvalError("ERROR (" + op + " with incorrect argument type) : " + sval);
		}

		if (val->atom.type == FLOAT) allInt = false;

		if (first) {
			floatResult = num;
			intResult = (int)num;
			first = false;
		} else {
			if (op == "+") {
				floatResult += num;
				intResult += (int)num;
			} else if (op == "-") {
				floatResult -= num;
				intResult -= (int)num;
			} else if (op == "*") {
				floatResult *= num;
				intResult *= (int)num;
			} else if (op == "/") {
				if (num == 0) throw EvalError("ERROR (division by zero) : /");
				floatResult /= num;
				intResult /= (int)num;
			}
		}
		curr = curr->right;
	}

	// 根據型態回傳結果
	if (allInt) return makeAtom(to_string(intResult), INT);

	ostringstream oss;
	oss << fixed << setprecision(3) << floatResult;
	return makeAtom(oss.str(), FLOAT);
}

Node* Evaluator::handleComparison(Node* expr, const string& op) {
	Node* curr = expr->right;

	// 檢查參數數量
	int count = 0;
	Node* tmp = curr;
	while (tmp) {
		count++;
		tmp = tmp->right;
	}
	if (count < 2)
		throw EvalError("ERROR (incorrect number of arguments) : " + op);

	// 存所有參數的數值（INT或FLOAT）
	vector<double> values;
	while (curr) {
		Node* val = Eval(curr->left);
		if (!val || !val->isAtom || val->atom.type == NIL ||
			(val->atom.type != INT && val->atom.type != FLOAT)) {
			throw EvalError("ERROR (" + op + " with incorrect argument type) : " + PrintRaw(val));
		}
		try {
			values.push_back(stod(val->atom.value));
		} catch (...) {
			throw EvalError("ERROR (" + op + " with incorrect argument type) : " + PrintRaw(val));
		}
		curr = curr->right;
	}

	// 比較
	for (size_t i = 1; i < values.size(); ++i) {
		double a = values[i - 1], b = values[i];
		if ((op == ">" && !(a > b)) ||
			(op == "<" && !(a < b)) ||
			(op == ">=" && !(a >= b)) ||
			(op == "<=" && !(a <= b)) ||
			(op == "=" && !(a == b))) {
			return makeAtom("nil", NIL);
		}
	}

	return makeAtom("#t", T);
}

Node* Evaluator::handleStringCompare(Node* expr, const string& op) {
	Node* curr = expr->right;

	// 檢查參數數量
	int count = 0;
	Node* tmp = curr;
	while (tmp) {
		count++;
		tmp = tmp->right;
	}
	if (op == "string-append") {
		if (count < 2) throw EvalError("ERROR (incorrect number of arguments) : " + op);

		ostringstream oss;
		while (curr) {
			Node* val = Eval(curr->left);
			if (!val->isAtom || val->atom.type != STRING)
				throw EvalError("ERROR (" + op + " with incorrect argument type) : " + PrintRaw(val));

			string s = val->atom.value;
			if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
				oss << s.substr(1, s.size() - 2);
			else
				oss << s;
			curr = curr->right;
		}
		return makeAtom("\"" + oss.str() + "\"", STRING);
	}

	if (count < 2)
		throw EvalError("ERROR (incorrect number of arguments) : " + op);

    // 是STRING存進vector
	vector<string> stringValues;
	while (curr) {
		Node* val = Eval(curr->left);
		if (!val || !val->isAtom || val->atom.type != STRING )
			throw EvalError("ERROR (" + op + " with incorrect argument type) : " + PrintRaw(val));
		stringValues.push_back(val->atom.value);
		curr = curr->right;
	}

	// 執行比較
	for (size_t i = 1; i < stringValues.size(); i++) {
		string prev = stringValues[i - 1];
		string now = stringValues[i];
		if ((op == "string<?" && !(prev < now)) ||
			(op == "string>?" && !(prev > now)) ||
			(op == "string=?" && !(prev == now))) {
			return makeAtom("nil", NIL);
		}
	}

	return makeAtom("#t", T);
}

Node* Evaluator::handleLogic(Node* expr, const string& op) {
	Node* curr = expr->right;

	if (op == "not") {
		if (!curr || curr->right) {
			throw EvalError("ERROR (incorrect number of arguments) : not");
		}
		Node* val = Eval(curr->left);
		return (val->atom.type == NIL) ? makeAtom("#t", T) : makeAtom("nil", NIL);
	}

	if (!curr) throw EvalError("ERROR (no return value) : " + op);
	
	Node* last = nullptr;
	while (curr) {
		Node* val = Eval(curr->left);
		if (op == "and" && val->atom.type == NIL) {
			return val;
		}
		if (op == "or" && val->atom.type != NIL) {
			return val;
		}
		last = val;
		curr = curr->right;
	}
	return (op == "and") ? last : makeAtom("nil", NIL);
}

bool Evaluator::equalRecursive(Node* a, Node* b) {
	if (a == nullptr && b != nullptr && b->isAtom && b->atom.type == NIL) return true;
    if (b == nullptr && a != nullptr && a->isAtom && a->atom.type == NIL) return true;
    if (a == nullptr && b == nullptr) return true;
    if (a == nullptr || b == nullptr) return false;

	if (a->isAtom != b->isAtom) return false;
	if (a->isAtom) {
		return a->atom.value == b->atom.value && a->atom.type == b->atom.type;
	}
	return equalRecursive(a->left, b->left) && equalRecursive(a->right, b->right);
}

Node* Evaluator::handleEquality(Node* expr, const string& op) {
	if (!expr->right || !expr->right->right || expr->right->right->right) {
		throw EvalError("ERROR (incorrect number of arguments) : " + op);
	}
	Node* a = Eval(expr->right->left);
	Node* b = Eval(expr->right->right->left);

	bool result = false;
    if (op == "eqv?") {
        result = (a == b) || 
                 (a && b &&
                  a->isAtom && b->isAtom &&
                  a->atom.type != STRING &&
                  a->atom.type == b->atom.type &&
                  a->atom.value == b->atom.value);    
	} else if (op == "equal?") {
		result = equalRecursive(a, b); // 結構相同、值相同
	}

	return result ? makeAtom("#t", T) : makeAtom("nil", NIL);
}

Node* Evaluator::handleQuote(Node* expr) {
    if (!expr->right || expr->right->right) {
        throw EvalError("ERROR (incorrect number of arguments) : quote");
    }
    return expr->right->left;
}

Node* Evaluator::handleDefine(Node* expr) {
    Parser printer;
    Node* args = expr->right;

    // 檢查基本參數數量
    if (!args || !args->right) {
        throw EvalError("ERROR (DEFINE format) : " + printer.PrettyPrintToString(expr, 0));
    }

    // 處理函數定義形式：(define (name params...) body...)
    if (!args->left->isAtom && args->left->left && args->left->left->isAtom) {
        Node* nameNode = args->left->left;
        string funcName = nameNode->atom.value;
        
        // 構造lambda表達式
        Node* lambdaNode = new Node();
        lambdaNode->left = new Node();
        lambdaNode->left->isAtom = true;
        lambdaNode->left->atom.type = SYMBOL;
        lambdaNode->left->atom.value = "lambda";
        
        lambdaNode->right = new Node();
        lambdaNode->right->left = args->left->right;
        lambdaNode->right->right = args->right;

        Node* lambdaVal = Eval(lambdaNode);
        lambdaVal->env = new Environment(env);  // 捕獲定義時的環境
        
        env.define(funcName, lambdaVal);
        
        if (env.isVerbose()) {
            return new Node{true, {STRING, funcName + " defined"}, nullptr, nullptr};
        }
        return new Node{true, {NIL, "nil"}, nullptr, nullptr};
    }
    
    // 處理變量定義形式 (define symbol value)
    else {
        // 參數數量驗證
        int count = 0;
        Node* tmp = args;
        while (tmp) {
            count++;
            tmp = tmp->right;
        }
        if (count != 2) {
            throw EvalError("ERROR (DEFINE format) : " + printer.PrettyPrintToString(expr, 0));
        }

        Node* sym = args->left;
        Node* val = args->right->left;
        if (!sym->isAtom || sym->atom.type != SYMBOL) {
            throw EvalError("ERROR (DEFINE format)");
        }
        Node* evaluatedVal = Eval(val);
        checkReturnValue(evaluatedVal, val);
        env.define(sym->atom.value, evaluatedVal);
        
        if (env.isVerbose()) {
            return new Node{true, {STRING, sym->atom.value + " defined"}, nullptr, nullptr};
        }
        return new Node{true, {NIL, "nil"}, nullptr, nullptr};
    }
}

Node* Evaluator::makeAtom(string val, TokenType type) {
    Node* node = new Node();
    node->isAtom = true;
    node->atom.type = type;
    node->atom.value = val;
    return node;
}

string Evaluator::PrintRaw(Node* node) {
    if (!node) return "nil";
    if (node->isAtom) {
        if (node->atom.type == FLOAT) {
			ostringstream oss;
			oss << fixed << setprecision(3) << stod(node->atom.value);
			return oss.str();
		}
        return node->atom.value;
    }
    Parser printer;
	string printed = printer.PrettyPrintToString(node, 0);
	return printed;
}

Node* Evaluator::handleLet(Node* expr) {
    if (!expr->right || !expr->right->left || !expr->right->right) {
        throw EvalError("ERROR (let format)");
    }

    Node* bindings = expr->right->left;
    Node* body = expr->right->right;

    // 創建新環境
    Environment* localEnv = new Environment(&env);

    // 綁定局部變量
    Node* curr = bindings;
    while (curr) {
        Node* binding = curr->left;
        if (!binding || !binding->left || !binding->left->isAtom || 
            binding->left->atom.type != SYMBOL || !binding->right) {
            throw EvalError("ERROR (let format)");
        }
        string symbol = binding->left->atom.value;
        Node* value = Eval(binding->right);
        checkReturnValue(value, binding->right);
        localEnv->define(symbol, value);
        curr = curr->right;
    }

    // 在新環境求值
    Evaluator localEval(*localEnv);
    Node* result = nullptr;
    while (body) {
        result = localEval.Eval(body->left);
        body = body->right;
    }
    checkReturnValue(result, expr);
    return result;
}

Node* Evaluator::handleLambda(Node* expr) {
    if (!expr->right || !expr->right->left || !expr->right->right) {
        throw EvalError("ERROR (lambda format)");
    }

    Node* params = expr->right->left;
    Node* body = expr->right->right;

    // 創建閉包節點
    Node* closure = new Node();
    closure->isAtom = true;
    closure->atom.type = SYMBOL;
    closure->atom.value = "#<procedure lambda>";
    closure->left = params;
    closure->right = body;
    closure->env = new Environment(env);  // 捕獲當前環境

    return closure;
}

Node* Evaluator::applyLambda(Node* lambda, Node* args) {
    Environment* closureEnv = new Environment(lambda->env);
    Node* params = lambda->left;
    Node* body = lambda->right;

    // 綁定參數
    Node* currParam = params;
    Node* currArg = args;
    while (currParam && currArg) {
        if (!currParam->left || !currParam->left->isAtom) {
            throw EvalError("ERROR (lambda parameter format)");
        }
        string paramName = currParam->left->atom.value;
        Node* argValue = Eval(currArg->left);
        checkReturnValue(argValue, currArg->left);
        closureEnv->define(paramName, argValue);
        currParam = currParam->right;
        currArg = currArg->right;
    }

    // 求值函數體
    Evaluator closureEval(*closureEnv);
    Node* result = nullptr;
    while (body) {
        result = closureEval.Eval(body->left);
        body = body->right;
    }
    checkReturnValue(result, lambda);
    return result;
}

void Evaluator::checkReturnValue(Node* result, Node* expr) {
    if (!result) {
        string msg = "ERROR (no return value) : " + PrintRaw(expr);
        throw EvalError(msg.c_str());
    }
}

Node* Evaluator::handleVerbose(Node* expr) {
    if (!expr->right || expr->right->right) {
        throw EvalError("ERROR (incorrect number of arguments) : verbose");
    }
    Node* arg = Eval(expr->right->left);
    bool mode = !(arg->isAtom && arg->atom.type == NIL);
    env.setVerbose(mode);
    return new Node{true, {mode ? T : NIL, mode ? "#t" : "nil"}, nullptr, nullptr};
}

Node* Evaluator::handleVerboseQuery(Node* expr) {
    return new Node{true, {env.isVerbose() ? T : NIL, env.isVerbose() ? "#t" : "nil"}, 
                   nullptr, nullptr};
}

bool isPureSyntaxForm(const string& op) {
    static vector<string> forms = {
        "define", "clean-environment", "quote"
    };
    return find(forms.begin(), forms.end(), op) != forms.end();
}

/***
 * 主程式：整合 Lex → Parse → Eval → Print
 */
int main() {
	cout << "Welcome to OurScheme!" << endl << endl;
    string problem;
    getline(cin, problem);
    
	Environment env;				// 建立一個新的環境表
	Evaluator evaluator(env);		// 建立評估器並傳入環境
	Parser parser;					// Parser 建構語法樹

	while (true) {
		cout << "> ";
		try {
			Node* expr = parser.Parse(); // 呼叫 Parser 取得一棵 S-expression

			// 若輸入為 (exit)，則結束程式
			if (parser.IsExactExit(expr)) {
				cout << endl << "Thanks for using OurScheme!" << endl;
				exit(0);
			}

            Node* result = evaluator.Eval(expr);
            
            // 處理verbose輸出
            if (result->isAtom && result->atom.type == STRING &&
                result->atom.value.find("defined") != string::npos) {
                if (env.isVerbose()) {
                    cout << result->atom.value << endl;
                }
            } else {
                parser.PrettyPrint(result);
            }
            parser.ResetLexer();
			cout << endl;
		} catch (LexError& e) {
			string errmsg = e.what();
            cout << errmsg << endl;

            if (errmsg == "ERROR (no more input) : END-OF-FILE encountered") {
                cout << "Thanks for using OurScheme!";
                exit(0);
            }
      
            cout << endl;
		} catch (EvalError& e) {
			cout << e.what() << endl << endl; // 處理語意錯誤
            parser.ResetLexer();  // 避免後續輸入接錯
            cin.clear();
		}
	}

	return 0;
}