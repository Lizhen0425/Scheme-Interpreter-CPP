import java.io.*;
import java.util.*;

class OurSchemeTEST {
    // Token類型定義，表示Scheme語言中的基本語法單元
    enum TokenType {
        LEFT_PAREN, RIGHT_PAREN, INT, FLOAT, STRING, SYMBOL,
        NIL, T, QUOTE, DOT, EOF, ERROR
    }

    // Token類，表示一個語法單元
    static class Token {
        final TokenType type; // Token的類型
        final String value;   // Token的值
        final int line;       // Token所在的行
        final int column;     // Token所在的列

        Token(TokenType type, String value, int line, int column) {
            this.type = type;
            this.value = value;
            this.line = line;
            this.column = column;
        }

        @Override
        public String toString() {
            return String.format("%s(%s)@%d:%d", type, value, line, column);
        }
    }

    // SchemeTokenizer類，負責將輸入流解析為Token
    static class SchemeTokenizer {
        private final BufferedReader reader; // 用於讀取輸入的緩衝讀取器
        private int line = 1;                // 當前行號
        private int column = 1;              // 當前列號
        private int currentChar = -1;        // 當前字符
        private final Deque<Integer> buffer = new ArrayDeque<>(); // 用於存儲回退字符的緩衝區

        SchemeTokenizer(InputStream in) {
            reader = new BufferedReader(new InputStreamReader(in));
        }

        // 讀取下一個字符，並更新行列號
        private int readChar() throws IOException {
            if (!buffer.isEmpty()) return buffer.pop(); // 如果有回退字符，優先返回
            currentChar = reader.read(); // 從輸入流讀取下一個字符
            if (currentChar == '\n') {  // 如果是換行符，更新行號並重置列號
                line++;
                column = 1;
            } else if (currentChar != -1) { // 如果不是EOF，增加列號
                column++;
            }
            return currentChar;
        }

        // 將字符回退到緩衝區
        private void unreadChar(int c) {
            if (c != -1) {
                buffer.push(c);
                if (c == '\n') line--; // 如果是換行符，回退行號
                else column--;         // 否則回退列號
            }
        }

        // 解析下一個Token
        Token nextToken() throws IOException, SyntaxErrorException {
            // 跳過空白和註釋
            while (true) {
                int c = readChar();
                if (c == -1) return new Token(TokenType.EOF, "", line, column); // EOF時返回EOF Token
                
                if (c == ';') { // 如果是註釋，跳過整行
                    while ((c = readChar()) != '\n' && c != -1);
                    continue;
                }
                
                if (!Character.isWhitespace(c)) { // 跳過空白字符
                    unreadChar(c);
                    break;
                }
            }

            int startLine = line;   // 記錄Token的起始行
            int startColumn = column; // 記錄Token的起始列
            int c = readChar();

            // 根據字符類型生成對應的Token
            switch (c) {
                case '(': return new Token(TokenType.LEFT_PAREN, "(", startLine, startColumn);
                case ')': return new Token(TokenType.RIGHT_PAREN, ")", startLine, startColumn);
                case '\'': return new Token(TokenType.QUOTE, "'", startLine, startColumn);
                case '"': return readString(startLine, startColumn); // 讀取字符串
                case '.': return handleDot(startLine, startColumn);  // 處理點號
                default:
                    if (Character.isDigit(c) || c == '+' || c == '-' || c == '.') {
                        return readNumber(c, startLine, startColumn); // 讀取數字
                    } else {
                        return readSymbol(c, startLine, startColumn); // 讀取符號
                    }
            }
        }

        // 讀取字符串Token
        private Token readString(int line, int column) throws IOException, SyntaxErrorException {
            StringBuilder sb = new StringBuilder();
            boolean escape = false; // 是否處於轉義狀態
            while (true) {
                int c = readChar();
                if (c == -1 || c == '\n') { // 如果遇到EOF或換行，拋出錯誤
                    throw new SyntaxErrorException("no closing quote", line, column);
                }
                
                if (escape) { // 處理轉義字符
                    switch (c) {
                        case 'n': sb.append('\n'); break;
                        case 't': sb.append('\t'); break;
                        case '"': sb.append('"'); break;
                        case '\\': sb.append('\\'); break;
                        default: sb.append((char)c);
                    }
                    escape = false;
                } else if (c == '\\') { // 遇到反斜杠，進入轉義狀態
                    escape = true;
                } else if (c == '"') { // 遇到閉合引號，結束字符串
                    break;
                } else {
                    sb.append((char)c);
                }
            }
            return new Token(TokenType.STRING, sb.toString(), line, column);
        }

        // 處理點號Token
        private Token handleDot(int line, int column) throws IOException {
            int next = readChar();
            if (isSeparator(next)) { // 如果點號後是分隔符，返回點號Token
                unreadChar(next);
                return new Token(TokenType.DOT, ".", line, column);
            } else { // 否則作為符號處理
                unreadChar(next);
                return readSymbol('.', line, column);
            }
        }

