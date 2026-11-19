const readline = require("readline");

/**
 * **字串轉義處理**
 * - `\n` → 換行
 * - `\t` → tab
 * - `\\` → 反斜線
 * - `\"` → 雙引號
 * - 非法轉義 (如 `\a`) 則保留 `\a`
 */
function parseString(str) {
    return str.replace(/\\(.)/g, (match, char) => {
        if (char === "n") return "\n";
        if (char === "t") return "\t";
        if (char === "\\") return "\\";
        if (char === '"') return '"';
        return "\\" + char; // 非法轉義則保留 `\`
    });
}

/**
 * **分詞器 (Tokenizer) - 解析輸入字串並轉成 Token**
 * 支援的 Token:
 * - 左括號 `(` 和右括號 `)`
 * - 單引號 `'`
 * - 數字 (整數、浮點數)
 * - 字串 (雙引號包住)
 * - 符號 (變數名稱等)
 * - `nil`, `#f`, `t`, `#t` 的特殊處理
 */
function tokenize(input) {
    const regex = /\(|\)|'|"(?:\\.|[^"])*"|[^\s()'"]+/g;
    let tokens = input.match(regex) || [];

    return tokens.map(token => {
        if (token.startsWith('"') && token.endsWith('"')) {
            return `"${parseString(token.slice(1, -1))}"`; // 保留雙引號
        }
        return token;
    });
}

/**
 * **解析器 (Parser) - 把 Token 轉成 S-Expression 樹狀結構**
 */
function parse(tokens, line = 1, column = 1) {
    if (tokens.length === 0) return null;
    let token = tokens.shift();

    if (token === "(") {
        let list = [];
        let hasDot = false;
        while (tokens.length > 0 && tokens[0] !== ")") {
            if (tokens[0] === ".") {
                if (hasDot || list.length === 0) {
                    throw `ERROR (unexpected token) : atom or '(' expected when token at Line ${line} Column ${column} is >>.<<`;
                }
                hasDot = true;
                tokens.shift();
                let cdr = parse(tokens);
                tokens.shift(); // 移除 `)`
                return [...list, ".", cdr];
            }
            list.push(parse(tokens));
        }
        tokens.shift(); // 移除 `)`
        return list;
    } else if (token === "'") {
        return ["quote", parse(tokens)];
    } else if (!isNaN(token)) {
        return token.includes(".") ? parseFloat(token).toFixed(3) : parseInt(token, 10);
    } else if (token === "nil" || token === "#f") {
        return "nil";
    } else if (token === "t" || token === "#t") {
        return "#t";
    } else {
        return token;
    }
}

/**
 * **Pretty Print (美化輸出)**
 */
/**
 * **Pretty Print (美化輸出)**
 */
function prettyPrint(exp, indent = 0, isRoot = true) {
    if (exp === "nil" || exp === "#f" || exp === "()") {
        if (isRoot) console.log("nil"); // 根節點為 nil 時輸出
        return;
    }

    if (Array.isArray(exp)) {
        if (exp.length === 0) {
            if (isRoot) console.log("nil"); // 空列表輸出 nil
            return;
        }

        // 檢查是否為點對 (dotted pair)
        const isDottedPair = exp.includes(".");
        if (isDottedPair) {
            const dotIndex = exp.indexOf(".");
            const car = exp.slice(0, dotIndex);   // 左側部分
            const cdr = exp[dotIndex + 1];        // 右側部分

            // 輸出根節點的左括號
            if (isRoot) console.log("(");

            // 先遞迴輸出左子節點
            car.forEach(e => prettyPrint(e, indent, false));

            // 如果 cdr 仍是列表（非單一原子），先輸出其左子節點
            if (Array.isArray(cdr)) {
                prettyPrint(cdr, indent, false);
            }

            // 在最深層才輸出 `.` 並接著輸出 cdr 原子值
            if (!Array.isArray(cdr) && cdr !== "nil" && cdr !== "#f" && cdr !== "()") {
                console.log(".");
                console.log(cdr);
            }

            // 輸出根節點的右括號
            if (isRoot) console.log(")");
        } else {
            // 普通列表
            if (isRoot) console.log("(");
            exp.forEach(e => prettyPrint(e, indent, false));
            if (isRoot) console.log(")");
        }
    } else {
        // 處理原子（數字、符號、字串等）
        console.log(exp);
    }
}

/**
 * **互動模式 (REPL)**
 */
const rl = readline.createInterface({ input: process.stdin, output: process.stdout });

console.log("Welcome to OurScheme!");

function repl() {
    rl.question("> ", (input) => {
        if (input.trim() === "(exit)") {
            console.log("Thanks for using OurScheme!");
            rl.close();
            return;
        }
        try {
            let tokens = tokenize(input);
            let parsed = parse(tokens);
            if (parsed === null || parsed === "nil" || parsed === "#f" || parsed === "()") {
                console.log("nil");
            } else {
                prettyPrint(parsed);
            }
        } catch (error) {
            console.log(error);
        }
        repl();
    });
}

repl();