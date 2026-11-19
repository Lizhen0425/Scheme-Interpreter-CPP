#include <iostream>
#include <string>
#include <cctype>
#include <iomanip>
#include <stdexcept>
using namespace std;

bool g_has_token = false; // 這一行輸入有沒有讀到任何有效token

enum TokenType {
	LEFT_PAREN, RIGHT_PAREN, INT, FLOAT, STRING, DOT, NIL, T, QUOTE, SYMBOL, END_OF_FILE
};

struct Token {
	TokenType type;
	string value;
	int line, column;
};

struct Node {
	bool isAtom;
	Token atom;
	Node* left;
	Node* right;
	Node() : isAtom(false), left(nullptr), right(nullptr) {}
};

int g_line = 1, g_column = 0;
int g_last_line = 1, g_last_column = 0;

char g_lastChar = '\0';
bool g_exit = false;
bool g_need_prompt = true;

void GetNextChar(char* ch) {
	if (g_lastChar != '\0') {
		*ch = g_lastChar;
		g_lastChar = '\0';
		g_last_line = g_line;
		g_last_column = g_column;

		if (*ch == '\n') {
			g_line++;
			g_column = 0;
		} else {
			g_column++;
		}
		return;
	}

	int c = getchar();
	if (c == EOF) {
		*ch = '\0';
		return;
	}
	*ch = (char)c;
	g_last_line = g_line;
	g_last_column = g_column;

	if (*ch == '\n') {
		g_line++;
		g_column = 0;
	} else {
		g_column++;
	}
}

void UngetChar(char ch) {
	g_lastChar = ch;
}

void SkipWhiteSpace(char* ch) {
	while (*ch == ' ' || *ch == '\t' || *ch == '\n') {
		GetNextChar(ch);
	}
}

Token GetToken() {
	char ch;

	// 每次抓 token 前，先跳過所有空白與註解
	while (true) {
		GetNextChar(&ch);
        bool passedNewLine = false;
		// 跳過所有空白
		while (ch == ' ' || ch == '\t' || ch == '\n') {
            if (ch == '\n') passedNewLine = true;
			GetNextChar(&ch);
		}

        if (!g_has_token && passedNewLine) {
            g_line = 1;
            g_column = 0;
        }

		// 如果是註解行 ';'，跳過直到下一行
		if (ch == ';') {
			while (ch != '\n' && ch != '\0') {
				GetNextChar(&ch);
			}
			// 跳過完一行後，再重新抓一次
		} else {
			// 遇到非註解且非空白字元，開始處理token
			break;
		}
	}

	Token token;
	token.line = g_line;
	token.column = g_column;

	if (ch == '\0') {
		token.type = END_OF_FILE;
		return token;
	} else if (ch == '(') {
		g_has_token = true;
		token.type = LEFT_PAREN;
		token.value = "(";
	} else if (ch == ')') {
		g_has_token = true;
		token.type = RIGHT_PAREN;
		token.value = ")";
	} else if (ch == '.') {
		g_has_token = true;
		char nextChar;
		GetNextChar(&nextChar);
		if (isdigit(nextChar)) {
			// 是FLOAT (ex:.123)
			token.type = FLOAT;
			token.value = "0.";
			token.value += nextChar;
			while (true) {
				GetNextChar(&nextChar);
				if (isdigit(nextChar)) {
					token.value += nextChar;
				} else {
					UngetChar(nextChar);
					break;
				}
			}
		} else {
			// 是DOT
			UngetChar(nextChar);
			token.type = DOT;
			token.value = ".";
		}
	} else if (ch == '\'') {
		g_has_token = true;
		token.type = QUOTE;
		token.value = "'";
	} else if (ch == '"') {
		g_has_token = true;
        token.type = STRING;
        token.value = "";
        bool escape = false;
        while (true) {
            GetNextChar(&ch);
            if (ch == '\0') {
                throw runtime_error("ERROR (no closing quote) : END-OF-FILE encountered at Line " + to_string(g_last_line) + " Column " + to_string(g_last_column));
            }
            if (ch == '\n') {
                throw runtime_error("ERROR (no closing quote) : END-OF-LINE encountered at Line " + to_string(g_last_line) + " Column " + to_string(g_last_column+1));
            }
            if (escape) {
                if (ch == 'n') token.value += '\n';
                else if (ch == 't') token.value += '\t';
                else if (ch == '\\') token.value += '\\';
                else if (ch == '"') token.value += '"';
                else {
                    token.value += '\\'; // 加回原本的'\'
                    token.value += ch;   // 再加上那個不認得的字
                }
                escape = false;
            } else if (ch == '\\') {
                escape = true;
            } else if (ch == '"') {
                break;
            } else {
                token.value += ch;
            }
        }
	} else if (isdigit(ch) || ch == '+' || ch == '-') {
		g_has_token = true;
        token.value = ch;
        bool isFloat = false;
        bool isSymbol = false;
        GetNextChar(&ch);
        while (!isspace(ch) && ch != '(' && ch != ')' && ch != '\'' && ch != '"' && ch != '\0') {
            if (isdigit(ch)) {
                token.value += ch;
            } else if (ch == '.') {
                isFloat = true;
                token.value += ch;
            } else {
                // 遇到非數字也非小數點（例如字母）
                isSymbol = true;
                token.value += ch;
            }
            GetNextChar(&ch);
        }
        UngetChar(ch);

        if (isSymbol) {
            token.type = SYMBOL;
        } else {
            token.type = isFloat ? FLOAT : INT;
        }
	} else {
		g_has_token = true;
		token.value = ch;
		GetNextChar(&ch);
		while (!isspace(ch) && ch != '(' && ch != ')' && ch != '\'' && ch != '"' && ch != '\0') {
			token.value += ch;
			GetNextChar(&ch);
		}
		UngetChar(ch);
		if (token.value == "nil" || token.value == "#f")
			token.type = NIL;
		else if (token.value == "t" || token.value == "#t")
			token.type = T;
		else
			token.type = SYMBOL;
	}

	return token;
}

