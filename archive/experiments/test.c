# include <stdio.h>
# include <stdbool.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>

# define NOT !
# define AND &&
# define OR ||
# define EQ ==

enum TokenType {
  IDENTIFIER = 34313,
  CONSTANT = 87424,
  SPECIAL = 29535
};

typedef char * CharPtr;

typedef struct TokenInfo {
  CharPtr token_str;
  enum TokenType token_type;
  int first_char_line;
  int first_char_column;
}TokenInfo;

typedef TokenInfo * TokenInfoPtr;

void GetNextChar(char *ch, int *line, int *column);
bool IsDigit(char ch);
bool IsLetter(char ch);
bool IsWhiteSpace(char ch);
bool IsSpecial(char ch);
void SkipWhiteSpaces(char *ch, int *line, int *column);
void SkipLineComment(char *ch, int *line, int *column);
void SkipMultiLineComment(char *ch, int *line, int *column);
CharPtr GetID(char *ch, int *line, int *column);
CharPtr GetNum(char *ch, int *line, int *column);
CharPtr GetCharConst(char *ch, int *line, int *column);
CharPtr GetStringConst(char *ch, int *line, int *column);
CharPtr GetSpecial(char *ch, int *line, int *column);
TokenInfoPtr GetToken(char *ch, int *current_line, int *current_column);

typedef struct Column {
  int column;
  struct Column * next;
}Column;

typedef Column * ColumnPtr;

typedef struct Line {
  int line;
  ColumnPtr first_appear_at;
  ColumnPtr last_appear_at;
  struct Line * next;
}Line;

typedef Line * LinePtr;

typedef struct Token {
  CharPtr token_str;
  enum TokenType type;
  LinePtr first_appear_on;
  LinePtr last_appear_on;
  struct Token * next;
}Token;

typedef Token * TokenPtr;

void PrintAllTokenInfo(TokenPtr head);
TokenPtr OrderInsertToken(TokenPtr head, TokenInfoPtr info);
void CountTokens(TokenPtr head);
void CountTypes(TokenPtr head);
void FindTokenAppearance(TokenPtr head, char *str);
void ListTokenAtLine(TokenPtr head, int line_no);
void HandleUI(TokenPtr head);

int g_next_char_line = 1;  // line-no of the next char
int g_next_char_column = 1;  // column-no of the next char

int main() {

  char ch = '\0';
  int current_line = -1;   // line-no of current char
  int current_column = -1;   // column-no of current char
  TokenInfoPtr info_ptr = NULL;
  TokenPtr list_head = NULL;

  GetNextChar(&ch, &current_line, &current_column);  // get the very first char

  do {
    info_ptr = GetToken(&ch, &current_line, &current_column);
   
    if ( info_ptr != NULL ) {
      if (strcmp(info_ptr->token_str, "END_OF_EVERYTHING") == 0) {
        free(info_ptr);
        info_ptr = NULL;
      } else
        list_head = OrderInsertToken(list_head, info_ptr);
    }
  } while ( info_ptr != NULL );

  HandleUI(list_head);
 
  return 0;
}

/* hidden code */
char last_char = '\0';
int last_line = -1;  
int last_column = -1;  
#include <ctype.h>
void GetNextChar(char* ch, int* line, int* column) {
  if (last_char != '\0') {
    *ch = last_char;
    *line = last_line;
    *column = last_column;
    last_char = '\0';
    return;
  }

  int nextchar = getchar();
  if (nextchar != EOF) {
    *ch = (char)nextchar;
    *line = g_next_char_line;
    *column = g_next_char_column;

    if (*ch == '\n') {
      g_next_char_line++;
      g_next_char_column = 1;
    } else {
      g_next_char_column++;
    }
  } else {
    *ch = '\0';
  }
}

void UngetChar(char ch, int line, int column) {
  last_char = ch;
  last_line = line;
  last_column = column;
}

/* Is ch a digit ('0', '1', '2', ..., '9')? */
bool IsDigit(char ch) {
  if (ch >= '0' && ch <= '9') {
    return true;
  }
  return false;
}

