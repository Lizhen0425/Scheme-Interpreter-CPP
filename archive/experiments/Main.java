import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.*;
import java.util.regex.*;

class ParsePosition {
    int line;
    int column;

    ParsePosition(int line, int column) {
        this.line = line;
        this.column = column;
    }
}

class TokenInfo {
    String token;
    ParsePosition position;

    TokenInfo(String token, int line, int column) {
        this.token = token;
        this.position = new ParsePosition(line, column);
    }
}

class OurScheme {
    private static String input;
    private static BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));

    private static String parseString(String str) {
        Pattern pattern = Pattern.compile("\\\\(.)");
        Matcher matcher = pattern.matcher(str);
        StringBuffer sb = new StringBuffer();
        while (matcher.find()) {
            String replacement;
            switch (matcher.group(1)) {
                case "n": replacement = "\n"; break;
                case "t": replacement = "\t"; break;
                case "\\": replacement = "\\"; break;
                case "\"": replacement = "\""; break;
                default: replacement = "\\" + matcher.group(1);
            }
            matcher.appendReplacement(sb, Matcher.quoteReplacement(replacement));
        }
        matcher.appendTail(sb);
        return sb.toString();
    }

    private static String checkUnclosedString(String input, int lineNumber) throws Exception {
        boolean inString = false;
        int quotePos = -1;
    
        for (int i = 0; i < input.length(); i++) {
            char c = input.charAt(i);
            if (c == '"') {
                if (!inString) {
                    quotePos = i + 1; // 字串開始，記錄 Column
                }
                inString = !inString;
            }
        }
    
        if (inString) {
            throw new Exception("ERROR (no closing quote) : END-OF-LINE encountered at Line "
                    + lineNumber + " Column " + (input.length() + 1));
        }
    
        return input;
    }
    

    private static List<TokenInfo> tokenize(int lineNumber) throws Exception {
        checkUnclosedString(input, lineNumber);  // Check before tokenizing
        input = input.toString().trim();

        List<TokenInfo> tokens = new ArrayList<>();
        Pattern pattern = Pattern.compile("\\(|\\)|'|\"(?:\\\\\"|[^\"])*\"|[^\\s()'\"]+");
        Matcher matcher = pattern.matcher(input);

        int currentIndex = 0; // Track the position in the input

        while (matcher.find()) {
            String token = matcher.group();
            int startIdx = matcher.start();

            // Calculate column: count non-whitespace characters from currentIndex to startIdx
            int column = 1;
            for (int i = currentIndex; i < startIdx; i++) {
                if (!Character.isWhitespace(input.charAt(i))) {
                    column++;
                }
            }

            column++; // column is 1-based, next non-whitespace character

            if (token.startsWith("\"")) {
                String unescaped = parseString(token.substring(1, token.length() - 1));
                tokens.add(new TokenInfo("\"" + unescaped + "\"", lineNumber, column));
            } else {
                tokens.add(new TokenInfo(token, lineNumber, column));
            }

            currentIndex = matcher.end();
        }

        return tokens;
    }

    private static Object parse(List<TokenInfo> tokens) throws Exception {
        if (tokens.isEmpty()) return null;
        TokenInfo tokenInfo = tokens.remove(0);
        String token = tokenInfo.token;
    
        if (token.equals("(")) {
            List<Object> list = new ArrayList<>();
            boolean hasDot = false;
    
            while (true) {
                if (tokens.isEmpty()) {
                    // 當遇到 EOF，但預期應該要有 ')'
                    throw new Exception("ERROR (unexpected token) : ')' expected when token at Line "
                            + tokenInfo.position.line + " Column " + tokenInfo.position.column + " is >>EOF<<");
                }
    
                TokenInfo nextToken = tokens.get(0);
                if (nextToken.token.equals(")")) {
                    tokens.remove(0);
                    return list;
                } else if (nextToken.token.equals(".")) {
                    if (hasDot || list.isEmpty()) {
                        TokenInfo dotInfo = tokens.remove(0);
                        throw new Exception("ERROR (unexpected token) : atom or '(' expected when token at Line "
                                + dotInfo.position.line + " Column " + dotInfo.position.column + " is >>.<<");
                    }
                    hasDot = true;
                    tokens.remove(0);
                    Object cdr = parse(tokens);
    
                    if (tokens.isEmpty()) {
                        throw new Exception("ERROR (unexpected token) : ')' expected when token at Line "
                                + tokenInfo.position.line + " Column " + tokenInfo.position.column + " is >>EOF<<");
                    }
    
                    TokenInfo closing = tokens.remove(0);
                    if (!closing.token.equals(")")) {
                        throw new Exception("ERROR (unexpected token) : ')' expected when token at Line "
                                + closing.position.line + " Column " + closing.position.column + " is >>"
                                + closing.token + "<<");
                    }
    
                    list.add(" . ");
                    list.add(cdr);
                    return list;
                } else {
                    list.add(parse(tokens));
                }
            }
    
        } else if (token.equals("'")) {
            return Arrays.asList("quote", parse(tokens));
        } else if (token.equals(")")) {
            throw new Exception("ERROR (unexpected token) : atom or '(' expected when token at Line "
                    + tokenInfo.position.line + " Column " + tokenInfo.position.column + " is >>)<<");
        } else if (token.equals(".")) {
            throw new Exception("ERROR (unexpected token) : atom or '(' expected when token at Line "
                    + tokenInfo.position.line + " Column " + tokenInfo.position.column + " is >>.<<");
        } else if (token.startsWith("\"") && token.endsWith("\"")) {
            return token;
        } else if (isNumber(token)) {
            return token.contains(".") ? String.format("%.3f", Double.parseDouble(token)) : Integer.parseInt(token);
        } else if (token.equals("nil") || token.equals("#f")) {
            return "nil";
        } else if (token.equals("t") || token.equals("#t")) {
            return "#t";
        } else {
            return token;
        }
    }       

    private static boolean isNumber(String s) {
        try {
            Double.parseDouble(s);
            return true;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    private static void prettyPrint(Object exp, boolean isRoot) {
        if (exp == null) return;

        if (exp instanceof List) {
            List<?> list = (List<?>) exp;
            if (list.isEmpty()) {
                if (isRoot) System.out.println("nil");
                return;
            }

            boolean isDotted = list.contains(" . ");
            if (isDotted) {
                int dotIndex = list.indexOf(" . ");
                List<?> car = list.subList(0, dotIndex);
                Object cdr = list.get(dotIndex + 1);

                if (isRoot) System.out.print("(");
                for (Object e : car) {
                    prettyPrint(e, false);
                }
                if (cdr instanceof List) {
                    prettyPrint(cdr, false);
                } else if (!cdr.equals("nil")) {
                    System.out.println(" . ");
                    prettyPrint(cdr, false);
                }
                if (isRoot) System.out.println(")");
            } else {
                if (isRoot) System.out.print("( ");
                for (Object e : list) {
                    prettyPrint(e, false);
                }
                if (isRoot) System.out.println(")");
            }
        } else {
            String s = exp.toString();
            if (s.equals("nil") && !isRoot) return;
            System.out.println(s);
        }
    }

    private static String checkAndTokenize(String input) {
        boolean inString = false;
        StringBuilder result = new StringBuilder();

        for (int i = 0; i < input.length(); i++) {
            char c = input.charAt(i);

            if (c == '"') {
                inString = !inString;
                result.append(c);
            } else if (c == ';' && !inString) {
                break;
            } else {
                result.append(c);
            }
        }

        return result.toString();
    }

    public static void repl() throws IOException {
        System.out.println("Welcome to OurScheme!");
        System.out.println();
        int lineNumber = 1;

        while (true) {
            System.out.print("> ");
            input = reader.readLine();
            boolean islegal = true;

            if (input == null) {
                System.out.println("ERROR (no more input) : END-OF-FILE encountered");
                System.out.println("Thanks for using OurScheme!");
                break;
            }

            // input = input.trim();

            if (input.equals("(exit)")) {
                System.out.println("\nThanks for using OurScheme!");
                break;
            }

            if (input.matches("[0-9]") || input.isEmpty()) {
                islegal = false;
            }

            input = checkAndTokenize(input);
            // System.out.println("有註解");
            if (input.trim().isEmpty()) {
                islegal = false;
            }

            while (!islegal) {
                input = reader.readLine();
                lineNumber++;
                islegal = true;
                if (input == null) {
                    System.out.println("ERROR (no more input) : END-OF-FILE encountered");
                    System.out.println("Thanks for using OurScheme!");
                    return;
                }

                if (input.equals("(exit)")) {
                    System.out.println("Thanks for using OurScheme!");
                    return;
                }

                if (input.matches("[0-9]") || input.isEmpty()) {
                    islegal = false;
                }

                input = checkAndTokenize(input);
                if (input.trim().isEmpty()) {
                    islegal = false;
                }
            }

            List<TokenInfo> tokens;
            try {
                tokens = tokenize(lineNumber);
            } catch (Exception e) {
                System.out.println(e.getMessage());
                System.out.println();
                lineNumber++;
                continue;
            }

            try {

                while (!tokens.isEmpty()) {
                    Object parsed = parse(tokens);
                
                    if (parsed == null || parsed.equals("nil")) {
                        System.out.println("nil");
                        
                    } else {
                        prettyPrint(parsed, true);
                        lineNumber = 0;
                    }
                    if (!tokens.isEmpty()) {F
                        System.out.println();
                        System.out.print("> ");
                    }
                    else {
                        System.out.println();
                    }
                }
                
            } catch (Exception e) {
                
                System.out.println(e.getMessage());
                
                System.out.println();
                
            }

            lineNumber++;
        }
    }
}

public class Main {
    public static void main(String[] args) {
        try {
            OurScheme.repl();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}