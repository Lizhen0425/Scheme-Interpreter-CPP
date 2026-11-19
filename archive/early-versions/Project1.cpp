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
    int line = 1, column = 1;
};

struct Node {
    bool isAtom;
    Token atom;
    Node* left;
    Node* right;
    Node() : isAtom(false), left(nullptr), right(nullptr) {}
};

class LexError : public runtime_error {
public:
    int line;
    int column;
    string symbol;
    
    LexError(const string& msg, int ln = 0, int col = 0, const string& sym = "")
        : runtime_error(msg), line(ln), column(col), symbol(sym) {}
};

class LexicalAnalyer {
    // 最後讀到的字元
    char last_char = '\0';

    // 最後讀到的字元的位置
    int last_line = 0, last_column = 0;

    // 指標目前所在的位置（待讀取/下一個字元的位置）
    int line = 1, column = 1;

	// EOF flag
    bool eof_flag = false;

    bool in_unfinished_sexp = false;
    
public:

    void SetUnfinishedSexp(bool state) {
        in_unfinished_sexp = state;
    }
    
    Token GetNextToken() {
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
    
    void reset() {
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
    
    void eat(){
        char ch;
        do {
            ch = cin.get();
        } while (ch != '\n' && ch != EOF);
        UngetChar();
        line = 0;
    }

private:
    char GetNextChar() {
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

    void UngetChar() {
        if (last_char != '\n') { 
            cin.unget();
            column--;
        } else {
            cin.unget();
            line--;
            column = last_column;
        }
    }
};

class Parser {
    LexicalAnalyer lexer;
    Token lookahead;
    bool hasLookahead = false;
    bool in_unfinished_sexp = false;

    Token GetToken() {
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

    void UngetToken(Token token) {
        lookahead = token;
        hasLookahead = true;
    }

public:
    void ResetLexer() {  
        if (!hasLookahead) {
            lexer.reset();
        }
    }
    
    Node* Parse() {
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

    void PrettyPrint(Node* node, int indent = 0) {

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
    
    bool IsExactExit(Node* node) {
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


private:
    Node* ParseList() {
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

    Node* CreateAtomNode(string value, TokenType type) {
        Node* node = new Node();
        node->isAtom = true;
        node->atom.value = value;
        node->atom.type = type;
        return node;
    }
};

int main() {
    cout << "Welcome to OurScheme!" << endl << endl;
    string problem;
    getline(cin, problem);

    Parser parser;
    bool need_prompt = true;

    while (true) {
        if (need_prompt) {
            cout << "> ";
            // parser.ResetLexer();
            need_prompt = false;
        }

        try {
            Node* expr = parser.Parse();
            if (!expr) continue;

            if (parser.IsExactExit(expr)) {
                cout << endl << "Thanks for using OurScheme!" << endl;
                exit(0);
            }



            parser.PrettyPrint(expr);
            cout << endl;

            parser.ResetLexer();
            need_prompt = true;
        } 
        catch (LexError& err) {
            string errmsg = err.what();
            cout << errmsg << endl;

            if (errmsg == "ERROR (no more input) : END-OF-FILE encountered") {
                cout << "Thanks for using OurScheme!";
                exit(0);
            }
            
            cout << endl;
            need_prompt = true;
        }
    }

    return 0;
}