/* Is ch a letter ('a', 'b', ..., 'z', 'A', 'B', ..., 'Z')? */
bool IsLetter(char ch) {
  if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
    return true;
  }
  return false;
}

bool IsWhiteSpace(char ch) {
  if (ch == '\t' || ch == '\n' || ch == ' ') {
    return true;
  }
  return false;
}

/* Is ch a special character ? */
bool IsSpecial(char ch) {
  if (ch == '^' || ch == ',') {
    return true;
  } else if (ch == '(' || ch == ')') {
    return true;
  } else if (ch == '[' || ch == ']') {
    return true;
  } else if (ch == '{' || ch == '}') {
    return true;
  } else if (ch == '!' || ch == ':') {
    return true;
  } else if (ch == ';' || ch == '#') {
    return true;
  } else if (ch == '?' || ch == '+') {
    return true;
  } else if (ch == '-' || ch == '*') {
    return true;
  } else if (ch == '/' || ch == '>') {
    return true;
  } else if (ch == '<' || ch == '=') {
    return true;
  } else if (ch == '%' || ch == '&') {
    return true;
  } else if (ch == '&' || ch == '|') {
    return true;
  }
  return false;
}

void SkipWhiteSpaces(char* ch, int* line, int* column) {
  while (*ch != EOF && IsWhiteSpace(*ch)) {
    GetNextChar(ch, line, column);
  }
}

void SkipLineComment(char* ch, int* line, int* column) {
  while (*ch != '\n' && *ch != EOF) {
    GetNextChar(ch, line, column);
  }

  if (*ch == '\n') {
    GetNextChar(ch, line, column);
  }
}

void SkipMultiLineComment(char* ch, int* line, int* column) {
  bool star = false;
  while (true) {
    GetNextChar(ch, line, column);

    if (*ch == EOF) {
      return;
    }

    if (*ch == '*') {
      star = true;
    } else if (star && *ch == '/') {
      GetNextChar(ch, line, column);
      return;
    } else {
      star = false;
    }
  }
}

CharPtr GetID(char* ch, int* line, int* column) {
  char tmpid[256];
  int index = 0;

  while (IsDigit(*ch) || IsLetter(*ch) || *ch == '_') {
    tmpid[index] = *ch;
    index++;
    GetNextChar(ch, line, column);
  }

  CharPtr id = (CharPtr)malloc(index + 1);

  for (int i = 0; i < index; i++) {
    id[i] = tmpid[i];
  }
  id[index] = '\0';

  return id;
}

CharPtr GetNum(char* ch, int* line, int* column) {
  char tmpnum[256];
  int index = 0;
  bool dot = false;

  tmpnum[index] = *ch;
  index++;
  if (*ch == '.') {
    dot = true;
  }

  while (true) {
    GetNextChar(ch, line, column);

    if (IsDigit(*ch)) {
      tmpnum[index] = *ch;
      index++;
    } else if (*ch == '.' && !dot) {
      tmpnum[index] = *ch;
      index++;
      dot = true;
    } else {
      break;
    }
  }

  CharPtr num = (CharPtr)malloc(index + 1);

  for (int i = 0; i < index; i++) {
    num[i] = tmpnum[i];
  }
  num[index] = '\0';
  return num;
}

CharPtr GetCharConst(char* ch, int* line, int* column) {
  char tmpchar[256];
  int index = 0;
  bool escape = false;

  tmpchar[index] = *ch;
  index++;

  while (true) {
    GetNextChar(ch, line, column);
    if (*ch == EOF) {
      break;
    }
    if (!escape && *ch == '\'') {
      tmpchar[index] = *ch;
      index++;
      break;
    }
    if (*ch == '\\' && !escape) {
      tmpchar[index] = *ch;
      index++;
      escape = true;
    } else {
      tmpchar[index] = *ch;
      index++;
      escape = false;
    }
  }

  CharPtr cha = (CharPtr)malloc(index + 1);

  for (int i = 0; i < index; i++) {
    cha[i] = tmpchar[i];
  }
  cha[index] = '\0';
  GetNextChar(ch, line, column);

  return cha;
}

