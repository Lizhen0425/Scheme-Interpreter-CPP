#include <iostream>

/**
 * @note 綜觀 4 個 project 的實作心得
 * 例外拋出建議不要單純丟出一個 string，像是 `throw "END-OF-LINE Encountered."`
 * 因為有些例外捕捉到後需要做些特別處理，但大多數的例外卻又不用處理直接印出來就好。
 * 建議用物件 struct、class 包裝，把例外物件當成類似 token 般的存在。
 * 因為它們有模糊的分群關係。
 */

using namespace std;

enum TokenType {
  LEFT_PAREN, RIGHT_PAREN, INT, FLOAT, STRING, DOT, NIL, T, QUOTE, SYMBOL
}

struct Token {
  TokenType type;   // 這是什麼類型的 token
  string value;     // 實際的值
  int line = 0, column = 0; // 它在程式碼的哪一行哪一列
};

struct Node {
  bool isAtom;     // 是不是原子（就是最基本的元素）
  Token atom;      // 如果是原子就存這裡
  Node* left;      // 左邊的小孩
  Node* right;     // 右邊的小孩
  Node() : isAtom(false), left(nullptr), right(nullptr) {} // 初始化
};

// 例外格式
struct LexError {
  string msg;
  int line;
  int column;
  string symbol;
};

class LexicalAnalyer {

  // 最後讀到的字元
  char last_char = '\0';

  // 最後讀到的字元的位置
  int last_line = 0, last_column = 0;

  // 光標目前所在的位置（待讀取/下一個字元的位置）
  int line = 1, column = 1;

 public:
  /**
   * @brief
   * 我負責給 Parser 呼叫，只要呼叫我必定可以拿到「合法的」Token！
   * 是 Parser 主動跟我拿 Token，不是我主動餵 Token 給 Parser！
   * 
   * @return Token!
   */
  Token GetNextToken();

  /**
   * @brief 我負責處理行列數的重置！
   * 等等，為什麼我是 public ?
   */
  void reset();

 private:
  /**
   * @note
   * 一切的根基，越簡單越好
   * 
   * @brief
   * 我只負責從 cin 拿字元，以及行數、列數的遞增！
   * 不負責：
   * - 任何例外處理
   * - 行列位置的重置
   * - 對獲取的字元做任何操作
   * - 其他奇奇怪怪的工作
   * 
   * @return 從 cin 取得的新鮮未加工字元！就算是 EOF 我也照樣回傳！我不負責！
   */
  char GetNextChar() {
    if (cin.eof()) {
      eof_flag = true;
      return '\0';
    }

    char ch = cin.get();
    if (ch == '\n') {
      line++;
      column = 1;
    } else {
      column++;
    }

    last_char = ch;
    last_line = line;
    last_column = column;
    return ch;
  }

  /**
   * 其他幫忙切 int、float、string 的子函數
   * 或者你想通通塞在 `GetNextToken()` 裡也可以
   * 
   * @note 共通行為：
   * 呼叫 `GetNextChar()` 卻拿到非預期的字元時，就將現有的字串回傳、包裝成 token
   * 將 EOF 視為一種例外 (error/exception)，但它不應該在這些函數裡處理，因為這些函數
   *                                          ．．．．．．．． (那麼應該在何時何地處理？)
   * 還有要將現有的字串回傳的使命，return 和 throw 兩人水火並不相容。
   * 但 No Closing Quote (string的括號未閉合) 這例外又是如何？
   * 
   * @example
   * > "I'm a sussy string
   * 
   * 這時 `GetString()` 有需要回傳這個「string」嗎？
   * `GetNextToken()` 應該把這「string」打包成 token 並回傳給 Parser 嗎？
   */
}


/**
 * Parser 在透過 `LexicalAnalyer::GetNextToken()` 拿 token 時，
 * 可能會發生拿了 token 但無法立即使用/消耗的狀況。
 * 會需要暫存拿到的 token，因為只有一個暫存需求，妳可以使用 bool 維護。
 * 除非妳對自己有十足的信心，不然建議寫個基底函式，
 * 讓暫存狀態的維護絕對不會因為粗心而出現意外。
 * 
 * 但還有比透過 bool 維護更安全的方法。
 */
class Parser {}


/**
 * 我呼叫 Parser 建樹，我也負責處理所有例外！
 */
int main() {
  return 0;
}