        // 判斷字符是否為分隔符
        private boolean isSeparator(int c) {
            return Character.isWhitespace(c) || c == '(' || c == ')' ||
                   c == '"' || c == ';' || c == '\'';
        }

        // 讀取數字Token
        private Token readNumber(int firstChar, int line, int column) throws IOException {
            StringBuilder sb = new StringBuilder().append((char)firstChar);
            boolean isFloat = false; // 是否為浮點數
            while (true) {
                int c = readChar();
                if (c == '.' && !isFloat) { // 遇到小數點，標記為浮點數
                    isFloat = true;
                    sb.append('.');
                } else if (Character.isDigit(c) || (c == '+' || c == '-') && 
                          (sb.length() > 0 && (sb.charAt(sb.length()-1) == 'e' || 
                           sb.charAt(sb.length()-1) == 'E'))) {
                    sb.append((char)c);
                } else {
                    unreadChar(c);
                    break;
                }
            }
            String num = sb.toString();
            try {
                if (isFloat || num.contains(".")) {
                    return new Token(TokenType.FLOAT, num, line, column); // 返回浮點數Token
                } else {
                    return new Token(TokenType.INT, num, line, column);   // 返回整數Token
                }
            } catch (NumberFormatException e) {
                return new Token(TokenType.SYMBOL, num, line, column);   // 如果解析失敗，作為符號處理
            }
        }

        // 讀取符號Token
        private Token readSymbol(int firstChar, int line, int column) throws IOException {
            StringBuilder sb = new StringBuilder().append((char)firstChar);
            while (true) {
                int c = readChar();
                if (c == -1 || isSeparator(c)) { // 遇到分隔符或EOF，結束符號讀取
                    unreadChar(c);
                    break;
                }
                sb.append((char)c);
            }
            String symbol = sb.toString();
            if (symbol.equalsIgnoreCase("nil") || symbol.equals("#f")) {
                return new Token(TokenType.NIL, "nil", line, column); // 處理nil
            } else if (symbol.equalsIgnoreCase("t") || symbol.equals("#t")) {
                return new Token(TokenType.T, "#t", line, column);   // 處理布林值
            }
            return new Token(TokenType.SYMBOL, symbol, line, column); // 返回符號Token
        }
    }

    interface SExp {}
    
    static class Atom implements SExp {
        final String value;
        final TokenType type;
        
        Atom(Token token) {
            switch (token.type) {
                case INT: case FLOAT: case STRING: case SYMBOL: case NIL: case T:
                    this.type = token.type;
                    this.value = token.value;
                    break;
                default: throw new IllegalArgumentException("Invalid atom type");
            }
        }

        @Override
        public String toString() {
            switch (type) {
                case FLOAT: return String.format("%.3f", Double.parseDouble(value));
                case NIL: return "nil";
                case T: return "#t";
                default: return value;
            }
        }
    }

    static class Pair implements SExp {
        final SExp car;
        final SExp cdr;
        
        Pair(SExp car, SExp cdr) {
            this.car = car;
            this.cdr = cdr;
        }
    }

    static class Parser {
        private final SchemeTokenizer tokenizer;
        private Token currentToken;

        Parser(SchemeTokenizer tokenizer) throws IOException, SyntaxErrorException {
            this.tokenizer = tokenizer;
            currentToken = tokenizer.nextToken();
        }

        SExp parse() throws IOException, SyntaxErrorException {
            return parseSExp();
        }

        private SExp parseSExp() throws IOException, SyntaxErrorException {
            if (currentToken.type == TokenType.LEFT_PAREN) {
                return parseList();
            } else if (currentToken.type == TokenType.QUOTE) {
                return parseQuote();
            } else {
                return parseAtom();
            }
        }

        private SExp parseList() throws IOException, SyntaxErrorException {
            consume(TokenType.LEFT_PAREN);
            SExp list = parseListElements();
            consume(TokenType.RIGHT_PAREN);
            return list;
        }

        private SExp parseListElements() throws IOException, SyntaxErrorException {
            if (currentToken.type == TokenType.RIGHT_PAREN) {
                return new Atom(new Token(TokenType.NIL, "nil", 0, 0));
            }

            SExp first = parseSExp();
            if (currentToken.type == TokenType.DOT) {
                consume(TokenType.DOT);
                SExp cdr = parseSExp();
                return new Pair(first, cdr);
            }

            return new Pair(first, parseListElements());
        }

        private SExp parseQuote() throws IOException, SyntaxErrorException {
            consume(TokenType.QUOTE);
            SExp quoted = parseSExp();
            return new Pair(new Atom(new Token(TokenType.SYMBOL, "quote", 0, 0)),
                           new Pair(quoted, new Atom(new Token(TokenType.NIL, "nil", 0, 0))));
        }

        private SExp parseAtom() throws IOException, SyntaxErrorException {
            Token token = currentToken;
            consume(token.type);
            return new Atom(token);
        }