CharPtr GetStringConst(char* ch, int* line, int* column) {
  char tmpstr[1024];
  int index = 0;
  bool is_escape = false;

  tmpstr[index] = *ch;
  index++;

  while (true) {
    GetNextChar(ch, line, column);
    if (*ch == EOF) {
      break;
    }
    if (*ch == '"' && !is_escape) {
      tmpstr[index++] = *ch;
      break;
    }

    if (*ch == '\\' && !is_escape) {
      is_escape = true;
      tmpstr[index++] = *ch;
      continue;
    }

    tmpstr[index++] = *ch;
    is_escape = false;
  }

  CharPtr str = (CharPtr)malloc(index + 1);

  for (int i = 0; i < index; i++) {
    str[i] = tmpstr[i];
  }
  str[index] = '\0';

  GetNextChar(ch, line, column);

  return str;
}

CharPtr GetSpecial(char* ch, int* line, int* column) {
  char tmpspecial[4] = { 0 };
  int index = 0;

  tmpspecial[index++] = *ch;

  GetNextChar(ch, line, column);

  switch (tmpspecial[0]) {
    case '+':
      if (*ch == '+' || *ch == '=') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '-':
      if (*ch == '-' || *ch == '=' || *ch == '>') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '*':
      if (*ch == '=') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '/':
      if (*ch == '=') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '=':
      if (*ch == '=') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '!':
      if (*ch == '=') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '%':
      if (*ch == '=') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '&':
      if (*ch == '&') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '|':
      if (*ch == '|') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '<':
      if (*ch == '<' || *ch == '=') {
        tmpspecial[index++] = *ch;
      }
      break;
    case '>':
      if (*ch == '>' || *ch == '=') {
        tmpspecial[index++] = *ch;
      }
      break;
    default:
      break;
  }

  CharPtr special = (CharPtr)malloc(index + 1);

  for (int i = 0; i < index; i++) {
    special[i] = tmpspecial[i];
  }
  special[index] = '\0';
  if (index > 1) {
    GetNextChar(ch, line, column);
  }
  return special;
}

int IsEndOfText(const char* str) {
  const char target_part1[] = { 'E', 'N', 'D', '_', 'O', 'F' };
  const char target_part2[] = { '_', 'F', 'I', 'L', 'E', '\0' };

  for (int i = 0; i < 6; i++) {
    if (str[i] != target_part1[i]) {
      return 0;
    }
  }
  for (int i = 0; target_part2[i] != '\0'; i++) {
    if (str[i + 6] != target_part2[i]) {
      return 0;
    }
  }

  return 1;
}

TokenInfoPtr GetToken(char* ch, int* current_line, int* current_column) {
  while (true) {
    SkipWhiteSpaces(ch, current_line, current_column);

    if (*ch == '/') {
      char next_ch;
      int saved_line = *current_line;
      int saved_column = *current_column;

      GetNextChar(&next_ch, current_line, current_column);

      if (next_ch == '/') {
        SkipLineComment(ch, current_line, current_column);
        continue;
      } else if (next_ch == '*') {
        SkipMultiLineComment(ch, current_line, current_column);
        continue;
      } else {
        UngetChar(next_ch, *current_line, *current_column);
        *current_line = saved_line;
        *current_column = saved_column;
        break;
      }
    } else {
      break;
    }
  }

  if (*ch == EOF || *ch == '\0') {
    return NULL;
  }

  TokenInfoPtr token = (TokenInfoPtr)malloc(sizeof(TokenInfo));

  token->first_char_line = *current_line;
  token->first_char_column = *current_column;

  if (IsDigit(*ch) || *ch == '.') {
    token->token_type = CONSTANT;
    token->token_str = GetNum(ch, current_line, current_column);
  } else if (IsLetter(*ch) || *ch == '_') {
    token->token_type = IDENTIFIER;
    token->token_str = GetID(ch, current_line, current_column);
    if (IsEndOfText(token->token_str)) {
      free(token->token_str);
      free(token);
      return NULL;
    }
  } else if (*ch == '\'') {
    token->token_type = CONSTANT;
    token->token_str = GetCharConst(ch, current_line, current_column);
  } else if (*ch == '"') {
    token->token_type = CONSTANT;
    token->token_str = GetStringConst(ch, current_line, current_column);
  } else if (IsSpecial(*ch)) {
    token->token_type = SPECIAL;
    token->token_str = GetSpecial(ch, current_line, current_column);
  } else {
    free(token);
    GetNextChar(ch, current_line, current_column);
    return NULL;
  }

  return token;
}

