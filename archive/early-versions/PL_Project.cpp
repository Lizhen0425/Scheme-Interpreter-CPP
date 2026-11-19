#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <stdexcept>
using namespace std;

enum TokenType {
    LEFT_PAREN, RIGHT_PAREN, INT, FLOAT, STRING, DOT, NIL, T, QUOTE, SYMBOL, END_OF_FILE
};

struct Token {
    TokenType type;
    string value;
    int line = 0, column = 0;
};

struct Node {
    bool isAtom;
    Token atom;
    Node* left;
    Node* right;
    Node() : isAtom(false), left(nullptr), right(nullptr) {}
};

struct LexError {
    string msg;
    int line;
    int column;
    string symbol;
};

class LexicalAnalyer {
    char last_char = '\0';
    int last_line = 0, last_column = 0;
    int line = 1, column = 0;
    bool eof_flag = false;

public:
    Token GetNextToken() {
        try {
            while (true) {
                char c = GetNextChar();
                if (eof_flag) {
                    return {END_OF_FILE, "", line, column};
                }
                if (c == ';') {
                    while (GetNextChar() != '\n' && !eof_flag);
                    continue;
                }
                if (!isspace(c)) {
                    UngetChar(c);
                    break;
                }
            }

            char c = GetNextChar();
            Token token;
            token.line = last_line;
            token.column = last_column;

            if (c == '(') {
                token.type = LEFT_PAREN; token.value = "(";
            } else if (c == ')') {
                token.type = RIGHT_PAREN; token.value = ")";
            } else if (c == '\'') {
                token.type = QUOTE; token.value = "'";
            } else if (c == '.') {
                char next = GetNextChar();
                if (isspace(next) || next == '(' || next == ')') {
                    token.type = DOT; token.value = ".";
                    UngetChar(next);
                } else {
                    UngetChar(next);
                    return GetNumberToken(c);
                }
            } else if (c == '"') {
                return GetStringToken();
            } else if (isdigit(c) || c == '+' || c == '-') {
                return GetNumberToken(c);
            } else {
                token.type = SYMBOL;
                string symbol;
                symbol += c;
                while (true) {
                    char next = GetNextChar();
                    if (eof_flag || isspace(next) || next == '(' || next == ')' || next == '"') {
                        UngetChar(next);
                        break;
                    }
                    symbol += next;
                }

                if (symbol == "nil" || symbol == "#f") {
                    token.type = NIL; token.value = "nil";
                } else if (symbol == "t" || symbol == "#t") {
                    token.type = T; token.value = "#t";
                } else {
                    token.value = symbol;
                }
            }
            return token;
        } catch (const LexError& err) {
            throw;
        }
    }

    void reset() {
        line = 1;
        column = 0;
        last_char = '\0';
        eof_flag = false;
    }

private:
    char GetNextChar() {
        last_char = cin.get();
        last_line = line;
        last_column = column + 1;
        if (last_char == EOF) {
            eof_flag = true;
            return EOF;
        }
        if (last_char == '\n') {
            line++;
            column = 0;
        } else {
            column++;
        }
        return last_char;
    }

    void UngetChar(char ch) {
        if (ch != EOF) {
            cin.unget();
            if (ch == '\n') {
                line--;
                column = last_column;
            } else {
                column--;
            }
        }
    }

    Token GetStringToken() {
        Token token;
        token.type = STRING;
        token.line = last_line;
        token.column = last_column;
        string value;
        bool escape = false;

        while (true) {
            char c = GetNextChar();
            if (eof_flag || c == '\n') {
                throw LexError{"no closing quote", token.line, token.column, "\""};
            }
            if (escape) {
                switch (c) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case '"': value += '"'; break;
                    case '\\': value += '\\'; break;
                    default: value += '\\'; value += c; break;
                }
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                break;
            } else {
                value += c;
            }
        }

        token.value = value;
        return token;
    }

    Token GetNumberToken(char first) {
        Token token;
        token.line = last_line;
        token.column = last_column;
        string numStr;
        bool isFloat = false;

        numStr += first;
        while (true) {
            char c = GetNextChar();
            if (eof_flag || !isdigit(c) && c != '.') {
                UngetChar(c);
                break;
            }
            if (c == '.') {
                if (isFloat) break;
                isFloat = true;
            }
            numStr += c;
        }

        if (isFloat) {
            token.type = FLOAT;
            ostringstream oss;
            oss << fixed << setprecision(3) << stod(numStr);
            token.value = oss.str();
        } else {
            token.type = INT;
            token.value = numStr;
        }
        return token;
    }
};

class Parser {
    LexicalAnalyer lexer;
    Token lookahead;
    bool hasLookahead = false;
	bool isInUnfinishedExpr = false;

    Token GetToken() {
        if (hasLookahead) {
            hasLookahead = false;
            return lookahead;
        }
        return lexer.GetNextToken();
    }

    void UngetToken(Token token) {
        lookahead = token;
        hasLookahead = true;
    }

    Node* CreateAtomNode(Token token) {
        Node* node = new Node();
        node->isAtom = true;
        node->atom = token;
        return node;
    }

