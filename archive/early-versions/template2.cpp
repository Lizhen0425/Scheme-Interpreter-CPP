#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iomanip>
#include <cctype>
#include <sstream>
using namespace std;

// ========================
// Token 與節點結構定義
// ========================

// Token 的類型枚舉
enum TokenType {
	LEFT_PAREN, RIGHT_PAREN, INT, FLOAT, STRING, DOT, NIL, T, QUOTE, SYMBOL, END_OF_FILE
};

// 表示一個詞彙單位
struct Token {
	TokenType type;
	string value;
	int line = 1, column = 1;
};

// S-expression 的節點結構
struct Node {
	bool isAtom;
	Token atom;
	Node* left;
	Node* right;
	Node() : isAtom(false), left(nullptr), right(nullptr) {}
};

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
            token.value = "";
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
                    else if (ch == '"') token.value += '"';
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
            char ch = GetNextChar();
            if (isspace(ch) || ch == '\0' || ch == '(' || ch == ')' || ch == '\n') isSymbol = true;
            while (!isspace(ch) && ch != '(' && ch != ')' && ch != '\'' && ch != '"' && ch != '\0' && ch != '\n') {
                if (ch == ';') {
                    while (ch != '\n' && ch != '\0') {
                        ch = GetNextChar();
                    }
                    break;
                }

                if (isdigit(ch)) token.value += ch;
                else if (ch == '.') {
                    if (isFloat) isSymbol = true;
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
            token.type = isSymbol ? SYMBOL : (isFloat ? FLOAT : INT);
        } 
        else {
            token.value = c;
            char ch = GetNextChar();
            while (!isspace(ch) && ch != '(' && ch != ')' && ch != '\'' && ch != '"' && ch != '\0' && ch != ';') {
                token.value += ch;
                ch = GetNextChar();
            }
            UngetChar();
            if (token.value == "nil" || token.value == "#f") token.type = NIL;
            else if (token.value == "t" || token.value == "#t") token.type = T;
            else token.type = SYMBOL;
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

void Parser::PrettyPrint(Node* node, int indent = 0) {

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
        else if (node->atom.type == STRING) cout << "\"" << node->atom.value << "\"" << endl;
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

// ========================
// 環境表：記錄變數與對應的值
// ========================
class Environment {
	map<string, Node*> table;

public:
	// 定義一個符號對應到某個值
	void define(const string& symbol, Node* value);

	// 查詢某個符號對應的值
	Node* lookup(const string& symbol);

	// 清空所有定義的符號（重設環境）
	void clear();

	// 檢查某個符號是否為內建 primitive（不可重新定義）
	bool isPrimitive(const string& symbol);
};

// ========================
// 評估器：負責將語法樹轉為實際語義值
// ========================
class EvalError : public runtime_error {
public:
	EvalError(const string& msg) : runtime_error(msg) {}
};

class Evaluator {
	Environment& env;

public:
	Evaluator(Environment& e);

	// 評估一個語法節點，並回傳結果節點
	Node* Eval(Node* expr);

private:
	// 處理 quote 特殊形式
	Node* handleQuote(Node* expr);

	// 處理 define 特殊形式
	Node* handleDefine(Node* expr);

	// 建立一個原子節點
	Node* makeAtom(string val, TokenType type);

	// 將語法樹轉為字串表示（用於錯誤訊息）
	string PrintRaw(Node* node);
};

Node* Evaluator::Eval(Node* expr) {
    if (!expr) return nullptr; // 空節點直接回傳

    if (expr->isAtom) {
        if (expr->atom.type == SYMBOL) {
            return env.lookup(expr->atom.value); // 查詢環境表
        } else {
            return expr; // 其他原子直接回傳
        }
    }

    if (expr->left->isAtom && expr->left->atom.type == SYMBOL) {
        string op = expr->left->atom.value;
        if (op == "quote") return handleQuote(expr);
        else if (op == "define") return handleDefine(expr);
        else if (op == "exit") return nullptr; // exit 指令不做任何事
        else if (op == "clean-environment") {
            env.clear(); // 清空環境表
            return nullptr;
        } else if (op == "begin") {
        } else if (op == "if") {
            if (!expr->right || !expr->right->isAtom) {
                throw EvalError("ERROR (IF format) : ( if ... )");
            }
            Node* cond = Eval(expr->right->left); // 評估條件
            if (cond && cond->isAtom && cond->atom.type == T) {
                return Eval(expr->right->right); // 如果條件為真，評估 then 部分
            } else {
                return Eval(expr->right->right->right); // 否則評估 else 部分
            }
        }
        else if (op == "cond") {
            Node* curr = expr->right;
            while (curr) {
                Node* cond = Eval(curr->left->left); // 評估條件
                if (cond && cond->isAtom && cond->atom.type == T) {
                    return Eval(curr->left->right); // 如果條件為真，評估 then 部分
                }
                curr = curr->right; // 繼續檢查下一個條件
            }
            return nullptr; // 所有條件都不成立，回傳 nil
        } else if (op == "cons") {
            if (!expr->right || !expr->right->isAtom) {
                throw EvalError("ERROR (CONS format) : ( cons ... )");
            }
        }
    }

    Node* func = Eval(expr->left); // 評估函數部分
    if (!func || !func->isAtom || func->atom.type != SYMBOL) {
        throw EvalError("ERROR (attempt to apply non-function): " + PrintRaw(func));
    }

    vector<Node*> args;
    Node* curr = expr->right;
    while (curr) {
        args.push_back(Eval(curr->left)); // 評估參數部分
        curr = curr->right;
    }

    return func; // 回傳函數結果（目前不支援實際計算）
}
// ========================
// 主程式：整合 Lex → Parse → Eval → Print
// ========================
int main() {
	cout << "Welcome to OurScheme!" << endl << endl;

	Environment env;				// 建立一個新的環境表
	Evaluator evaluator(env);		// 建立評估器並傳入環境
	Parser parser;					// 使用 Project1 的 Parser 建構語法樹

	while (true) {
		cout << "> ";
		try {
			Node* expr = parser.Parse(); // 呼叫 Parser 取得一棵 S-expression

			// 若輸入為 (exit)，則結束程式
			if (parser.IsExactExit(expr)) {
				cout << endl << "Thanks for using OurScheme!" << endl;
				exit(0);
			}

			// 執行語意評估並印出結果
			Node* result = evaluator.Eval(expr);
			parser.PrettyPrint(result);
			cout << endl;
		} catch (LexError& e) {
			cout << e.what() << endl << endl; // 處理語法錯誤
		} catch (EvalError& e) {
			cout << e.what() << endl << endl; // 處理語意錯誤
		}
	}

	return 0;
}