TokenPtr OrderInsertToken(TokenPtr head, TokenInfoPtr info) {
  if (!info)
    return head;

  TokenPtr new_token = NULL, prev = NULL, curr = head;
  while (curr) {
    int cmp = strcmp(info->token_str, curr->token_str);
    if (cmp == 0)
      break;
    if (cmp < 0)
      break;
    prev = curr;
    curr = curr->next;
  }

  if (curr && strcmp(info->token_str, curr->token_str) == 0) {
    int line = info->first_char_line, col = info->first_char_column;
    LinePtr lp = curr->first_appear_on;
    LinePtr prev_line = NULL;

    // 找到是否已存在該行
    while (lp && lp->line < line) {
      prev_line = lp;
      lp = lp->next;
    }

    if (lp && lp->line == line) {
      // 行已存在，插入column
      ColumnPtr cp = lp->first_appear_at;
      ColumnPtr prev_col = NULL;

      while (cp && cp->column < col) {
        prev_col = cp;
        cp = cp->next;
      }

      if (cp && cp->column == col) {
        // 已存在該column
        return head;
      }

      // 新增column
      ColumnPtr new_col = (ColumnPtr) malloc(sizeof(Column));
      new_col->column = col;
      new_col->next = cp;

      if (!prev_col) {
        lp->first_appear_at = new_col;
      } else {
        prev_col->next = new_col;
      }

      lp->last_appear_at = new_col;
    } else {
      // 行不存在，新增 Line 節點 + 第一個 Column
      LinePtr new_line = (LinePtr) malloc(sizeof(Line));
      ColumnPtr new_col = (ColumnPtr) malloc(sizeof(Column));
      new_col->column = col;
      new_col->next = NULL;

      new_line->line = line;
      new_line->first_appear_at = new_col;
      new_line->last_appear_at = new_col;
      new_line->next = lp;

      if (!prev_line) {
        curr->first_appear_on = new_line;
      } else {
        prev_line->next = new_line;
      }

      // 更新 last_appear_on
      LinePtr temp = curr->first_appear_on;
      while (temp->next) temp = temp->next;
      curr->last_appear_on = temp;
    }

    return head;
  }

  new_token = (TokenPtr)malloc(sizeof(Token));
  new_token->token_str = (CharPtr)malloc(strlen(info->token_str)+1);
  strcpy(new_token->token_str, info->token_str);
  new_token->type = info->token_type;
  LinePtr line_node = (LinePtr)malloc(sizeof(Line));
  ColumnPtr col_node = (ColumnPtr)malloc(sizeof(Column));
  col_node->column = info->first_char_column;
  col_node->next = NULL;
  line_node->line = info->first_char_line;
  line_node->first_appear_at = col_node;
  line_node->last_appear_at = col_node;
  line_node->next = NULL;
  new_token->first_appear_on = line_node;
  new_token->last_appear_on = line_node;
  new_token->next = curr;

  if (!prev)
    return new_token;
  prev->next = new_token;
  return head;
}

void PrintAllTokenInfo(TokenPtr head) {
  TokenPtr current_token = head;
 
  while (current_token != NULL) {
    printf("%s ", current_token->token_str);

    switch (current_token->type) {
      case IDENTIFIER:
        printf("IDENTIFIER ");
        break;
      case CONSTANT:
        printf("CONSTANT ");
        break;
      case SPECIAL:
        printf("SPECIAL ");
        break;
    }

    LinePtr current_line = current_token->first_appear_on;
    while (current_line != NULL) {
      ColumnPtr current_col = current_line->first_appear_at;
      while (current_col != NULL) {
        printf("(%d,%d)", current_line->line, current_col->column);
        current_col = current_col->next;
      }
      current_line = current_line->next;
    }
    printf("\n");
   
    current_token = current_token->next;
  }
}