        private void consume(TokenType expected) throws IOException, SyntaxErrorException {
            if (currentToken.type != expected) {
                throw new SyntaxErrorException(
                    String.format("Expected %s, got %s", expected, currentToken.type),
                    currentToken.line,
                    currentToken.column
                );
            }
            currentToken = tokenizer.nextToken();
        }
    }

    static class PrettyPrinter {
        static String print(SExp exp) {
            return print(exp, 0, true);
        }

        private static String print(SExp exp, int indent, boolean isFirst) {
            StringBuilder sb = new StringBuilder();
            if (exp instanceof Atom) {
                Atom atom = (Atom) exp;
                if (!isFirst) sb.append(indent(indent)); // 只有非第一個元素才縮進
                sb.append(atom);
            } else if (exp instanceof Pair) {
                Pair pair = (Pair) exp;
                if (isList(pair)) {
                    if (!isFirst) sb.append(indent(indent)); // 只有非第一個元素才縮進
                    sb.append("(\n");
                    printList(pair, sb, indent + 2);
                    sb.append(indent(indent)).append(")");
                } else {
                    if (!isFirst) sb.append(indent(indent)); // 只有非第一個元素才縮進
                    sb.append("(\n");
                    printDottedPair(pair, sb, indent + 2);
                    sb.append(indent(indent)).append(")");
                }
            }
            return sb.toString();
        }

        private static boolean isList(Pair pair) {
            SExp cdr = pair.cdr;
            while (cdr instanceof Pair) {
                cdr = ((Pair) cdr).cdr;
            }
            return cdr instanceof Atom && ((Atom) cdr).type == TokenType.NIL;
        }

        private static void printList(Pair pair, StringBuilder sb, int indent) {
            SExp current = pair;
            boolean firstElement = true;
            while (current instanceof Pair) {
                Pair p = (Pair) current;
                sb.append(print(p.car, indent, firstElement)).append("\n"); // 每個元素後換行
                current = p.cdr;
                firstElement = false;
            }
            if (current instanceof Atom && ((Atom) current).type != TokenType.NIL) {
                sb.append(indent(indent)).append(".\n"); // 處理點對結構
                sb.append(print(current, indent, true));
            }
        }

        private static void printDottedPair(Pair pair, StringBuilder sb, int indent) {
            sb.append(print(pair.car, indent, true)).append("\n"); // 處理 car 部分
            sb.append(indent(indent)).append(".\n"); // 處理點
            sb.append(print(pair.cdr, indent, true)); // 處理 cdr 部分
        }

        private static String indent(int count) {
            return " ".repeat(Math.max(0, count)); // 生成縮進空格
        }
    }

    static class SyntaxErrorException extends Exception {
        SyntaxErrorException(String message, int line, int column) {
            super(String.format("ERROR (unexpected token) at line %d column %d: %s", line, column, message));
        }
    }

    public static void repl() throws IOException {
        System.out.println("Welcome to OurScheme!\n> ");
        
        SchemeTokenizer tokenizer = new SchemeTokenizer(System.in);
        while (true) {
            try {
                System.out.print("> ");
                String inputLine = tokenizer.reader.readLine(); // 讀取整行輸入

                if (inputLine == null) {
                    System.out.println("ERROR (no more input) : END-OF-FILE encountered");
                    System.out.println("Thanks for using OurScheme!");
                    return;
                }

                inputLine = inputLine.trim(); // 去除前後空白

                if (inputLine.equals("(exit)")) {
                    System.out.println("\nThanks for using OurScheme!");
                    return;
                }

                // 將整行輸入傳遞給 tokenizer 進行解析
                tokenizer = new SchemeTokenizer(new ByteArrayInputStream(inputLine.getBytes()));
                
                // 解析並處理整行輸入中的所有字元
                while (true) {
                    try {
                        Parser parser = new Parser(tokenizer);
                        SExp expr = parser.parse();
                        
                        if (isExit(expr)) {
                            System.out.println("\nThanks for using OurScheme!");
                            return;
                        }
                        
                        System.out.println(PrettyPrinter.print(expr));
                    } catch (SyntaxErrorException e) {
                        System.out.println(e.getMessage());
                        break; // 跳出內部迴圈，繼續下一行輸入
                    } catch (IOException e) {
                        break; // 跳出內部迴圈，繼續下一行輸入
                    }
                }
            } catch (IOException e) {
                System.out.println("ERROR (no more input) : END-OF-FILE encountered");
                System.out.println("Thanks for using OurScheme!");
                return;
            }
        }
    }

    private static boolean isExit(SExp expr) {
        if (expr instanceof Pair) {
            Pair p = (Pair) expr;
            if (p.car instanceof Atom && ((Atom) p.car).value.equals("exit")) {
                return true;
            }
        }
        return false;
    }
}

public class Maintest {
    public static void main(String[] args) throws IOException {
        OurSchemeTEST.repl();
    }
}