Node* ParseSExp();

Node* ParseList() {
	Token token = GetToken();
	if (token.type == RIGHT_PAREN) return nullptr;
	if (token.type == DOT) throw runtime_error("ERROR (unexpected token) : atom or '(' expected when token at Line " + to_string(token.line) + " Column " + to_string(token.column) + " is >>.<<");
	UngetChar(token.value[0]);
	Node* node = new Node();
	node->left = ParseSExp();
	token = GetToken();
	if (token.type == DOT) {
		node->right = ParseSExp();
		token = GetToken();
		if (token.type != RIGHT_PAREN) throw runtime_error("ERROR (unexpected token) : ')' expected");
	} else {
		UngetChar(token.value[0]);
		node->right = ParseList();
	}
	return node;
}

Node* ParseSExp() {
	Token token = GetToken();

	if (token.type == LEFT_PAREN) {
		return ParseList();
	}

	if (token.type == QUOTE) {
		Node* quote = new Node();
		quote->left = new Node();
		quote->left->isAtom = true;
		quote->left->atom.type = SYMBOL;
		quote->left->atom.value = "quote";
		quote->right = new Node();
		quote->right->left = ParseSExp();
		quote->right->right = nullptr;
		return quote;
	}

	if (token.type == DOT) {
		throw runtime_error("ERROR (unexpected token) : atom or '(' expected when token at Line " + to_string(token.line) + " Column " + to_string(token.column) + " is >>.<<");
	}

	if (token.type == RIGHT_PAREN) {
		throw runtime_error("ERROR (unexpected token) : atom or '(' expected when token at Line " + to_string(token.line) + " Column " + to_string(token.column) + " is >>)<<");
	}

	if (token.type == END_OF_FILE) {
		throw runtime_error("ERROR (no more input) : END-OF-FILE encountered");
	}

	// 正常原子
	Node* node = new Node();
	node->isAtom = true;
	node->atom = token;
	return node;
}

void PrintSExp(Node* node, int indent = 0) {
	if (!node) {
        return;
    }
    
	for (int i = 0; i < indent; i++) cout << " ";
	if (node->isAtom) {
		if (node->atom.type == FLOAT) {
            try {
                cout << fixed << setprecision(3) << stof(node->atom.value) << endl;
            } catch (const invalid_argument& e) {
                cout << node->atom.value << endl; // 出錯時就直接印原字串，不轉float
            }
        }
		else if (node->atom.type == T) {
            cout << "#t" << endl;
        }
		else if (node->atom.type == NIL) {
            cout << "nil" << endl;
        }
        else if (node->atom.type == STRING) {
            cout << "\"" << node->atom.value << "\"" << endl;
        }
        else if (node->atom.type == INT) {
            if (node->atom.value[0] == '+') {
                cout << node->atom.value.substr(1) << endl; 
            } else {
                cout << node->atom.value << endl;
            }
        }        
		else cout << node->atom.value << endl;
	} else {
		cout << "( ";

        Node* curr = node;
        bool first = true;
    
        while (curr) {
          if (!first) {
            cout<<endl;
            for (int i = 0; i < indent + 2; ++i) cout << " ";
          }
          if (curr->left) {
            PrintSExp(curr->left, indent);
          }
          curr = curr->right;
          first = false;
        }
    
        cout << ")" <<endl;
	}
}

int main() {
	cout << "Welcome to OurScheme!" << endl << endl;

    string problem;
    getline(cin, problem); 

	while (!g_exit) {
		if (g_need_prompt) {
			cout << "> ";
			g_line = 1;
			g_column = 0;
			g_need_prompt = false;
		}

		g_has_token = false; // 重置

		try {
			Node* expr = ParseSExp();

			if (!g_has_token) {
				// 空白行或註解行，不輸出
				continue;
			}

			if (!expr) {
				// ()，印出nil
				cout << "nil" << endl;
				cout << endl;
				g_need_prompt = true;  
				continue;
			}

			// if (expr->isAtom && expr->atom.type == INT && expr->atom.value.size() == 1 && isdigit(expr->atom.value[0])) {
			// 	// 0~9，不輸出
			// 	g_need_prompt = false;
			// 	continue;
			// }     

			PrintSExp(expr);
			cout << endl;
			g_need_prompt = true;  

		} catch (runtime_error& e) {
			string errmsg = e.what();
			cout << errmsg << endl;

			// 清空當前這一行剩下的輸入
			char ch;
			do {
				GetNextChar(&ch);
			} while (ch != '\n' && ch != '\0');

			if (errmsg == "ERROR (no more input) : END-OF-FILE encountered") {
				cout << "Thanks for using OurScheme!" << endl;
				exit(0);
			}

			cout << endl;
			g_need_prompt = true; 
		}
	}

	return 0;
}