void CountTokens(TokenPtr head) {
  int cnt = 0;
  TokenPtr current_token = head;
  while (current_token != NULL) {
    cnt++;
    current_token = current_token->next;
  }
  printf("There are %d different tokens in total.\n", cnt);
}

void CountTypes(TokenPtr head) {
  int idcnt = 0, constcnt = 0, speccnt = 0;
  TokenPtr current_token = head;
  while (current_token != NULL) {
    switch (current_token->type) {
      case IDENTIFIER:
        idcnt++;
        break;
      case CONSTANT:
        constcnt++;
        break;
      case SPECIAL:
        speccnt++;
        break;
    }
    current_token = current_token->next;
  }
  printf("Identifer : %d\n", idcnt);
  printf("Constant : %d\n", constcnt);
  printf("Special : %d\n", speccnt);
}

// 3. Find token appearance
void FindTokenAppearance(TokenPtr head, char *str) {
  TokenPtr current_token = head;
  bool found = false; // 用來檢查是否有找到token
  while (current_token != NULL) { // 檢查目前的token是否是要找的token
    if (strcmp(current_token->token_str, str) == 0) { // 找到token
      // 印出token的位置
      LinePtr current_line = current_token->first_appear_on;
      while (current_line != NULL) {
        ColumnPtr current_col = current_line->first_appear_at;
        while (current_col != NULL) {
          printf("(%d,%d)", current_line->line, current_col->column);
          current_col = current_col->next;
        }
        current_line = current_line->next;
      }
      printf("\n");
      found = true;
      break;
    }
    current_token = current_token->next;
  }

  if (!found) {
    printf("There is no such token : %s\n", str);
  }
}
// 4. List token at line no
void ListTokenAtLine(TokenPtr head, int line_no) {
  TokenPtr current_token = head;
  char alrdy_print[256][256];
  int print_cnt = 0; // 紀錄被印出來的個數
  while (current_token != NULL) {
    LinePtr nowline = current_token->first_appear_on;
    bool exits = false;
    // 檢查目前的token是否存在在line_no這行
    while (nowline != NULL) {
      if (nowline->line == line_no) {
        exits = true;
        break;
      }
      nowline = nowline->next;
    }
    // 如果存在就檢查有印出過了嗎
    if (exits) {
      bool already_print = false;
      // 檢查已印出的陣列是否有目前token
      for (int i = 0; i < print_cnt; i++) {
        if (strcmp(alrdy_print[i], current_token->token_str) == 0) {
          already_print = true;
          break;
        }
      }
      if (!already_print) {
        printf("%s\n", current_token->token_str);
        // 將已印出來的token加入陣列
        strcpy(alrdy_print[print_cnt], current_token->token_str);
        print_cnt++;
      }
    }
    current_token = current_token->next;
  }
}

typedef struct {
  char** tokens;
  int count;
  int position;
} TokenList;

void TokenBufferInit(TokenList* buf) {
  buf->tokens = NULL;
  buf->count = 0;
  buf->position = 0;
}

void TokenBufferClean(TokenList* buf) {
  for (int i = 0; i < buf->count; i++) {
    free(buf->tokens[i]);
  }
  free(buf->tokens);
  buf->tokens = NULL;
  buf->count = 0;
  buf->position = 0;
}

void AddToken(TokenList* list, const char* token) {
  list->tokens = 
      (char**)realloc(list->tokens, sizeof(char*) * (list->count + 1));
  list->tokens[list->count] = strdup(token);
  list->count++;
}