    Node* CreateNilNode() {
        Node* node = new Node();
        node->isAtom = true;
        node->atom.type = NIL;
        node->atom.value = "nil";
        return node;
    }

public:
    Node* Parse() {
        Token token = GetToken();
        if (token.type == END_OF_FILE)
            throw LexError{"no more input", token.line, token.column, "EOF"};

        if (token.type == LEFT_PAREN) {
			isInUnfinishedExpr = true;
            Node* result = ParseList();
			isInUnfinishedExpr = false;
			return result;
        } else if (token.type == QUOTE) {
            Node* quote = new Node();
            quote->isAtom = false;
            quote->left = CreateAtomNode({SYMBOL, "quote", token.line, token.column});
            quote->right = new Node();
            quote->right->left = Parse();
            quote->right->right = CreateNilNode();
            return quote;
        } else if (token.type == DOT || token.type == RIGHT_PAREN) {
            throw LexError{"unexpected token", token.line, token.column, token.value};
        }

        return CreateAtomNode(token);
    }

	bool inUnfinishedExpr() {
		return isInUnfinishedExpr;
	}

	void reset() {
		hasLookahead = false;
		isInUnfinishedExpr = false;
	}	

private:
    Node* ParseList() {
        Token token = GetToken();
        if (token.type == RIGHT_PAREN) {
            return CreateNilNode();
        }
        if (token.type == DOT) {
            throw LexError{"unexpected token", token.line, token.column, token.value};
        }
        UngetToken(token);
        Node* node = new Node();
        node->isAtom = false;
        node->left = Parse();

        Token next = GetToken();
        if (next.type == DOT) {
            node->right = Parse();
            Token closing = GetToken();
            if (closing.type != RIGHT_PAREN)
                throw LexError{") expected", closing.line, closing.column, closing.value};
        } else if (next.type == RIGHT_PAREN) {
            node->right = CreateNilNode();
        } else {
            UngetToken(next);
            node->right = ParseList();
        }
        return node;
    }
};

void PrettyPrint(Node* node, int indent = 0) {
    if (!node) {
        cout << string(indent, ' ') << "nil" << endl;
        return;
    }
    if (node->isAtom) {
        cout << string(indent, ' ');
        if (node->atom.type == FLOAT) {
            try {
                cout << fixed << setprecision(3) << stof(node->atom.value) << endl;
            } catch (...) {
                cout << node->atom.value << endl;
            }
        } else if (node->atom.type == STRING) {
            cout << '"' << node->atom.value << '"' << endl;
        } else if (node->atom.type == T) {
            cout << "#t" << endl;
        } else if (node->atom.type == NIL) {
            cout << "nil" << endl;
        } else if (node->atom.type == INT && node->atom.value[0] == '+') {
            cout << node->atom.value.substr(1) << endl;
        } else {
            cout << node->atom.value << endl;
        }
        return;
    }
    cout << string(indent, ' ') << "(" << endl;
    Node* curr = node;
    while (curr) {
        PrettyPrint(curr->left, indent + 2);
        if (curr->right && curr->right->isAtom) {
            if (curr->right->atom.type == NIL) break;
            cout << string(indent + 2, ' ') << "." << endl;
            PrettyPrint(curr->right, indent + 2);
            break;
        } else if (curr->right && !curr->right->isAtom) {
            curr = curr->right;
        } else {
            break;
        }
    }
    cout << string(indent, ' ') << ")" << endl;
}

int main() {
    cout << "Welcome to OurScheme!" << endl;
    string problem;
    getline(cin, problem);

	bool g_need_prompt = true;

    LexicalAnalyer lexer;
    Parser parser;

    while (true) {
		if (g_need_prompt) {
			cout << "> ";
			g_need_prompt = false;
		}

		bool should_output = true;

        try {
            Node* expr = parser.Parse();
            if (!expr) {
				if (should_output) {
                    cout << "nil" << endl << endl;
                }
                g_need_prompt = true;
				continue;
			}

            if (!expr->isAtom && expr->left && expr->left->isAtom &&
                expr->left->atom.type == SYMBOL && expr->left->atom.value == "exit" &&
                expr->right && expr->right->isAtom && expr->right->atom.type == NIL) {
                cout << "\nThanks for using OurScheme!" << endl;
                break;
            }

            if (parser.inUnfinishedExpr()) {
				// 尚未完成，等待下一行
				should_output = false;
				g_need_prompt = false;
			} else {
				PrettyPrint(expr);
				cout << endl;
				g_need_prompt = true;
			}

        } catch (const LexError& err) {
            cerr << "ERROR (" << err.msg << ") : at Line "
                 << err.line << " Column " << err.column
                 << " >>" << err.symbol << "<<" << endl << endl;

			cerr.flush();  // <<<<<<<< 加上這一行
			cout.flush();  // 若你還有用 cout 也一併刷新
            // 清除後續換行直到下一筆輸入
            while (true) {
				Token t = lexer.GetNextToken();
				if (t.type == END_OF_FILE || t.line != err.line) break;
			}

            if (err.msg == "no more input") {
                cout << "\nThanks for using OurScheme!" << endl;
                break;
            }
			parser.reset(); 
			g_need_prompt = true;
        }
    }
    return 0;
}