TokenList TokenizeCommandLine(const char* input) {
  TokenList list = {NULL, 0};
  int len = strlen(input);
  int pos = 0;

  while (pos < len) {
    // 跳過空白字符
    while (pos < len && isspace(input[pos])) {
      pos++;
    }
    if (pos >= len) 
      break;

    // 處理命令數字 (單獨的數字如3)
    if (input[pos] == '3' || input[pos] == '4') {
      char cmd[2] = {input[pos++], '\0'};
      AddToken(&list, cmd);
      continue;
    }

    // 處理引號包圍的字串
    if (input[pos] == '"' || input[pos] == '\'') {
      char quote = input[pos];
      int start = pos;
      pos++;
      
      while (pos < len && input[pos] != quote) {
        if (input[pos] == '\\' && pos+1 < len) {
          pos += 2; // 跳過轉義字符
        } else {
          pos++;
        }
      }
        
      if (pos < len && input[pos] == quote) {
        pos++;
      }
        
      int token_len = pos - start;
      char* token = (char*)malloc(token_len + 1);
      strncpy(token, input + start, token_len);
      token[token_len] = '\0';
      AddToken(&list, token);
      free(token);
      continue;
    }

    // 處理一般token (直到下一個空白或特殊字符)
    int start = pos;
    while (pos < len && !isspace(input[pos]) && input[pos] != '3' &&
           !IsSpecial(input[pos])) {
      pos++;
    }
      
    if (pos > start) {
      int token_len = pos - start;
      char* token = (char*)malloc(token_len + 1);
      strncpy(token, input + start, token_len);
      token[token_len] = '\0';
      AddToken(&list, token);
      free(token);
    }
  }

  return list;
}

void TokenBufferAdd(TokenList* buf, char** tokens, int count) {
  for (int i = 0; i < count; i++) {
    AddToken(buf, tokens[i]);
  }
}

void HandleUI(TokenPtr head) {
  char buffer[10000];
  TokenList token_buf;
  TokenBufferInit(&token_buf);
  int pending_command = 0;

  while (1) {
    // 讀取新輸入並轉換成 tokens
    if (token_buf.position >= token_buf.count) {
      // 清空緩衝區
      TokenBufferClean(&token_buf);

      // 讀取新行
      if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
      buffer[strcspn(buffer, "\n")] = '\0';
      if (strlen(buffer) == 0) continue;

      // 分割新tokens
      TokenList new_tokens = TokenizeCommandLine(buffer);
      TokenBufferAdd(&token_buf, new_tokens.tokens, new_tokens.count);
      free(new_tokens.tokens);
    }

    // 處理緩衝區中的token
    while (token_buf.position < token_buf.count) {
      char* current = token_buf.tokens[token_buf.position];

      if (pending_command) {
        if (pending_command == 3) {
          FindTokenAppearance(head, current);
          pending_command = 0;
          token_buf.position++;
        } else if (pending_command == 4) {
          int line = atoi(current);
          if (line > 0) 
            ListTokenAtLine(head, line);
          pending_command = 0;
          token_buf.position++;
        }
        continue;
      }

      if (strcmp(current, "1") == 0) {
        CountTokens(head);
        token_buf.position++;
      } else if (strcmp(current, "2") == 0) {
        CountTypes(head);
        token_buf.position++;
      } else if (strcmp(current, "3") == 0) {
        if (token_buf.position + 1 < token_buf.count) {
          FindTokenAppearance(head, token_buf.tokens[token_buf.position + 1]);
          token_buf.position += 2;
        } else {
          pending_command = 3;
          token_buf.position++;
          break;  // 等待下一個輸入
        }
      } else if (strcmp(current, "4") == 0) {
        if (token_buf.position + 1 < token_buf.count) {
          int line = atoi(token_buf.tokens[token_buf.position + 1]);
          if (line > 0) ListTokenAtLine(head, line);
          token_buf.position += 2;
        } else {
          pending_command = 4;
          token_buf.position++;
          break;  // 等待下一個輸入
        }
      } else if (strcmp(current, "5") == 0) {
        TokenBufferClean(&token_buf);
        return;
      } else {
        token_buf.position++;
      }
    }
  }
  TokenBufferClean(&token_buf);
}